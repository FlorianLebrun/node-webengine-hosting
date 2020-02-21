
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <assert.h>
#include <Shlwapi.h>
#include <http.h>
#include <vector>
#include <map>
#include <sstream>
#include <codecvt>
#include <random>
#include <mutex>
#include <algorithm>

#include <webagent-hosting/webx.h>

#pragma comment(lib, "httpapi.lib")

struct HttpServerWin32;
struct HttpRequest;

static const char* HttpVerbString[] = {
  "Unparsed","Unknown","Invalid","OPTIONS","GET",
  "HEAD","POST","PUT","DELETE","TRACE","CONNECT",
  "TRACK","MOVE","COPY","PROPFIND","PROPPATCH",
  "MKCOL","LOCK","UNLOCK","SEARCH"
};

void printError(const char* pattern, ...) {
  const char** x = &pattern;
  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 4|8);
  printf(x[0],x[1],x[2],x[3],x[4],x[5],x[6],x[7],x[8]);
  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
}

void printColored(int color, const char* pattern, ...) {
  /* 
  colorIDs:
  > con_DarkBlue      = 1,
  > con_DarkGreen     = 2,
  > con_DarkCyan      = 3,
  > con_DarkRed       = 4,
  > con_DarkMagenta   = 5,
  > con_DarkYellow    = 6,
  > con_DarkWhite     = 7,
  */
  const char** x = &pattern;
  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color|8);
  printf(x[0],x[1],x[2],x[3],x[4],x[5],x[6],x[7],x[8]);
  SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
}

uint64_t ComputeCaseInsensitiveHash(const char* ptr, int size) {
  std::hash<std::string> hasher;
  std::string data(ptr, size);
  std::transform(data.begin(), data.end(), data.begin(), ::tolower);
  return hasher(data);
}

struct HttpEventQueue {
  int frameId;
  std::vector<HttpRequest*> listeners;
  std::mutex lock;
  HttpEventQueue(): frameId(0) {}
  void addListener(HttpRequest* listener);
  void send(webx::IEvent* evt);
};

struct HttpSessionWin32 : webx::Releasable<webx::ISessionHost, HttpSessionWin32> {
  webx::Ref<webx::ISession> handle;
  HttpServerWin32* server;
  std::string sessionType;
  std::string sessionID;
  HttpEventQueue eventsQueue;

  HttpSessionWin32(HttpServerWin32* server) {
    this->server = server;
  }
  virtual ~HttpSessionWin32() override {
  }

  void sendRequest(HttpRequest* request);
  virtual bool disconnect(webx::ISession* session) override;
  void terminate();

  virtual void dispatchDatagram(webx::IDatagram *datagram) override
  {
    printf("\nRunAsService doesn't support datagram sending:\n");
    datagram->getAttributs()->print();
    datagram->discard();
  }
  virtual void dispatchEvent(webx::IEvent *evt) override
  {
    this->eventsQueue.send(evt);
  }
};

struct HttpServerWin32 : webx::Releasable<webx::IEngineHost, HttpServerWin32> {
  std::string url;
  webx::IEngine* context;
  std::map<std::string, HttpSessionWin32*> sessions;
  HttpEventQueue eventsQueue;
  std::mutex lock;

  HANDLE hReqQueue;
  const char* HttpRequestHeaderIdToName[HttpHeaderRequestMaximum+1];
  std::map<uint64_t, int> HttpResponseHeaderIdFromName;

  HttpServerWin32();

