const express = require("express");
const favicon = require("serve-favicon");
const morgan = require("morgan");
// const zip = require("express-easy-zip");
const fs = require("fs");
const path = require("path");
// const formidable = require("formidable");
// const nodemailer = require("nodemailer");

//global variables
const ADMIN_KEYS = [];
let timouts = [];

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
// app.use(zip());
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

//routes
app.get("/", (req, res) => {
  res.sendFile("index.html", { root: __dirname });
});

app.get("/about", (req, res) => {
  res.sendFile("pages/about.html", { root: __dirname });
});

app.get("/admin", (req, res) => {
  res.sendFile("pages/admin-login.html", { root: __dirname });
});

//check key against list of valid keys
const adminAuth = (url) => {
  const key = new URL("locahost:8000" + url).searchParams.get("key");
  const valid = new URL("locahost:8000" + url).searchParams.get("valid");
  if (key && ADMIN_KEYS.includes(key) && valid === "true") {
    return true;
  } else {
    return false;
  }
};

app.get("/admin-view", (req, res) => {
  //if user has a valid key, send admin page, else redirect to login
  let login = adminAuth(req.url);
  if (login) {
    res.sendFile("pages/admin.html", { root: __dirname });
  } else {
    res.redirect("/admin");
  }
});

//used to fetch submission data
app.get("/admin-view/submissions", async (req, res) => {
  //check if key is valid
  let login = adminAuth(req.url);
  if (login) {
    const submissions = fs.readdirSync("submissions");
    const list = [];
    //get info.json for each submission
    submissions.forEach((submission) => {
      if (submission != "temp") {
        let doc = JSON.parse(
          fs.readFileSync("submissions/" + submission + "/info.json")
        );
        list.push({
          number: doc.number,
          title: doc.title,
          members: doc.members,
          files: doc.oldNames,
        });
      }
    });
    res.json({ list });
  } else {
    res.status(401).sendFile("pages/401.html", { root: __dirname });
  }
});

//used to download submissions
app.get("/admin-view/download", async (req, res) => {
  //check if key is valid
  let login = adminAuth(req.url);
  if (login) {
    //get data from url
    const fileReq = new URL("locahost:8000" + req.url).searchParams;
    const files = fileReq.getAll("files");
    const number = fileReq.get("number");
    const single = fileReq.get("single");
    const genericPath = __dirname + "/submissions/" + number + "/files/";
    try {
      let doc = JSON.parse(
        fs.readFileSync("submissions/" + number + "/info.json")
      );
      let index = doc.oldNames.indexOf(files[0]);
      //download the single file if it exists
      if (single === "true") {
        if (fs.existsSync(genericPath + doc.newNames[index])) {
          res.download(genericPath + doc.newNames[index], doc.oldNames[index]);
        } else {
          throw (
            "no such file or directory: " + genericPath + doc.newNames[index]
          );
        }
      } else {
        //otherwise zip all the requested files into a .zip with the team number
        const zipArray = [];
        if (files.length > 1) {
          for (let i = 0; i < files.length; i++) {
            zipArray.push({
              path: genericPath + doc.newNames[i],
              name: doc.oldNames[i],
            });
          }
          res.zip({
            files: zipArray,
            filename: "team-" + number + ".zip",
          });
        } else {
          if (fs.existsSync(genericPath + doc.newNames[0], doc.oldNames[0])) {
            res.download(genericPath + doc.newNames[0], doc.oldNames[0]);
          } else {
            throw "no such file or directory: " + genericPath + doc.newNames[0];
          }
        }
      }
    } catch (err) {
      log("[ERROR] " + err);
      res.status(501).sendFile("pages/501.html", { root: __dirname });
    }
  } else {
    res.status(401).sendFile("pages/401.html", { root: __dirname });
  }
});

//handles logging in with a password
app.post("/admin-login", (req, res) => {
  if (req.body.input === "smucka!%(159") {
    log("A user logged into admin");
    //create a key
    let key = "";
    for (let i = 0; i < 10; i++) {
      let num = (Math.random().toFixed(2) * 100).toFixed(0);
      if (num > 49) {
        let letters = ["a", "c", "e", "g", "i", "k", "m", "o", "q", "s"];
        num = (num / 7).toFixed();
        num %= 10;
        key += letters[num];
      } else {
        num %= 10;
        key += num.toString();
      }
    }
    const date = new Date();
    date.setTime(date.getTime() + 86400000);
    let expires = date.getTime() - Date.now();
    //set a timeout to remove the key in 24 hrs
    timouts.push(
      setTimeout(() => {
        ADMIN_KEYS.splice(ADMIN_KEYS.indexOf(key), 1);
      }, expires)
    );
    //add the key to the array
    ADMIN_KEYS.push(key);
    res.redirect("/admin-view?valid=true&key=" + key);
  } else {
    log("Failed login attempt. Input: " + req.body.input);
    res.redirect("/");
  }
});

//handles auto-login if the user has a key
app.post("/admin-key", (req, res) => {
  if (ADMIN_KEYS.includes(req.body.key)) {
    res.redirect("/admin-view?valid=true&key=" + req.body.key);
  } else {
    res.redirect("/admin?valid=false");
  }
});

//404
app.use((req, res) => {
  res.status(404).sendFile("pages/404.html", { root: __dirname });
});

app.listen(8000, () => {
  log("listening on port 8000");
});
