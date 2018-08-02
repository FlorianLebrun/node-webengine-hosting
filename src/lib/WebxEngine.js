import { debug } from "@common"
const addon = require("bindings")("addon")

export class WebxSession {
  readyState: boolean = false
  handle: addon.WebxSession = null
  engine: WebxEngine

  get name() {
    return this.handle && this.handle.getName()
  }
  connect(name: string, options: Object, engine: WebxEngine) {
    if (!this.handle && !this.readyState) {
      this.engine = engine
      this.handle = new addon.WebxSession(
        engine.handle,
        name,
        JSON.stringify(options),
        WebxSession__handleEvent.bind(this)
      )
    }
    else throw new Error("WebxSession.connect invalid")
  }
  disconnect() {
    if (this.readyState) {
      debug.info("WebxSession.disconnect", this.name)
      this.readyState = false
      this.handle.close()
    }
    else throw new Error("WebxSession.disconnect invalid")
  }
  onStartup(data: any) {
    throw new Error("WebxEngine.onStartup shall be overriden")
  }
  onEvent(type: string, data: any) {
    throw new Error("WebxEngine.onEvent shall be overriden")
  }
  onTerminate(data: any) {
    throw new Error("WebxEngine.onTerminate shall be overriden")
  }
  dispatch(req, res) {
    if (req.upgrade) {
      const stream = new addon.WebxWebSocketStream(this.handle, req, (data) => {
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
      const transaction = new addon.WebxHttpTransaction(this.handle, req,
        function handleSend(status, headers, buffer) {
          res.set(headers).status(status).send(buffer)
        },
        function handleChunk(bufferOut) {
          res.write(bufferOut)
        },
        function handleEnd() {
          res.end()
        }
      )
      req.on("data", function handleChunk(bufferIn) {
        transaction.write(bufferIn)
      })
      req.on("end", function handleEnd() {
        transaction.close()
      })
    }
  }
}

export class WebxEngine {
  readyState: boolean = false
  handle: addon.WebxEngine = null

  get name() {
    return this.handle && this.handle.getName()
  }
  connect(options) {
    if (!this.handle && !this.readyState) {
      this.options = options

      // Update process environement
      options.cd && process.chdir(options.cd)
      for (const key in options.envs) {
        process.env[key] = options.envs[key]
      }

      // Open engine
      this.handle = new addon.WebxEngine(
        options.dll.path,
        options.dll.entryName,
        JSON.stringify(options.config),
        WebxEngine__handleEvent.bind(this)
      )
    }
    else throw new Error("WebxEngine.connect invalid")
  }
  disconnect() {
    if (this.readyState) {
      debug.info("WebxEngine.disconnect", this.name)
      this.readyState = false
      this.handle.close()
    }
    else throw new Error("WebxEngine.disconnect invalid")
  }
  onRuntimeStartup(data: any) {
    throw new Error("WebxEngine.onRuntimeStartup shall be overriden")
  }
  onRuntimeTerminate(data: any) {
    throw new Error("WebxEngine.onRuntimeStartup shall be overriden")
  }
  onStartup(data: any) {
    throw new Error("WebxEngine.onStartup shall be overriden")
  }
  onEvent(type: string, data: any) {
    throw new Error("WebxEngine.onEvent shall be overriden")
  }
  onTerminate(data: any) {
    throw new Error("WebxEngine.onTerminate shall be overriden")
  }
}

function WebxSession__handleEvent(type: string, data: any) {
  switch (type) {
    case "Session.startup":
      debug.info("[Session.startup]", this.name)
      this.readyState = true
      return this.onStartup(data)
    case "Session.terminate":
      this.readyState = false
      debug.info("[Session.terminate]", this.name)
      return this.onTerminate(data)
    default:
      console.log("[", type, "]", data && JSON.stringify(data))
      return this.onEvent(type, data)
  }
}

function WebxEngine__handleEvent(type: string, data: any) {
  switch (type) {
    case "Runtime.startup":
      debug.info("[Runtime.startup]", this.name)
      return this.onRuntimeStartup(data)
    case "Runtime.terminate":
      this.readyState = false
      this.handle = null
      debug.info("[Runtime.terminate]", this.name)
      return this.onRuntimeTerminate(data)
    default:
      return WebxSession__handleEvent.call(this, type, data)
  }
}