  void removeSession(HttpSessionWin32* session) {
    this->lock.lock();
    this->sessions.erase(this->sessions.find(session->sessionID));
    this->lock.unlock();
    session->release();
  }
  HttpSessionWin32* createSession(std::string sessionType) {
    HttpSessionWin32* session = 0;
    this->lock.lock();
    if(context->hasSessionAffinity(sessionType.c_str())) {

      // Generate session ID
      std::string sessionID;
      do {
        sessionID = this->generateSessionID();
      } while(this->sessions.find(sessionID) != this->sessions.end());

      // Create session
      session = new HttpSessionWin32(this);
      session->sessionID = sessionID;
      session->handle.Box(this->context->createSession(sessionType.c_str(), (sessionType + "-" + sessionID).c_str(), session));
      if(!session->handle) {
        printError("Cannot create session '%s'\n", sessionType.c_str());
        delete session;
        session = 0;
      }
      else {
        // Register session with new ID
        do {
          session->sessionID = this->generateSessionID();
        } while(this->sessions.find(session->sessionID) != this->sessions.end());
        session->sessionType = sessionType;
        this->sessions[session->sessionID] = session;
      }
    } 
    else {
      auto found = this->sessions.find(sessionType);
      if(found == this->sessions.end()) {
        session = new HttpSessionWin32(this);
        session->handle.Box(this->context->createSession(sessionType.c_str(), sessionType.c_str(), session));
        if(!session->handle) {
          printError("Cannot create session '%s'\n", sessionType.c_str());
          delete session;
          session = 0;
        }
        else {
          session->sessionID = sessionType;
          session->sessionType = sessionType;
          this->sessions[sessionType] = session;
        }
      }
      else {
        session = found->second;
      }
    }
    this->lock.unlock();
    return session;
  }
  std::string generateSessionID() {
    const char alphanum[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz"; //salt alphanum

    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_int_distribution<> dis(0, sizeof(alphanum)-2); //Uniform distribution on an interval
    char sessionID[22];          // 21 useful characters in salt (as in original code)
    for(char& c: sessionID) {
      c = alphanum[dis(gen)];
    }
    sessionID[21] = 0;
    return std::string(sessionID);
  }
  void deleteSession(HttpSessionWin32* session) {
    this->sessions.erase(this->sessions.find(session->sessionID));
    delete session;
  }
  bool Init(std::string url, webx::IEngine* context);
  void Loop();

  void RegisterKnownRequestHeader(HTTP_HEADER_ID HeaderId, const char* Name);
  void RegisterKnownResponseHeader(HTTP_HEADER_ID HeaderId, const char* Name);
  int getKnownHeaderId(const std::string& Name);

  virtual void dispatchDatagram(webx::IDatagram *datagram) override
  {
    printf("\nRunAsService doesn't support datagram sending:\n");
    datagram->getAttributs()->print();
    datagram->discard();
  }
  virtual void dispatchEvent(webx::IEvent *evt) override
  {
    this->eventsQueue.send(evt);
  }
  virtual bool disconnect(webx::ISession* session) override
  {
    throw "no imp";
  }
  virtual bool terminate() override
  {
    this->context = 0;
    return 0;
  }
};

struct HttpRequest : webx::Releasable<webx::IDatagram, HttpRequest>, webx::IDatagramHandler {
  HttpServerWin32* server;
  HttpSessionWin32* session;
  std::string sessionType;
  std::string sessionID;
  std::string path;

  HTTP_REQUEST_ID requestId;
  webx::DataQueue requestCkunks;
  webx::IDatagramHandler *requestHandler;
  webx::StringMapValue requestHeaders;

  webx::Ref<webx::IDatagram> responseDatagram;
  int responseFrame;

  HttpRequest(HttpServerWin32* server, PHTTP_REQUEST pRequestBuffer, int RequestBufferLength) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;

    this->server = server;
    this->session = 0;
    this->responseFrame = 0;
    this->requestId = PHTTP_REQUEST(pRequestBuffer)->RequestId;
    this->requestHandler = 0;

    // Fill method
    const char* method;
    if(PHTTP_REQUEST(pRequestBuffer)->Verb != HttpVerbUnknown) method = HttpVerbString[PHTTP_REQUEST(pRequestBuffer)->Verb];
    else if(PHTTP_REQUEST(pRequestBuffer)->pUnknownVerb) method = PHTTP_REQUEST(pRequestBuffer)->pUnknownVerb;
    else method = HttpVerbString[1];
    this->requestHeaders.values[":method"] = method;

    // Fill path
    std::string fullpath = utf8_conv.to_bytes(std::wstring(
      PHTTP_REQUEST(pRequestBuffer)->CookedUrl.pAbsPath,
      (PHTTP_REQUEST(pRequestBuffer)->CookedUrl.AbsPathLength+PHTTP_REQUEST(pRequestBuffer)->CookedUrl.QueryStringLength)/2
      ));
    const char* c_path = fullpath.c_str();
    int sessionType_len = 0;
    if(c_path[sessionType_len] == '/') {
      sessionType_len++;
      while(c_path[sessionType_len] && c_path[sessionType_len] != '/' && c_path[sessionType_len] != '?') sessionType_len++;
      this->sessionType = std::string(&c_path[1], sessionType_len-1);
    }
    this->path = &c_path[sessionType_len];
    this->requestHeaders.values[":path"] = &c_path[sessionType_len];

    // Fill authority
    this->requestHeaders.values[":authority"] = utf8_conv.to_bytes(std::wstring(
      PHTTP_REQUEST(pRequestBuffer)->CookedUrl.pHost, 
      PHTTP_REQUEST(pRequestBuffer)->CookedUrl.HostLength/2
      ));

    // Parse cookie session ID
    HTTP_KNOWN_HEADER& CookieHeader = PHTTP_REQUEST(pRequestBuffer)->Headers.KnownHeaders[HttpHeaderCookie];
    std::string cookie(CookieHeader.pRawValue, CookieHeader.RawValueLength);
    std::string sessionID_Tag = "WS-"+this->sessionType+"=";
    int sessionIDIndex = cookie.find(sessionID_Tag);
    if(sessionIDIndex >= 0) {
      sessionIDIndex += sessionID_Tag.size();
      int sessionIDEndIndex = cookie.find(';', sessionIDIndex);
      this->sessionID = cookie.substr(sessionIDIndex, sessionIDEndIndex-sessionIDIndex);
    }

