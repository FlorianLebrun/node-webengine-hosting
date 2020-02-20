import process from "process"
import express from "express"
import cookieParser from "cookie-parser"
import { WebxRouter } from "./WebxRouter"

const options = {
  name: "test",
  entrypoint: {
    module: "my_webagent.dll",
    name: "connect",
  }
}
const router = new WebxRouter()
router.connect(options)

const app = express()
app.use(cookieParser())
app.use(router.route.bind(router))
const server = app.listen(9944, function () {
  const port = server.address().port
  console.log("Process " + process.pid + " is listening on " + port)
})
