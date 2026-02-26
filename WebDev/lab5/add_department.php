<?php
session_start();

if (!isset($_SESSION['logged_in']) || $_SESSION['logged_in'] !== true) {
    header('Location: login.php');
    exit;
}

// -------------------------------------------------------
// Determine the current step.
// Priority: POST action > GET step > last_step cookie > 1
// -------------------------------------------------------
$action = $_POST['action'] ?? '';

// Handle "Previous" buttons
if ($action === 'go_step1') {
    $_SESSION['dept_step'] = 1;
    setcookie('last_step', 1, time() + (7 * 24 * 3600), '/');
    header('Location: add_department.php');
    exit;
}
if ($action === 'go_step2') {
    $_SESSION['dept_step'] = 2;
    setcookie('last_step', 2, time() + (7 * 24 * 3600), '/');
    header('Location: add_department.php');
    exit;
}

// Handle Step 1 submission
if ($action === 'save_step1') {
    $dept_name = trim($_POST['dept_name'] ?? '');
    $hod_name  = trim($_POST['hod_name'] ?? '');
    $err = [];
    if (!$dept_name) $err[] = 'Department Name is required.';
    if (!$hod_name)  $err[] = 'HOD Name is required.';

    if (empty($err)) {
        $_SESSION['dept_data']['dept_name'] = $dept_name;
        $_SESSION['dept_data']['hod_name']  = $hod_name;
        $_SESSION['dept_step'] = 2;
        setcookie('last_step', 2, time() + (7 * 24 * 3600), '/');
        header('Location: add_department.php');
        exit;
    }
    // Fall through to display with errors
    $step_errors = $err;
}

// Handle Step 2 (Faculty) submission
if ($action === 'save_step2') {
    $fac_ids    = $_POST['fac_id']    ?? [];
    $fac_names  = $_POST['fac_name']  ?? [];
    $fac_desigs = $_POST['fac_desig'] ?? [];
    $fac_emails = $_POST['fac_email'] ?? [];
    $err = [];
    $faculty_list = [];

    foreach ($fac_ids as $i => $fid) {
        $fid    = trim($fid);
        $fname  = trim($fac_names[$i] ?? '');
        $fdesig = trim($fac_desigs[$i] ?? '');
        $femail = trim($fac_emails[$i] ?? '');
        if (!$fid && !$fname && !$fdesig && !$femail) continue; // skip blank rows
        if (!$fid)    $err[] = "Faculty row " . ($i+1) . ": ID is required.";
        if (!$fname)  $err[] = "Faculty row " . ($i+1) . ": Name is required.";
        if (!$fdesig) $err[] = "Faculty row " . ($i+1) . ": Designation is required.";
        if (!$femail) $err[] = "Faculty row " . ($i+1) . ": Email is required.";
        elseif (!filter_var($femail, FILTER_VALIDATE_EMAIL))
            $err[] = "Faculty row " . ($i+1) . ": Invalid email.";
        if (empty($err)) {
            $faculty_list[] = ['id' => $fid, 'name' => $fname, 'designation' => $fdesig, 'email' => $femail];
        }
    }

    if (empty($faculty_list) && empty($err)) {
        $err[] = 'Please add at least one faculty member.';
    }

    if (empty($err)) {
        $_SESSION['dept_data']['faculty'] = $faculty_list;
        $_SESSION['dept_step'] = 3;
        setcookie('last_step', 3, time() + (7 * 24 * 3600), '/');
        header('Location: add_department.php');
        exit;
    }
    $step_errors = $err;
}