    // Fill http_1_0 attributs
    HTTP_REQUEST_HEADERS& Headers = PHTTP_REQUEST(pRequestBuffer)->Headers;
    for(int i=0;i<HttpHeaderRequestMaximum;i++) {
      HTTP_KNOWN_HEADER& Header = Headers.KnownHeaders[i];
      if(Header.RawValueLength) {
        const char* name = this->server->HttpRequestHeaderIdToName[i];
        this->requestHeaders.values[name] = std::string(Header.pRawValue, Header.RawValueLength);
      }
    }

    // Fill custom attributs
    for(int i=0;i<Headers.UnknownHeaderCount;i++) {
      HTTP_UNKNOWN_HEADER& Header = Headers.pUnknownHeaders[i];
      if(Header.RawValueLength) {
        std::string name(Header.pName, Header.NameLength);
        this->requestHeaders.values[name] = std::string(Header.pRawValue, Header.RawValueLength);
      }
    }
  }
  virtual webx::IValue* getAttributs() override {
    return &this->requestHeaders;
  }
  virtual bool accept(webx::IDatagramHandler *handler) override {
    if(!this->requestHandler) {
      this->requestHandler = handler;
      handler->onData(this);
      handler->onComplete(this);
      return true;
    }
    return false;
  }
  virtual void discard() override {
    this->sendResponse();
  }
  virtual bool send(webx::IDatagram* response) override {
    if(!this->responseDatagram) {
      this->responseDatagram = response;
      response->accept(this);
      return true;
    }
    return false;
  }
  virtual void onData(webx::IDatagram* from) override {
    assert(this->responseDatagram == from);
  }
  virtual void onComplete(webx::IDatagram* from) override {
    assert(this->responseDatagram == from);
    this->sendResponse();
  }
  virtual webx::IData* pullData() override {
    return this->requestCkunks.flush();
  }
  virtual webx::tIOStatus getStatus() override {
    return this->requestCkunks.status;
  }
  virtual void disconnect() override {
    this->responseDatagram = 0;
    this->release();
  }
  bool sendResponse();
  bool sendChunkTransfert(int status, const char* content, int contentLength, const char* contentType);
};

std::string GetWin32ErrorMessage(DWORD code) {
  std::string result = "error(";
  result += code;
  result += ") ";
  LPVOID lpMsgBuf;
  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,NULL,code,0,(LPTSTR) &lpMsgBuf,0,NULL );
  result += (LPCTSTR)lpMsgBuf;
  LocalFree( lpMsgBuf );
  return result;
}

