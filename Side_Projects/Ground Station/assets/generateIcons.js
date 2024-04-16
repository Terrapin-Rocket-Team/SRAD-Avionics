const { Icns, IcnsImage } = require("@fiahfy/icns");
const { convert } = require("convert-svg-to-png");
const pngico = require("png-to-ico");
const fs = require("fs");
const path = require("path");

(async function main() {
  const svgPath = path.join(__dirname, "logo.svg");
  const fileNames = [];

  try {
    if (!fs.existsSync(path.join(__dirname, "temp")))
      fs.mkdirSync(path.join(__dirname, "temp"));
  } catch (err) {
    console.error("Failed making temp directory: " + err.message);
  }

  try {
    const logoSVG = fs.readFileSync(svgPath);
    const icns = new Icns();
    const iconsets = [
      ["icp4"],
      ["icp5", "ic11"],
      ["icp6", "ic12"],
      ["ic07"],
      ["ic08", "ic13"],
      ["ic09", "ic14"],
      ["ic10"],
    ];

    // 16 32 64 128 256 512 1024
    for (let i = 0; i < 7; i++) {
      let size = 16 * Math.pow(2, i);

      fileNames[i] = path.join(
        __dirname,
        "temp",
        "logo" + size + "x" + size + ".png"
      );

      let currentPNG = await convert(logoSVG, {
        height: size,
        width: size,
      });

      let numOfIcons = iconsets[i].length;
      for (let j = 0; j < numOfIcons; j++) {
        icns.append(IcnsImage.fromPNG(currentPNG, iconsets[i][j]));
      }

      fs.writeFileSync(fileNames[i], currentPNG);
    }

    fs.writeFileSync(path.join(__dirname, "logo.icns"), icns.data);
  } catch (err) {
    console.error("Failed generating ICNS file: " + err.message);
  }

  try {
    pngico(fileNames).then((ico) => {
      fs.writeFileSync(path.join(__dirname, "logo.ico"), ico);
    });
  } catch (err) {
    console.error("Failed generating ICO file: " + err.message);
  }

  try {
    //save the 1024x1024 png
    fs.renameSync(fileNames.pop(), path.join(__dirname, "logo.png"));

    //copy the svg logo to src/images
    fs.copyFileSync(
      svgPath,
      path.join(__dirname, "..", "src", "images", "logo.svg")
    );

    //remove temp folder
    fs.rm(path.join(__dirname, "temp"), { recursive: true }, () => {});
  } catch (err) {
    console.error("Failed while finishing up: " + err.message);
  }

  console.log("Successfully generated icons.");
})();
