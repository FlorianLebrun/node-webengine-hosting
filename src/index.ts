import os from "os"
import fs from "fs"
import Path from "path"

let addon: any = {}
if (process.env.NODE_ENV === "development") {
  addon = require("bindings")("addon")
}
else {
  const addonName = `./addon-${os.platform()}-${os.arch()}.node`
  const addonPath = Path.resolve(__dirname, addonName)
  if (fs.existsSync(addonPath)) {
    addon = require(addonPath)
  }
  else {
    console.log(`\x1b[1m\x1b[31m'node-webengine-hosting' has no build for the platform '${addonName}' \x1b[0m`)
  }
}

export default addon