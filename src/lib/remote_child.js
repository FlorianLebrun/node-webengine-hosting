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
  webxEngine.connect(msg)

  const app = express()
  app.use(webxEngine.dispatch.bind(webxEngine))

  const server = app.listen(0, function () {
    const { port } = server.address()
    process.send({
      type: "webx-listen",
      engine: webxEngine.getName(),
      address: "http://localhost:" + port,
      host: "localhost",
      port,
    })
  })
  server.on("upgrade", function (req, socket, head) {
    debug.warning(req.method, req.url)
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