// Handle Step 3 (Students) final submission
if ($action === 'save_step3') {
    $st_rolls  = $_POST['st_roll']  ?? [];
    $st_names  = $_POST['st_name']  ?? [];
    $st_sems   = $_POST['st_sem']   ?? [];
    $st_cgpas  = $_POST['st_cgpa']  ?? [];
    $err = [];
    $student_list = [];

    foreach ($st_rolls as $i => $roll) {
        $roll  = trim($roll);
        $sname = trim($st_names[$i] ?? '');
        $sem   = trim($st_sems[$i] ?? '');
        $cgpa  = trim($st_cgpas[$i] ?? '');
        if (!$roll && !$sname && !$sem && !$cgpa) continue;
        if (!$roll)  $err[] = "Student row " . ($i+1) . ": Roll No is required.";
        if (!$sname) $err[] = "Student row " . ($i+1) . ": Name is required.";
        if (!$sem)   $err[] = "Student row " . ($i+1) . ": Semester is required.";
        if (!$cgpa)  $err[] = "Student row " . ($i+1) . ": CGPA is required.";
        elseif (!is_numeric($cgpa) || $cgpa < 0 || $cgpa > 10)
            $err[] = "Student row " . ($i+1) . ": CGPA must be between 0 and 10.";
        if (empty($err)) {
            $student_list[] = ['roll' => $roll, 'name' => $sname, 'semester' => $sem, 'cgpa' => $cgpa];
        }
    }

    if (empty($student_list) && empty($err)) {
        $err[] = 'Please add at least one student.';
    }

    if (empty($err)) {
        $_SESSION['dept_data']['students'] = $student_list;
        // Now build and save XML
        $result = buildAndSaveXML($_SESSION['dept_data']);
        if ($result['success']) {
            // Clear step data
            unset($_SESSION['dept_step']);
            setcookie('last_step', '', time() - 3600, '/');
            header('Location: summary.php');
            exit;
        } else {
            $xml_errors = $result['errors'];
        }
    } else {
        $step_errors = $err;
    }
}

// -------------------------------------------------------
// Determine which step to display
// -------------------------------------------------------
// If step not set in session, check cookie, else default to 1
if (!isset($_SESSION['dept_step'])) {
    $cookie_step = intval($_COOKIE['last_step'] ?? 1);
    $_SESSION['dept_step'] = ($cookie_step >= 1 && $cookie_step <= 3) ? $cookie_step : 1;
}
$current_step = $_SESSION['dept_step'];

// -------------------------------------------------------
// Function: Build XML, save, validate against DTD and XSD
// -------------------------------------------------------
function buildAndSaveXML(array $data): array
{
    $dir      = __DIR__;
    $xml_file = $dir . '/department.xml';
    $xsd_file = $dir . '/department.xsd';

    // --- Build XML string ---
    $dept_name = htmlspecialchars($data['dept_name'], ENT_XML1);
    $hod_name  = htmlspecialchars($data['hod_name'],  ENT_XML1);

    $faculty_xml = '';
    foreach ($data['faculty'] as $f) {
        $faculty_xml .= sprintf(
            "\t\t<faculty id=\"%s\">\n\t\t\t<name>%s</name>\n\t\t\t<designation>%s</designation>\n\t\t\t<email>%s</email>\n\t\t</faculty>\n",
            htmlspecialchars($f['id'],          ENT_XML1),
            htmlspecialchars($f['name'],        ENT_XML1),
            htmlspecialchars($f['designation'], ENT_XML1),
            htmlspecialchars($f['email'],       ENT_XML1)
        );
    }

    $student_xml = '';
    foreach ($data['students'] as $s) {
        $student_xml .= sprintf(
            "\t\t<student roll=\"%s\">\n\t\t\t<name>%s</name>\n\t\t\t<semester>%s</semester>\n\t\t\t<cgpa>%s</cgpa>\n\t\t</student>\n",
            htmlspecialchars($s['roll'],     ENT_XML1),
            htmlspecialchars($s['name'],     ENT_XML1),
            htmlspecialchars($s['semester'], ENT_XML1),
            htmlspecialchars($s['cgpa'],     ENT_XML1)
        );
    }

    // NOTE: DTD reference uses absolute path so DOMDocument can locate it
    //       regardless of the web server's working directory.
    $xml_string  = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    $xml_string .= "<!DOCTYPE department SYSTEM \"{$dir}/department.dtd\">\n";
    $xml_string .= "<department>\n";
    $xml_string .= "\t<dept_name>{$dept_name}</dept_name>\n";
    $xml_string .= "\t<hod_name>{$hod_name}</hod_name>\n";
    $xml_string .= "\t<faculty_list>\n{$faculty_xml}\t</faculty_list>\n";
    $xml_string .= "\t<student_list>\n{$student_xml}\t</student_list>\n";
    $xml_string .= "</department>\n";

    // --- Write file; fail early if write fails ---
    $bytes = file_put_contents($xml_file, $xml_string);
    if ($bytes === false) {
        return [
            'success' => false,
            'errors'  => ['Could not write department.xml. Check that the web server has write permission to: ' . $dir],
        ];
    }

    $errors = [];

    // --- DTD Validation ---
    // Load the XML string directly (avoids stale-file issues).
    // DOMDocument will fetch the DTD using the absolute SYSTEM path embedded above.
    libxml_use_internal_errors(true);
    $dom_dtd = new DOMDocument();
    $dom_dtd->loadXML($xml_string);
    // validate() checks the document against its DOCTYPE/DTD
    if (!$dom_dtd->validate()) {
        foreach (libxml_get_errors() as $e) {
            $errors[] = '[DTD] Line ' . $e->line . ': ' . trim($e->message);
        }
    }
    libxml_clear_errors();
    libxml_use_internal_errors(false);

    // --- XSD Validation ---
    // Load from the saved file path so XSD can resolve any relative imports.
    if (file_exists($xsd_file)) {
        libxml_use_internal_errors(true);
        $dom_xsd = new DOMDocument();
        $dom_xsd->load($xml_file);
        if (!$dom_xsd->schemaValidate($xsd_file)) {
            foreach (libxml_get_errors() as $e) {
                $errors[] = '[XSD] Line ' . $e->line . ': ' . trim($e->message);
            }
        }
        libxml_clear_errors();
        libxml_use_internal_errors(false);
    } else {
        $errors[] = '[XSD] department.xsd file not found at: ' . $xsd_file;
    }

    if (empty($errors)) {
        // Rewrite the final XML with the relative DTD path (cleaner saved file)
        $clean_xml  = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        $clean_xml .= "<!DOCTYPE department SYSTEM \"department.dtd\">\n";
        $clean_xml .= substr($xml_string, strpos($xml_string, '<department>'));
        file_put_contents($xml_file, $clean_xml);
        return ['success' => true, 'errors' => []];
    }
    return ['success' => false, 'errors' => $errors];
}
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Add Department - Department Management System</title>
</head>
<body>
<h2>Add Department Data</h2>
<p>Step <?= $current_step ?> of 3</p>
<hr>

