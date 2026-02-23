const loginForm = document.getElementById('loginForm');
const forgotLink = document.getElementById('forgotPasswordLink');

loginForm.addEventListener('submit', function (e) {
    e.preventDefault();

    const enteredRegNo = document.getElementById('loginRegNo').value.trim();
    const enteredPass = document.getElementById('loginPassword').value.trim();

    const storedUserJSON = localStorage.getItem('studentUser');

    if (!storedUserJSON) {
        alert("No user found. Please sign up first.");
        return;
    }

    const storedUser = JSON.parse(storedUserJSON);

    if (enteredRegNo === storedUser.regNo && enteredPass === storedUser.password) {

        sessionStorage.setItem('isLoggedIn', 'true');
        sessionStorage.setItem('currentUser', storedUser.name);

        sessionStorage.setItem('loginTime', Date.now());

        alert("Login Successful!");
        window.location.href = "dashboard.html";
    }
    else {
        alert("Invalid Registration Number or Password.");
    }
});

forgotLink.addEventListener('click', function (e) {
    e.preventDefault();
    alert("Please contact the admin to reset your password.");
});