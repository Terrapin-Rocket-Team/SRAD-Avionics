//opens/closes hamburger menu
document.getElementById("hamburger-menu").addEventListener("click", () => {
  document.getElementById("small-nav").classList.toggle("visible");
  checkActiveDropdown();
});

//closes hamburger menu
document.getElementById("back").addEventListener("click", () => {
  document.getElementById("small-nav").classList.toggle("visible");
});

//closes dropdown menu if it is active when the hamburger menu opens
const checkActiveDropdown = () => {
  const dropdown = document.getElementById("tutorial-select");
  if (dropdown && dropdown.classList.contains("active")) {
    const options = document.getElementById("tutorial-options");
    options.style.display = "none";
    document
      .getElementById("tutorial-arrow")
      .setAttribute("src", "data/images/arrow_right_white.svg");
    dropdown.classList.toggle("active");
    dropdown.classList.toggle("inactive");
    options.classList.toggle("active");
  }
};