<?php if (!empty($step_errors)): ?>
    <ul style="color:red;">
        <?php foreach ($step_errors as $e): ?>
            <li><?= htmlspecialchars($e) ?></li>
        <?php endforeach; ?>
    </ul>
<?php endif; ?>

<?php if (!empty($xml_errors)): ?>
    <h3 style="color:red;">Validation Failed</h3>
    <ul style="color:red;">
        <?php foreach ($xml_errors as $e): ?>
            <li><?= htmlspecialchars($e) ?></li>
        <?php endforeach; ?>
    </ul>
<?php endif; ?>

<!-- ===================== STEP 1 ===================== -->
<?php if ($current_step === 1): ?>
<h3>Step 1: Department Information</h3>
<form method="post" action="add_department.php">
    <input type="hidden" name="action" value="save_step1">
    <table>
        <tr>
            <td><label for="dept_name">Department Name:</label></td>
            <td><input type="text" id="dept_name" name="dept_name" required
                       value="<?= htmlspecialchars($_SESSION['dept_data']['dept_name'] ?? '') ?>"></td>
        </tr>
        <tr>
            <td><label for="hod_name">HOD Name:</label></td>
            <td><input type="text" id="hod_name" name="hod_name" required
                       value="<?= htmlspecialchars($_SESSION['dept_data']['hod_name'] ?? '') ?>"></td>
        </tr>
        <tr>
            <td></td>
            <td><input type="submit" value="Next &rarr;"></td>
        </tr>
    </table>
</form>

