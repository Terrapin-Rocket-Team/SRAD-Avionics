window.onload = () => {
  //get stored submission form info
  let number = sessionStorage.getItem("number");
  let name = sessionStorage.getItem("name");
  let members = sessionStorage.getItem("members");

  //if there is stored info, update the inputs
  if (number !== null) document.getElementById("number").value = number;
  if (name !== null) document.getElementById("name").value = name;
  if (members !== null) document.getElementById("members").value = members;

  let files = [];

  //update the stored info on the change event
  document.getElementById("number").addEventListener("change", () => {
    sessionStorage.setItem("number", document.getElementById("number").value);
  });

  document.getElementById("name").addEventListener("change", () => {
    sessionStorage.setItem("name", document.getElementById("name").value);
  });

  document.getElementById("members").addEventListener("change", () => {
    sessionStorage.setItem("members", document.getElementById("members").value);
  });

  //prevent default action to set up file drag and drop
  document.getElementById("dropzone").addEventListener("dragover", (event) => {
    event.preventDefault();
  });

  //handle drag and dropped files
  document.getElementById("dropzone").addEventListener("drop", (event) => {
    event.preventDefault();

    //get file data
    if (event.dataTransfer.items) {
      let len = event.dataTransfer.items.length;
      for (let i = 0; i < len; i++) {
        let item = event.dataTransfer.items[i];
        if (item.kind === "file") {
          let file = item.getAsFile();
          files.push(file);
        }
      }
    }

    //clear previous file list
    const nodes = document.getElementsByClassName("new-file");
    const len = nodes.length;
    for (let i = 0; i < len; i++) {
      event.target.removeChild(nodes[0]);
    }

    //create new file list
    let counter = 0;
    files.forEach((file) => {
      const div = document.createElement("DIV");
      div.className = "new-file";
      const span = document.createElement("SPAN");
      span.textContent = file.name;
      span.id = "file-" + counter;
      const img = document.createElement("IMG");
      img.setAttribute("src", "data/images/close.svg");
      img.setAttribute("alt", "Remove");
      img.addEventListener("click", (event) => {
        event.stopPropagation();
        let index = event.target.parentElement.firstChild.id.split("-")[1];
        files.splice(index, 1);
        const nodes = document.getElementsByClassName("new-file");
        const drop = document.getElementById("dropzone");
        console.log(nodes, index);
        drop.removeChild(nodes[index]);

        let len = nodes.length;
        for (let i = 0; i < len; i++) {
          nodes[i].firstChild.id = "file-" + i;
        }
        if (len == 0) {
          document.getElementById("drop-text").style.display = "block";
        }
      });
      div.appendChild(span);
      div.appendChild(img);
      document.getElementById("dropzone").appendChild(div);
      counter++;
    });

    if (counter > 0) {
      document.getElementById("drop-text").style.display = "none";
    } else {
      document.getElementById("drop-text").style.display = "block";
    }
  });

  //handle form submission
  document.getElementById("submit-form").addEventListener("submit", (event) => {
    event.preventDefault();
    //make sure all fields are filled out
    if (
      document.getElementById("number").value != "" &&
      document.getElementById("name").value != "" &&
      document.getElementById("members").value != "" &&
      files.length > 0
    ) {
      //gather and send form data and files
      let data = new FormData(document.getElementById("submit-form"));
      files.forEach((file) => {
        data.append("files", file);
      });
      document.getElementById("loading").className = "";
      fetch("/new-submission", {
        method: "POST",
        body: data,
      }).then((res) => {
        //accept redirect
        window.location.href = res.url;
      });
    } else {
      document.getElementById("loading").className = "hidden";
      alert("Please fill out all the fields.");
    }
  });
};
