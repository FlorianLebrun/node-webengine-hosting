import { debug } from "@common"
import express from "express"
import { WebxEngine } from "../lib/WebxEngine"
import { WebsocketResponse } from "./WebsocketResponse"

const modules = {}

function ipc_require(msg) {
  modules[msg.name] = require(msg.path)
}

function ipc_webx_connect(msg) {
  const webxEngine = new WebxEngine()

  const app = express()
  app.use(webxEngine.dispatch)

  webxEngine.addEventListener((type, data) => {
    process.send({ type: "webx-event", event: type, data })
  })

  const server = app.listen(0, function () {
    webxEngine.connect(msg, () => {
      const { port } = server.address()
      process.send({
        type: "webx-listen",
        engine: webxEngine.getName(),
        address: "http://localhost:" + port,
        host: "localhost",
        port,
      })
    })
  }).on("upgrade", function (req, socket, head) {
    app.handle(req, WebsocketResponse(req, socket, head))
  })
}

function ipc_exec(msg) {
  const module = modules[msg.name]
  module[msg.method].apply(module, msg.args)
}

process.on("message", function (msg) {
  console.log(" > child:", msg.type, msg.id !== undefined ? msg.id : "")
  switch (msg.type) {
    case "exec":
      return ipc_exec(msg)
    case "require":
      return ipc_require(msg)
    case "webx-connect":
      return ipc_webx_connect(msg)
    default:
      console.log("IPC receive unsupported messsage with type: ", msg.type)
  }
})
