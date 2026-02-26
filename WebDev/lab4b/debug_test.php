<?php
require_once 'functions.php';

// Test file writing
$test = ['status' => 'ok'];
$res = file_put_contents('test_db_write.json', json_encode($test));

if ($res === false) {
    echo "Error writing to file. Check permissions.";
} else {
    echo "File write successful. ";
}

// Test session
if (isset($_SESSION)) {
    $_SESSION['test'] = 123;
    echo "Session is active.";
} else {
    echo "Session is NOT active.";
}

// Check users.json
if (file_exists('users.json')) {
    echo " users.json exists.";
} else {
    echo " users.json does NOT exist.";
}
?>
