<?php
session_start();

if (!isset($_SESSION['logged_in']) || $_SESSION['logged_in'] !== true) {
    header('Location: login.php');
    exit;
}

$xml_file = __DIR__ . '/department.xml';
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>View Department - Department Management System</title>
</head>
<body>
<h2>View Department Data</h2>
<hr>

<?php if (!file_exists($xml_file)): ?>
    <p>No department data found. <a href="add_department.php">Add Data</a></p>
<?php else:
    $xml = simplexml_load_file($xml_file);
?>
    <h3>Department Information</h3>
    <table border="1" cellpadding="4">
        <tr><th>Department Name</th><td><?= htmlspecialchars((string)$xml->dept_name) ?></td></tr>
        <tr><th>HOD Name</th><td><?= htmlspecialchars((string)$xml->hod_name) ?></td></tr>
    </table>

    <h3>Faculty List</h3>
    <table border="1" cellpadding="4">
        <thead>
            <tr><th>Faculty ID</th><th>Name</th><th>Designation</th><th>Email</th></tr>
        </thead>
        <tbody>
        <?php foreach ($xml->faculty_list->faculty as $f): ?>
            <tr>
                <td><?= htmlspecialchars((string)$f['id']) ?></td>
                <td><?= htmlspecialchars((string)$f->name) ?></td>
                <td><?= htmlspecialchars((string)$f->designation) ?></td>
                <td><?= htmlspecialchars((string)$f->email) ?></td>
            </tr>
        <?php endforeach; ?>
        </tbody>
    </table>

    <h3>Student List</h3>
    <table border="1" cellpadding="4">
        <thead>
            <tr><th>Roll No</th><th>Name</th><th>Semester</th><th>CGPA</th></tr>
        </thead>
        <tbody>
        <?php foreach ($xml->student_list->student as $s): ?>
            <tr>
                <td><?= htmlspecialchars((string)$s['roll']) ?></td>
                <td><?= htmlspecialchars((string)$s->name) ?></td>
                <td><?= htmlspecialchars((string)$s->semester) ?></td>
                <td><?= htmlspecialchars((string)$s->cgpa) ?></td>
            </tr>
        <?php endforeach; ?>
        </tbody>
    </table>
<?php endif; ?>

<hr>
<p><a href="dashboard.php">Back to Dashboard</a></p>
</body>
</html>