HttpServerWin32::HttpServerWin32() {
  this->hReqQueue = 0;
  memset(this->HttpRequestHeaderIdToName, 0, sizeof(this->HttpRequestHeaderIdToName));
  this->RegisterKnownRequestHeader(HttpHeaderCacheControl, "Cache-Control");
  this->RegisterKnownRequestHeader(HttpHeaderConnection, "Connection");
  this->RegisterKnownRequestHeader(HttpHeaderDate, "Date");
  this->RegisterKnownRequestHeader(HttpHeaderKeepAlive, "Keep-Alive");
  this->RegisterKnownRequestHeader(HttpHeaderPragma, "Pragma");
  this->RegisterKnownRequestHeader(HttpHeaderTrailer, "Trailer");
  this->RegisterKnownRequestHeader(HttpHeaderTransferEncoding, "Transfer-Encoding");
  this->RegisterKnownRequestHeader(HttpHeaderUpgrade, "Upgrade");
  this->RegisterKnownRequestHeader(HttpHeaderVia, "Via");
  this->RegisterKnownRequestHeader(HttpHeaderWarning, "Warning");
  this->RegisterKnownRequestHeader(HttpHeaderAllow, "Allow");
  this->RegisterKnownRequestHeader(HttpHeaderContentLength, "Content-Length");
  this->RegisterKnownRequestHeader(HttpHeaderContentType, "Content-Type");
  this->RegisterKnownRequestHeader(HttpHeaderContentEncoding, "Content-Encoding");
  this->RegisterKnownRequestHeader(HttpHeaderContentLanguage, "Content-Language");
  this->RegisterKnownRequestHeader(HttpHeaderContentLocation, "Content-Location");
  this->RegisterKnownRequestHeader(HttpHeaderContentMd5, "Content-Md5");
  this->RegisterKnownRequestHeader(HttpHeaderContentRange, "Content-Range");
  this->RegisterKnownRequestHeader(HttpHeaderExpires, "Expires");
  this->RegisterKnownRequestHeader(HttpHeaderLastModified, "Last-Modified");
  this->RegisterKnownRequestHeader(HttpHeaderAccept, "Accept");
  this->RegisterKnownRequestHeader(HttpHeaderAcceptCharset, "Accept-Charset");
  this->RegisterKnownRequestHeader(HttpHeaderAcceptEncoding, "Accept-Encoding");
  this->RegisterKnownRequestHeader(HttpHeaderAcceptLanguage, "Accept-Language");
  this->RegisterKnownRequestHeader(HttpHeaderAuthorization, "Authorization");
  this->RegisterKnownRequestHeader(HttpHeaderCookie, "Cookie");
  this->RegisterKnownRequestHeader(HttpHeaderExpect, "Expect");
  this->RegisterKnownRequestHeader(HttpHeaderFrom, "From");
  this->RegisterKnownRequestHeader(HttpHeaderHost, "Host");
  this->RegisterKnownRequestHeader(HttpHeaderIfMatch, "If-Match");
  this->RegisterKnownRequestHeader(HttpHeaderIfModifiedSince, "If-Modified-Since");
  this->RegisterKnownRequestHeader(HttpHeaderIfNoneMatch, "If-None-Match");
  this->RegisterKnownRequestHeader(HttpHeaderIfRange, "If-Range");
  this->RegisterKnownRequestHeader(HttpHeaderIfUnmodifiedSince, "If-Unmodified-Since");
  this->RegisterKnownRequestHeader(HttpHeaderMaxForwards, "Max-Forwards");
  this->RegisterKnownRequestHeader(HttpHeaderProxyAuthorization, "Proxy-Authorization");
  this->RegisterKnownRequestHeader(HttpHeaderReferer, "Referer");
  this->RegisterKnownRequestHeader(HttpHeaderRange, "Range");
  this->RegisterKnownRequestHeader(HttpHeaderTe, "Te");
  this->RegisterKnownRequestHeader(HttpHeaderTranslate, "Translate");
  this->RegisterKnownRequestHeader(HttpHeaderUserAgent, "User-Agent");
  this->RegisterKnownRequestHeader(HttpHeaderRequestMaximum, "Request-Maximum");


  this->RegisterKnownResponseHeader(HttpHeaderCacheControl, "Cache-Control");
  this->RegisterKnownResponseHeader(HttpHeaderConnection, "Connection");
  this->RegisterKnownResponseHeader(HttpHeaderDate, "Date");
  this->RegisterKnownResponseHeader(HttpHeaderKeepAlive, "Keep-Alive");
  this->RegisterKnownResponseHeader(HttpHeaderPragma, "Pragma");
  this->RegisterKnownResponseHeader(HttpHeaderTrailer, "Trailer");
  this->RegisterKnownResponseHeader(HttpHeaderTransferEncoding, "Transfer-Encoding");
  this->RegisterKnownResponseHeader(HttpHeaderUpgrade, "Upgrade");
  this->RegisterKnownResponseHeader(HttpHeaderVia, "Via");
  this->RegisterKnownResponseHeader(HttpHeaderWarning, "Warning");
  this->RegisterKnownResponseHeader(HttpHeaderAllow, "Allow");
  this->RegisterKnownResponseHeader(HttpHeaderContentLength, "Content-Length");
  this->RegisterKnownResponseHeader(HttpHeaderContentType, "Content-Type");
  this->RegisterKnownResponseHeader(HttpHeaderContentEncoding, "Content-Encoding");
  this->RegisterKnownResponseHeader(HttpHeaderContentLanguage, "Content-Language");
  this->RegisterKnownResponseHeader(HttpHeaderContentLocation, "Content-Location");
  this->RegisterKnownResponseHeader(HttpHeaderContentMd5, "Content-Md5");
  this->RegisterKnownResponseHeader(HttpHeaderContentRange, "Content-Range");
  this->RegisterKnownResponseHeader(HttpHeaderExpires, "Expires");
  this->RegisterKnownResponseHeader(HttpHeaderLastModified, "Last-Modified");
  this->RegisterKnownResponseHeader(HttpHeaderAcceptRanges, "Accept-Ranges");
  this->RegisterKnownResponseHeader(HttpHeaderAge, "Age");
  this->RegisterKnownResponseHeader(HttpHeaderEtag, "Etag");
  this->RegisterKnownResponseHeader(HttpHeaderLocation, "Location");
  this->RegisterKnownResponseHeader(HttpHeaderProxyAuthenticate, "Proxy-Authenticate");
  this->RegisterKnownResponseHeader(HttpHeaderRetryAfter, "Retry-After");
  this->RegisterKnownResponseHeader(HttpHeaderServer, "Server");
  this->RegisterKnownResponseHeader(HttpHeaderSetCookie, "Set-Cookie");
  this->RegisterKnownResponseHeader(HttpHeaderVary, "Vary");
  this->RegisterKnownResponseHeader(HttpHeaderWwwAuthenticate, "Www-Authenticate");
}

void HttpServerWin32::RegisterKnownRequestHeader(HTTP_HEADER_ID HeaderId, const char* Name) {
  if(HeaderId <= HttpHeaderRequestMaximum) {
    this->HttpRequestHeaderIdToName[HeaderId] = Name;
  }
  else throw std::exception("out of range");
}

