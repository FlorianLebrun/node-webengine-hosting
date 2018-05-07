
class DataQueue
{
public:
  webx::IData *first, *_;
  DataQueue()
  {
    this->first = 0;
  }
  ~DataQueue()
  {
    IData *d = this->first;
    while (d)
    {
      IData *dn = d->next;
      d->release();
      d = dn;
    }
  }
  void push(IData *data)
  {
    data->next = 0;
    if (this->first)
      this->_->next = data;
    else
      this->first = data;
    this->_ = data;
  }
  IData *pop()
  {
    IData *data = this->first;
    if (data)
    {
      this->first = data->next;
      data->next = 0;
    }
    return data;
  }
  IData *flush()
  {
    IData *data = this->first;
    this->first = 0;
    return data;
  }
  operator bool() {
    return this->first != 0;
  }
};

inline void webx::IAttributs::IVisitor::visitInt(const char *name, int64_t value)
{
  char tmp[64];
  this->visitString(name, _i64toa(value, tmp, 10));
}
inline void webx::IAttributs::IVisitor::visitFloat(const char *name, double value)
{
  char tmp[64];
  _gcvt_s(tmp, 64, value, 64);
  this->visitString(name, tmp);
}

template <class T>
class NoAttributs : public T
{
public:
  virtual int32_t getAttributCount() override
  {
    return 0;
  }
  virtual bool visitAttributs(webx::IAttributs::IVisitor *visitor) override
  {
    return false;
  }
  virtual bool hasAttribut(const char *name) override
  {
    return false;
  }
  virtual bool removeAttribut(const char *name) override
  {
    return false;
  }
  virtual int64_t getAttributInt(const char *name) override
  {
    return _atoi64(this->getAttributString(name));
  }
  virtual bool setAttributInt(const char *name, int64_t value) override
  {
    return false;
  }
  virtual double getAttributFloat(const char *name) override
  {
    _CRT_DOUBLE value;
    _atodbl(&value, (char *)this->getAttributString(name));
    return value.x;
  }
  virtual bool setAttributFloat(const char *name, double value) override
  {
    return false;
  }
  virtual const char *getAttributString(const char *name) override
  {
    return "";
  }
  virtual bool setAttributString(const char *name, const char *value, int size = -1) override
  {
    return false;
  }
  void printAttributs()
  {
    struct Visitor : public IAttributs::IVisitor
    {
      virtual void visitInt(const char *name, int64_t value) override
      {
        printf("%s: %ld\n", name, value);
      }
      virtual void visitFloat(const char *name, double value) override
      {
        printf("%s: %lg\n", name, value);
      }
      virtual void visitString(const char *name, const char *value) override
      {
        printf("%s: %s\n", name, value);
      }
    };
    this->visitAttributs(&Visitor());
  }
};

template <class T>
class StringMapBasedAttributs : public NoAttributs<T>
{
public:
  typedef std::map<std::string, std::string> tAttributs;

  tAttributs attributs;

  virtual bool hasAttribut(const char *name) override
  {
    return this->attributs.find(name) != this->attributs.end();
  }
  virtual int getAttributCount() override
  {
    return this->attributs.size();
  }
  virtual bool visitAttributs(webx::IAttributs::IVisitor *visitor) override
  {
    tAttributs::const_iterator end = this->attributs.end();
    for (tAttributs::const_iterator it = this->attributs.begin(); it != end; ++it)
    {
      visitor->visitString(it->first.c_str(), it->second.c_str());
    }
    return true;
  }
  virtual const char *getAttributString(const char *name) override
  {
    return this->attributs[name].c_str();
  }
  virtual bool setAttributString(const char *name, const char *value, int size) override
  {
    if (size < 0)
      this->attributs[name] = value;
    else
      this->attributs[name] = std::string(value, size);
    return true;
  }
  void print()
  {
    tAttributs::const_iterator end = this->attributs.end();
    for (tAttributs::const_iterator it = this->attributs.begin(); it != end;
         ++it)
    {
      std::cout << it->first << ": " << it->second << "\n";
    }
  }
};

inline webx::IData *webx::IData::New(int size)
{
  class Data : public NoAttributs<IData>
  {
  public:
    char buffer[1];
    Data(int size)
    {
      this->next = 0;
      this->from = 0;
      this->size = size;
      this->bytes = this->buffer;
    }
    virtual void release() override
    {
      if (this->next) this->next->release();
      ::free(this);
    }
  };
  return new (::malloc(sizeof(IData) + size)) Data(size);
}
