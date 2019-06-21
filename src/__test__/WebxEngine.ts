import { debug } from "./common"
import addon from ".."

export class WebxSessionBase {
  name: string
  readyState: boolean = false
  handle: any = null //addon.WebxSession

  disconnect() {
    if (this.readyState) {
      this.readyState = false
      this.handle.close()
    }
    else throw new Error("WebxSession.disconnect invalid")
  }
  onStartup(data: any) {
    debug.info(`\n[${this.name}] Session.startup`)
  }
  onEvent(type: string, data: any) {
    debug.success(`\n[event] ${type}     -- on '${this.name}'`)
  }
  onTerminate(data: any) {
    debug.info(`\n[${this.name}] Session.exit`)
  }
  dispatch(req, res, callback) {
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

export class WebxSession extends WebxSessionBase {
  engine: WebxEngine

  connect(type: string, name: string, engine: WebxEngine) {
    if (!this.handle && !this.readyState) {
      this.engine = engine
      this.name = name
      this.handle = new addon.WebxSession(
        engine.handle,
        type,
        name,
        WebxSession__handleEvent.bind(this)
      )
    }
    else throw new Error("WebxSession.connect invalid")
  }
}

export class WebxEngine extends WebxSessionBase {
  options: any

  connect(options) {
    if (!this.handle && !this.readyState) {
      this.options = options
      this.name = options.name || "admin"

      // Update process environement
      options.cd && process.chdir(options.cd)
      for (const key in options.envs) {
        process.env[key] = options.envs[key]
      }
      if (Array.isArray(options.paths)) {
        process.env["PATH"] = process.env["PATH"] + ";" + options.paths.join(";")
      }

      // Open engine
      this.handle = new addon.WebxEngine(
        options.entrypoint.module,
        options.entrypoint.name,
        options.configuration ? JSON.stringify(options.configuration, null, 2) : "{}",
        WebxEngine__handleEvent.bind(this)
      )

      // Close engine on exit
      process.on('beforeExit', (code) => {
        this.handle && this.handle.disconnect();
      })
    }
    else throw new Error("WebxEngine.connect invalid")
  }
  onRuntimeStartup(data: any) {
    debug.warning(`[Runtime.startup]`)
  }
  onRuntimeTerminate(data: any) {
    debug.warning(`[Runtime.exit]`)
  }
}

function WebxSession__handleEvent(type: string, data: any) {
  switch (type) {
    case "/runtime/session/startup":
      this.readyState = true
      this.name = this.handle.getName()
      return this.onStartup(data)
    case "/runtime/session/exit":
      this.readyState = false
      return this.onTerminate(data)
    default:
      return this.onEvent(type, data)
  }
}

function WebxEngine__handleEvent(type: string, data: any) {
  switch (type) {
    case "/runtime/startup":
      return this.onRuntimeStartup(data)
    case "/runtime/exit":
      this.readyState = false
      this.handle = null
      return this.onRuntimeTerminate(data)
    default:
      return WebxSession__handleEvent.call(this, type, data)
  }
}