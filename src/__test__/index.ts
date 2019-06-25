import crypto from "crypto"
import process from "process"
import express from "express"
import cookieParser from "cookie-parser"
import { fs, debug } from "./common"
import { WebxEngine, WebxSession } from "./WebxEngine"

process.env.NODE_ENV = "development"

export class WebxSessionWithTimeOut extends WebxSession {
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

export class WebxRouter extends WebxEngine {
  sessions: { [id: string]: WebxSessionWithTimeOut } = {}
  sessionsTimeout: number

  connect(options) {
    this.sessionsTimeout = options.sessionsTimeout || 1000
    this.checkSessionTimeouts()
    super.connect(options)
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
  onRuntimeTerminate(data: any) {
    process.exit(0)
  }
  route(req, res, next) {
    const { url } = req
    const sessionTypeLength = url.indexOf("/", 1)
    const sessionType = req.url.substring(1, sessionTypeLength)
    const sessionConfig = options.configuration.sessions[sessionType]
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
        session = new WebxSessionWithTimeOut(sessionType)
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
        session = new WebxSessionWithTimeOut(sessionID)
        session.connect(sessionType, sessionID, this)
        res.cookie(cookieName, sessionID)
        this.sessions[sessionID] = session
      }
      session.route(req, res, next)
    }
  }
}

const options = fs.readJsonSync("./webx.options.json")
const router = new WebxRouter()
router.connect(options)

const app = express()
app.use(cookieParser())
app.use(router.route.bind(router))
const server = app.listen(9944, function () {
  const port = server.address().port
  console.log("Process " + process.pid + " is listening on " + port)
})
