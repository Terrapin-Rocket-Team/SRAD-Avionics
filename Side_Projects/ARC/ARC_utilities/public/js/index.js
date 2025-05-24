window.onload = () => {
  const v = document.getElementById("video-el");

  const getBaseURL = () => {
    let match = document.URL.match("https?://[^/]+/");
    return match ? match[0] : "";
  };

  let baseURL = getBaseURL();
  document.getElementById("download-latest").href = baseURL + "download/latest";
  document.getElementById("video-el").src =
    baseURL.substring(0, baseURL.lastIndexOf(":") + 1) + "7999";
  v.setAttribute("playsinline", "");

  document.getElementById("record-video").addEventListener("click", () => {
    fetch(baseURL + "record", {
      method: "post",
      body: {},
    });
  });
  document.getElementById("transmit-video").addEventListener("click", () => {
    fetch(baseURL + "transmit", {
      method: "post",
      body: {},
    });
  });
  document.getElementById("stream-video").addEventListener("click", () => {
    fetch(baseURL + "stream", {
      method: "post",
      body: {},
    });
    let src = v.getAttribute("src");
    v.setAttribute("src", "");
    setTimeout(() => {
      v.setAttribute("src", src);
      v.load();
    }, 1000);
  });
  document.getElementById("stop-video").addEventListener("click", () => {
    fetch(baseURL + "stop", {
      method: "post",
      body: {},
    });
  });
  document.getElementById("reload-video").addEventListener("click", () => {
    v.load();
  });

  let suffixes = ["kB", "MB", "GB", "TB", "PB"];
  const downloadEl = document.getElementById("downloads-list");
  fetch(baseURL + "files", {
    method: "get",
  })
    .then((res) => res.json())
    .then((res) => {

      res.list.forEach((item) => {
        let div = document.createElement("DIV");
        div.className = "download-item";

        let span = document.createElement("SPAN");
        span.textContent = new Date(item.time * 1000)
          .toISOString()
          .split(".")[0];

        let aVideo = document.createElement("A");
        aVideo.href = "#";
        aVideo.className =
          "download-button" + (item.video < 0 ? " disabled" : "");
        if (item.video > 0) {
          let text = item.video + " bytes";
          let index = 0;
          while (item.video > 1024) {
            item.video /= 1024;
            text = item.video.toFixed(1) + " " + suffixes[index];
            index++;
          }
          aVideo.title = text;
        }

        let iVideo = document.createElement("IMG");
        iVideo.src = "data/images/download_video.svg";
        iVideo.alt = "Download";

        let aLog = document.createElement("A");
        aLog.href = "#";
        aLog.className = "download-button" + (item.log < 0 ? " disabled" : "");
        if (item.log > 0) {
          let text = item.log + " bytes";
          let index = 0;
          while (item.log > 1024) {
            item.log /= 1024;
            text = item.log.toFixed(1) + " " + suffixes[index];
            index++;
          }
          aLog.title = text;
        }

        let iLog = document.createElement("IMG");
        iLog.src = "data/images/download_log.svg";
        iLog.alt = "Download";

        let downloadURL = new URL(baseURL + "download");
        if (item.video > 0) {
          let videoParams = new URLSearchParams();
          videoParams.append("type", "video");
          videoParams.append("file", item.time);
          downloadURL.search = videoParams;
          aVideo.href = downloadURL;
        } else {
          aVideo.onclick = "return false;";
        }

        if (item.log > 0) {
          let logParams = new URLSearchParams();
          logParams.append("type", "log");
          logParams.append("file", item.time);
          downloadURL.search = logParams;
          aLog.href = downloadURL;
        } else {
          aLog.onclick = "return false;";
        }

        aVideo.appendChild(iVideo);
        aLog.appendChild(iLog);
        div.appendChild(span);
        div.appendChild(aVideo);
        div.appendChild(aLog);

        downloadEl.appendChild(div);
      });
    });
};
