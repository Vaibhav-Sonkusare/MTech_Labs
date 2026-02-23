// 1. Select the form and inputs
const signupForm = document.getElementById('signupform');
const nameInput = document.getElementById('name');
const regNoInput = document.getElementById('regno');
const emailInput = document.getElementById('emailid');
const passwordInput = document.getElementById('inputPassword');

signupForm.addEventListener('submit', function (e) {
    e.preventDefault();

    const name = nameInput.value.trim();
    const regNo = regNoInput.value.trim();
    const email = emailInput.value.trim();
    const password = passwordInput.value.trim();

    if (name === "" || regNo === "" || email === "" || password === "") {
        alert("Please fill in all fields.");
        return;
    }

    const user = {
        name: name,
        regNo: regNo,
        email: email,
        password: password
    };

    localStorage.setItem('studentUser', JSON.stringify(user));

    alert("Signup Successful! Redirecting to login...");

    window.location.href = "login.html";
});