const fs = require("fs");
const path = require("path");
const { BrowserWindow } = require("electron");

/**
 *
 * @param {string} level the log level of the log
 * @returns {string} the prefix for the log
 */
const getLogPrefix = (level) => {
  const e = new Error();
  let stackArr = e.stack.split("\n")[4].split(path.sep);
  let info = stackArr[stackArr.length - 1].split(":");
  return (
    "[" +
    info[0] +
    ":" +
    info[1] +
    "]" +
    "[" +
    level.toUpperCase() +
    " " +
    new Date().toString().match(/[0-9][0-9]:[0-9][0-9]:[0-9][0-9]/g) +
    "] "
  );
};

/**
 * A class to combine logging data in multiple locations
 */
class Debug {
  /**
   *
   * @param {BrowserWindow} [win] the debug window the logger will send logs to
   * @param {string} [logPath] custom log path for the logger
   */
  constructor(win, logPath) {
    this.win = win;
    this.useDebug = false;
    if (!logPath) logPath = "./debug.log";
    this.ws = fs.createWriteStream(logPath);
  }

  /**
   *
   * @param {BrowserWindow} win sets the debug window the logger will send logs to
   */
  setWin(win) {
    this.win = win;
  }

  removeWin() {
    this.win = null;
  }

  /**
   *
   * @param {string} message the message to be logged
   * @param {string} level the level for the logger to use
   */
  println(message, level) {
    message = getLogPrefix(level) + message;

    console.log(message);
    this.ws.write(message + "\n");
    if (this.win) this.win.webContents.send("print", message + "\n", level);
  }

  /**
   *
   * @param {string} message the message to be logged
   */
  debug(message) {
    if (this.useDebug) this.println(message, "debug");
  }

  /**
   *
   * @param {string} message the message to be logged
   */
  info(message) {
    this.println(message, "info");
  }

  /**
   *
   * @param {string} message the message to be logged
   */
  warn(message) {
    this.println(message, "warn");
  }

  /**
   *
   * @param {string} message the message to be logged
   */
  err(message) {
    this.println(message, "error");
  }
}

const log = new Debug();

module.exports = { log, Debug };
