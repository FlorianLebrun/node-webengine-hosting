import { debug } from "@common"
import child_process, { ChildProcess } from "child_process"
import httpProxy from "http-proxy"
import Path from "path"

function NotOnline(req, res) { res.status(404).send("Application not online") }

export class WebxEngineRemote {
  host: ChildProcess
  name: string = "(remote)"
  proxy: HttpProxy = null
  connecting: Array = null
  listeners: Array = []

  getName() {
    return this.name
  }
  addEventListener(callback: Function) {
    callback && this.listeners.push(callback)
  }
  removeEventListener(callback: Function) {
    callback && this.listeners.splice(this.listeners.indexOf(callback), 1)
  }
  connect(options: Object, callback: Function): Promise {
    if (!this.host && !this.connecting) {
      this.host = child_process.fork(Path.dirname(__filename) + "../../../dist/lib/remote_child.js", [], {
        cwd: process.cwd(),
        stdio: ["ipc"],
        //execArgv: ["--inspect"],
        execArgv: [],
      })
      this.host.stdout.on('data', (data) => {
        this.handleEvent("stdout", data.toString())
      });
      this.host.on("message", this.handleMessage)
      this.host.on("exit", this.handleExit)
      this.host.send({ ...options, type: "webx-connect" })
      this.connecting = []
    }
    if (callback) {
      if (!this.connecting) callback()
      else this.connecting.push(callback)
    }
  }
  disconnect() {
    if (this.host) this.host.kill()
    else throw new Error("Webx engine not connected")
  }
  dispatch = (req, res, next) => {
    try {
      if (this.proxy) {
        if (req.upgrade) {
          this.proxy.ws(req, res.socket, res.head)
        }
        else {
          this.proxy.web(req, res)
        }
      }
      else NotOnline(req, res)
    }
    catch (e) {
      res.status(500).send(e.toString())
    }
  }
  handleListen(msg) {
    console.log(msg)
    this.name = msg.engine
    this.proxy = httpProxy.createProxyServer({
      target: msg.address,
      ws: true,
    }).on("error", () => { })
    this.connecting && this.connecting.forEach(cb => cb())
    this.connecting = null
    this.handleEvent("connected", msg)
  }
  handleEvent = (event, data) => {
    for (const cb of this.listeners) {
      cb(event, data)
    }
  }
  handleExit = () => {
    this.host = null
    if (this.proxy) {
      this.proxy.close()
      this.proxy = null
    }
    this.handleEvent("exit")
  }
  handleMessage = (msg) => {
    console.log(" > parent:", msg.type, msg.id !== undefined ? msg.id : "")
    switch (msg.type) {
      case "webx-listen":
        return this.handleListen(msg)
      case "webx-event":
        return this.handleEvent(msg.event, msg.data)
      default:
        console.log("IPC receive unsupported messsage with type: ", msg.type)
    }
  }
}