void HttpServerWin32::RegisterKnownResponseHeader(HTTP_HEADER_ID HeaderId, const char* Name) {
  uint64_t hash = ComputeCaseInsensitiveHash(Name, strlen(Name));
  this->HttpResponseHeaderIdFromName.insert(std::pair<uint64_t, int>(hash, HeaderId));
}

int HttpServerWin32::getKnownHeaderId(const std::string& Name) {
  uint64_t hash = ComputeCaseInsensitiveHash(Name.c_str(), Name.size());
  std::map<uint64_t, int>::const_iterator it = this->HttpResponseHeaderIdFromName.find(hash);
  if ( it != this->HttpResponseHeaderIdFromName.end() ) {
    return it->second;
  }
  return HTTP_HEADER_ID(-1);
}

bool HttpServerWin32::Init(std::string url, webx::IEngine* context) {
  ULONG retCode;
  HTTPAPI_VERSION HttpApiVersion = HTTPAPI_VERSION_1;

  // Init server data
  this->url = url;
  this->context = context;
  this->hReqQueue = 0;

  // Initialize HTTP Server APIs
  retCode = HttpInitialize(HttpApiVersion, HTTP_INITIALIZE_SERVER,NULL);
  if (retCode != NO_ERROR) {
    std::cout<<"server-error: Win32.HttpInitialize failed with "<<GetWin32ErrorMessage(retCode)<<"\n";
    return retCode;
  }

  // Create a Request Queue Handle
  retCode = HttpCreateHttpHandle(&hReqQueue, 0);
  if (retCode != NO_ERROR) {
    std::cout<<"server-error: Win32.HttpCreateHttpHandle failed with "<<GetWin32ErrorMessage(retCode)<<"\n";
    goto CleanUp;
  }

  // Connect a Request Url
  size_t convertedChars = 0;
  wchar_t locationUrlW[255];
  mbstowcs_s(&convertedChars, locationUrlW, strlen(this->url.c_str()) + 1, this->url.c_str(), _TRUNCATE);
  retCode = HttpAddUrl(hReqQueue,locationUrlW,NULL);
  if (retCode != NO_ERROR) {
    std::cout<<"server-error: Win32.HttpAddUrl failed with "<<GetWin32ErrorMessage(retCode)<<"\n";
    goto CleanUp;
  }
  else {
    std::cout<<"server-info: Listening for requests on the following url: "<<this->url<<"\n";
  }

  // Launch server
  this->Loop();
  return true;

CleanUp:
  HttpRemoveUrl(hReqQueue, locationUrlW);

  if(hReqQueue)CloseHandle(hReqQueue);
  HttpTerminate(HTTP_INITIALIZE_SERVER, NULL);
  return false;
}

class HttpData : public webx::Releasable<webx::IData, HttpData>
{
public:
  ULONG allocated;
  ULONG size;
  char buffer[1];
  HttpData(uint32_t allocated)
  {
    this->size = 0;
    this->allocated = allocated;
  }
  virtual ~HttpData() override {
  }
  virtual bool getData(char* &buffer, uint32_t &size) {
    buffer = this->buffer;
    size = this->size;
    return true;
  }
  inline static HttpData *New(int allocated)
  {
    return new (::malloc(sizeof(HttpData) + allocated)) HttpData(allocated);
  }
};

void HttpServerWin32::Loop()
{
  int RequestBufferLength = 1024;
  PHTTP_REQUEST pRequestBuffer = PHTTP_REQUEST(::malloc(RequestBufferLength));

  HTTP_REQUEST_ID requestId;
  HTTP_SET_NULL_ID( &requestId );
  while(true) {
    DWORD bytesRead;

    // Load request header
    ULONG result = HttpReceiveHttpRequest(hReqQueue, requestId, 0, PHTTP_REQUEST(pRequestBuffer), RequestBufferLength,&bytesRead, NULL);
    requestId = PHTTP_REQUEST(pRequestBuffer)->RequestId;
    while(result == ERROR_MORE_DATA) {
      ::free(pRequestBuffer);
      RequestBufferLength += 1024;
      pRequestBuffer = PHTTP_REQUEST(::malloc(RequestBufferLength));
      if (pRequestBuffer == NULL) throw "ERROR_NOT_ENOUGH_MEMORY";
      result = HttpReceiveHttpRequest(hReqQueue, requestId, 0, PHTTP_REQUEST(pRequestBuffer), RequestBufferLength,&bytesRead, NULL);
    };

    if(NO_ERROR == result)
    {
      webx::Ref<HttpRequest> request;
      request.Box(new HttpRequest(this, pRequestBuffer, RequestBufferLength));

      // Receive the request body
      if(PHTTP_REQUEST(pRequestBuffer)->Flags & HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS) {
        do {
          HttpData* data = HttpData::New(1024);
          result = HttpReceiveRequestEntityBody(hReqQueue, PHTTP_REQUEST(pRequestBuffer)->RequestId, 0,
            data->buffer,
            data->allocated,
            &data->size, NULL);

          if(data->size) {
            request->requestCkunks.pushBox(data);
            if(request->requestHandler) {
              request->requestHandler->onData(request);
            }
          }
          else data->release();

        } while(result == NO_ERROR);

      }

      // Send the request
      if(result == NO_ERROR || result == ERROR_HANDLE_EOF) {
        request->requestCkunks.status.data_end = 1;
        if(request->sessionType == "admin") {
          if(request->path == "/.events") {
            this->eventsQueue.addListener(request);
          }
          else {
            this->context->dispatchDatagram(request);
          }
        }
        else {
          HttpSessionWin32* session = 0;

          // Acquire session
          std::map<std::string, HttpSessionWin32*>::iterator it = this->sessions.find(request->sessionID);
          if(it != this->sessions.end()) session = it->second;
          else session = this->createSession(request->sessionType);

          // Dispatch request
          request->session = session;
          if(session) session->sendRequest(request);
          else request->discard();
        }
      }

      HTTP_SET_NULL_ID( &requestId );
    }
    else if(ERROR_CONNECTION_INVALID == result && !HTTP_IS_NULL_ID(&requestId))
    {
      // Failure on http channel
      HTTP_SET_NULL_ID( &requestId );
    }
    else
    {
      break;
    }
  }
  ::free(pRequestBuffer);

  size_t convertedChars = 0;
  wchar_t locationUrlW[255];
  mbstowcs_s(&convertedChars, locationUrlW, strlen(this->url.c_str()) + 1, this->url.c_str(), _TRUNCATE);
  HttpRemoveUrl(this->hReqQueue, locationUrlW);
  if(this->hReqQueue)CloseHandle(this->hReqQueue);
  HttpTerminate(HTTP_INITIALIZE_SERVER, NULL);
}

