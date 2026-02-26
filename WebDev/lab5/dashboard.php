<?php
session_start();

if (!isset($_SESSION['logged_in']) || $_SESSION['logged_in'] !== true) {
    header('Location: login.php');
    exit;
}

$username = htmlspecialchars($_SESSION['username']);
$fullname = htmlspecialchars($_SESSION['fullname']);
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Dashboard - Department Management System</title>
</head>
<body>
<h2>Dashboard</h2>
<p>Welcome, <?= $fullname ?> (<?= $username ?>)</p>
<hr>
<ul>
    <li><a href="add_department.php">Add Department Data</a></li>
    <li><a href="view_department.php">View Department Data</a></li>
    <li><a href="logout.php">Logout</a></li>
</ul>
</body>
</html>
