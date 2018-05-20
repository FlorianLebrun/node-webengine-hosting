import { debug } from "@common"
import express from "express"

const addon = require("bindings")("addon")

function NotOnline(req, res) { res.status(404).send("Application not online") }

export class WebxEngine {
  host: addon.WebxEngineHost = null
  router: express.Router = null
  connecting: Array = null
  listeners: Array = []

  getName() {
    return this.host && this.host.getName()
  }
  addEventListener(callback: Function) {
    this.listeners.push(callback)
  }
  removeEventListener(callback: Function) {
    this.listeners.splice(this.listeners.indexOf(callback), 1)
  }
  connect(options: Object, callback: Function): Promise {
    if (!this.host && !this.connecting) {
      options.cd && process.chdir(options.cd)
      for (const key in options.envs) {
        process.env[key] = options.envs[key]
      }
      this.host = new addon.WebxEngineHost(this.handleEvent)
      this.host.connect(options.dll.path, options.dll.entryName, JSON.stringify(options.config))
      this.connecting = []
    }
    if (callback) {
      if (!this.connecting) callback()
      else this.connecting.push(callback)
    }
  }
  disconnect() {

  }
  handleEvent = (type: string, data: any) => {
    switch (type) {
      case "Runtime.startup":
        this.connecting && this.connecting.forEach(cb => cb())
        this.connecting = null
        return
      case "Runtime.terminate":
        debug.info("[engine-terminate]")
        this.connecting && this.connecting.forEach(cb => cb("connection failed: " + data))
        this.connecting = null
        this.host = null
    }
    for (const cb of this.listeners) {
      cb(type, data)
    }
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