struct HttpResponseBuffer {
  PHTTP_RESPONSE pResponse;
  DWORD ResponseLength;
  char* ResponseReserve;
  char* ResponseReserveLimit;

  HttpRequest* request;
  webx::Ref<webx::IData> responseChunks;
  char s_contentLength[16];

  static const int maxHeaderUnknownCount = 32;

  HttpResponseBuffer(HttpRequest* request, int status, const char* content, int contentLength, const char* contentType) {
    this->request = request;

    this->createResponseBuffer(contentLength?1:0, 4096);
    this->pResponse->StatusCode = status;

    HTTP_RESPONSE_HEADERS& Headers = this->pResponse->Headers;
    if(contentLength) {
      // Write content chunks
      PHTTP_DATA_CHUNK curDataChunk = PHTTP_DATA_CHUNK(&pResponse[1]);
      this->pResponse->EntityChunkCount = 1;
      curDataChunk->DataChunkType = HttpDataChunkFromMemory;
      curDataChunk->FromMemory.pBuffer = (char*)content;
      curDataChunk->FromMemory.BufferLength = contentLength;
    }
    else this->pResponse->EntityChunkCount = 0;

    // Write ContentType header
    Headers.KnownHeaders[HttpHeaderContentType].pRawValue = contentType;
    Headers.KnownHeaders[HttpHeaderContentType].RawValueLength = strlen(contentType);
  }