<!-- ===================== STEP 2 ===================== -->
<?php elseif ($current_step === 2): ?>
<h3>Step 2: Add Faculty Members</h3>
<form method="post" action="add_department.php" id="faculty_form">
    <input type="hidden" name="action" value="save_step2">
    <table id="faculty_table" border="1" cellpadding="4">
        <thead>
            <tr>
                <th>Faculty ID</th>
                <th>Name</th>
                <th>Designation</th>
                <th>Email</th>
                <th>Action</th>
            </tr>
        </thead>
        <tbody id="faculty_body">
        <?php
        $faculty_rows = $_SESSION['dept_data']['faculty'] ?? [[]];
        foreach ($faculty_rows as $f):
        ?>
            <tr>
                <td><input type="text" name="fac_id[]"    value="<?= htmlspecialchars($f['id'] ?? '') ?>"></td>
                <td><input type="text" name="fac_name[]"  value="<?= htmlspecialchars($f['name'] ?? '') ?>"></td>
                <td><input type="text" name="fac_desig[]" value="<?= htmlspecialchars($f['designation'] ?? '') ?>"></td>
                <td><input type="email" name="fac_email[]" value="<?= htmlspecialchars($f['email'] ?? '') ?>"></td>
                <td><button type="button" onclick="removeRow(this)">Remove</button></td>
            </tr>
        <?php endforeach; ?>
        </tbody>
    </table>
    <br>
    <button type="button" onclick="addFacultyRow()">+ Add Faculty Row</button>
    <br><br>
    <input type="submit" value="&larr; Previous" formaction="add_department.php" formmethod="post"
           onclick="document.getElementById('faculty_form').querySelector('[name=action]').value='go_step1'">
    <input type="submit" value="Next &rarr;">
</form>

<script>
function addFacultyRow() {
    var tbody = document.getElementById('faculty_body');
    var row = document.createElement('tr');
    row.innerHTML = '<td><input type="text" name="fac_id[]"></td>' +
                    '<td><input type="text" name="fac_name[]"></td>' +
                    '<td><input type="text" name="fac_desig[]"></td>' +
                    '<td><input type="email" name="fac_email[]"></td>' +
                    '<td><button type="button" onclick="removeRow(this)">Remove</button></td>';
    tbody.appendChild(row);
}
function removeRow(btn) {
    var row = btn.closest('tr');
    var tbody = document.getElementById('faculty_body');
    if (tbody.rows.length > 1) {
        row.remove();
    } else {
        alert('At least one faculty row is required.');
    }
}
</script>

<!-- ===================== STEP 3 ===================== -->
<?php elseif ($current_step === 3): ?>
<h3>Step 3: Add Students</h3>
<form method="post" action="add_department.php" id="student_form">
    <input type="hidden" name="action" value="save_step3">
    <table id="student_table" border="1" cellpadding="4">
        <thead>
            <tr>
                <th>Roll No</th>
                <th>Name</th>
                <th>Semester</th>
                <th>CGPA</th>
                <th>Action</th>
            </tr>
        </thead>
        <tbody id="student_body">
        <?php
        $student_rows = $_SESSION['dept_data']['students'] ?? [[]];
        foreach ($student_rows as $s):
        ?>
            <tr>
                <td><input type="text"   name="st_roll[]" value="<?= htmlspecialchars($s['roll'] ?? '') ?>"></td>
                <td><input type="text"   name="st_name[]" value="<?= htmlspecialchars($s['name'] ?? '') ?>"></td>
                <td><input type="text"   name="st_sem[]"  value="<?= htmlspecialchars($s['semester'] ?? '') ?>"></td>
                <td><input type="number" name="st_cgpa[]" step="0.01" min="0" max="10"
                           value="<?= htmlspecialchars($s['cgpa'] ?? '') ?>"></td>
                <td><button type="button" onclick="removeRow(this)">Remove</button></td>
            </tr>
        <?php endforeach; ?>
        </tbody>
    </table>
    <br>
    <button type="button" onclick="addStudentRow()">+ Add Student Row</button>
    <br><br>
    <input type="submit" value="&larr; Previous" formaction="add_department.php" formmethod="post"
           onclick="document.getElementById('student_form').querySelector('[name=action]').value='go_step2'">
    <input type="submit" value="Submit">
</form>

<script>
function addStudentRow() {
    var tbody = document.getElementById('student_body');
    var row = document.createElement('tr');
    row.innerHTML = '<td><input type="text" name="st_roll[]"></td>' +
                    '<td><input type="text" name="st_name[]"></td>' +
                    '<td><input type="text" name="st_sem[]"></td>' +
                    '<td><input type="number" name="st_cgpa[]" step="0.01" min="0" max="10"></td>' +
                    '<td><button type="button" onclick="removeRow(this)">Remove</button></td>';
    tbody.appendChild(row);
}
function removeRow(btn) {
    var row = btn.closest('tr');
    var tbody = document.getElementById('student_body');
    if (tbody.rows.length > 1) {
        row.remove();
    } else {
        alert('At least one student row is required.');
    }
}
</script>
<?php endif; ?>

<hr>
<p><a href="dashboard.php">Back to Dashboard</a></p>
</body>
</html>
