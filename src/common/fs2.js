import fs from "fs"
import Path from "path"

function makeDirSync(path: string) {
  if (arguments.length > 1) path = Path.join.apply(Path, arguments)
  if (path && !fs.existsSync(path)) {
    makeDirSync(Path.parse(path).dir)
    fs.mkdirSync(path)
  }
  return path
}

function removeDirSync(dir_path) {
  if (fs.existsSync(dir_path)) {
    fs.readdirSync(dir_path).forEach(function (entry) {
      var entry_path = Path.join(dir_path, entry);
      if (fs.lstatSync(entry_path).isDirectory()) {
        removeDirSync(entry_path)
      } else {
        fs.unlinkSync(entry_path)
      }
    })
    fs.rmdirSync(dir_path)
  }
}

function makePathSync(path: string) {
  if (arguments.length > 1) path = Path.join.apply(Path, arguments)
  const pars = Path.parse(path)
  return makeDirSync(pars.dir) + Path.sep + pars.base
}

function createTextFileSync(data: any, ...paths: string) {
  fs.writeFileSync(makePathSync(Path.join.apply(Path, paths)), data, 'utf8')
}

function createBinaryFileSync(data: any, ...paths: string) {
  fs.writeFileSync(makePathSync(Path.join.apply(Path, paths)), data, 'binary')
}

function readFileSync() {
  try {
    return fs.readFileSync(Path.join.apply(Path, arguments))
  }
  catch (e) { return null }
}

function writeFileSync(data: any, ...paths: string) {
  fs.writeFileSync(Path.join.apply(Path, paths), data)
}

function readJsonSync() {
  try {
    const data = fs.readFileSync(Path.join.apply(Path, arguments))
    return JSON.parse(data)
  }
  catch (e) { return null }
}

function createJsonSync(data: any, ...paths: string) {
  writeJsonSync(data, makePathSync(Path.join.apply(Path, paths)))
}

function writeJsonSync(object: any, ...paths: string) {
  const data = JSON.stringify(object, null, 2)
  fs.writeFileSync(Path.join.apply(Path, paths), data)
}

function readJsEvalSync() {
  const data = fs.readFileSync(Path.join.apply(Path, arguments))
  const result = eval(`(function(){${data.toString()}})`).call(this)
  return result
}

function readdirSync() {
  return fs.readdirSync(Path.join.apply(Path, arguments))
}

function statSync() {
  return fs.statSync(Path.join.apply(Path, arguments))
}

export default {
  ...fs,
  makeDirSync,
  makePathSync,
  removeDirSync,
  createTextFileSync,
  createBinaryFileSync,
  readFileSync,
  writeFileSync,
  createJsonSync,
  readJsonSync,
  writeJsonSync,
  readJsEvalSync,
  readdirSync,
}
