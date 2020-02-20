var Path = require("path");
var Module = require('module');
var Process = require('process');
var baseDir = process.cwd()

var std_require = Module.prototype.require
Module.prototype.require = function (path) {
  console.assert(arguments.length === 1)
  return std_require.call(this, path)
}

try {
  const modulePath = Path.resolve(baseDir, Process.argv[2])
  require(modulePath)
}
catch (e) {
  console.error("\x1b[1m\x1b[31m", e, "\x1b[0m")
  Process.exit(-1)
}
