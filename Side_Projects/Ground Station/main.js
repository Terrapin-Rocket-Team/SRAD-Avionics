//TODO: launch the radio and data logging as a separate process (maybe) so saving data does not rely on the main app to work

const { app, BrowserWindow, ipcMain } = require("electron");
if (require("electron-squirrel-startup")) app.quit(); //for app maker
const fs = require("fs");
const path = require("path");
const { log } = require("./debug");
const { radio } = require("./serial/serial");
const { APRSMessage } = require("./serial/APRS");

let mainWin,
  debugWin,
  config,
  cacheMeta,
  closed,
  csvCreated,
  lastHeading,
  lastSpeed,
  currentCSV;

/*
Config options:
scale: 1 is default, scales the application window
debugScale: 1 is default, scales the debug window
debug: false is default, whether debug statements will be logged
noGUI: false is default, loads only the debug window
tileCache: true by default, whether tiles will be cached - work in progress
cacheMaxSize: 100000000 (100MB) is default, max tile cache size in bytes
baudRate: 115200 is default, baudrate to use with the connected serial port
*/
try {
  //load config
  config = JSON.parse(fs.readFileSync("./config.json"));
  log.useDebug = config.debug;
  log.debug("Config loaded");
} catch (err) {
  config = {
    scale: 1,
    debugScale: 1,
    debug: false,
    noGUI: false,
    //tileCache: true, //added tile toggling here - work in progress
    cacheMaxSize: 100000000,
    baudRate: 115200,
  };
  log.warn('Failed to load config file, using defaults: "' + err.message + '"');
  try {
    if (!fs.existsSync("./config.json")) {
      fs.writeFileSync("./config.json", JSON.stringify(config, null, "\t"));
      log.info("Config file successfully created");
    }
  } catch (err) {
    log.err('Failed to create config file: "' + err.message + '"');
  }
}

try {
  //load cache metadata
  cacheMeta = JSON.parse(
    fs.readFileSync(path.join(__dirname, "src/cachedtiles/metadata.json"))
  );
  log.debug("Cache metadata loaded");
} catch (err) {
  cacheMeta = {
    tiles: {},
    fileList: [],
    runningSize: 0,
  };
  log.warn(
    'Failed to load cache metadata file, using defaults: "' + err.message + '"'
  );
  try {
    if (!fs.existsSync(path.join(__dirname, "src/cachedtiles")))
      fs.mkdirSync(path.join(__dirname, "src/cachedtiles"));
    if (!fs.existsSync(path.join(__dirname, "src/cachedtiles/metadata.json"))) {
      fs.writeFileSync(
        path.join(__dirname, "src/cachedtiles/metadata.json"),
        JSON.stringify(cacheMeta, null, "\t")
      );
      log.info("Metadata file successfully created");
    }
  } catch (err) {
    log.err('Failed to create metadata file: "' + err.message + '"');
  }
}

//creates the main electron window
const createWindow = () => {
  const width = 1200,
    height = 800;

  const iconSuffix =
    process.platform === "win32"
      ? ".ico"
      : process.platform === "darwin"
      ? ".icns"
      : ".png";
  mainWin = new BrowserWindow({
    width: width * config.scale,
    height: height * config.scale,
    resizable: false,
    frame: false,
    autoHideMenuBar: true,
    icon: "assets/logo" + iconSuffix,
    webPreferences: {
      preload: path.join(__dirname, "preload.js"),
    },
  });

  mainWin.loadFile(path.join(__dirname, "src/index.html"));

  if (config.debug) mainWin.webContents.openDevTools({ mode: "detach" });
  log.debug("Main window created");

  //make sure messages are not sent to a destroyed window
  mainWin.once("close", () => {
    if (!config.noGUI) {
      closed = true;
      radio.close();
    }
    mainWin.webContents.send("close"); // unused
    if (!config.noGUI && debugWin) debugWin.close();
  });

  mainWin.once("closed", () => {
    mainWin = null;
  });
};

//creates the debug electron window
const createDebug = () => {
  const width = 600,
    height = 400;
  debugWin = new BrowserWindow({
    width: width * config.debugScale,
    height: height * config.debugScale,
    resizable: false,
    autoHideMenuBar: true,
    webPreferences: {
      preload: path.join(__dirname, "preload.js"),
    },
  });

  log.setWin(debugWin);

  debugWin.loadFile(path.join(__dirname, "src/debug/debug.html"));

  if (config.debug) debugWin.webContents.openDevTools({ mode: "detach" });

  //send the debug window all previous logs once it is ready
  debugWin.webContents.once("dom-ready", () => {
    try {
      if (fs.existsSync("./debug.log"))
        debugWin.webContents.send(
          "previous-logs",
          fs.readFileSync("./debug.log").toString()
        );
    } catch (err) {
      log.err('Could not load previous logs: "' + err.message + '"');
    }
  });

  //reset when the window is closed
  debugWin.once("close", () => {
    if (config.noGUI) {
      closed = true;
      radio.close();
    }
    debugWin.webContents.send("close"); // unused
    log.removeWin();
    if (config.noGUI && mainWin) mainWin.close();
  });

  debugWin.once("closed", () => {
    debugWin = null;
  });
  log.debug("Debug window created");
};

