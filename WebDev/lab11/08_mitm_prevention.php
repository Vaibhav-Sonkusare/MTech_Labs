<?php
/**
 * Program 8: MITM Attack Prevention using Cryptographic Techniques
 * Demonstrates:
 *  - Message integrity via HMAC (detect tampering)
 *  - Replay attack prevention via nonces + timestamps
 *  - Certificate pinning concept (simulated)
 *  - TLS enforcement recommendation
 */

session_start();
define('SHARED_SECRET', 'sharedHmacSecret_32bytes_here!!!!');
define('NONCE_WINDOW',  30); // seconds; reject messages older than this

$msg   = '';
$error = '';
$action = $_POST['action'] ?? '';

// Initialize nonce store
if (!isset($_SESSION['used_nonces'])) {
    $_SESSION['used_nonces'] = [];
}

// Clean expired nonces
$_SESSION['used_nonces'] = array_filter($_SESSION['used_nonces'], fn($t) => (time() - $t) < NONCE_WINDOW);

// ---- Helper functions ----
function generate_mac(string $message, string $timestamp, string $nonce, string $secret): string {
    return hash_hmac('sha256', $message . '|' . $timestamp . '|' . $nonce, $secret);
}

// ---- Sender: prepare a signed message ----
if ($action === 'send') {
    $message   = trim($_POST['message'] ?? '');
    $timestamp = time();
    $nonce     = bin2hex(random_bytes(8)); // unique per message
    $mac       = generate_mac($message, $timestamp, $nonce, SHARED_SECRET);

    $_SESSION['mitm_packet'] = compact('message', 'timestamp', 'nonce', 'mac');
    $msg = "Message prepared and signed with HMAC-SHA256.";
}

// ---- Simulated MITM tamper ----
if ($action === 'tamper' && isset($_SESSION['mitm_packet'])) {
    $_SESSION['mitm_packet']['message'] .= ' [TAMPERED]';
    $msg = "⚠ Packet tampered by MITM (message modified without updating MAC).";
}

// ---- Receiver: verify the packet ----
if ($action === 'receive' && isset($_SESSION['mitm_packet'])) {
    $p         = $_SESSION['mitm_packet'];
    $timeDiff  = time() - (int)$p['timestamp'];
    $nonce     = $p['nonce'];

    // 1) Timestamp freshness check
    if ($timeDiff > NONCE_WINDOW) {
        $error = "REPLAY ATTACK DETECTED: Message too old ({$timeDiff}s > " . NONCE_WINDOW . "s window).";
    }
    // 2) Nonce replay check
    elseif (isset($_SESSION['used_nonces'][$nonce])) {
        $error = "REPLAY ATTACK DETECTED: Nonce already used.";
    }
    // 3) HMAC integrity check
    else {
        $expectedMac = generate_mac($p['message'], $p['timestamp'], $nonce, SHARED_SECRET);
        if (!hash_equals($expectedMac, $p['mac'])) {
            $error = "MITM DETECTED: HMAC mismatch! Message was tampered in transit.";
        } else {
            $_SESSION['used_nonces'][$nonce] = time();
            $msg = "Message received & verified: <strong>" . htmlspecialchars($p['message']) . "</strong>";
        }
    }
}

if ($action === 'reset') {
    unset($_SESSION['mitm_packet'], $_SESSION['used_nonces']);
    $msg = 'State reset.';
}

$packet = $_SESSION['mitm_packet'] ?? null;
?>
<!DOCTYPE html>
<html>
<head>
    <title>8. MITM Prevention</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 750px; margin: 30px auto; padding: 0 15px; }
        input[type=text] { width: 100%; padding: 5px; margin: 4px 0; box-sizing: border-box; }
        button { padding: 6px 14px; margin: 4px 4px 4px 0; }
        .ok { color: green; } .error { color: red; } .warn { color: orangered; }
        table { border-collapse: collapse; margin-top: 8px; width: 100%; }
        td, th { border: 1px solid #ccc; padding: 6px; font-size: 13px; word-break: break-all; }
        code { background: #f4f4f4; padding: 2px 4px; }
        a { display: block; margin-top: 20px; }
    </style>
</head>
<body>
    <h2>8. MITM Attack Prevention using Cryptography</h2>

    <!-- Step 1: Sender -->
    <h3>Sender: Compose & Sign</h3>
    <form method="POST">
        <input type="text" name="message" placeholder="Message to send" value="<?= htmlspecialchars($packet['message'] ?? '') ?>">
        <button type="submit" name="action" value="send">Send (Sign with HMAC)</button>
    </form>

    <?php if ($packet): ?>
    <h3>Packet in Transit</h3>
    <table>
        <tr><th>Field</th><th>Value</th></tr>
        <tr><td>message</td><td><?= htmlspecialchars($packet['message']) ?></td></tr>
        <tr><td>timestamp</td><td><?= $packet['timestamp'] ?> (<?= date('H:i:s', $packet['timestamp']) ?>)</td></tr>
        <tr><td>nonce</td><td><?= htmlspecialchars($packet['nonce']) ?></td></tr>
        <tr><td>mac (HMAC-SHA256)</td><td><code><?= htmlspecialchars($packet['mac']) ?></code></td></tr>
    </table>

    <!-- Simulate MITM -->
    <h3>Simulate MITM Tampering</h3>
    <form method="POST">
        <button type="submit" name="action" value="tamper">Tamper Message (Simulate MITM)</button>
    </form>

    <!-- Receiver -->
    <h3>Receiver: Verify Packet</h3>
    <form method="POST">
        <button type="submit" name="action" value="receive">Verify & Accept</button>
    </form>
    <?php endif; ?>

    <?php if ($msg):   ?><p class="ok"><?= $msg ?></p><?php endif; ?>
    <?php if ($error): ?><p class="error"><?= htmlspecialchars($error) ?></p><?php endif; ?>

    <form method="POST" style="margin-top:10px">
        <button type="submit" name="action" value="reset">Reset</button>
    </form>

    <hr>
    <h3>Techniques Demonstrated</h3>
    <ul>
        <li><strong>HMAC-SHA256</strong>: Any change to the message makes the MAC invalid.</li>
        <li><strong>Timestamp check</strong>: Messages older than <?= NONCE_WINDOW ?>s are rejected (replay prevention).</li>
        <li><strong>Nonce</strong>: Each message carries a unique token; reused nonces are rejected.</li>
        <li><strong>Constant-time comparison</strong>: <code>hash_equals()</code> prevents timing attacks on MAC verification.</li>
        <li><strong>TLS (in production)</strong>: All of the above should run over HTTPS to prevent passive eavesdropping.</li>
    </ul>
    <a href="index.php">← Back to Index</a>
</body>
</html>
