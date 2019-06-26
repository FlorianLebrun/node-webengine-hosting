const fs = require('fs');
const Path = require('path');
const child_process = require('child_process');
const package_json = eval("(" + fs.readFileSync("./package.json").toString() + ")")

function copyFileSync(target, source) {
  var targetFile = target;
  if (fs.existsSync(target)) {
    if (fs.lstatSync(target).isDirectory()) {
      targetFile = Path.join(target, Path.basename(source));
    }
  }
  fs.writeFileSync(targetFile, fs.readFileSync(source));
}

/*
   Bundle dts
*/
child_process.execSync("tsc", { stdio: 'inherit' })
const dts = require('./dts-bundle-index.js');
const dts_result = dts.bundle({
  name: package_json.name,
  main: './.build/index.d.ts',
  baseDir: "./.build",
  out: '../dist/index.d.ts'
});

/*
   Build native code
*/
process.env.PATH += ";" + Path.resolve("./node_modules/.bin")
child_process.execSync("node-gyp rebuild --arch=x64 --release", { stdio: 'inherit' })
copyFileSync("./dist/addon-win32-x64.node", "./build/Release/addon.node")

/*
   Bundle typescript
*/
const rollup = require('rollup');
const typescript = require('rollup-plugin-typescript');
const dependencies = Object.keys(package_json.dependencies || {})
return rollup.rollup({
  input: './src/index.ts',
  external: dependencies.concat([
    "fs",
    "path",
    "mime",
    "node-watch",
    "semver",
    "crypto",
  ]),
  plugins: [
    typescript({ typescript: require('typescript') }),
  ]
}).then(bundle => {
  return bundle.write({
    file: './dist/index.js',
    format: 'cjs',
    name: 'react-application-server',
    sourcemap: false
  });
});
