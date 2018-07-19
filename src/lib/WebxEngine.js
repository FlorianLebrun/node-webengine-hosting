import { debug } from "@common"

const addon = require("bindings")("addon")

function NotOnline(req, res) { res.status(404).send("Application not online") }

export class WebxSession {
  handle: addon.WebxSession = null
  engine: WebxEngine
  listeners = []
  onReady: Function

  constructor(engine: WebxEngine, onReady: Function) {
    this.engine = engine
    this.onReady = onReady
  }
  get name() {
    return this.handle.getName()
  }
  openSession(name: string, options: Object) {
    this.handle = new addon.WebxSession(
      this.engine.handle,
      name,
      JSON.stringify(options.config),
      this.handleEvent.bind(this)
    )
    return this
  }
  openMainSession(options: Object) {
    this.handle = new addon.WebxMainSession(
      this.engine.handle,
      JSON.stringify(options.config),
      this.handleEvent.bind(this)
    )
    return this
  }
  addEventListener(callback: Function) {
    this.listeners.push(callback)
  }
  removeEventListener(callback: Function) {
    this.listeners && this.listeners.splice(this.listeners.indexOf(callback), 1)
  }
  handleEvent(type: string, data: any) {
    switch (type) {
      case "Session.startup":
        debug.info("[Session.startup]", this.name)
        this.readyState = true
        this.onReady(this)
        break
      case "Session.terminate":
        this.readyState = false
        debug.info("[Session.terminate]", this.name)
        break
      default:
        console.log("[", type, "]", data && JSON.stringify(data))
    }
    for (const cb of this.listeners) {
      cb(type, data)
    }
  }
  dispatch(req, res, next) {
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
        function (status, headers) {
          res.set(headers).status(status)
        }, function (bufferOut) {
          res.write(bufferOut)
        }, function () {
          res.end()
        })

      req.on("data", function (bufferIn) {
        transaction.write(bufferIn)
      })

      req.on("end", () => {
        transaction.close()
      })
    }
  }
}

export class WebxEngine {
  handle: addon.WebxEngine
  mainSession: WebxSession = null
  listeners = []
  onReady: Function

  constructor(options: Object, onReady: Function): Promise {
    this.options = options
    this.onReady = onReady

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
      this.handleEvent.bind(this)
    )
  }
  get name() {
    return this.handle.getName()
  }
  createMainSession(config: Object, callback: Function): WebxSession {
    if (!this.mainSession) {
      this.mainSession = new WebxSession(this, callback)
      this.mainSession.openMainSession(config)
      return this.mainSession
    }
    else {
      throw new Error("main session already exists")
    }
  }
  createSession(name: string, config: Object, callback: Function): WebxSession {
    const session = new WebxSession(this, callback)
    session.openSession(name, config)
    return session
  }
  addEventListener(callback: Function) {
    this.listeners.push(callback)
  }
  removeEventListener(callback: Function) {
    this.listeners && this.listeners.splice(this.listeners.indexOf(callback), 1)
  }
  handleEvent(type: string, data: any) {
    switch (type) {
      case "Runtime.startup":
        debug.info("[Runtime.startup]", this.name)
        this.readyState = true
        this.onReady(this)
        break
      case "Runtime.terminate":
        this.readyState = false
        debug.info("[Runtime.terminate]", this.name)
        break
      default:
        console.log("[", type, "]", data && JSON.stringify(data))
    }
    for (const cb of this.listeners) {
      cb(type, data)
    }
  }
  dispatch(req, res, next) {
    if (!this.mainSession) {
      this.mainSession.dispatch(req, res, next)
    }
    else {
      NotOnline(req, res, next)
    }
  }
}

export function CreateWebxEngine(options: Object, callback: Function): WebxEngine {
  return new WebxEngine(options, callback)
}
