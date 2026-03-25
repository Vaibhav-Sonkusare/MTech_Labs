<?php
session_start();

// ── Join form ───────────────────────────────────────────────────────────────
if (!isset($_SESSION['username'])) {
    if ($_SERVER['REQUEST_METHOD'] === 'POST' && !empty($_POST['username'])) {
        $u = htmlspecialchars($_POST['username']);
?>
<!DOCTYPE html>
<html lang="en">
<head><meta charset="UTF-8"><title>Joining...</title>
<link rel="stylesheet" href="style.css"></head>
<body>
<p>Joining as <strong><?= $u ?></strong>...</p>
<script>
    const fd = new FormData();
    fd.append('action', 'join');
    fd.append('username', '<?= addslashes($u) ?>');
    fetch('api.php', { method: 'POST', body: fd })
        .then(() => window.location.href = 'chat.php');
</script>
</body></html>
<?php
        exit;
    }
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Join Chat Room</title>
    <link rel="stylesheet" href="style.css">
</head>
<body>
<div class="container">
    <h2>Join Chat Room</h2>
    <form method="POST">
        <label>Username: <input type="text" name="username" required autofocus></label>
        <button type="submit">Join</button>
        <a href="index.php">Cancel</a>
    </form>
</div>
</body></html>
<?php
    exit;
}

$user = $_SESSION['username'];
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Chat Room</title>
    <link rel="stylesheet" href="style.css">
</head>
<body>
<div class="container">
    <h2>Chat Room &mdash; Logged in as: <em><?= htmlspecialchars($user) ?></em></h2>

    <!-- Search -->
    <div class="search-bar">
        <input type="text" id="searchInput" placeholder="Search messages...">
        <button onclick="searchMessages()">Search</button>
        <button onclick="clearSearch()">Clear Search</button>
    </div>

    <!-- Message display window -->
    <div id="chatBox">
        <div id="noMsg" style="color:gray;">No messages yet. Start the conversation!</div>
    </div>

    <!-- Send bar -->
    <div class="input-bar">
        <input type="text" id="msgInput" placeholder="Type a message..."
               onkeypress="if(event.key==='Enter') sendMessage()">
        <button onclick="sendMessage()">Send</button>
        <button class="btn-danger" onclick="disconnect()">Disconnect</button>
    </div>

    <p><a href="active_users.php">View Active Users</a> | <a href="index.php">Home</a></p>
</div>

<script>
    const currentUser = '<?= addslashes($user) ?>';
    let nextIndex = 0;       // last read line index in messages.log
    let searching  = false;  // true when showing search results

    // ── Render a single message entry ──────────────────────────────────────
    function renderMsg(user, msg, type) {
        const chatBox = document.getElementById('chatBox');
        const noMsg   = document.getElementById('noMsg');
        if (noMsg) noMsg.remove();

        const div = document.createElement('div');
        if (type === 'system') {
            div.className = 'msg-system';
            div.textContent = msg;
        } else {
            div.className = 'msg-user';
            div.innerHTML = '<strong>' + user + ':</strong> ' + msg;
        }
        chatBox.appendChild(div);
        chatBox.scrollTop = chatBox.scrollHeight;
    }

    // ── Short-poll: fetch new messages since nextIndex ─────────────────────
    function poll() {
        if (searching) return;
        fetch('api.php?action=poll&from=' + nextIndex)
            .then(r => r.json())
            .then(data => {
                if (data.messages && data.messages.length > 0) {
                    data.messages.forEach(m => renderMsg(m.user, m.msg, m.type));
                }
                nextIndex = data.next ?? nextIndex;
            })
            .catch(() => {})   // silently ignore network errors
            .finally(() => {
                if (!searching) setTimeout(poll, 2000);  // poll every 2 s
            });
    }

    // ── Send a message via AJAX ────────────────────────────────────────────
    function sendMessage() {
        const input = document.getElementById('msgInput');
        const msg   = input.value.trim();
        if (!msg) return;
        input.value = '';

        const fd = new FormData();
        fd.append('action', 'send');
        fd.append('msg', msg);
        fetch('api.php', { method: 'POST', body: fd });
    }

    // ── Disconnect and return to index ─────────────────────────────────────
    function disconnect() {
        fetch('api.php?action=disconnect').then(() => {
            window.location.href = 'index.php';
        });
    }

    // ── Search messages ────────────────────────────────────────────────────
    function searchMessages() {
        const q = document.getElementById('searchInput').value.trim();
        if (!q) return;
        searching = true;

        const fd = new FormData();
        fd.append('action', 'search');
        fd.append('query', q);
        fetch('api.php', { method: 'POST', body: fd })
            .then(r => r.json())
            .then(data => {
                const chatBox = document.getElementById('chatBox');
                chatBox.innerHTML = '';
                if (data.results && data.results.length > 0) {
                    data.results.forEach(m => renderMsg(m.user, m.msg, m.type));
                } else {
                    chatBox.innerHTML = '<div style="color:gray;">No results for "' + q + '"</div>';
                }
            });
    }

    // ── Clear search and resume polling ────────────────────────────────────
    function clearSearch() {
        document.getElementById('searchInput').value = '';
        document.getElementById('chatBox').innerHTML = '<div id="noMsg" style="color:gray;">Loading messages...</div>';
        searching  = false;
        nextIndex  = 0;   // reload all messages
        poll();
    }

    // Start polling immediately
    poll();
</script>
</body>
</html>
