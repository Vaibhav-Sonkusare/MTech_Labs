const isLoggedIn = sessionStorage.getItem('isLoggedIn');
const currentUser = sessionStorage.getItem('currentUser');
const loginTime = sessionStorage.getItem('loginTime');

const SESSION_TIMEOUT = 5 * 60 * 1000;

if (!isLoggedIn || !loginTime) {
    alert("You haven't logged in yet!");
    window.location.href = "login.html";
} else {
    const currentTime = Date.now();

    if (currentTime - loginTime > SESSION_TIMEOUT) {
        alert("Session expired. Please login again.");
        sessionStorage.clear();
        window.location.href = "login.html";
    } else {
        document.getElementById('displayUsername').innerText = currentUser;
    }
}

document.getElementById('logoutBtn').addEventListener('click', function () {
    sessionStorage.clear();
    window.location.href = "login.html";
});

document.addEventListener('click', () => {
    sessionStorage.setItem('loginTime', Date.now());
});


const addBtn = document.getElementById('addCourseBtn');
const courseSelect = document.getElementById('courseSelect');
const tableBody = document.getElementById('courseTableBody');

let enrolledCourses =
    JSON.parse(localStorage.getItem(`courses_${currentUser}`)) || [];

function renderCourses() {
    tableBody.innerHTML = "";

    enrolledCourses.forEach(course => {
        const row = document.createElement('tr');

        const nameCell = document.createElement('td');
        nameCell.innerText = course;

        const actionCell = document.createElement('td');
        const removeBtn = document.createElement('button');
        removeBtn.innerText = "Remove";
        removeBtn.className = "btn btn-danger btn-sm";

        removeBtn.addEventListener('click', function () {
            enrolledCourses = enrolledCourses.filter(c => c !== course);

            localStorage.setItem(
                `courses_${currentUser}`,
                JSON.stringify(enrolledCourses)
            );

            renderCourses();
        });

        actionCell.appendChild(removeBtn);
        row.appendChild(nameCell);
        row.appendChild(actionCell);
        tableBody.appendChild(row);
    });
}

renderCourses();

addBtn.addEventListener('click', function () {
    const selectedCourse = courseSelect.value;

    if (!selectedCourse || selectedCourse === "Choose a course...") {
        alert("Please select a valid course first.");
        return;
    }

    if (enrolledCourses.includes(selectedCourse)) {
        alert("You are already enrolled in this course!");
        return;
    }

    enrolledCourses.push(selectedCourse);

    localStorage.setItem(
        `courses_${currentUser}`,
        JSON.stringify(enrolledCourses)
    );

    renderCourses();
});
