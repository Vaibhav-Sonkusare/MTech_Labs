<?php
session_start();
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Real-Time Chat System</title>
    <link rel="stylesheet" href="style.css">
</head>
<body>
<div class="container">
    <h1>Real-Time Chat System</h1>

    <?php if (!empty($_GET['msg'])): ?>
        <p class="msg-alert"><?= htmlspecialchars($_GET['msg']) ?></p>
    <?php endif; ?>

    <ul>
        <li><a href="start_server.php">Start Chat Server</a></li>
        <li><a href="chat.php">Join Chat Room</a></li>
        <li><a href="active_users.php">View Active Users</a></li>
        <li><a href="api.php?action=logout" class="btn-danger">Logout</a></li>
    </ul>
</div>
</body>
</html>
