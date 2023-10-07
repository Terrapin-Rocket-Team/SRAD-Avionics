//global variables to be used with CmdList.js
let printD;

window.onload = () => {
  //the main console output
  let con = document.getElementById("debug-console");
  // the main console input
  let inputBox = document.getElementById("input-box");
  let inputBoxText = document.getElementById("input-box-text");
  //previous commands list
  const previousCommands = [];
  //position in previous commands list
  let upArrowCounter = 0;
  //whether the main console output is currently scrolled
  let scrolled = false;
  // settings object
  let config = {};

  //add a new print to the debug window
  printD = (data) => {
    // limit the number of messages to 10000
    if (con.childNodes.length > 10000) con.remove(con.firstChild);

    //create a new message element
    const text = document.createElement("PRE");
    text.textContent = data.message;
    text.className = data.level;
    con.insertBefore(text, inputBox);

    // resume autoscrolling if the user scrolls to the bottom of the page
    if (
      scrolled &&
      window.scrollY >= con.scrollHeight - window.innerHeight - 50
    )
      scrolled = false;
    //autoscroll
    if (!scrolled) window.scrollTo(0, con.scrollHeight);
    // fix css for when the main console window starts scrolling
    if (con.scrollHeight > window.innerHeight) inputBox.style.bottom = "unset";
  };

  //listens to the print event from the preload api, allows printing directly from main
  api.on("print", (data) => {
    printD(data);
  });

  //load previous logs into the window
  api.on("previous-logs", (data) => {
    let msgs = data.split("\n");
    msgs.forEach((msg) => {
      // weed out empty strings and create a new print for each log message
      if (msg) {
        const text = document.createElement("SPAN"); //should possibly switch to pre like above
        text.textContent = msg;
        // get the message level
        text.className = msg.match(/(?<=\]\[).+(?=\])/g)[0].toLowerCase();
        con.insertBefore(text, inputBox);
      }
    });
    // css fix for when the main console window scrolls
    if (con.scrollHeight > window.innerHeight) inputBox.style.bottom = "unset";
    // if the commands cause scrolling, scroll to the bottom
    window.scrollTo(0, con.scrollHeight);
  });

  //event listener to help handle autoscrolling
  document.addEventListener("scroll", () => {
    scrolled = true;
  });

  // resets the previous command position if the command is edited
  inputBoxText.addEventListener("keyup", (event) => {
    if (event.key === "Backspace") upArrowCounter = 0;
  });

  inputBoxText.addEventListener("keydown", (event) => {
    if (event.key === "Enter") {
      //prevent newline
      event.preventDefault();

      //execute the command
      let cmd = inputBoxText.textContent;
      upArrowCounter = 0;

      previousCommands.push(cmd);
      //only store the last 50 commands
      if (previousCommands.length > 50) previousCommands.shift();

      CMDS.executeCmd(cmd.trim().split(" "));

      inputBoxText.innerHTML = "";
    }
    if (event.key === "ArrowUp") {
      event.preventDefault();
      // show previous commands if available
      if (upArrowCounter !== previousCommands.length) {
        let range, selection;
        upArrowCounter++;
        inputBoxText.textContent =
          previousCommands[previousCommands.length - upArrowCounter];

        // move the cursor to the end
        range = document.createRange(); //Create a range (a range is a like the selection but invisible)
        range.selectNodeContents(inputBoxText); //Select the entire contents of the element with the range
        range.collapse(false); //collapse the range to the end point. false means collapse to end rather than the start
        selection = window.getSelection(); //get the selection object (allows you to change selection)
        selection.removeAllRanges(); //remove any selections already made
        selection.addRange(range); //make the range you have just created the visible selection
      }
    }
    if (event.key === "ArrowDown") {
      event.preventDefault();
      // show newer commands if available
      if (upArrowCounter !== 1) {
        let range, selection;
        upArrowCounter--;
        inputBoxText.textContent =
          previousCommands[previousCommands.length - upArrowCounter];

        // move the cursor to the end
        range = document.createRange();
        range.selectNodeContents(inputBoxText);
        range.collapse(false);
        selection = window.getSelection();
        selection.removeAllRanges();
        selection.addRange(range);
      }
    }
  });

  // get the current settings from main
  api.getSettings().then((c) => {
    config = c;
  });

  //decide where to put this
  let commands = [
    new CmdNode("help", "Lists possible commands", null, () => {
      printM("Available command types:", 1);
      printM("Type the name of a command for more information", 1);
      let length = commands.length;
      for (let i = 1; i < length; i++) {
        printM(commands[i].name + " - " + commands[i].description, 2);
      }
    }),
    new CmdNode("window", "Provides general app and window controls", [
      new CmdNode(
        "-reload",
        "Reloads debug window and closes serial port connections",
        null,
        () => {
          api.reload();
        }
      ),
      new CmdNode(
        "-clear",
        "Clears all messages from the console window",
        null,
        () => {
          let con = document.getElementById("debug-console");
          let len = con.childNodes.length;
          for (let i = 0; i < len - 2; i++) {
            con.removeChild(con.firstChild);
          }
        }
      ),
      new CmdNode("-devtools", "Opens the chromium devtools", null, () => {
        api.devTools();
      }),
      new CmdNode("-opengui", "Opens the ground station GUI", null, () => {
        api.openGUI();
      }),
    ]),
    new CmdNode("settings", "Change various app settings", [
      new CmdNode(
        "-set",
        "Changes the settings specified by the next argument",
        [
          //need to sanitize inputs better
          new CmdNode("-scale", "Change the scale of the GUI window", [
            new CmdNode(1, 0, null, (cmd) => {
              config.scale = parseFloat(cmd);
            }),
          ]),
          new CmdNode("-debugScale", "The scale of the debug window", [
            new CmdNode(1, 0, null, (cmd) => {
              config.debugScale = parseFloat(cmd);
            }),
          ]),
          new CmdNode("-debug", "Turn debug mode on or off", [
            new CmdNode(1, 0, null, (cmd) => {
              if (cmd === "false" || cmd === "False") config.debug = false;
              if (cmd === "true" || cmd === "True") config.debug = true;
            }),
          ]),
          new CmdNode("-noGUI", "Turn the GUI window on or off", [
            new CmdNode(1, 0, null, (cmd) => {
              if (cmd === "false" || cmd === "False") config.noGUI = false;
              if (cmd === "true" || cmd === "True") config.noGUI = true;
            }),
          ]),
          new CmdNode(
            "-maxCacheSize",
            "The maximum size of the map tile cache",
            [
              new CmdNode(1, 0, null, (cmd) => {
                config.maxCacheSize = parseInt(cmd);
              }),
            ]
          ),
          new CmdNode(
            "-baudRate",
            "The baud rate used for connecting via serial",
            [
              new CmdNode(1, 0, null, (cmd) => {
                config.baudRate = parseInt(cmd);
              }),
            ]
          ),
        ]
      ),
      new CmdNode("-save", "Save the current settings", null, () => {
        api.setSettings(config);
      }),
      new CmdNode("-scale", "The scale of the GUI window", null, () => {
        printM(String(config.scale), 1);
      }),
      new CmdNode("-debugScale", "The scale of the debug window", null, () => {
        printM(String(config.debugScale), 1);
      }),
      new CmdNode("-debug", "Turn debug mode on or off", null, () => {
        printM(String(config.debug), 1);
      }),
      new CmdNode("-noGUI", "Turn the GUI window on or off", null, () => {
        printM(String(config.noGUI), 1);
      }),
      new CmdNode(
        "-cacheMaxSize",
        "The maximum size of the map tile cache",
        null,
        () => {
          printM(String(config.cacheMaxSize), 1);
        }
      ),
      new CmdNode(
        "-baudRate",
        "The baud rate used for connecting via serial",
        null,
        () => {
          printM(String(config.baudRate), 1);
        }
      ),
    ]),
    new CmdNode("serial", "Controls the serial port connections", [
      new CmdNode(
        "-connect",
        "Connect to the serial port specified by the next argument",
        [
          new CmdNode(1, 0, null, (cmd) => {
            api.setPort(cmd);
          }),
        ]
      ),
      new CmdNode(
        "-disconnect",
        "Disconnect from the current serial port",
        null,
        () => {
          api.closePort();
        }
      ),
      new CmdNode("-status", "Status of the serial connection", null, () => {
        api.getPortStatus().then((status) => {
          if (status.connected) {
            printM('Serial is connected on port "' + status.port + '"', 1);
          } else {
            printM("Serial is disconnected", 1);
          }
        });
      }),
      new CmdNode("-list", "Lists available serial connections", null, () => {
        api.getPorts().then((ports) => {
          printM("Available ports: ", 1);
          if (ports.length > 0) {
            ports.forEach((port) => {
              printM(port.path, 2);
            });
          } else {
            printM("None", 2);
          }
        });
      }),
    ]),
  ];

  const CMDS = new CmdList(commands);
};
