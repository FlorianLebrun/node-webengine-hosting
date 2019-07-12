import crypto from "crypto"
import process from "process"
import Path from "path"
import { debug } from "./common"
import addon from ".."

const LOCAL_HOST_DIRECTORY = process.cwd()
function relocalizePath(path) {
  path = Path.resolve(path.replace("%LOCAL_HOST_DIRECTORY%", LOCAL_HOST_DIRECTORY))
  console.log(path)
  return path
}

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
  sessionID: string
  lastActivityTime: number
  workings: number
  engine: WebxRouter

  constructor(sessionID: string) {
    super()
    this.sessionID = sessionID
    this.lastActivityTime = Date.now()
    this.workings = 0
  }
  connect(type: string, name: string, engine: WebxRouter) {
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
  checkTimeout(time: number) {
    const inactivityTime = time - this.lastActivityTime
    if (inactivityTime > this.engine.sessionsTimeout) {
      if (this.workings === 0) {
        this.disconnect()
        delete this.engine.sessions[this.sessionID]
      }
    }
  }
  route(req, res, next?: Function) {
    this.workings++
    this.dispatch(req, res, () => {
      this.lastActivityTime = Date.now()
      this.workings--
    })
  }
}

export class WebxRouter extends WebxSessionBase {

  sessions: { [id: string]: WebxSession } = {}
  sessionsTimeout: number
  options: any

  connect(options) {
    if (!this.handle) {
      this.options = options
      this.name = options.name || "admin"
      this.sessionsTimeout = options.sessionsTimeout || (20 * 60 * 1000) /* 20 minutes */

      // Update process environement
      options.cd && process.chdir(relocalizePath(options.cd))
      for (const key in options.envs) {
        process.env[key] = relocalizePath(options.envs[key])
      }
      if (Array.isArray(options.paths)) {
        process.env["PATH"] = process.env["PATH"] + ";" + options.paths.map(relocalizePath).join(";")
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

      // Session cleaner
      this.checkSessionTimeouts()
    }
    else throw new Error("WebxEngine.connect invalid")
  }
  generateSessionID(): string {
    var sha = crypto.createHash('sha256')
    sha.update(Math.random().toString())
    return sha.digest('hex').substr(0, 8)
  };
  checkSessionTimeouts = () => {
    const time = Date.now()
    for (const key in this.sessions) {
      this.sessions[key].checkTimeout(time)
    }
    setTimeout(this.checkSessionTimeouts, this.sessionsTimeout / 10)
  }
  route(req, res, next) {
    const { url } = req
    const sessionTypeLength = url.indexOf("/", 1)
    const sessionType = req.url.substring(1, sessionTypeLength)
    const sessionConfig = this.options.configuration.sessions[sessionType]
    req.url = url.substr(sessionTypeLength)

    if (sessionType === "admin") {
      this.dispatch(req, res, next)
    }
    else if (!sessionConfig) {
      next()
    }
    else if (sessionConfig.type === "stateless") {
      let session = this.sessions[sessionType]
      if (!session) {
        session = new WebxSession(sessionType)
        session.connect(sessionType, sessionType, this)
        this.sessions[sessionType] = session
      }
      session.route(req, res, next)
    }
    else {
      const cookieName = "SID-" + sessionType
      let sessionID = req.cookies[cookieName]
      let session = sessionID && this.sessions[sessionID]
      if (!session) {
        sessionID = sessionType + "-" + this.generateSessionID()
        session = new WebxSession(sessionID)
        session.connect(sessionType, sessionID, this)
        res.cookie(cookieName, sessionID)
        this.sessions[sessionID] = session
      }
      session.route(req, res, next)
    }
  }
}

function __handleEvent(type: string, data: any) {
  return this.onEvent(type, data)
}
