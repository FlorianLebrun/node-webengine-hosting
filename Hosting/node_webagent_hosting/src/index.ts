import os from "os"
import fs from "fs"
import Path from "path"

const addonName = `node_webagent_hosting_${process.versions.modules}_${os.arch()}`
const addonPath = Path.resolve(__dirname, `./addons/${addonName}/${addonName}.node`)

let addon: any = {}
if (fs.existsSync(addonPath)) addon = require(addonPath)
else console.log(`\x1b[1m\x1b[31m'node-webengine-hosting' has no build for the platform '${addonName}' \x1b[0m`)
export default addon
