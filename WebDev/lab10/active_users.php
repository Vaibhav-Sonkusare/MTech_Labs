<?php
session_start();
if (!isset($_SESSION['username'])) {
    header("Location: chat.php");
    exit;
}
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Active Users</title>
    <link rel="stylesheet" href="style.css">
</head>
<body>
<div class="container">
    <h2>Active Users</h2>
    <p>Total: <strong id="totalUsers">0</strong></p>
    <ul id="userList"><li>Loading...</li></ul>
    <button onclick="refreshUsers()">Refresh</button>
    <a href="chat.php">Back to Chat</a>
    <a href="index.php">Home</a>
</div>

<script>
    const me = '<?= addslashes($_SESSION['username']) ?>';

    function refreshUsers() {
        document.getElementById('userList').innerHTML = '<li>Loading...</li>';
        fetch('api.php?action=get_users')
            .then(r => r.json())
            .then(data => {
                const list  = document.getElementById('userList');
                const users = data.users || [];
                document.getElementById('totalUsers').textContent = users.length;
                if (users.length === 0) {
                    list.innerHTML = '<li>No active users.</li>';
                } else {
                    list.innerHTML = users.map(u =>
                        '<li>' + u + (u === me ? ' (You)' : '') + '</li>'
                    ).join('');
                }
            })
            .catch(() => {
                document.getElementById('userList').innerHTML = '<li>Could not connect to server.</li>';
            });
    }

    refreshUsers();                       // load on page open
    setInterval(refreshUsers, 10000);     // auto-refresh every 10 s
</script>
</body>
</html>
