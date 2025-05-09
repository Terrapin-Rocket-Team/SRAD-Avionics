window.onload = () => {
  const v = document.getElementById("video-el");

  document.getElementById("stream-video").addEventListener("click", () => {
    v.load();
    v.play();
  });

  // setInterval(() => {
  //   let l = v.buffered.length;
  //   let str = "";
  //   for (i = 0; i < l; i++) {
  //     str += "[" + v.buffered.start(i) + ", " + v.buffered.end(i) + "] ";
  //   }
  //   console.log(str);
  // }, 100);
};
