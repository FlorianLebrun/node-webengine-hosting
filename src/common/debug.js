const colors = {
  Reset: "\x1b[0m",
  Bright: "\x1b[1m",
  Dim: "\x1b[2m",
  Underscore: "\x1b[4m",
  Blink: "\x1b[5m",
  Reverse: "\x1b[7m",
  Hidden: "\x1b[8m",

  Black: "\x1b[30m",
  Red: "\x1b[31m",
  Green: "\x1b[32m",
  Yellow: "\x1b[33m",
  Blue: "\x1b[34m",
  Magenta: "\x1b[35m",
  Cyan: "\x1b[36m",
  White: "\x1b[37m",

  BgBlack: "\x1b[40m",
  BgRed: "\x1b[41m",
  BgGreen: "\x1b[42m",
  BgYellow: "\x1b[43m",
  BgBlue: "\x1b[44m",
  BgMagenta: "\x1b[45m",
  BgCyan: "\x1b[46m",
  BgWhite: "\x1b[47m",
}

const listeners = []

export default {
  log: function (...args) {
    console.log(colors.White, ...args, colors.Reset)
    listeners.forEach(l => l(args.join()))
  },
  debug: function (...args) {
    console.log(colors.Magenta, ...args, colors.Reset)
    listeners.forEach(l => l(args.join()))
  },
  warning: function (...args) {
    console.log(colors.Magenta, ...args, colors.Reset)
    listeners.forEach(l => l(args.join()))
  },
  error: function (...args) {
    console.log(colors.Bright + colors.Red, ...args, colors.Reset)
    listeners.forEach(l => l(args.join()))
  },
  success: function (...args) {
    console.log(colors.Green, ...args, colors.Reset)
    listeners.forEach(l => l(args.join()))
  },
  title: function (...args) {
    console.log(colors.Cyan, ...args, colors.Reset)
    listeners.forEach(l => l(args.join()))
  },
  info: function (...args) {
    console.log(colors.Yellow, ...args, colors.Reset)
    listeners.forEach(l => l(args.join()))
  },
  exception: function (e) {
    console.log(colors.Bright + colors.Red, e.message, colors.Reset)
    console.log(colors.Red, e.stack, colors.Reset)
    listeners.forEach(l => l(e.message))
    listeners.forEach(l => l(e.stack))
    /*const err = new Error(e.message)
        err.stack = e.stack
        throw err*/
  },
  listen: function (callback: Function) {
    listeners.push(callback)
  },
  unlisten: function (callback: Function) {
    listeners.splice(listeners.indexOf(callback), 1)
  }
}
