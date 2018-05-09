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
  connect(options: Object): Promise {
    if (this.host) throw new Error("Webx engine already connected")
    options.cd && process.chdir(options.cd)
    for (const key in options.envs) {
      process.env[key] = options.envs[key]
    }
    this.host = new addon.WebxEngineHost()
    this.host.connect(options.dll.path, options.dll.entryName, JSON.stringify(options.config))
  }
  disconnect() {

  }
  dispatch = (req, res, next) => {
    debug.info(req.method, req.url)
    if (!this.host) {
      NotOnline(req, res, next)
    }
    else if (req.upgrade) {
      const stream = new addon.WebxWebSocketStream(this.host, req, (data) => {
        const ws = res.accept()
        ws.send("hello in here")

        ws.on("message", (msg) => {
          //ws.send("recv: " + msg)
          stream.write(msg)
        })

        ws.on("close", () => {
          stream.close()
          console.log("WebSocket closed")
        })

        stream.on("message", (data) => {
          console.log("server send", data)
          ws.send(data)
        })

        stream.on("close", (data) => {
          ws.close()
          console.log("WebSocket closed by server")
        })

      }, (code) => {
        res.status(code).reject()
      })

    }
    else {
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
