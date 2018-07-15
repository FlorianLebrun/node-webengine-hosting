import { debug } from "@common"
import express from "express"
import { CreateWebxEngine, WebxEngine } from "../lib/WebxEngine"
import { WebsocketResponse } from "./WebsocketResponse"

const modules = {}

function ipc_require(msg) {
  modules[msg.name] = require(msg.path)
}

function ipc_webx_connect(config) {
  CreateWebxEngine(config, (engine: WebxEngine) => {
    engine.createMainSession(config, (session: WebxSession) => {
      openServer(engine, session)
    })
  })
}

function ipc_exec(msg) {
  const module = modules[msg.name]
  module[msg.method].apply(module, msg.args)
}

function openServer(engine, session) {
  const app = express()
  const server = app.listen(0, function () {
    try {
      app.use(session.dispatch.bind(session))

      session.addEventListener((type, data) => {
        process.send({ type: "webx-event", event: type, data })
      })

      const { port } = server.address()
      process.send({
        type: "webx-listen",
        engine: engine.name,
        address: "http://localhost:" + port,
        host: "localhost",
        port,
      })
    }
    catch (e) {
      console.error(e)
    }
  }).on("upgrade", function (req, socket, head) {
    app.handle(req, WebsocketResponse(req, socket, head))
  })
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
