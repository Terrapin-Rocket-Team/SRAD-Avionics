//TODO: comments

window.onload = () => {
  let config;

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

  /**
   * Sets the state of a custom toggle
   * @param {Boolean} state The on or off state of the toggle
   * @param {HTMLDivElement} toggle The toggle to be modified
   */
  const setToggle = (state, toggle) => {
    // get the style (including that from external css sheets)
    let style = getComputedStyle(toggle);
    // convert the width in pixels to vw
    let width = (parseInt(style.width) * 100) / window.innerWidth;
    // get the current left offset (in vw)
    let pos = parseFloat(style.getPropertyValue("--toggle-pos"));
    // set the toggle to the right if it is not already there and desired state is true
    if (state && pos < width / 2) {
      toggle.style.setProperty("--toggle-pos", width * 0.5 + pos + 0.05 + "vw");
      toggle.style.setProperty("background-color", "#ca0000");
    }
    // set the toggle to the left if it is not already there and desired state is false
    if (!state && pos > width / 2) {
      toggle.style.setProperty("--toggle-pos", pos - 0.5 * width - 0.05 + "vw");
      toggle.style.setProperty("background-color", "lightgray");
    }
  };

  // the main settings inputs
  let scale = document.getElementById("scale-selected"),
    debugScale = document.getElementById("debug-scale-selected"),
    debugToggle = document.getElementById("debug-toggle"),
    noGUIToggle = document.getElementById("noGUI-toggle"),
    cacheMaxSize = document.getElementById("cacheMaxSize-input"),
    baudRate = document.getElementById("baudRate-input");

  // get the current settings from main and set each input accordingly
  api.getSettings().then((c) => {
    config = c;
    scale.textContent = config.scale.toFixed(2);
    debugScale.textContent = config.debugScale.toFixed(2);
    setToggle(config.debug, debugToggle);
    setToggle(config.noGUI, noGUIToggle);
    cacheMaxSize.value = config.cacheMaxSize;
    baudRate.value = config.baudRate;
  });

  // click listener for activating the scale dropdown
  document.getElementById("scale-drop").addEventListener("click", () => {
    const drop = document.getElementById("scale-drop");
    const options = document.getElementById("scale-options");
    if (drop.classList.contains("active")) {
      options.style.display = "none";
      document
        .getElementById("scale-arrow")
        .setAttribute("src", "../images/arrow_right.svg");
    } else {
      options.style.display = "block";
      document
        .getElementById("scale-arrow")
        .setAttribute("src", "../images/arrow_down.svg");
    }
    drop.classList.toggle("active");
    drop.classList.toggle("inactive");
    options.classList.toggle("active");
  });

  // click listeners for each scale option
  const scaleOpts = document.getElementsByClassName("scale");
  const scaleSelected = document.getElementById("scale-selected");
  let scaleLen = scaleOpts.length;
  for (let i = 0; i < scaleLen; i++) {
    scaleOpts[i].addEventListener("click", () => {
      scaleSelected.textContent = scaleOpts[i].textContent;
    });
  }

  // click listener for the debug scale dropdown
  document.getElementById("debug-scale-drop").addEventListener("click", () => {
    const drop = document.getElementById("debug-scale-drop");
    const options = document.getElementById("debug-scale-options");
    if (drop.classList.contains("active")) {
      options.style.display = "none";
      document
        .getElementById("debug-scale-arrow")
        .setAttribute("src", "../images/arrow_right.svg");
    } else {
      options.style.display = "block";
      document
        .getElementById("debug-scale-arrow")
        .setAttribute("src", "../images/arrow_down.svg");
    }
    drop.classList.toggle("active");
    drop.classList.toggle("inactive");
    options.classList.toggle("active");
  });

  // click listeners for each debug scale option
  const dScaleOpts = document.getElementsByClassName("debug-scale");
  const dScaleSelected = document.getElementById("debug-scale-selected");
  let dScaleLen = dScaleOpts.length;
  for (let i = 0; i < dScaleLen; i++) {
    dScaleOpts[i].addEventListener("click", () => {
      dScaleSelected.textContent = dScaleOpts[i].textContent;
    });
  }

  // click listeners for the toggles
  debugToggle.addEventListener("click", () => {
    config.debug = !config.debug;
    setToggle(config.debug, debugToggle);
  });

  noGUIToggle.addEventListener("click", () => {
    config.noGUI = !config.noGUI;
    setToggle(config.noGUI, noGUIToggle);
  });

  // click listener for the reset settings button
  document.getElementById("reset-settings").addEventListener("click", () => {
    // reset the object (should be the same as the default settings in main.js)
    config = {
      scale: 1,
      debugScale: 1,
      debug: false,
      noGUI: false,
      cacheMaxSize: 100000000,
      baudRate: 115200,
    };

    // reset the inputs
    scale.textContent = config.scale.toFixed(2);
    debugScale.textContent = config.debugScale.toFixed(2);
    setToggle(config.debug, debugToggle);
    setToggle(config.noGUI, noGUIToggle);
    cacheMaxSize.value = config.cacheMaxSize;
    baudRate.value = config.baudRate;

    // save the settings
    api.setSettings(config);
  });

  // click listener for the clear map cache button
  document.getElementById("clear-cache").addEventListener("click", () => {
    api.clearTileCache();
  });

  // saves the settings when the page is unloaded
  addEventListener("unload", () => {
    if (cacheMaxSize.value != "") {
      config.cacheMaxSize = parseInt(cacheMaxSize.value);
    }
    if (baudRate.value != "") {
      config.baudRate = parseInt(baudRate.value);
    }
    config.scale = parseFloat(scale.textContent);
    config.debugScale = parseFloat(debugScale.textContent);
    api.setSettings(config);
  });
};
