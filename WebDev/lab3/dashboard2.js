const isLoggedIn = sessionStorage.getItem('isLoggedIn');
const currentUser = sessionStorage.getItem('currentUser');

if (!isLoggedIn) {
    alert("You haven't logged in yet!");
    window.location.href = "login.html";
} else {
    document.getElementById('displayUsername').innerText = currentUser;

    const storedUserContentJSON = localStorage.getItem('studentUserData');

    if (storedUserContentJSON) {
        const storedUserContent = JSON.parse(storedUserContentJSON)

        for (let i=0; i< storedUserContent.)
    }
}

// Logout Logic
document.getElementById('logoutBtn').addEventListener('click', function() {
    sessionStorage.clear();
    window.location.href = "login.html";
});


const addBtn = document.getElementById('addCourseBtn');
const courseSelect = document.getElementById('courseSelect');
const tableBody = document.getElementById('courseTableBody');

addBtn.addEventListener('click', function() {
    const selectedCourse = courseSelect.value;

    if (selectedCourse === "Choose a course...") {
        alert("Please select a valid course first.");
        return;
    }

    const existingRows = tableBody.getElementsByTagName('tr');
    for (let i = 0; i < existingRows.length; i++) {
        if (existingRows[i].cells[0].innerText === selectedCourse) {
            alert("You are already enrolled in this course!");
            return;
        }
    }

    const newRow = document.createElement('tr');

    const nameCell = document.createElement('td');
    nameCell.innerText = selectedCourse;

    const actionCell = document.createElement('td');
    const removeBtn = document.createElement('button');
    removeBtn.innerText = "Remove";
    removeBtn.className = "btn btn-danger btn-sm";
    
    removeBtn.addEventListener('click', function() {
        tableBody.removeChild(newRow);
    });

    actionCell.appendChild(removeBtn);

    newRow.appendChild(nameCell);
    newRow.appendChild(actionCell);

    tableBody.appendChild(newRow);
});