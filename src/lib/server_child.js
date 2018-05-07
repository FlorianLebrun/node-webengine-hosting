import express from "express"
import expressWs from "./express-websocket"
import { WebxEngine } from "../lib/WebxEngine"

const modules = {}

function ipc_require(msg) {
  modules[msg.name] = require(msg.path)
}

function ipc_webx_connect(msg) {
  const webxEngine = new WebxEngine()
  webxEngine.connect(msg)

  const app = expressWs(express())
  app.use("/", webxEngine.route())
  const server = app.listen(0, function () {
    process.send({
      type: "webx-listen",
      engine: webxEngine.getName(),
      host: "localhost",
      port: server.address().port,
    })
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
