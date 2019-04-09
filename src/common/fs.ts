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

function makeFileDirSync(path: string) {
  if (arguments.length > 1) path = Path.join.apply(Path, arguments)
  if (path) {
    return makeDirSync(Path.parse(path).dir)
  }
  return path
}

function removeDirSync(dir_path: string) {
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

function createDirSync(path: string) {
  if (arguments.length > 1) path = Path.join.apply(Path, arguments)
  const pars = Path.parse(path)
  return makeDirSync(pars.dir) + Path.sep + pars.base
}

function createTextFileSync(data: any, ...paths: string[]) {
  fs.writeFileSync(createDirSync(Path.join.apply(Path, paths)), data, 'utf8')
}

function createBinaryFileSync(data: any, ...paths: string[]) {
  fs.writeFileSync(createDirSync(Path.join.apply(Path, paths)), data, 'binary')
}

function createJsonFileSync(data: any, ...paths: string[]) {
  writeJsonSync(data, createDirSync(Path.join.apply(Path, paths)))
}

function writeFileSync(data: any, ...paths: string[]) {
  fs.writeFileSync(Path.join.apply(Path, paths), data)
}

function writeJsonSync(object: any, ...paths: string[]) {
  const data = JSON.stringify(object, null, "  ")
  fs.writeFileSync(Path.join.apply(Path, paths), data, 'utf8')
}


function readFileSync(...paths: string[]) {
  try {
    return fs.readFileSync(Path.join.apply(Path, paths))
  }
  catch (e) {
    return null
  }
}

function readTextSync(...paths: string[]) {
  try {
    return fs.readFileSync(Path.join.apply(Path, paths)).toString()
  }
  catch (e) {
    return null
  }
}
function readJsonSync(...paths: string[]) {
  try {
    const path = Path.join.apply(Path, paths)
    const data = fs.readFileSync(path)
    try { return JSON.parse(data.toString()) }
    catch (e) {
      console.error("The json file at", path, "is invalid:", e.toString())
      return eval("(" + data + ")") // eslint-disable-line
    }
  }
  catch (e) {
    return null
  }
}

function readJsEvalSync(...paths: string[]) {
  const data = fs.readFileSync(Path.join.apply(Path, paths))
  const result = eval(`(function(){${data.toString()}})`).call(this)
  return result
}

function readdirSync(...paths: string[]) {
  return fs.readdirSync(Path.join.apply(Path, paths))
}

export default {
  ...fs,
  makeDirSync,
  makeFileDirSync,
  createDirSync,
  removeDirSync,
  createTextFileSync,
  createJsonFileSync,
  createBinaryFileSync,
  readFileSync,
  readJsonSync,
  readTextSync,
  writeFileSync,
  writeJsonSync,
  readJsEvalSync,
  readdirSync,
}
