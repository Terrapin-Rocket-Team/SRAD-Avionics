window.onload = () => {
  //get stored contact form info
  let name = sessionStorage.getItem("name");
  let email = sessionStorage.getItem("email");
  let message = sessionStorage.getItem("message");

  //if there is stored info, update the inputs
  if (name !== null) document.getElementById("name").value = name;
  if (email !== null) document.getElementById("email").value = email;
  if (message !== null) document.getElementById("message").value = message;

  //update the stored info on the change event
  document.getElementById("name").addEventListener("change", () => {
    sessionStorage.setItem("name", document.getElementById("name").value);
  });

  document.getElementById("email").addEventListener("change", () => {
    sessionStorage.setItem("email", document.getElementById("email").value);
  });

  document.getElementById("message").addEventListener("change", () => {
    sessionStorage.setItem("message", document.getElementById("message").value);
  });

  //form submission override
  document
    .getElementById("contact-form")
    .addEventListener("submit", (event) => {
      event.preventDefault();
      //make sure all fields are filled out
      if (
        document.getElementById("name").value != "" &&
        document.getElementById("email").value != "" &&
        document.getElementById("message").value != ""
      ) {
        let data = {
          name: document.getElementById("name").value,
          email: document.getElementById("email").value,
          message: document.getElementById("message").value,
        };
        //gather and send data
        fetch("/contact-form", {
          method: "POST",
          body: JSON.stringify(data),
          headers: {
            "Content-Type": "application/json",
          },
        })
          .then((res) => res.json())
          .then((text) => {
            //wait for a success or error from server
            if (text.status === "success") {
              document.getElementById("name").value = "";
              document.getElementById("email").value = "";
              document.getElementById("message").value = "";
              alert("Message sent.");
            }
            if (text.status === "error") {
              alert("An error occurred.");
            }
          });
      } else {
        alert("Please fill out all the fields.");
      }
    });
};
