/**
 * Class that holds a list of console commands
 */
class CmdList {
  /**
   * @param {Array<CmdNode>} commands The basis commands for the console
   */
  constructor(commands) {
    this.rootCommands = commands;
  }

  /**
   * Searches the command list for the specific command and executes that command
   * @param {Array<String>} cmd The command to be executed
   */
  executeCmd(cmd) {
    //print the command to the console
    if (cmd) printC(cmd.join(" "));
    //check if there are child commands to be processed
    if (cmd && cmd[0]) {
      let cmdFound = false;
      //loop through the root commands until one matches the first given command
      this.rootCommands.forEach((command) => {
        if (command.name === cmd[0]) {
          //remove the first command and traverse with the resulting array
          cmd.shift();
          command.traverse(cmd);
          cmdFound = true;
        }
      });
      // if the command cannot be found in the command list print an error
      if (!cmdFound) printE('Command "' + cmd[0] + '" not found');
    }
  }
}

/**
 * Class that represents a single node in the command list tree
 */
class CmdNode {
  /**
   * @param {String} name The name used to call the command
   * @param {String} description A short description of the purpose of the command
   * @param {Array<CmdNode>} children An array containing the child commands
   * @param {Function} action The function to call when the command is used
   */
  constructor(name, description, children, action) {
    this.name = name;
    this.description = description;
    this.children = children;
    this.action = action;
  }

  /**
   * Look through the tree to find and execute the correct string of commands
   * @param {Array<String>} cmd The command to be executed
   */
  traverse(cmd) {
    //check if there are remaining commands and whether this command has child commands
    if (cmd && cmd[0] && this.children) {
      let cmdFound = false;
      // loop through the child commands until one matches the first given command
      this.children.forEach((child) => {
        // if the name of the child is 1, the child command is really an input field, so don't remove the input
        if (child.name === 1) {
          child.traverse(cmd);
          cmdFound = true;
        } else if (child.name === cmd[0]) {
          // otherwise if the name matches remove the first command and continue traversing
          cmd.shift();
          child.traverse(cmd);
          cmdFound = true;
        }
      });
      // if the command cannot be found in the child command list print an error
      if (!cmdFound) printE('Command "' + cmd[0] + '" not found');
    } else {
      if (!this.action) {
        //if the command does not have an action, list its child commands in the console
        this.listChildren();
      } else if (this.name === 1) {
        // if the name of the command is 1 pass the first command (the input) as an argument to the command's action
        this.action(cmd[0]);
      } else {
        //otherwise just carry out the action
        this.action();
      }
    }
    return;
  }

  /**
   * Lists the child commands of the calling command node
   */
  listChildren() {
    printM(this.name + " : " + this.description, 1);
    if (this.children) {
      // loop through the command's children and print their name and description
      this.children.forEach((child) => {
        if (child.name !== 1) printM(child.name + " : " + child.description, 2);
      });
    }
  }
}

/**
 * Prints the command to the console when it is run
 * @param {String} message The command to to printed
 */
const printC = (message) => {
  printD({ message: "cmd > " + message, level: "debug" });
};

/**
 * Prints a message to the console
 * @param {String} message The message to be printed
 * @param {Number} indentLevel The amount of indent for the message
 */
const printM = (message, indentLevel) => {
  if (indentLevel < 1) indentLevel = 1;
  printD({
    message: "      " + "   ".repeat(indentLevel - 1) + message,
    level: "debug",
  });
};

/**
 * Prints an error message to the console
 * @param {String} message The error message to be printed
 */
const printE = (message) => {
  printD({ message: "      " + message, level: "error" });
};
