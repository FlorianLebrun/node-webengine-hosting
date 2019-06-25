import { debug } from "./common"
import addon from ".."

export class WebxSessionBase {
  name: string
  handle: any = null //addon.WebxSession

  disconnect() {
    this.handle.close()
    this.handle = null
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
    if (!this.handle) {
      this.engine = engine
      this.name = name
      this.handle = new addon.WebxSession(
        engine.handle,
        type,
        name,
        __handleEvent.bind(this)
      )
    }
    else throw new Error("WebxSession.connect invalid")
  }
}

export class WebxEngine extends WebxSessionBase {
  options: any

  connect(options) {
    if (!this.handle) {
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
        __handleEvent.bind(this)
      )

      // Close engine on exit
      process.on('beforeExit', (code) => {
        this.handle && this.handle.disconnect();
      })
    }
    else throw new Error("WebxEngine.connect invalid")
  }
}

function __handleEvent(type: string, data: any) {
  return this.onEvent(type, data)
}
