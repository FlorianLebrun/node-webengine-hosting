const fs = require('fs');
const package_json = eval("(" + fs.readFileSync("./package.json").toString() + ")")

/*
   Compile with tsc
*/
const child_process = require('child_process');
child_process.execSync("tsc", { stdio: 'inherit' })

/*
   Bundle dts
*/
const dts = require('./dts-bundle-index.js');
const dts_result = dts.bundle({
  name: 'react-application-server',
  main: './.build/index.d.ts',
  baseDir: "./.build",
  out: '../dist/index.d.ts'
});

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
