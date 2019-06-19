import crypto from "crypto"
import express from "express"
import { fs, debug } from "../common"
import { WebxEngine, WebxSession } from "../lib"


export class WebxSessionWithTimeOut extends WebxSession {
  sessionID: string
  lastActivityTime: number
  pendings: any[]// Array<HttpRequest | HttpResponse>
  workings: number
  engine: WebxRouter

  constructor(sessionID: string) {
    super()
    this.sessionID = sessionID
    this.lastActivityTime = Date.now()
    this.pendings = null
    this.workings = 0
  }
  onStartup(data: any) {
    super.onStartup(data)
    if (this.pendings) {
      const pendings = this.pendings
      this.pendings = null
      while (pendings.length) {
        const res = pendings.pop()
        const req = pendings.pop()
        this.route(req, res)
      }
    }
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
    if (this.readyState) {
      this.workings++
      this.dispatch(req, res, () => {
        this.lastActivityTime = Date.now()
        this.workings--
      })
    }
    else {
      if (!this.pendings) this.pendings = []
      this.pendings.push(req)
      this.pendings.push(res)
    }
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
    let session = null
    let sessionID: string = req.cookies["sessionID"]
    session = sessionID && this.sessions[sessionID]
    if (!session) {
      sessionID = this.generateSessionID()
      session = new WebxSessionWithTimeOut(sessionID)
      session.connect(sessionID, this.options.sessionsConfig, this)
      res.cookie("sessionID", sessionID)
      this.sessions[sessionID] = session
    }
    session.route(req, res, next)
  }
}

console.log("hello")
const options = fs.readJsonSync("./webx.options.json")
const router = new WebxRouter()
router.connect(options)

const app = express()
app.use(router.route.bind(router))
const server = app.listen(9944, function () {
  const port = server.address().port
  console.log("Process " + process.pid + " is listening on " + port)
})
