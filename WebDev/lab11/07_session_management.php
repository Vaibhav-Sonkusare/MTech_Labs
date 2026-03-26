<?php
/**
 * Program 7: Secure Session Management with Encrypted Session Data
 * Demonstrates: session fixation prevention, encrypted payload, HMAC integrity,
 * user-agent/IP binding, and idle timeout.
 */

define('SESSION_KEY',     'sessEncKey_32_bytes_exact!!!!!!');
define('SESSION_HMAC_KEY','sessHmacKey_for_integrity_check!');
define('CIPHER',          'AES-256-CBC');
define('IDLE_TIMEOUT',    30); // 30 seconds

// Custom session handler: encrypt/decrypt data on write/read
// For simplicity, we encrypt the payload stored under a known session key.

session_start();

// --- Helpers ---
function session_encrypt(string $data): string {
    $iv  = openssl_random_pseudo_bytes(openssl_cipher_iv_length(CIPHER));
    $enc = openssl_encrypt($data, CIPHER, SESSION_KEY, OPENSSL_RAW_DATA, $iv);
    return base64_encode($iv . $enc);
}

function session_decrypt(string $data): string|false {
    $raw   = base64_decode($data);
    $ivLen = openssl_cipher_iv_length(CIPHER);
    return openssl_decrypt(substr($raw, $ivLen), CIPHER, SESSION_KEY, OPENSSL_RAW_DATA, substr($raw, 0, $ivLen));
}

function session_hmac(string $data): string {
    return hash_hmac('sha256', $data, SESSION_HMAC_KEY);
}

function current_fingerprint(): string {
    return hash('sha256', ($_SERVER['HTTP_USER_AGENT'] ?? '') . ($_SERVER['REMOTE_ADDR'] ?? ''));
}

// --- Session Security Checks ---
$msg   = '';
$error = '';

// Idle timeout check
if (isset($_SESSION['last_active']) && (time() - $_SESSION['last_active']) > IDLE_TIMEOUT) {
    session_unset(); session_destroy(); session_start();
    $error = 'Session expired due to inactivity. Please log in again.';
}

// Fingerprint check (session hijacking detection)
if (isset($_SESSION['fingerprint']) && $_SESSION['fingerprint'] !== current_fingerprint()) {
    session_unset(); session_destroy(); session_start();
    $error = 'Session mismatch detected (possible hijack). Session terminated.';
}

$action = $_POST['action'] ?? '';

// Login: regenerate session ID (prevent fixation), set fingerprint
if ($action === 'login') {
    $user = trim($_POST['username'] ?? '');
    if ($user === '') { $error = 'Username required.'; }
    else {
        session_regenerate_id(true); // prevent session fixation
        $_SESSION['fingerprint'] = current_fingerprint();
        $_SESSION['last_active'] = time();
        // Store sensitive data encrypted
        $payload = json_encode(['user' => $user, 'role' => 'student', 'login_time' => time()]);
        $enc     = session_encrypt($payload);
        $hmac    = session_hmac($enc);
        $_SESSION['secure_payload'] = $enc;
        $_SESSION['payload_hmac']   = $hmac;
        $msg = "Logged in as '{$user}'. Session ID regenerated & data encrypted.";
    }
}

if ($action === 'logout') {
    session_unset(); session_destroy();
    header('Location: ' . $_SERVER['PHP_SELF']);
    exit;
}

if ($action === 'refresh' && isset($_SESSION['last_active'])) {
    $_SESSION['last_active'] = time();
    $msg = 'Session activity refreshed.';
}

// Decode payload for display
$decoded = null;
$hmacOk  = false;
if (isset($_SESSION['secure_payload'])) {
    $hmacOk = hash_equals($_SESSION['payload_hmac'], session_hmac($_SESSION['secure_payload']));
    if ($hmacOk) {
        $decoded = json_decode(session_decrypt($_SESSION['secure_payload']), true);
    }
}

// Update last active
if (isset($_SESSION['last_active'])) {
    $_SESSION['last_active'] = time();
}
?>
<!DOCTYPE html>
<html>
<head>
    <title>7. Secure Session Management</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 700px; margin: 30px auto; padding: 0 15px; }
        input[type=text] { width: 100%; padding: 5px; margin: 4px 0; box-sizing: border-box; }
        button { padding: 6px 14px; margin: 4px 4px 4px 0; }
        .ok { color: green; } .error { color: red; }
        table { border-collapse: collapse; margin-top: 8px; }
        td, th { border: 1px solid #ccc; padding: 6px; }
        pre { background: #f4f4f4; padding: 8px; font-size: 11px; word-break: break-all; white-space: pre-wrap; }
        a { display: block; margin-top: 20px; }
    </style>
</head>
<body>
    <h2>7. Secure Session Management</h2>

    <?php if ($msg):   ?><p class="ok"><?= htmlspecialchars($msg) ?></p><?php endif; ?>
    <?php if ($error): ?><p class="error"><?= htmlspecialchars($error) ?></p><?php endif; ?>

    <?php if (!isset($_SESSION['fingerprint'])): ?>
    <h3>Login</h3>
    <form method="POST">
        <input type="text" name="username" placeholder="Enter username">
        <button type="submit" name="action" value="login">Login</button>
    </form>
    <?php else: ?>
    <h3>Active Session</h3>
    <table>
        <tr><th>Session ID</th><td><code><?= session_id() ?></code></td></tr>
        <tr><th>Idle Timeout</th><td><?= IDLE_TIMEOUT ?>s (<?= IDLE_TIMEOUT/60 ?> min)</td></tr>
        <tr><th>HMAC Integrity</th><td><?= $hmacOk ? '<span class="ok">✓ Valid</span>' : '<span class="error">✗ Tampered</span>' ?></td></tr>
        <tr><th>Fingerprint</th><td><code><?= htmlspecialchars(current_fingerprint()) ?></code></td></tr>
    </table>

    <?php if ($decoded): ?>
    <h3>Decrypted Session Payload</h3>
    <table>
        <?php foreach ($decoded as $k => $v): ?>
        <tr><th><?= htmlspecialchars($k) ?></th><td><?= htmlspecialchars(is_int($v) ? date('Y-m-d H:i:s', $v) : $v) ?></td></tr>
        <?php endforeach; ?>
    </table>
    <?php endif; ?>

    <h3>Encrypted Payload (as stored in session)</h3>
    <pre><?= htmlspecialchars($_SESSION['secure_payload'] ?? '') ?></pre>

    <form method="POST" style="margin-top:10px">
        <button type="submit" name="action" value="refresh">Refresh Activity</button>
        <button type="submit" name="action" value="logout">Logout</button>
    </form>
    <?php endif; ?>

    <hr>
    <h3>Security Measures</h3>
    <ul>
        <li><strong>Session fixation</strong> prevented via <code>session_regenerate_id(true)</code> on login.</li>
        <li><strong>Idle timeout</strong>: session invalidated after <?= IDLE_TIMEOUT ?>s of inactivity.</li>
        <li><strong>Session hijacking</strong>: User-Agent + IP fingerprint checked on every request.</li>
        <li><strong>Encrypted payload</strong>: Sensitive data in session encrypted with AES-256-CBC.</li>
        <li><strong>HMAC integrity</strong>: Payload tamper-detection using HMAC-SHA256.</li>
    </ul>
    <a href="index.php">← Back to Index</a>
</body>
</html>
