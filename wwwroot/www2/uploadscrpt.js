const uploadFIle = async () => {
  const fileInput = document.getElementById("file1");
  const formData = new FormData();
  if (!fileInput.files || fileInput.files.length == 0) {
    alert("Please select some file");
    return;
  }
  formData.append("file1", fileInput.files[0]);
  try {
    const response = await fetch("/uploads", {
      method: "POST",
      body: formData,
    });
    const text = await response.text();
    if (text == "") {
      document.getElementById("divOutput").innerHTML = "Created";
      if (confirm("Seems OK, open uploads/ folder?")) window.open("/uploads/");
    } else document.getElementById("divOutput").innerHTML = text;
  } catch (error) {
    alert("Fail");
  }
};
