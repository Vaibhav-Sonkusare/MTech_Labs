<!DOCTYPE html>
<html>
<head>
    <title>Student Information System</title>
    <style>
        body { font-family: sans-serif; padding: 20px; }
        .section { border: 1px solid #ccc; padding: 15px; margin-bottom: 20px; }
        .message { color: green; font-weight: bold; margin-bottom: 10px; }
        .error { color: red; font-weight: bold; margin-bottom: 10px; }
        input[type="text"], input[type="email"] { margin-bottom: 10px; display: block; }
        table { border-collapse: collapse; width: 100%; margin-top: 10px; }
        th, td { border: 1px solid #ccc; padding: 8px; text-align: left; }
        th { background-color: #f4f4f4; }
    </style>
</head>
<body>
    <h1>Student Information System</h1>
    
    <div id="statusMessage"></div>

    <div class="section">
        <h2>Admin Panel (SOAP Service)</h2>
        <form id="soapForm" onsubmit="event.preventDefault(); return false;">
            <label>Student ID: <input type="text" id="student_id" required></label>
            <label>Name: <input type="text" id="student_name"></label>
            <label>Email: <input type="email" id="student_email"></label>
            <button type="button" onclick="setAction('add')">Add Student</button>
            <button type="button" onclick="setAction('update')">Update Student</button>
            <button type="button" onclick="setAction('delete')">Delete Student (ID only)</button>
        </form>
    </div>

    <div class="section">
        <h2>Student Portal (REST Service)</h2>
        <p><i>API Endpoints (Click to view raw JSON):</i>
           <br>- <a href="rest_service.php?endpoint=courses" target="_blank">GET /courses</a>
           <br>- <a href="rest_service.php?endpoint=results/1" target="_blank">GET /results/{Student_ID} (Example ID: 1)</a>
        </p>
        <form onsubmit="event.preventDefault(); getResults();">
            <label>Enter Student ID to view results: 
                <input type="text" id="student_query_id" required>
            </label>
            <button type="submit">Get Results</button>
        </form>
        <div id="resultsContainer"></div>

        <hr>
        
        <form onsubmit="event.preventDefault(); getCourses();">
            <button type="submit">View All Courses</button>
        </form>
        <div id="coursesContainer"></div>
    </div>

    <div class="section">
        <h2>Reports</h2>
        <a href="report.php" target="_blank"><button>View System Report (XML to HTML via XSLT)</button></a>
    </div>

    <script>
        let currentSoapAction = '';

        function setAction(action) {
            currentSoapAction = action;
            // Native HTML5 validation check
            const form = document.getElementById('soapForm');
            if (!form.checkValidity()) {
                form.reportValidity();
                return;
            }
            submitSoap();
        }

        function showStatus(message, isError = false) {
            const statusDiv = document.getElementById('statusMessage');
            statusDiv.className = isError ? 'error' : 'message';
            statusDiv.innerText = message;
        }

        async function submitSoap() {
            if (!currentSoapAction) return;
            const id = document.getElementById('student_id').value;
            const name = document.getElementById('student_name').value;
            const email = document.getElementById('student_email').value;

            if (!id) {
                showStatus('Student ID is required.', true);
                return;
            }

            // Simple validation for add/update
            if ((currentSoapAction === 'add' || currentSoapAction === 'update') && (!name || !email)) {
                showStatus('Please provide name and email for add/update.', true);
                return;
            }

            const envelope = '<' + '?xml version="1.0" encoding="UTF-8"?' + '>\n' +
`<SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/" xmlns:ns1="http://localhost/student_service" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
<SOAP-ENV:Body>
<ns1:${currentSoapAction}Student>
<param0 xsi:type="xsd:string">${id}</param0>
<param1 xsi:type="xsd:string">${name}</param1>
<param2 xsi:type="xsd:string">${email}</param2>
</ns1:${currentSoapAction}Student>
</SOAP-ENV:Body>
</SOAP-ENV:Envelope>`;

            showStatus('Sending SOAP request...');
            try {
                const response = await fetch('soap_service.php', {
                    method: 'POST',
                    headers: { 
                        'Content-Type': 'text/xml; charset=utf-8',
                        'SOAPAction': `"http://localhost/student_service#${currentSoapAction}Student"`
                    },
                    body: envelope
                });

                const text = await response.text();
                
                // Parse out the return value from SOAP response
                const parser = new DOMParser();
                const xmlDoc = parser.parseFromString(text, "text/xml");
                const returnNode = xmlDoc.getElementsByTagName('return')[0];
                const faultNode = xmlDoc.getElementsByTagName('SOAP-ENV:Fault')[0] || xmlDoc.getElementsByTagName('Fault')[0];

                if (returnNode) {
                    const msg = returnNode.textContent;
                    const isError = msg.includes('not found') || msg.includes('already exists');
                    showStatus(msg, isError);
                } else if (faultNode || text.includes("Fault")) {
                     showStatus("SOAP Fault occurred: " + text, true);
                } else {
                    showStatus("Operation completed.", false);
                }
            } catch (err) {
                showStatus('SOAP Error: ' + err.message, true);
            }
        }

        async function getResults() {
            const id = document.getElementById('student_query_id').value;
            if (!id) return;

            showStatus('Fetching REST data...');
            const container = document.getElementById('resultsContainer');
            container.innerHTML = 'Loading...';

            try {
                const response = await fetch('rest_service.php?endpoint=results/' + encodeURIComponent(id));
                const data = await response.json();

                if (data.error) {
                    showStatus('Error: ' + data.error, true);
                    container.innerHTML = '';
                    return;
                }

                if (data.length === 0) {
                    container.innerHTML = '<p>No results found for this student.</p>';
                    return;
                }

                let html = '<h3>Results for Student ID: ' + htmlspecialchars(id) + '</h3>';
                html += '<table><tr><th>Course ID</th><th>Title</th><th>Grade</th></tr>';
                data.forEach(r => {
                    html += `<tr><td>${htmlspecialchars(r.courseId)}</td><td>${htmlspecialchars(r.title)}</td><td>${htmlspecialchars(r.grade)}</td></tr>`;
                });
                html += '</table>';
                container.innerHTML = html;
                // Removed rapid clearing of status message
            } catch (err) {
                showStatus('REST Error: ' + err.message, true);
                container.innerHTML = '';
            }
        }

        async function getCourses() {
            showStatus('Fetching REST data...');
            const container = document.getElementById('coursesContainer');
            container.innerHTML = 'Loading...';

            try {
                const response = await fetch('rest_service.php?endpoint=courses');
                const data = await response.json();

                if (data.error) {
                    showStatus('Error: ' + data.error, true);
                    container.innerHTML = '';
                    return;
                }

                if (data.length === 0) {
                    container.innerHTML = '<p>No courses found.</p>';
                    return;
                }

                let html = '<h3>Available Courses</h3>';
                html += '<table><tr><th>Course ID</th><th>Title</th><th>Credits</th></tr>';
                data.forEach(c => {
                    html += `<tr><td>${htmlspecialchars(c.id)}</td><td>${htmlspecialchars(c.title)}</td><td>${htmlspecialchars(c.credits)}</td></tr>`;
                });
                html += '</table>';
                container.innerHTML = html;
                // Removed rapid clearing of status message
            } catch (err) {
                showStatus('REST Error: ' + err.message, true);
                container.innerHTML = '';
            }
        }

        function htmlspecialchars(str) {
            if (!str) return '';
            return String(str)
                .replace(/&/g, '&amp;')
                .replace(/</g, '&lt;')
                .replace(/>/g, '&gt;')
                .replace(/"/g, '&quot;')
                .replace(/'/g, '&#039;');
        }
    </script>
</body>
</html>