  HttpResponseBuffer(HttpRequest* request) {

    // Pull response data
    webx::IDatagram* response = request->responseDatagram;
    if(response) {
      int contentLength = 0;
      this->request = request;

      this->responseChunks.Box(response->pullData());

      // Create response buffer
      int contentChunkCount = this->responseChunks->getDataCount();
      this->createResponseBuffer(contentChunkCount, 4096);

      // Write content chunks
      PHTTP_DATA_CHUNK curDataChunk = PHTTP_DATA_CHUNK(&pResponse[1]);
      for(webx::IData* chunk=this->responseChunks; chunk; chunk=(webx::IData*)*chunk->next) {
        char* buffer; uint32_t size;
        if(chunk->getData(buffer, size)) {
          if(size > 0){
            this->pResponse->EntityChunkCount++;
            curDataChunk->DataChunkType = HttpDataChunkFromMemory;
            curDataChunk->FromMemory.pBuffer = buffer;
            curDataChunk->FromMemory.BufferLength = size;
            contentLength += size; 
          }
        }
      }

      // Write cookie
      HttpSessionWin32* session = request->session;
      if (session) {
        std::string cookie = "WS-"+session->sessionType+"="+session->sessionID+";path=/;OnlyHttp";
        this->setHeader("Set-Cookie", webx::StringRefValue(cookie));
      }

      // Write headers
      webx::IValue* attributs = response->getAttributs();
      this->pResponse->StatusCode = 501;
      attributs->foreach([this](const webx::IValue& key, const webx::IValue& value) {
        std::string name = key.toString();
        if(name[0] == ':') {
          if(name == ":status") this->pResponse->StatusCode = value.toInteger();
        }
        else {
          this->setHeader(name, value);
        }
      });
    }
    else {
      const char* c_Content_404 = "no service";
      this->HttpResponseBuffer::HttpResponseBuffer(request, 404, c_Content_404, strlen(c_Content_404), "text/plain");
    }
  }
  char* push(const std::string &value) {
    return this->push(value.c_str(), value.size());
  }
  char* push(const char* bytes, int len) {
    char* ptr = this->ResponseReserve;
    this->ResponseReserve += len;
    if(this->ResponseReserve<this->ResponseReserveLimit) {
      memcpy(ptr, bytes, len);
      return ptr;
    }
    else {
      printf("\n************* OUT OF MEMORY **************\n");
      return 0;
    }
  }
  void setHeader(const std::string name, const webx::IValue &value) {
    HTTP_RESPONSE_HEADERS& Headers = this->pResponse->Headers;
    std::string s_value = value.toString();
    const char* pRawValue = this->push(name);
    int RawValueLength = name.size();

    // Add known header
    int knownHeaderId = this->request->server->getKnownHeaderId(name);
    if(knownHeaderId>=0) {
      Headers.KnownHeaders[knownHeaderId].pRawValue = this->push(s_value);
      Headers.KnownHeaders[knownHeaderId].RawValueLength = s_value.size();
    }
    // Add unknown header
    else if(Headers.UnknownHeaderCount < this->maxHeaderUnknownCount) {
      HTTP_UNKNOWN_HEADER& UnknownHeader = Headers.pUnknownHeaders[Headers.UnknownHeaderCount++];
      UnknownHeader.pName = this->push(name);
      UnknownHeader.NameLength = name.size();
      UnknownHeader.pRawValue = this->push(s_value);
      UnknownHeader.RawValueLength = s_value.size();
    }
  }
  void createResponseBuffer(int maxChunkCount, int reserve) {
    this->ResponseLength = sizeof(HTTP_RESPONSE)+maxChunkCount*sizeof(HTTP_DATA_CHUNK)+maxHeaderUnknownCount*sizeof(HTTP_UNKNOWN_HEADER)+reserve;
    this->pResponse = (PHTTP_RESPONSE)::malloc(ResponseLength);
    RtlZeroMemory(this->pResponse, sizeof(HTTP_RESPONSE));
    this->pResponse->pEntityChunks = PHTTP_DATA_CHUNK(PCHAR(this->pResponse)+sizeof(HTTP_RESPONSE));
    this->pResponse->Headers.pUnknownHeaders = PHTTP_UNKNOWN_HEADER(PCHAR(this->pResponse->pEntityChunks) + maxChunkCount*sizeof(HTTP_DATA_CHUNK));
    this->ResponseReserve = (PCHAR(this->pResponse->Headers.pUnknownHeaders) + maxHeaderUnknownCount*sizeof(HTTP_UNKNOWN_HEADER));
    this->ResponseReserveLimit = this->ResponseReserve+reserve;
  }
};

bool HttpRequest::sendChunkTransfert(int status, const char* content, int contentLength, const char* contentType) {
  DWORD result;
  _ASSERT(this->responseDatagram == 0);

  // Send the response
  if(this->responseFrame == 0) {
    HttpResponseBuffer response(this, status, content, contentLength, contentType);
    response.setHeader("Cache-Control", webx::C_StringValue("no-cache"));
    response.setHeader("Connection", webx::C_StringValue("keep-alive"));
    response.setHeader("Transfer-Encoding", webx::C_StringValue("chunked"));
    response.setHeader("Access-Control-Allow-Headers", webx::C_StringValue("Content-Type"));
    response.setHeader("Access-Control-Allow-Methods", webx::C_StringValue("GET,PUT,POST,DELETE"));
    response.setHeader("Access-Control-Allow-Origin", webx::C_StringValue("*"));

    DWORD bytesSent;
    result = HttpSendHttpResponse(
      this->server->hReqQueue,           // ReqQueueHandle
      this->requestId, // Request ID
      HTTP_SEND_RESPONSE_FLAG_MORE_DATA,       // Flags
      response.pResponse,  // HTTP response
      NULL,                // pReserved1
      &bytesSent,          // bytes sent  (OPTIONAL)
      NULL,                // pReserved2  (must be NULL)
      0,                   // Reserved3   (must be 0)
      NULL,                // LPOVERLAPPED(OPTIONAL)
      NULL                 // pReserved4  (must be NULL)
      ); 
  }
  else {
    DWORD bytesSent;
    HTTP_DATA_CHUNK chunk;
    chunk.DataChunkType = HttpDataChunkFromMemory;
    chunk.FromMemory.pBuffer = (char*)content;
    chunk.FromMemory.BufferLength = contentLength;
    result = HttpSendResponseEntityBody(
      this->server->hReqQueue,           // ReqQueueHandle
      this->requestId, // Request ID
      HTTP_SEND_RESPONSE_FLAG_MORE_DATA,       // Flags
      1,  // EntityChunkCount
      &chunk,                // EntityChunks
      &bytesSent,          // bytes sent  (OPTIONAL)
      NULL,                // pReserved2  (must be NULL)
      0,                   // Reserved3   (must be 0)
      NULL,                // LPOVERLAPPED(OPTIONAL)
      NULL                 // pReserved4  (must be NULL)
      );
  }
  this->responseFrame++;

  // Check error
  if(result != NO_ERROR) {
    printError("HttpSendHttpResponse failed with %lu \n", result);
    return false;
  }
  return true;
}

