window.onload = () => {
  //refresh the key to expire in another 24 hrs
  const date = new Date();
  date.setTime(date.getTime() + 86400000);
  let expires = "expires=" + date.toUTCString();
  const key = new URL(window.location.href).searchParams.get("key");
  const valid = new URL(window.location.href).searchParams.get("valid");
  document.cookie = "admin_key=" + key + ";" + expires + ";path=/";
  //get submission data
  fetch("/admin-view/submissions?valid=" + valid + "&key=" + key, {
    method: "Get",
    headers: {
      Accept: "application/json",
    },
  })
    .then((res) => res.json())
    .then((obj) => {
      //add submission data to DOM
      let list = obj.list;
      const main = document.getElementById("page-content");
      list.forEach((el) => {
        const div = document.createElement("DIV");
        div.className = "submission-info";
        div.id = "team-" + el.number;

        const short = document.createElement("DIV");
        short.className = "collapsed";

        const long = document.createElement("DIV");
        long.className = "expanded";

        const small = document.createElement("DIV");
        small.className = "small small-info";

        const team = document.createElement("P");
        team.textContent = "Team members: " + el.members;
        team.className = "team";
        long.appendChild(team);

        const longDes = document.createElement("P");
        longDes.textContent = "All Files:";
        long.appendChild(longDes);
        let singleUrl = new URL("https://urbanagamejam.com/admin-view/download");
        let singleParams = new URLSearchParams();
        singleParams.append("valid", valid);
        singleParams.append("key", key);
        singleParams.append("number", el.number);
        singleParams.append("single", true);
        el.files.forEach((file) => {
          const a = document.createElement("A");
          a.textContent = file;
          a.title = file;
          singleParams.set("files", file);
          singleUrl.search = singleParams;
          a.setAttribute("href", singleUrl);
          long.appendChild(a);
        });

        const num = document.createElement("SPAN");
        num.className = "collapsed-info";

        const title = document.createElement("SPAN");
        title.className = "collapsed-info";

        const files = document.createElement("SPAN");
        files.className = "collapsed-info files";

        num.textContent = "Team number: " + el.number;
        title.textContent = "Project title: " + el.title;
        files.textContent = "Files: " + el.files.length;

        const downloadAll = document.createElement("A");
        downloadAll.textContent = "Download All Files";
        let url = new URL("https://urbanagamejam.com/admin-view/download");
        let params = new URLSearchParams();
        params.append("valid", valid);
        params.append("key", key);
        el.files.forEach((file) => {
          params.append("files", file);
        });
        params.append("number", el.number);
        params.append("single", false);
        url.search = params;
        downloadAll.setAttribute("href", url);

        const arrow = document.createElement("IMG");
        arrow.setAttribute("src", "data/images/arrow_right.svg");
        arrow.setAttribute("alt", "Arrow");
        arrow.addEventListener("click", () => {
          document
            .getElementById("team-" + el.number)
            .classList.toggle("submission-expanded");
          if (arrow.getAttribute("src") == "data/images/arrow_right.svg") {
            arrow.setAttribute("src", "data/images/arrow_down.svg");
          } else if (
            arrow.getAttribute("src") == "data/images/arrow_down.svg"
          ) {
            arrow.setAttribute("src", "data/images/arrow_right.svg");
          }
        });

        const filesSmall = files.cloneNode();
        filesSmall.textContent = "Files: " + el.files.length;
        const downloadAllSmall = downloadAll.cloneNode();
        downloadAllSmall.textContent = "Download All Files";

        short.appendChild(num);
        short.appendChild(title);
        short.appendChild(files);
        short.appendChild(downloadAll);
        short.appendChild(arrow);
        small.appendChild(filesSmall);
        small.appendChild(downloadAllSmall);
        long.prepend(small);
        div.appendChild(short);
        div.appendChild(long);
        main.appendChild(div);
      });
    });
};
