import { debug } from "@common"
const addon = require("bindings")("addon")

export class WebxSession {
  name: string
  readyState: boolean = false
  handle: addon.WebxSession = null
  engine: WebxEngine

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
      this.readyState = false
      this.handle.close()
    }
    else throw new Error("WebxSession.disconnect invalid")
  }
  onStartup(data: any) {
    debug.info(`[${this.name}] Session.startup`)
  }
  onEvent(type: string, data: any) {
    debug.title(`[${this.name}] ${type} ${(data && JSON.stringify(data)) || ""}`)
  }
  onTerminate(data: any) {
    debug.info(`[${this.name}] Session.exit`)
  }
  dispatch(req, res, callback) {
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
          callback && callback()
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
          callback && callback()
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
  name: string
  readyState: boolean = false
  handle: addon.WebxEngine = null

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

      // Close engine on exit
      process.on('beforeExit', (code) => {
        this.handle && this.handle.disconnect();
      })
    }
    else throw new Error("WebxEngine.connect invalid")
  }
  disconnect() {
    if (this.readyState) {
      this.readyState = false
      this.handle.close()
    }
    else throw new Error("WebxEngine.disconnect invalid")
  }
  onRuntimeStartup(data: any) {
    debug.warning(`[Runtime.startup]`)
  }
  onRuntimeTerminate(data: any) {
    debug.warning(`[Runtime.exit]`)
  }
  onStartup(data: any) {
    debug.info(`[${this.name}] Session.startup`)
  }
  onEvent(type: string, data: any) {
    debug.title(`[${this.name}] ${type} ${(data && JSON.stringify(data)) || ""}`)
  }
  onTerminate(data: any) {
    debug.info(`[${this.name}] Session.exit`)
  }
}

function WebxSession__handleEvent(type: string, data: any) {
  switch (type) {
    case "Session.startup":
      this.readyState = true
      this.name = this.handle.getName()
      return this.onStartup(data)
    case "Session.exit":
      this.readyState = false
      return this.onTerminate(data)
    default:
      return this.onEvent(type, data)
  }
}

function WebxEngine__handleEvent(type: string, data: any) {
  switch (type) {
    case "Runtime.startup":
      return this.onRuntimeStartup(data)
    case "Runtime.exit":
      this.readyState = false
      this.handle = null
      return this.onRuntimeTerminate(data)
    default:
      return WebxSession__handleEvent.call(this, type, data)
  }
}
