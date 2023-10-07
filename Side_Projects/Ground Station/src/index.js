//TODO: preserve gui state (charts) when switching to settings

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

  //listener that switches to the charts in the diagrams panel
  document.getElementById("switcher-graphs").addEventListener("click", () => {
    const highlight = document.getElementById("switcher-highlight");
    highlight.style.top = 0;

    document.getElementById("chart-wrapper").classList.toggle("active");
    document.getElementById("map-wrapper").classList.toggle("active");
  });

  //listener that switches to the map in the diagrams panel
  document.getElementById("switcher-map").addEventListener("click", () => {
    const highlight = document.getElementById("switcher-highlight");
    highlight.style.top = "50%";

    refreshMap(14);

    document.getElementById("chart-wrapper").classList.toggle("active");
    document.getElementById("map-wrapper").classList.toggle("active");
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

  let size = document.getElementById("data").offsetWidth * 0.31;

  alt.setAttribute("data-width", size);
  alt.setAttribute("data-height", size);
  spd.setAttribute("data-width", size);
  spd.setAttribute("data-height", size);
  hdg.setAttribute("data-width", size);
  hdg.setAttribute("data-height", size);

  let altwr = document.getElementById("alt-wrapper");
  let spdwr = document.getElementById("spd-wrapper");

  //persistent variables for the api data event handler
  let counter = 0;
  let lastCoords = [];
  let lastStage = 0;
  let lastAlt = 0;
  let apogeeCounter = 0;
  let tPlusSet = false;
  let chartState = "seconds";
  let startTime = 0;

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

    const processCharts = () => {
      let altData = altG.data.datasets[0].data;
      let spdData = spdG.data.datasets[0].data;
      let altLabels = altG.data.labels;
      let spdLabels = spdG.data.labels;

      if (counter > 120 && counter < 120 * 60 && chartState != "minutes") {
        if (altData && spdData && altLabels && spdLabels) {
          for (let i = 0; i < counter - 1; i++) {
            altData[i].x =
              (altData[i].x / 60) % 1 > 0
                ? (altData[i].x / 60).toFixed(2)
                : altData[i].x / 60;
            spdData[i].x =
              (spdData[i].x / 60) % 1 > 0
                ? (spdData[i].x / 60).toFixed(2)
                : spdData[i].x / 60;
            altLabels[i] =
              (altLabels[i] / 60) % 1 > 0
                ? (altLabels[i] / 60).toFixed(2)
                : altLabels[i] / 60;
            spdLabels[i] =
              (spdLabels[i] / 60) % 1 > 0
                ? (spdLabels[i] / 60).toFixed(2)
                : spdLabels[i] / 60;
          }
        }
        altwr.innerHTML = '<canvas id="alt-graph" class="chart"></canvas>';
        spdwr.innerHTML = '<canvas id="spd-graph" class="chart"></canvas>';

        altG = createChart("alt-graph", "Altitude", "min", "ft", 1 / 60, 1);
        spdG = createChart("spd-graph", "Speed", "min", "ft/s", 1 / 60, 1);
        altG.data.datasets[0].data = altData;
        spdG.data.datasets[0].data = spdData;
        altG.data.labels = altLabels;
        spdG.data.labels = spdLabels;
        altG.options.scales.x.min = startTime;
        spdG.options.scales.x.min = startTime;
        altG.options.scales.x.suggestedMax = counter + 10;
        spdG.options.scales.x.suggestedMax = counter + 10;
        chartState = "minutes";
      } else if (counter > 120 * 60 && chartState != "hours") {
        if (altData && spdData && altLabels && spdLabels) {
          for (let i = 0; i < counter - 1; i++) {
            altData[i].x =
              (altData[i].x / 3600) % 1 > 0
                ? (altData[i].x / 3600).toFixed(2)
                : altData[i].x / 3600;
            spdData[i].x =
              (spdData[i].x / 3600) % 1 > 0
                ? (spdData[i].x / 3600).toFixed(2)
                : spdData[i].x / 3600;
            altLabels[i] =
              (altLabels[i] / 3600) % 1 > 0
                ? (altLabels[i] / 3600).toFixed(2)
                : altLabels[i] / 3600;
            spdLabels[i] =
              (spdLabels[i] / 3600) % 1 > 0
                ? (spdLabels[i] / 3600).toFixed(2)
                : spdLabels[i] / 3600;
          }
        }
        altwr.innerHTML = '<canvas id="alt-graph" class="chart"></canvas>';
        spdwr.innerHTML = '<canvas id="spd-graph" class="chart"></canvas>';

        altG = createChart("alt-graph", "Altitude", "hrs", "ft", 1 / 3600, 1);
        spdG = createChart("spd-graph", "Speed", "hrs", "ft/s", 1 / 3600, 1);
        altG.data.datasets[0].data = altData;
        spdG.data.datasets[0].data = spdData;
        altG.data.labels = altLabels;
        spdG.data.labels = spdLabels;
        altG.options.scales.x.min = startTime;
        spdG.options.scales.x.min = startTime;
        altG.options.scales.x.suggestedMax = counter + 10;
        spdG.options.scales.x.suggestedMax = counter + 10;
        chartState = "hours";
      }
    };

    //set T+
    if (!tPlusSet) {
      let time = Date.now() - msg.getT0ms();
      let t = document.getElementById("t");
      t.textContent = mstohhmmss(time);

      tPlusSet = true;

      counter = parseInt(time / 1000);

      let arr = [];
      let arr1 = [];
      for (let i = 0; i < counter + 10; i++) {
        if (i < counter) arr[i] = { x: i, y: null };
        arr1[i] = i;
      }
      //must create a deep copy of arr and arr1 or the altitude and speed graphs will show the same data
      altG.data.datasets[0].data = JSON.parse(JSON.stringify(arr));
      spdG.data.datasets[0].data = JSON.parse(JSON.stringify(arr));
      altG.data.labels = JSON.parse(JSON.stringify(arr1));
      spdG.data.labels = JSON.parse(JSON.stringify(arr1));

      startTime = counter;

      processCharts();

      setInterval(() => {
        let time = Date.now() - msg.getT0ms();
        t.textContent = mstohhmmss(time);
        counter++;

        let factor = chartState == "minutes" ? 60 : 3600;
        let label =
          (counter / factor) % 1 > 0
            ? (counter / factor).toFixed(2)
            : counter / factor;

        altG.data.labels.push(label);
        spdG.data.labels.push(label);
        altG.options.scales.x.max = counter + 10;
        spdG.options.scales.x.max = counter + 10;

        processCharts();
      }, 1000);
    }

    //update charts
    if (counter > 0) {
      spdG.data.datasets[0].data.push({
        x: counter,
        y: msg.getSpeed() ? msg.getSpeed() : 0,
      });
      altG.data.datasets[0].data.push({
        x: counter,
        y: msg.getAlt() ? msg.getAlt() : 0,
      });
      spdG.update();
      altG.update();
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

    if (msg.getAlt() > lastAlt) {
      lastAlt = msg.getAlt();
      apogeeCounter = 0;
    }
    if (msg.getAlt() < lastAlt) apogeeCounter++;
    if (apogeeCounter === 3) alert("Apogee at " + lastAlt + "ft");

    setTimeout(() => {
      recvStatus.setAttribute("src", "./images/recv_off.svg");
      recvStatus.setAttribute("title", "No Message");
      recvStatus.setAttribute("alt", "Off");
    }, 600);
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
