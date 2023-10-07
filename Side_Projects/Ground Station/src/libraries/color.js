/*
Color list:
--main-bg:
--top-button-hover: 
--stage-incomplete:
--stage-in-progress:
--stage-complete:
--stage-red: rgb(211, 3, 3)
--info-color: rgb(0, 64, 255)
--warn-color: rgb(240, 156, 0)
--error-color: rgb(242, 3, 3)
*/

toDesertMode = () => {
  const debug = document.getElementById("debug-console");
  if (!debug) {
  }
  if (debug) {
    const root = document.documentElement;
    root.style.setProperty("--info-color", "black");
    root.style.setProperty("--warn-color", "black");
    root.style.setProperty("--error-color", "black");
  }
};

toNormalMode = () => {
  const debug = document.getElementById("debug-console");
  if (!debug) {
  }
  if (debug) {
    const root = document.documentElement;
    root.style.setProperty("--info-color", "rgb(0, 64, 255)");
    root.style.setProperty("--warn-color", "rgb(240, 156, 0)");
    root.style.setProperty("--error-color", "rgb(242, 3, 3)");
  }
};
