var nodeWatch = require("node-watch");
var process = require('process');
var child_process = require('child_process');

var current = null;
var currentRestart_timer = undefined;
const currentCommandArgs = process.argv.slice(2)

function start() {
  console.log(`\x1b[32m[monitor] Start process\x1b[0m`)
  current = child_process.spawn(
    "node",
    ["--inspect", "./scripts/launch", ...currentCommandArgs],
    {
      stdio: [process.stdin, process.stdout, process.stderr],
    }
  );
  current.on("exit", (code) => {
    current = null
    if (currentRestart_timer !== undefined) {
      console.log(`\x1b[32m[monitor] Restart after 'change'\x1b[0m`)
      currentRestart_timer = undefined;
      start()
    }
    else {
      console.log(`\x1b[${code < 0 ? "31m" : "33m"}[monitor] Process 'exit' (Press enter to restart)\x1b[0m`)
      process.stdin.on('readable', () => {
        process.stdin.read()
        process.stdin.removeAllListeners()
        start()
      })
    }
  })
}
function restart() {
  if (current !== null && currentRestart_timer === undefined) {
    const timer = setTimeout(() => {
      if (currentRestart_timer === timer) {
        console.log(`\x1b[32m[monitor] Kill process, after 'change'\x1b[0m`)
        current.kill();
      }
    }, 100);
    currentRestart_timer = timer
  }
}

start();
nodeWatch("./.build", { recursive: true }).on('change', (change, path) => {
  restart();
})