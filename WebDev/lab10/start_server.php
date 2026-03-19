<?php
session_start();

$pidFile = __DIR__ . '/server.pid';

// Kill existing server if running
if (file_exists($pidFile)) {
    $pid = (int)trim(file_get_contents($pidFile));
    if ($pid > 0) {
        shell_exec("kill -9 $pid 2>/dev/null");
    }
    @unlink($pidFile);
}

// Clear old messages and users
file_put_contents(__DIR__ . '/messages.log', '');
file_put_contents(__DIR__ . '/users.json', '[]');

// Start the socket server as a background process
$logFile = __DIR__ . '/server_output.log';
$cmd = 'php ' . escapeshellarg(__DIR__ . '/server.php') . ' > ' . escapeshellarg($logFile) . ' 2>&1 &';
shell_exec($cmd);

// Wait a moment for the server to start and write its PID
sleep(1);

if (file_exists($pidFile)) {
    $msg = 'Server+started+successfully+on+port+8888';
} else {
    $msg = 'Server+may+not+have+started+correctly.+Check+server_output.log';
}

header("Location: index.php?msg=$msg");
exit;
