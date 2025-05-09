window.onload = () => {
  //event listeners for side menu
  const tabs = document.getElementsByClassName("tutorial-type");
  const content = document.getElementsByClassName("content-tab");
  const len = tabs.length;
  for (let i = 0; i < len; i++) {
    tabs[i].addEventListener("click", () => {
      for (let j = 0; j < len; j++) {
        content[j].className = "content-tab";
      }
      content[i].className = "content-tab current-tab";
    });
  }

  //event listener for dropdown activation
  document.getElementById("tutorial-select").addEventListener("click", () => {
    const drop = document.getElementById("tutorial-select");
    const options = document.getElementById("tutorial-options");
    if (drop.classList.contains("active")) {
      options.style.display = "none";
      document
        .getElementById("tutorial-arrow")
        .setAttribute("src", "data/images/arrow_right_white.svg");
    } else {
      options.style.display = "block";
      document
        .getElementById("tutorial-arrow")
        .setAttribute("src", "data/images/arrow_down_white.svg");
    }
    drop.classList.toggle("active");
    drop.classList.toggle("inactive");
    options.classList.toggle("active");
  });

  //event listeners for dropdown options
  const tutorialOptions = document.getElementsByClassName("tutorial");
  for (let i = 0; i < len; i++) {
    tutorialOptions[i].addEventListener("click", (event) => {
      for (let j = 0; j < len; j++) {
        content[j].className = "content-tab";
      }
      content[i].className = "content-tab current-tab";
      document.getElementById("tutorial-selected").textContent =
        tutorialOptions[i].textContent;
    });
  }
};