//when electron has initialized, create the appropriate window
app.whenReady().then(() => {
  if (!config.noGUI) {
    createWindow();
  } else {
    createDebug();
  }

  //open a new window if there are none when the app is opened and is still running (MacOS)
  app.on("activate", () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      if (!config.noGUI) {
        createWindow();
      } else {
        createDebug();
      }
    }
  });
});

//quit the app if all windows are closed on MacOS
app.on("window-all-closed", () => {
  app.quit();
});

//app control
ipcMain.on("close", () => {
  log.debug("Closing main window");
  mainWin.close();
});

ipcMain.on("minimize", () => {
  mainWin.minimize();
});

ipcMain.on("reload", (event, args) => {
  log.debug("Reloading windows");
  radio.close();
  if (mainWin) mainWin.webContents.reloadIgnoringCache();
  if (debugWin) {
    debugWin.webContents.reloadIgnoringCache();
    debugWin.webContents.once("dom-ready", () => {
      try {
        if (fs.existsSync("./debug.log"))
          debugWin.webContents.send(
            "previous-logs",
            fs.readFileSync("./debug.log").toString()
          );
      } catch (err) {
        log.err('Could not load previous logs: "' + err.message + '"');
      }
    });
  }
});

ipcMain.on("dev-tools", (event, args) => {
  if (mainWin) mainWin.webContents.openDevTools({ mode: "detach" });
  if (debugWin) debugWin.webContents.openDevTools({ mode: "detach" });
});

ipcMain.on("open-gui", (event, args) => {
  log.debug("Main window opened from debug");
  if (!mainWin) createWindow();
});

ipcMain.on("open-debug", (event, args) => {
  log.debug("Debug window opened from main");
  if (!debugWin) createDebug();
});

ipcMain.on("cache-tile", (event, tile, tilePathNums) => {
  try {
    tilePath = [tilePathNums[0], tilePathNums[1], tilePathNums[2]];
    while (cacheMeta.runningSize + tile.byteLength > config.cacheMaxSize) {
      // shift off the fileList and delete file and containing folders if necessary
      let oldTile = cacheMeta.fileList.shift();
      let oldFolders = oldTile.split(path.sep);
      let fileSize = fs.lstatSync(
        path.join(__dirname, "src", "cachedtiles", oldTile + ".png")
      ).size;

      //remove the file
      fs.rmSync(path.join(__dirname, "src", "cachedtiles", oldTile + ".png"));

      cacheMeta.tiles[oldFolders[0]][oldFolders[1]].splice(
        cacheMeta.tiles[oldFolders[0]][oldFolders[1]].indexOf(oldFolders[2]),
        1
      );

      //remove the folder one level above the file if it is empty
      if (
        fs.readdirSync(
          path.join(
            __dirname,
            "src",
            "cachedtiles",
            oldFolders[0],
            oldFolders[1]
          )
        ).length === 0
      ) {
        fs.rmdirSync(
          path.join(
            __dirname,
            "src",
            "cachedtiles",
            oldFolders[0],
            oldFolders[1]
          )
        );
        delete cacheMeta.tiles[oldFolders[0]][oldFolders[1]];
      }

      //remove the folder two levels above the file if it is empty
      if (
        fs.readdirSync(
          path.join(__dirname, "src", "cachedtiles", oldFolders[0])
        ).length === 0
      ) {
        fs.rmdirSync(path.join(__dirname, "src", "cachedtiles", oldFolders[0]));
        delete cacheMeta.tiles[oldFolders[0]];
      }

      cacheMeta.runningSize -= fileSize;
    }

    let folderPath = path.join(
      __dirname,
      "src",
      "cachedtiles",
      tilePath[0],
      tilePath[1]
    );

    //create folders if necessary
    if (!fs.existsSync(folderPath)) {
      fs.mkdirSync(folderPath, {
        recursive: true,
      });
    }

    //see what folders/files already exist
    let hasZoom = cacheMeta.tiles[tilePath[0]];
    let hasX = hasZoom ? cacheMeta.tiles[tilePath[0]][tilePath[1]] : 0;
    let hasY = hasX
      ? cacheMeta.tiles[tilePath[0]][tilePath[1]].includes(tilePath[2])
      : 0;

    //add the appropriate amount of stucture to the metadata
    if (!hasZoom) {
      cacheMeta.tiles[tilePath[0]] = {
        [tilePath[1]]: [tilePath[2]],
      };
    } else if (!hasX) {
      cacheMeta.tiles[tilePath[0]][tilePath[1]] = [tilePath[2]];
    } else if (!hasY) {
      cacheMeta.tiles[tilePath[0]][tilePath[1]].push(tilePath[2]);
    }

    if (hasY) {
      //if the file already exists, check if it is a different size
      let fileSize = fs.lstatSync(
        path.join(folderPath, tilePath[2] + ".png")
      ).size;
      if (fileSize != tile.byteLength) {
        cacheMeta.runningSize -= fileSize;
        cacheMeta.runningSize += tile.byteLength;
        fs.writeFileSync(
          path.join(folderPath, tilePath[2] + ".png"),
          Buffer.from(tile)
        );
      }
    } else {
      //add the file
      cacheMeta.runningSize += tile.byteLength;
      cacheMeta.fileList.push(path.join(tilePath[0], tilePath[1], tilePath[2]));
      fs.writeFileSync(
        path.join(folderPath, tilePath[2] + ".png"),
        Buffer.from(tile)
      );
    }

    //write metadata
    fs.writeFileSync(
      path.join(__dirname, "src/cachedtiles/metadata.json"),
      JSON.stringify(cacheMeta, null, "\t")
    );
  } catch (err) {
    log.err('Error caching tile: "' + err.message + '"');
  }
});

