window.onload = () => {
  //app control button listeners
  document.getElementById("reload").addEventListener("click", () => {
    api.reload();
  });
  document.getElementById("minimize").addEventListener("click", () => {
    api.minimize();
  });
  document.getElementById("close").addEventListener("click", () => {
    api.close();
  });
  document.getElementById("debug").addEventListener("click", () => {
    api.openDebug();
  });

  let switcherState = 0;
  //listener that switches to the charts in the diagrams panel
  document.getElementById("switcher-graphs").addEventListener("click", () => {
    const highlight = document.getElementById("switcher-highlight");
    highlight.style.top = 0;

    if (switcherState) {
      document.getElementById("chart-wrapper").classList.toggle("active");
      document.getElementById("map-wrapper").classList.toggle("active");
      switcherState = 0;
    }
  });

  //listener that switches to the map in the diagrams panel
  document.getElementById("switcher-map").addEventListener("click", () => {
    const highlight = document.getElementById("switcher-highlight");
    highlight.style.top = "50%";

    refreshMap(14);

    if (!switcherState) {
      refreshMap(14);

      document.getElementById("chart-wrapper").classList.toggle("active");
      document.getElementById("map-wrapper").classList.toggle("active");
      switcherState = 1;
    }
  });

  //create map and chart elements
  buildMap("map");

  let altG = createChart("alt-graph", "Altitude", "s", "ft", 1, 1);
  let spdG = createChart("spd-graph", "Speed", "s", "ft/s", 1, 1);

  //custom dropdown listener
  document.getElementById("serial-drop").addEventListener("click", () => {
    const drop = document.getElementById("serial-drop");
    const options = document.getElementById("serial-options");
    if (drop.classList.contains("active")) {
      options.style.display = "none";
      document
        .getElementById("serial-arrow")
        .setAttribute("src", "./images/arrow_right.svg");
    } else {
      options.style.display = "block";
      getAvailPorts();
      document
        .getElementById("serial-arrow")
        .setAttribute("src", "./images/arrow_down.svg");
    }
    drop.classList.toggle("active");
    drop.classList.toggle("inactive");
    options.classList.toggle("active");
  });

  //adds available ports to the custom dropdown
  const getAvailPorts = () => {
    api.getPorts().then((ports) => {
      const options = document.getElementById("serial-options");
      while (options.childElementCount > 0) {
        options.removeChild(options.firstChild);
      }
      const selected = document.getElementById("serial-selected");
      if (ports.length === 0) {
        const span = document.createElement("SPAN");
        span.className = "serial";
        span.textContent = "No available ports";
        span.addEventListener("click", () => {
          selected.textContent = "Select Port";
        });
        options.appendChild(span);
      } else {
        ports.forEach((port) => {
          const span = document.createElement("SPAN");
          span.className = "serial";
          span.textContent = port.path;
          span.addEventListener("click", () => {
            api.setPort(port.path).then((success) => {
              const img = document.getElementById("serial-connection");
              if (success) {
                selected.textContent = port.path;
                img.setAttribute("src", "./images/serial_connected.svg");
                img.setAttribute("title", "Serial Connected");
              } else {
                img.setAttribute("src", "./images/serial_disconnected.svg");
                img.setAttribute("title", "Connection Error");
              }
            });
          });
          options.appendChild(span);
        });
      }
    });
  };

  getAvailPorts();

  //set gauge.js gauge sizing
  let alt = document.getElementById("altitude");
  let spd = document.getElementById("speed");
  let hdg = document.getElementById("heading");

  let size =
    document.getElementById("data").offsetWidth * 0.15 +
    document.getElementById("data").offsetWidth * 0.15;

  alt.setAttribute("data-width", size);
  alt.setAttribute("data-height", size);
  spd.setAttribute("data-width", size);
  spd.setAttribute("data-height", size);
  hdg.setAttribute("data-width", size);
  hdg.setAttribute("data-height", size);

  let altwr = document.getElementById("alt-wrapper");
  let spdwr = document.getElementById("spd-wrapper");

  //persistent variables for the api data event handler
  let lastCoords = [];
  let lastStage = 0;
  let lastAlt = 0;
  let apogeeTime = 0;
  let tPlusSet = false;
  let chartState = "seconds";

  // load previous data if it exists
  {
    altG.data.datasets[0].data = sessionStorage.getItem("altData")
      ? JSON.parse(sessionStorage.getItem("altData"))
      : [];
    spdG.data.datasets[0].data = sessionStorage.getItem("spdData")
      ? JSON.parse(sessionStorage.getItem("spdData"))
      : [];

    if (
      sessionStorage.getItem("apogee") &&
      parseInt(sessionStorage.getItem("apogee"))
    )
      document.getElementById("apogee").textContent =
        parseInt(sessionStorage.getItem("apogee")) + " ft";
  }

  api.on("data", (data) => {
    let msg = new APRSMessage(data);

    let recvStatus = document.getElementById("recv-status");

    recvStatus.setAttribute("src", "./images/recv_on.svg");
    recvStatus.setAttribute("title", "Receiving Message");
    recvStatus.setAttribute("alt", "On");

    //update signal strength
    let ss = msg.getSignalStrength();
    const serialEl = document.getElementById("radio-connection");
    if (ss === "High") {
      serialEl.setAttribute("src", "./images/signal_strong.svg");
      serialEl.setAttribute("alt", "Signal Strong");
      serialEl.title = "Signal Strong";
    }
    if (ss === "Med") {
      serialEl.setAttribute("src", "./images/signal_mid.svg");
      serialEl.setAttribute("alt", "Signal Medium");
      serialEl.title = "Signal Medium";
    }
    if (ss === "Low") {
      serialEl.setAttribute("src", "./images/signal_weak.svg");
      serialEl.setAttribute("alt", "Signal Weak");
      serialEl.title = "Signal Weak";
    }
    if (ss === "None") {
      serialEl.setAttribute("src", "./images/no_signal.svg");
      serialEl.setAttribute("alt", "No Signal");
      serialEl.title = "No Signal";
    }

    //set T+
    if (!tPlusSet && msg.getStageNumber() > 0) {
      let t = document.getElementById("t");
      let time = Date.now() - msg.getT0ms();

      tPlusSet = true;
      t.textContent = mstohhmmss(time);
      let ts = time / 1000;

      if (altG.data.datasets[0].data.length === 0)
        altG.data.datasets[0].data = [{ x: ts, y: null }];
      if (spdG.data.datasets[0].data.length === 0)
        spdG.data.datasets[0].data = [{ x: ts, y: null }];

      setInterval(() => {
        t.textContent = mstohhmmss(Date.now() - msg.getT0ms());
      }, 10);
    }
    if (tPlusSet) {
      //update charts
      let time = Date.now() - msg.getT0ms();
      let ts = time / 1000;

      if (ts > 120 && ts < 120 * 60 && chartState != "minutes") {
        let altData = altG.data.datasets[0].data;
        let spdData = spdG.data.datasets[0].data;
        let altLabels = altG.data.labels;
        let spdLabels = spdG.data.labels;

        altwr.innerHTML = '<canvas id="alt-graph" class="chart"></canvas>';
        spdwr.innerHTML = '<canvas id="spd-graph" class="chart"></canvas>';

        altG = createChart("alt-graph", "Altitude", "min", "ft", 1 / 60, 1);
        spdG = createChart("spd-graph", "Speed", "min", "ft/s", 1 / 60, 1);
        altG.data.datasets[0].data = altData;
        spdG.data.datasets[0].data = spdData;
        altG.data.labels = altLabels;
        spdG.data.labels = spdLabels;
        chartState = "minutes";
      } else if (ts > 120 * 60 && chartState != "hours") {
        let altData = altG.data.datasets[0].data;
        let spdData = spdG.data.datasets[0].data;
        let altLabels = altG.data.labels;
        let spdLabels = spdG.data.labels;

        altwr.innerHTML = '<canvas id="alt-graph" class="chart"></canvas>';
        spdwr.innerHTML = '<canvas id="spd-graph" class="chart"></canvas>';

        altG = createChart("alt-graph", "Altitude", "hrs", "ft", 1 / 3600, 1);
        spdG = createChart("spd-graph", "Speed", "hrs", "ft/s", 1 / 3600, 1);
        altG.data.datasets[0].data = altData;
        spdG.data.datasets[0].data = spdData;
        altG.data.labels = altLabels;
        spdG.data.labels = spdLabels;
        chartState = "hours";
      }

      let factor =
        chartState == "minutes" ? 30 : chartState == "hours" ? 320 : 1;

      let interval = parseInt(
        (ts - altG.data.datasets[0].data[0].x + 5 * factor) / 4
      );

      let arrL = [];
      for (let i = 0; i < 5; i++) {
        arrL[i] = Math.floor(altG.data.datasets[0].data[0].x) + i * interval;
      }

      altG.options.scales.x.min = arrL[0] < 0 ? 0 : arrL[0];
      spdG.options.scales.x.min = arrL[0] < 0 ? 0 : arrL[0];
      altG.options.scales.x.suggestedMax = ts + 10 * factor;
      spdG.options.scales.x.suggestedMax = ts + 10 * factor;

      altG.data.labels = JSON.parse(JSON.stringify(arrL));
      spdG.data.labels = JSON.parse(JSON.stringify(arrL));

      altG.data.datasets[0].data.push({
        x: ts,
        y: msg.getAlt() ? msg.getAlt() : 0,
      });
      spdG.data.datasets[0].data.push({
        x: ts,
        y: msg.getSpeed() ? msg.getSpeed() : 0,
      });

      sessionStorage.setItem(
        "altData",
        JSON.stringify(altG.data.datasets[0].data)
      );
      sessionStorage.setItem(
        "spdData",
        JSON.stringify(spdG.data.datasets[0].data)
      );

      altG.update();
      spdG.update();
    }

    //update map
    let coords = msg.getLatLong();
    if (coords[0] !== lastCoords[0] || coords[1] !== lastCoords[1]) {
      updateMarker(
        coords[0],
        coords[1],
        `<span style="font-size:1.5vh;font-weight:520;">Approximate Location: </span><br><span style="font-size:1.3vh;">${msg.getLatLongFormat()}</span>`
      );
      lastCoords = coords;
    }

    //update gauges
    if (msg.getAlt() || msg.getAlt() === 0) {
      alt.setAttribute("data-value-text", msg.getAlt());
      alt.setAttribute("data-value", msg.getAlt() / 1000);
      document.getElementById("alt-text").textContent = msg.getAlt() + " ft";
    } else {
      alt.setAttribute("data-value-text", "\u2014");
    }
    if (msg.getSpeed() || msg.getSpeed() === 0) {
      spd.setAttribute("data-value-text", msg.getSpeed());
      spd.setAttribute("data-value", msg.getSpeed() / 100);
      document.getElementById("spd-text").textContent =
        msg.getSpeed() + " ft/s";
    } else {
      spd.setAttribute("data-value-text", "\u2014");
    }
    if (msg.getHeading() || msg.getHeading() === 0) {
      hdg.setAttribute("data-value-text", "false");
      hdg.setAttribute("data-value", msg.getHeading());
    } else {
      hdg.setAttribute("data-value-text", "\u2014");
    }

    //update lat/long
    let fcoords = msg.getLatLongFormat();
    document.getElementById("lat").textContent = fcoords
      ? fcoords.split("/")[0]
      : "00.0000\u00b0N";
    document.getElementById("long").textContent = fcoords
      ? fcoords.split("/")[1]
      : "000.0000\u00b0W";

    //update stage
    let prog = document.getElementById("stage-progress");
    let sn = msg.getStageNumber();
    let percents = [16, 33, 50, 67, 84, 100];
    if (sn >= 0) {
      prog.textContent = percents[sn] + "%";
      prog.setAttribute("value", percents[sn]);
      document.getElementById("s" + sn).className = "stage in-progress";
      if (sn > 0)
        document.getElementById("s" + (sn - 1)).className = "stage complete";
      for (let i = lastStage; i < sn; i++) {
        document.getElementById("s" + i).className = "stage complete";
      }
      lastStage = sn;
    }

    // apogee check
    if (msg.getAlt() >= lastAlt) {
      lastAlt = msg.getAlt();
      apogeeTime = Date.now();
    }
    if (Date.now() - apogeeTime > 6000) {
      document.getElementById("apogee").textContent = lastAlt + " ft";
      sessionStorage.setItem("apogee", lastAlt);
    }

    setTimeout(() => {
      recvStatus.setAttribute("src", "./images/recv_off.svg");
      recvStatus.setAttribute("title", "No Message");
      recvStatus.setAttribute("alt", "Off");
    }, 10);
  });

  //update UI if serial connection is lost
  api.on("radio-close", () => {
    const img = document.getElementById("serial-connection");
    img.setAttribute("src", "./images/serial_disconnected.svg");
    img.setAttribute("title", "Connection Error");

    const serialEl = document.getElementById("radio-connection");
    serialEl.setAttribute("src", "./images/no_signal.svg");
    serialEl.setAttribute("alt", "No Signal");
    serialEl.title = "No Signal";
  });
};

// convert milliseconds to HH:MM:SS format
const mstohhmmss = (ms) => {
  let seconds =
    Math.floor((ms / 1000) % 60) > 0 ? Math.floor((ms / 1000) % 60) : 0;
  let minutes =
    Math.floor((ms / (1000 * 60)) % 60) > 0
      ? Math.floor((ms / (1000 * 60)) % 60)
      : 0;
  let hours =
    Math.floor((ms / (1000 * 60 * 60)) % 24) > 0
      ? Math.floor((ms / (1000 * 60 * 60)) % 24)
      : 0;

  return `${hours < 10 ? "0" + hours : hours}:${
    minutes < 10 ? "0" + minutes : minutes
  }:${seconds < 10 ? "0" + seconds : seconds}`;
};
