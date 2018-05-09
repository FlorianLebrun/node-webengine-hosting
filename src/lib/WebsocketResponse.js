
import { ServerResponse } from 'http'
import crypto from "crypto"

const WebSocket = require("ws")

export function WebsocketResponse(req, socket, head) {
  var res = new ServerResponse(req)
  res.assignSocket(socket)
  res.head = head
  res.reject = res.send = function (chunk) {
    try {
      if (res.statusCode < 400) {
        res.statusCode = 500
      }
      this.end(chunk)
      this.socket.write(this.output.join(""))
    }
    catch (e) { }
    socket.destroy()
  }
  res.accept = function (): WebSocket {
    var key = req.headers['sec-websocket-key'];
    var version = parseInt(req.headers['sec-websocket-version'], 10)

    if (version < 13 || !key) {
      res.status(400).send()
      return
    }

    // calc key
    var shasum = crypto.createHash('sha1');
    shasum.update(key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
    key = shasum.digest('base64');

    var headers = [
      'HTTP/1.1 101 Switching Protocols'
      , 'Upgrade: websocket'
      , 'Connection: Upgrade'
      , 'Sec-WebSocket-Accept: ' + key
    ]

    const protocol = req.headers['sec-websocket-protocol']
    if (protocol) headers.push("Sec-WebSocket-Protocol: " + protocol)

    socket.setTimeout(0);
    socket.setNoDelay(true);
    try {
      socket.write(headers.concat('', '').join('\r\n'));
    }
    catch (e) {
      // if the upgrade write fails, shut the connection down hard
      try { socket.destroy() } catch (e) { }
      return
    }
    return new WebSocket([req, socket, head], {
      protocolVersion: version,
      protocol: protocol,
      //  extensions: extensions,
      //   maxPayload: self.options.maxPayload
    });
    // return new WebSocket(socket)
  }
  return res
}

