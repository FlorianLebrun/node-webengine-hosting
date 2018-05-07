import http, { ServerResponse } from 'http'
import { Server as WebSocketServer } from 'ws'

export default function (app, options) {

  app.listen = function () {
    var server = http.createServer(this);
    server.wsServer = new WebSocketServer({
      server,
      verifyClient: function (info, cb) {
        var reqUrl = info.req.originalUrl || info.req.url;
        var handled = false;
        var res = new ServerResponse(info.req);

        // Close the connection if attempting to send a normal HTTP response
        res.writeHead = function (statusCode) {
          if (handled) {
            console.log('writing response headers for already handled request for %s', reqUrl);
          }
          else {
            console.log('rejected due to writeHead %s for %s', statusCode, reqUrl);
            handled = true;
            cb(false, statusCode);
          }
        };

        res._websocket = {
          info: info,
          cb: routeCb
        };

        console.log('verifying websocket connection for %s', reqUrl);

        // Set the fake HTTP methpd
        info.req.method = "websocket".toUpperCase();

        // Route the request through the application
        app.handle(info.req, res, function () {
          // Close the connection if unhandled
          if (!handled) {
            console.log('unhandled websocket connection for %s', reqUrl);
            handled = true;
            cb(false, 404);
          }
        });

        function routeCb(connectHandler) {
          if (handled) {
            console.log('cb called for already handled request for %s', reqUrl);
            throw new Error('websocket already handled');
          }

          handled = true;
          if (!connectHandler) {
            console.log('rejected websocket connection for %s', reqUrl);
            cb.apply(null, arguments);
          }
          else if (typeof connectHandler !== 'function') {
            console.log('cb called with non-function for %s', reqUrl);
            cb(false, 500);
            throw new Error('Web socket route must pass a function when accepting a connection');
          }
          else {
            console.log('accepted websocket connection for %s', reqUrl);
            info.req._websocketHandler = connectHandler;
            cb(true);
          }
        }
      }
    })
      .on('connection', function (webSocket) {
        var handler = webSocket.upgradeReq._websocketHandler;
        delete webSocket.upgradeReq._websocketHandler;

        // TODO: catch error?
        handler(webSocket, webSocket.upgradeReq);
      })

    return server.listen.apply(server, arguments);
  };

  return app;
}
