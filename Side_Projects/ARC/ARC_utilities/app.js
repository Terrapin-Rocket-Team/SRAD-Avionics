const express = require("express");
const favicon = require("serve-favicon");
const morgan = require("morgan");
const fs = require("fs");
const path = require("path");
const { execSync } = require("child_process");
const os = require("os");

//global variables
let videoDir, logsDir;

if (os.platform() === "win32") {
  videoDir = path.join(__dirname, "ARC_video");
  logsDir = path.join(__dirname, "ARC_log");
} else {
  videoDir = path.join(os.homedir(), "ARC_video");
  logsDir = path.join(os.homedir(), "ARC_log");
}

const logPath = path.join(__dirname, "logs");
const morganLogs = path.join(logPath, "morgan.log");
const systemLogs = path.join(logPath, "system.log");

//initialize web server
const app = express();

//make sure log and submissions folders exist
if (!fs.existsSync(logPath)) fs.mkdirSync(logPath);
if (!fs.existsSync(morganLogs)) fs.writeFileSync(morganLogs, "");
if (!fs.existsSync(systemLogs)) fs.writeFileSync(systemLogs, "");
// if (!fs.existsSync("submissions")) fs.mkdirSync("submissions");
// if (!fs.existsSync("submissions/temp")) fs.mkdirSync("submissions/temp");

//clear logs
fs.truncate(morganLogs, (err) => {
  if (err) console.error(err);
});

fs.truncate(systemLogs, (err) => {
  if (err) console.error(err);
});

//middleware
app.use(favicon(path.join(__dirname, "public/data/images/favicon.png")));
app.use(express.static(path.join(__dirname, "public")));
app.use(express.json());
app.use(
  morgan("combined", {
    stream: fs.createWriteStream(morganLogs, { flags: "a" }),
  })
);

//set up writestream to the system log
const logStream = fs.createWriteStream(systemLogs, { flags: "a" });
const log = (string, level) => {
  let logStr =
    "[" +
    (level ? level + " : " : "") +
    new Date().toLocaleString() +
    "] - " +
    string;
  console.log(logStr);
  logStream.write(logStr + "\n", (err) => {
    if (err) console.error(err);
  });
};
log("Initialized server");
log("Reading videos from: " + videoDir);
log("Reading logs from: " + logsDir);

//routes
app.get("/", (req, res) => {
  res.sendFile("index.html", { root: __dirname });
});

const getFileSize = (p) => {
  if (!path.isAbsolute(p)) {
    log("[Warn] Could not find file size, path not absolute");
    return -1;
  }
  return fs.statSync(p).size;
};

const getAvailFiles = () => {
  const list = [];
  try {
    const videoList = [];
    const logList = [];

    // get times for each video and log
    fs.readdirSync(videoDir).forEach((video) => {
      videoList.push(video.split("_")[0]);
    });
    fs.readdirSync(logsDir).forEach((logFile) => {
      logList.push(logFile.split("_")[0]);
    });

    // combine two lists taking into account whether corresponding video/log files exist
    videoList.forEach((vEntry) => {
      list.push({
        time: parseInt(vEntry),
        video: getFileSize(path.join(videoDir, vEntry + "_video.av1")),
        log: logList.includes(vEntry)
          ? getFileSize(path.join(logsDir, vEntry + "_log.txt"))
          : -1,
      });
    });

    logList.forEach((lEntry) => {
      // if (!videoList.includes(lEntry)) {
      // }
      list.push({
        time: parseInt(lEntry),
        video: -1,
        log: getFileSize(path.join(logsDir, lEntry + "_log.txt")),
      });
    });

  } catch (err) {
    log("[ERROR] " + err);
  }
  return list;
};

app.get("/files", (req, res) => {
  // {time: python start time, video: video size, log: log size}
  res.json({ list: getAvailFiles() });
});

app.get("/download", (req, res) => {
  //get data from url
  const fileReq = new URL("locahost:8000" + req.url).searchParams;
  const type = fileReq.get("type"); // log or video
  const file = fileReq.get("file"); // 0000000 (time)
  if (file.match(/^[0-9]+$/g)) {
    // make sure file name matches correct format
    try {
      let dir = "";
      if (type === "log") dir = path.join(logsDir, file + "_log.txt");
      else if (type === "video") dir = path.join(videoDir, file + "_video.av1");
      else res.status(400).redirect("/");

      if (fs.existsSync(dir)) {
        res.download(dir, path.basename(dir));
      } else {
        throw "no such file or directory: " + dir;
      }
    } catch (err) {
      log("[ERROR] " + err);
      res.status(501).sendFile("pages/501.html", { root: __dirname });
    }
  } else {
    res.status(400).redirect("/");
  }
});

app.get("/download/latest", (req, res) => {
  let list = getAvailFiles();

  if (list.length > 0) {
    let maxIndexLog = -1;
    let maxIndexVideo = -1;
    for (let i = 0; i < list.length; i++) {
      if (list[i].time >= list[Math.max(maxIndexLog, 0)].time && list[i].log > 0) maxIndexLog = i;
      if (list[i].time >= list[Math.max(maxIndexVideo, 0)].time && list[i].video > 0) maxIndexVideo = i;
    }

    let type = "video";
    let index = maxIndexVideo;

    if (maxIndexVideo === -1 && maxIndexLog > 0) {
      index = maxIndexLog;
      type = "log";
    } else {
      index = 0;
    }

    res.redirect("/download?type=" + type + "&file=" + list[index].time);
  } else {
    res.status(501).sendFile("pages/501.html", { root: __dirname });
  }
});

//cmd_list = ["start recording", "start transmitting", "stop video", "stop interface"]

app.post("/record", (req, res) => {
  //log("PYCMD| stop video");
  log("PYCMD| start recording");
  res.status(200);
});
app.post("/transmit", (req, res) => {
  //log("PYCMD| stop video");
  log("PYCMD| start transmitting");
  res.status(200);
});
app.post("/stream", (req, res) => {
  //log("PYCMD| stop video");
  log("PYCMD| start streaming");
  res.status(200);
});
app.post("/stop", (req, res) => {
  log("PYCMD| stop video");
  res.status(200);
});

// 404
app.use((req, res) => {
  res.status(404).sendFile("pages/404.html", { root: __dirname });
});

app.listen(8000, () => {
  log("listening on port 8000");
});
