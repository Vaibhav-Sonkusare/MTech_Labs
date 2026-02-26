<?php
session_start();

// Destroy all session data
$_SESSION = [];
session_destroy();

// Remove last_step cookie (keep remember_username cookie intact)
setcookie('last_step', '', time() - 3600, '/');

header('Location: login.php');
exit;