bool HttpRequest::sendResponse() {
  HttpResponseBuffer response(this);

  // Send the response
  DWORD bytesSent;
  DWORD result = HttpSendHttpResponse(
    this->server->hReqQueue,           // ReqQueueHandle
    this->requestId, // Request ID
    0,       // Flags
    response.pResponse,  // HTTP response
    NULL,                // pReserved1
    &bytesSent,          // bytes sent  (OPTIONAL)
    NULL,                // pReserved2  (must be NULL)
    0,                   // Reserved3   (must be 0)
    NULL,                // LPOVERLAPPED(OPTIONAL)
    NULL                 // pReserved4  (must be NULL)
    );

  // Release response datagram
  this->responseDatagram = 0;

  // Release request handler
  if(this->requestHandler) {
    this->requestHandler->disconnect();
    this->requestHandler = 0;
  }

  // Check error
  if(result != NO_ERROR) {
    printError("HttpSendHttpResponse failed with %lu \n", result);
    return false;
  }
  return true;
}

void HttpEventQueue::addListener(HttpRequest* listener) {
  printf("\n------ new listener ------\n");
  if(listener->sendChunkTransfert(200, "2\r\n\r\n\r\n", 7, "text/event-stream")) {
    this->lock.lock();
    _ASSERT(listener->nref>0);
    listener->retain();
    this->listeners.push_back(listener);
    this->lock.unlock();
  }
  else  {
    printError("addListener failed\n");
    listener->release();
  }
}

void HttpEventQueue::send(webx::IEvent* evt) {
  printColored(2, "\n[event] %s\n", evt->eventName());

  std::stringstream out;
  out<<"\r\n";
  out<<"id:"<<(this->frameId++)<<"\n";
  out<<"event:"<<evt->eventName()<<"\n";
  out<<"data:";evt->toJSON(out);out<<"\n\n";
  out<<"\r\n";

  char size_hexa[8];
  std::string content = itoa(int(out.tellp())-4, size_hexa, 16)+out.str();

  int i=0;
  this->lock.lock();
  while(i<this->listeners.size()) {
    HttpRequest* listener = this->listeners[i];
    _ASSERT(listener->nref==1);
    if(!listener->sendChunkTransfert(200, content.c_str(), content.size(), "text/event-stream")) {
      this->listeners.erase(this->listeners.begin() + i);
      listener->release();
    }
    else i++;
  }
  this->lock.unlock();
  evt->release();
}

void HttpSessionWin32::sendRequest(HttpRequest* request) {
  if(request->path == "/.events") {
    this->eventsQueue.addListener(request);
  }
  else if(request->path == "/.terminate") {
    request->session = 0;
    request->discard();
    this->terminate();
  }
  else if(this->handle) {
    this->handle->dispatchDatagram(request);
  }
  else request->discard();
}

bool HttpSessionWin32::disconnect(webx::ISession* session) {
  if(this->handle == session) {
    this->handle = 0;
    this->server->removeSession(this);
    return true;
  }
  return false;
}

void HttpSessionWin32::terminate() {
  if(this->handle) {
    this->handle->close();
    this->disconnect(this->handle);
  }
}

std::map<std::string,std::string> parse_arguments(const char** argv, int argc) {
  std::map<std::string,std::string> args;
  for(int i=0;i<argc;i++) {
    if(argv[i][0] == '-') {
      if(++i < argc) args[argv[i-1]] = argv[i];
    }
  }
  return args;
}

int main(const char** argv, int argc) {
  auto args = parse_arguments(argv, argc);

  // Read configuration file
  printf("Load with configuration at '%s'\n", args["--config"].c_str());
  HANDLE hFile = CreateFile(args["--config"].c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if(hFile == INVALID_HANDLE_VALUE) {
    printError("Configuration file not found");
    return -1;
  }
  DWORD size = GetFileSize(hFile, 0);
  char* config = (char*)malloc(size+1);
  ReadFile(hFile, &config[0], size, &size, nullptr);
  config[size] = 0;
  CloseHandle(hFile);

  // Init server
  HttpServerWin32 server;
  auto hAgentDll = LoadLibraryA(args["--dll-path"].c_str());
  auto agentConnect = (webx::tEngineConnectProc)GetProcAddress(hAgentDll, args["--dll-entrypoint"].c_str());
  webx::IEngine* context = agentConnect(&server, config);
  if(!context) {
    printError("server-error: agent connect failed");
    return -1;
  }

  // Run server
  if(server.Init("http://127.0.0.1:" + args["--port"] + "/", context)) {
    server.Loop();
    return 0;
  }
  else return -1;
}
