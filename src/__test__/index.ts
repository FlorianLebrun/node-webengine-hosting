import crypto from "crypto"
import express from "express"
import cookieParser from "cookie-parser"
import { fs, debug } from "../common"
import { WebxEngine, WebxSession } from "../lib"


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
  checkValidity(time) {
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
  sessions: { [sessinId: string]: WebxSessionWithTimeOut }
  sessionsTimeout: number
  sessionsMax: number
  options: any

  connect(options) {
    this.sessionsTimeout = options.sessionsTimeout || 600000
    this.sessionsMax = options.sessionsMax || 20
    this.sessions = {}
    this.garbageSessions()
    super.connect(options)
  }
  generateSessionID(): string {
    var sha = crypto.createHash('sha256')
    sha.update(Math.random().toString())
    return sha.digest('hex').substr(0, 8)
  };
  garbageSessions = () => {
    const time = Date.now()
    for (const key in this.sessions) {
      this.sessions[key].checkValidity(time)
    }
    setTimeout(this.garbageSessions, this.sessionsTimeout / 10)
  }
  onRuntimeTerminate(data: any) {
    process.exit(0)
  }
  route(req, res, next) {
    const { url } = req
    const sessionTypeLength = url.indexOf("/", 1)
    const sessionType = req.url.substring(1, sessionTypeLength)
    req.url = url.substr(sessionTypeLength)

    if (sessionType === "admin") {
      this.dispatch(req, res, next)
    }
    else if (options.configuration.sessions[sessionType]) {
      const cookieName = "SID-" + sessionType
      let session = null
      let sessionID: string = req.cookies[cookieName]
      session = sessionID && this.sessions[sessionID]
      if (!session) {
        sessionID = sessionType + "-" + this.generateSessionID()
        session = new WebxSessionWithTimeOut(sessionID)
        session.connect(sessionType, sessionID, this)
        res.cookie(cookieName, sessionID)
        this.sessions[sessionID] = session
      }
      session.route(req, res, next)
    }
    else {
      next()
    }
  }
}

console.log("hello")
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
