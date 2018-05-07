import { debug } from "@common"
import express from "express"

const addon = require("bindings")("addon")

function NotOnline(req, res) { res.status(404).send("Application not online") }

export class WebxEngine {
  host: addon.WebxEngineHost = null
  router: express.Router = null

  getName() {
    return this.host && this.host.getName()
  }
  route(): Router {
    if (!this.router && this.host) {
      this.router = express.Router()
      this.router.use(this.__middleware)
    }
    return this.router
  }
  dispatch(req, res) {
    const router = this.route()
    if (router) router.handle(req, res, NotOnline)
    else NotOnline(req, res)
  }
  connect(dllPath: string, dllEntryName: string, config: Object): Promise {
    if (this.host) throw new Error("Webx engine already connected")
    this.host = new addon.WebxEngineHost()
    this.host.connect(dllPath, dllEntryName, JSON.stringify(config))
  }
  disconnect() {

  }
  __middleware = (req, res, next) => {
    if (req.method === "WEBSOCKET") {
      const accept = res._websocket.cb
      const info = res._websocket.info
      debug.info("websocketMiddleware", req.url)
      accept((ws) => {
        ws.send("hello in here")
        const stream = new addon.WebxWebSocketStream(this.host, req, (data) => {
          console.log("server send", data)
          ws.send(data)
        }, () => {
          console.log("WebSocket closed by server")
        })

        ws.on("message", (msg) => {
          //ws.send("recv: " + msg)
          stream.write(msg)
        })

        ws.on("close", () => {
          stream.close()
          console.log("WebSocket closed")
        })
      })
    }
    else {
      console.log("websocketMiddleware")
      const transaction = new addon.WebxHttpTransaction(this.host, req, (status, headers, buffer) => {
        res.set(headers).status(status).send(buffer)
      })

      req.setEncoding("utf8")

      req.on("data", function (chunk) {
        transaction.write(chunk)
      })

      req.on("end", () => {
        transaction.close()
      })
    }
  }
}
