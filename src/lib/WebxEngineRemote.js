import { debug } from "@common"
import child_process from "child_process"
import httpProxy from "http-proxy"
import Path from "path"

function NotOnline(req, res) { res.status(404).send("Application not online") }

export class WebxEngineRemote {
  name: string = "(remote)"
  proxy = null

  getName() {
    return this.name
  }
  connect(options: Object): Promise {
    if (this.child) throw new Error("Webx engine already connected")

    this.child = child_process.fork(Path.dirname(__filename) + "../../../dist/lib/remote_child.js", [], {
      cwd: process.cwd(),
      stdio: [0, 1, 2, "ipc"],
      //execArgv: ["--inspect"],
      execArgv: [],
    })
    this.child.on("message", this._ipc.bind(this))
    this.child.send({ ...options, type: "webx-connect" })
  }
  disconnect() {
    if (!this.child) throw new Error("Webx engine not connected")
    this.child.kill()
    this.child = null
    this.proxy = null
  }
  dispatch = (req, res, next) => {
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
  _ipc_webx_listen(msg) {
    console.log(msg)
    this.name = msg.engine
    this.proxy = httpProxy.createProxyServer({
      target: msg.address,
      ws: true,
    })
  }
  _ipc(msg) {
    console.log(" > parent:", msg.type, msg.id !== undefined ? msg.id : "")
    switch (msg.type) {
      case "webx-listen":
        return this._ipc_webx_listen(msg)
      default:
        console.log("IPC receive unsupported messsage with type: ", msg.type)
    }
  }
}
