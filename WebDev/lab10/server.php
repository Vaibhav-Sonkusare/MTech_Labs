<?php
/**
 * PHP Socket Server (Daemon)
 * Run separately: php server.php &
 *
 * Uses socket_create, socket_bind, socket_listen, socket_accept, socket_select
 * to handle multiple simultaneous clients.
 *
 * All messages are written to messages.log so the web server (php -S) can
 * read them without any blocking socket connections.
 * Active users are written to users.json for the same reason.
 */

set_time_limit(0);
error_reporting(E_ALL);
ini_set('display_errors', 1);

define('HOST', '127.0.0.1');
define('PORT', 8888);
define('MSG_LOG',   __DIR__ . '/messages.log');
define('USERS_JSON', __DIR__ . '/users.json');
define('PID_FILE',  __DIR__ . '/server.pid');

// ── Initialise shared files ──────────────────────────────────────────────────
if (!file_exists(MSG_LOG))   file_put_contents(MSG_LOG, '');
file_put_contents(USERS_JSON, json_encode([]));
file_put_contents(PID_FILE,   getmypid());

// ── Create, bind and listen ──────────────────────────────────────────────────
$server = socket_create(AF_INET, SOCK_STREAM, SOL_TCP);
if ($server === false) die("socket_create failed: " . socket_strerror(socket_last_error()) . "\n");

socket_set_option($server, SOL_SOCKET, SO_REUSEADDR, 1);

if (!socket_bind($server, HOST, PORT)) {
    die("socket_bind failed: " . socket_strerror(socket_last_error($server)) . "\n");
}
if (!socket_listen($server, 10)) {
    die("socket_listen failed: " . socket_strerror(socket_last_error($server)) . "\n");
}

echo "Server listening on " . HOST . ":" . PORT . " (PID " . getmypid() . ")\n";

$all_sockets  = [$server];   // sockets watched by socket_select
$active_users = [];           // username => last_seen timestamp

// ── Helper: write message to log file ────────────────────────────────────────
function append_message(string $user, string $msg, string $type): void {
    $entry = json_encode([
        'user'      => $user,
        'msg'       => $msg,
        'type'      => $type,
        'timestamp' => time(),
    ]) . "\n";
    file_put_contents(MSG_LOG, $entry, FILE_APPEND | LOCK_EX);
}

// ── Helper: persist active user list to users.json ───────────────────────────
function save_users(array &$active_users): void {
    // Evict anyone who hasn't pinged in 60 seconds
    $now = time();
    foreach ($active_users as $u => $ts) {
        if ($now - $ts > 60) unset($active_users[$u]);
    }
    file_put_contents(USERS_JSON, json_encode(array_keys($active_users)), LOCK_EX);
}

// ── Helper: send a JSON reply on a socket ────────────────────────────────────
function sock_reply($socket, array $data): void {
    $payload = json_encode($data) . "\n";
    @socket_write($socket, $payload, strlen($payload));
}

// ── Main event loop ──────────────────────────────────────────────────────────
while (true) {
    $read   = $all_sockets;
    $write  = null;
    $except = null;

    // socket_select blocks up to 2 s; returns number of sockets ready
    $ready = @socket_select($read, $write, $except, 2);
    if ($ready === false || $ready < 1) continue;

    // ── Accept new connection ────────────────────────────────────────────────
    if (in_array($server, $read)) {
        $client = socket_accept($server);
        if ($client !== false) {
            $all_sockets[] = $client;
        }
        $key = array_search($server, $read);
        unset($read[$key]);
    }

    // ── Handle data from existing clients ────────────────────────────────────
    foreach ($read as $sock) {
        $raw = @socket_read($sock, 4096, PHP_NORMAL_READ);

        // Client disconnected
        if ($raw === false || $raw === '') {
            $key = array_search($sock, $all_sockets);
            if ($key !== false) unset($all_sockets[$key]);
            @socket_close($sock);
            continue;
        }

        $raw = trim($raw);
        if ($raw === '') continue;

        $msg = json_decode($raw, true);
        if (!is_array($msg) || empty($msg['action'])) continue;

        $user   = $msg['user'] ?? 'unknown';
        $action = $msg['action'];

        // ── action: heartbeat / join (keeps user alive in active list) ────────
        if ($action === 'heartbeat' || $action === 'join') {
            $active_users[$user] = time();
            save_users($active_users);

            if ($action === 'join') {
                append_message('System', "$user joined the chat.", 'system');
            }
            sock_reply($sock, ['status' => 'ok']);
        }

        // ── action: send ──────────────────────────────────────────────────────
        elseif ($action === 'send') {
            $active_users[$user] = time();
            save_users($active_users);
            append_message($user, $msg['msg'] ?? '', 'user');
            sock_reply($sock, ['status' => 'sent']);
        }

        // ── action: disconnect ────────────────────────────────────────────────
        elseif ($action === 'disconnect') {
            unset($active_users[$user]);
            save_users($active_users);
            append_message('System', "$user left the chat.", 'system');
            sock_reply($sock, ['status' => 'disconnected']);
        }

        // ── action: get_users ─────────────────────────────────────────────────
        elseif ($action === 'get_users') {
            save_users($active_users);
            sock_reply($sock, ['status' => 'ok', 'users' => array_keys($active_users)]);
        }

        // Close the connection after each request (stateless per call)
        $key = array_search($sock, $all_sockets);
        if ($key !== false) unset($all_sockets[$key]);
        @socket_close($sock);
    }
}
