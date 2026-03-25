<?php
/**
 * api.php — HTTP bridge between the browser and the socket server / shared files.
 *
 * Why file-based polling?
 * php -S is single-threaded: if one request blocks (e.g. waiting on a socket
 * read), ALL other requests are frozen.  For polling we therefore read directly
 * from messages.log rather than opening a long-lived socket to server.php.
 * For write actions (send, join, disconnect) we open a short socket, fire the
 * command, and read the quick acknowledgement — these complete in milliseconds
 * so they do not block the web server in practice.
 */

session_start();
error_reporting(E_ALL);
ini_set('display_errors', 0);

define('MSG_LOG',    __DIR__ . '/messages.log');
define('USERS_JSON', __DIR__ . '/users.json');

$action = $_POST['action'] ?? $_GET['action'] ?? '';
$user   = $_SESSION['username'] ?? '';

// ── Logout (GET redirect) ────────────────────────────────────────────────────
if ($action === 'logout') {
    if ($user) sock_send(['action' => 'disconnect', 'user' => $user], false);
    session_destroy();
    header('Location: index.php');
    exit;
}

header('Content-Type: application/json');

if (!$user && $action !== 'join') {
    echo json_encode(['error' => 'Not logged in']);
    exit;
}

// ── Fire-and-forget (or quick ack) socket call ──────────────────────────────
function sock_send(array $data, bool $want_reply = true) {
    $sock = @socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
    if (!$sock) return false;

    // Timeout so we never block the web server for more than 3 s
    socket_set_option($sock, SOL_SOCKET, SO_SNDTIMEO, ['sec' => 3, 'usec' => 0]);
    socket_set_option($sock, SOL_SOCKET, SO_RCVTIMEO, ['sec' => 3, 'usec' => 0]);

    if (!@socket_connect($sock, '127.0.0.1', 8888)) {
        @socket_close($sock);
        return false;
    }

    $payload = json_encode($data) . "\n";
    @socket_write($sock, $payload, strlen($payload));

    $result = true;
    if ($want_reply) {
        $raw    = @socket_read($sock, 4096, PHP_NORMAL_READ);
        $result = $raw ? json_decode(trim($raw), true) : false;
    }

    @socket_close($sock);
    return $result;
}

// ── action: join ─────────────────────────────────────────────────────────────
if ($action === 'join') {
    $username = trim($_POST['username'] ?? '');
    if ($username === '') {
        echo json_encode(['error' => 'Username required']);
        exit;
    }
    $_SESSION['username'] = $username;

    // Tell server → it writes the "joined" message to messages.log
    $ok = sock_send(['action' => 'join', 'user' => $username]);
    echo json_encode(['status' => $ok ? 'success' : 'server_offline']);
    exit;
}

// ── action: send ─────────────────────────────────────────────────────────────
if ($action === 'send') {
    $msg = trim($_POST['msg'] ?? '');
    if ($msg === '') {
        echo json_encode(['error' => 'Empty message']);
        exit;
    }
    $ok = sock_send(['action' => 'send', 'user' => $user, 'msg' => $msg]);
    if (!$ok) {
        // Fallback: write directly to log even if server is down
        file_put_contents(MSG_LOG,
            json_encode(['user'=>$user,'msg'=>$msg,'type'=>'user','timestamp'=>time()])."\n",
            FILE_APPEND | LOCK_EX);
    }
    echo json_encode(['status' => 'success']);
    exit;
}

// ── action: poll (read messages.log from given line index) ───────────────────
// This NEVER opens a socket — reads a plain file, so php -S stays unblocked.
if ($action === 'poll') {
    session_write_close(); // release session lock early
    $from = max(0, (int)($_GET['from'] ?? 0));

    // Send a quick heartbeat so the server keeps the user in active list
    // (fire-and-forget, 3 s timeout max)
    sock_send(['action' => 'heartbeat', 'user' => $user], false);

    $messages = [];
    if (file_exists(MSG_LOG)) {
        $lines = file(MSG_LOG, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES);
        $total = count($lines);
        for ($i = $from; $i < $total; $i++) {
            $d = json_decode($lines[$i], true);
            if ($d) $messages[] = $d;
        }
        $next = $total;
    } else {
        $next = 0;
    }

    echo json_encode(['messages' => $messages, 'next' => $next]);
    exit;
}

// ── action: get_users ────────────────────────────────────────────────────────
// Read users.json written by server — no blocking socket needed.
if ($action === 'get_users') {
    $users = [];
    if (file_exists(USERS_JSON)) {
        $users = json_decode(file_get_contents(USERS_JSON), true) ?? [];
    }
    echo json_encode(['users' => $users]);
    exit;
}

// ── action: disconnect ───────────────────────────────────────────────────────
if ($action === 'disconnect') {
    sock_send(['action' => 'disconnect', 'user' => $user], false);
    // Write leave message locally in case server is slow
    file_put_contents(MSG_LOG,
        json_encode(['user'=>'System','msg'=>"$user left the chat.",'type'=>'system','timestamp'=>time()])."\n",
        FILE_APPEND | LOCK_EX);
    session_destroy();
    echo json_encode(['status' => 'success']);
    exit;
}

// ── action: search ────────────────────────────────────────────────────────────
if ($action === 'search') {
    $query   = strtolower(trim($_POST['query'] ?? ''));
    $results = [];
    if ($query !== '' && file_exists(MSG_LOG)) {
        $lines = file(MSG_LOG, FILE_IGNORE_NEW_LINES | FILE_SKIP_EMPTY_LINES);
        foreach ($lines as $line) {
            $d = json_decode($line, true);
            if ($d && strpos(strtolower($d['msg']), $query) !== false) {
                $results[] = $d;
            }
        }
    }
    echo json_encode(['status' => 'success', 'results' => $results]);
    exit;
}

echo json_encode(['error' => 'Unknown action']);