ipcMain.handle("get-tiles", () => {
  return cacheMeta.tiles;
});

ipcMain.on("close-port", (event, args) => {
  radio.close();
});

ipcMain.on("clear-tile-cache", (event, args) => {
  try {
    if (fs.existsSync(path.join(__dirname, "src", "cachedtiles"))) {
      cacheMeta = {
        tiles: {},
        fileList: [],
        runningSize: 0,
      };
      fs.rmSync(path.join(__dirname, "src", "cachedtiles"), {
        recursive: true,
        force: true,
      });
      log.debug("Tile cache successfully cleared");
    }
  } catch (err) {
    log.err('Error clearing tile cache: "' + err.message + '"');
  }
});

//getters
ipcMain.handle("get-ports", (event, args) => {
  return radio.getAvailablePorts();
});

ipcMain.handle("get-port-status", (event, args) => {
  return radio.isConnected();
});

ipcMain.handle("get-settings", (event, args) => {
  return config;
});

//setters
ipcMain.handle("set-port", (event, port) => {
  return new Promise((res, rej) => {
    radio
      .connect(port, config.baudRate)
      .then((result) => {
        log.info("Successfully connected to port " + port);
        res(1);
      })
      .catch((err) => {
        log.err(
          "Failed to connect to port " + port + ': "' + err.message + '"'
        );
        res(0);
      });
  });
});

ipcMain.on("update-settings", (event, settings) => {
  if (settings) {
    config = settings;
    try {
      fs.writeFileSync("./config.json", JSON.stringify(config, null, "\t"));
      log.debug("Successfully updated settings");
    } catch (err) {
      log.err('Failed to update settings: "' + err.message + '"');
    }
  }
});

//serial communication
radio.on("data", (data) => {
  if (!data.getHeading() && !data.getSpeed()) {
    data.body.heading = lastHeading;
    data.body.speed = lastSpeed;
  } else {
    lastHeading = data.body.heading;
    lastSpeed = data.body.speed;
  }
  log.info(data.toString());
  if (mainWin) mainWin.webContents.send("data", data);
  try {
    if (!csvCreated) {
      if (!fs.existsSync("./data")) fs.mkdirSync("./data");
      currentCSV = new Date().toISOString().replace(/:/g, "-") + ".csv";
      fs.writeFileSync(path.join("./data", currentCSV), "");
    }
    fs.appendFileSync(path.join("./data", currentCSV), data.toCSV(csvCreated));
    if (!csvCreated) csvCreated = true;
    //write data from serial to be used in testing if debug is on
    if (config.debug) fs.writeFileSync("./test.json", JSON.stringify(data));
  } catch (err) {
    log.err('Error writing data: "' + err.message + '"');
  }
});

radio.on("error", (message) => {
  log.err("Error parsing APRS message: " + message);
});

radio.on("close", () => {
  log.info("Serial disconnected");
  if (!closed && mainWin) mainWin.webContents.send("radio-close");
});

//testing
if (config.debug && !config.noGUI) {
  //test to see whether the json file exists
  fs.stat("./test.json", (err1, stats) => {
    if (err1) {
      log.warn('Failed to find test.json file: "' + err1.message + '"');
    } else {
      setInterval(() => {
        if (!closed && mainWin)
          //throw an error if the file cannot be read
          try {
            mainWin.webContents.send(
              "data",
              new APRSMessage(JSON.parse(fs.readFileSync("./test.json")))
            );
          } catch (err) {
            log.warn('Failed to read test.json file: "' + err.message + '"');
          }
      }, 2000);
    }
  });
}
