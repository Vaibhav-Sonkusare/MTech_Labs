<?php
/**
 * Program 5: RSA Encryption for Client-Server Communication
 * Simulates asymmetric encryption: client encrypts with server's public key,
 * server decrypts with its private key.
 */

session_start();

$output = '';
$error  = '';
$action = $_POST['action'] ?? '';

// Generate server RSA key pair
if ($action === 'generate' || empty($_SESSION['rsa_private'])) {
    $config = [
        'digest_alg'       => 'sha256',
        'private_key_bits' => 2048,
        'private_key_type' => OPENSSL_KEYTYPE_RSA,
    ];
    $res = openssl_pkey_new($config);
    openssl_pkey_export($res, $privKey);
    $details = openssl_pkey_get_details($res);
    $_SESSION['rsa_private'] = $privKey;
    $_SESSION['rsa_public']  = $details['key'];
    if ($action === 'generate') {
        $output = 'New RSA 2048-bit key pair generated for the server.';
    }
}

// Client: encrypt a message with the server's public key
if ($action === 'encrypt') {
    $msg = $_POST['message'] ?? '';
    $pub = openssl_pkey_get_public($_SESSION['rsa_public']);
    if (openssl_public_encrypt($msg, $encrypted, $pub, OPENSSL_PKCS1_OAEP_PADDING)) {
        $_SESSION['rsa_ciphertext'] = base64_encode($encrypted);
        $output = 'Message encrypted with server public key (OAEP padding).';
    } else {
        $error = 'Encryption failed: ' . openssl_error_string();
    }
}

// Server: decrypt with private key
if ($action === 'decrypt') {
    $cipherB64 = $_POST['ciphertext'] ?? '';
    $priv      = openssl_pkey_get_private($_SESSION['rsa_private']);
    $raw       = base64_decode($cipherB64);
    if (openssl_private_decrypt($raw, $plaintext, $priv, OPENSSL_PKCS1_OAEP_PADDING)) {
        $output = 'Server decrypted: <strong>' . htmlspecialchars($plaintext) . '</strong>';
    } else {
        $error = 'Decryption failed: ' . openssl_error_string();
    }
}
?>
<!DOCTYPE html>
<html>
<head>
    <title>5. RSA Encryption</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 750px; margin: 30px auto; padding: 0 15px; }
        textarea { width: 100%; height: 65px; box-sizing: border-box; margin: 4px 0; }
        button { padding: 6px 14px; margin: 4px 4px 4px 0; }
        pre { background: #f4f4f4; padding: 8px; font-size: 11px; word-break: break-all; white-space: pre-wrap; }
        .ok { color: green; } .error { color: red; }
        a { display: block; margin-top: 20px; }
    </style>
</head>
<body>
    <h2>5. RSA Encryption for Client-Server Communication</h2>

    <!-- Server Key Management -->
    <h3>Server Key Pair</h3>
    <form method="POST">
        <button type="submit" name="action" value="generate">Re-generate Key Pair</button>
    </form>
    <details>
        <summary>Server Public Key (shared with clients)</summary>
        <pre><?= htmlspecialchars($_SESSION['rsa_public'] ?? '') ?></pre>
    </details>

    <!-- Client Side -->
    <h3>Client: Encrypt Message</h3>
    <form method="POST">
        <label>Plaintext message (max ~214 bytes for 2048-bit RSA):</label>
        <textarea name="message" placeholder="Hello, Server!"></textarea>
        <button type="submit" name="action" value="encrypt">Encrypt with Server Public Key</button>
    </form>

    <!-- Server Side -->
    <h3>Server: Decrypt Ciphertext</h3>
    <form method="POST">
        <label>Ciphertext (Base64):</label>
        <textarea name="ciphertext"><?= htmlspecialchars($_SESSION['rsa_ciphertext'] ?? '') ?></textarea>
        <button type="submit" name="action" value="decrypt">Decrypt with Server Private Key</button>
    </form>

    <?php if ($output): ?><p class="ok"><?= $output ?></p><?php endif; ?>
    <?php if ($error):  ?><p class="error"><?= htmlspecialchars($error) ?></p><?php endif; ?>

    <?php if (!empty($_SESSION['rsa_ciphertext'])): ?>
    <h3>Current Ciphertext</h3>
    <pre><?= htmlspecialchars($_SESSION['rsa_ciphertext']) ?></pre>
    <?php endif; ?>

    <hr>
    <h3>How it works</h3>
    <ul>
        <li>Server generates a 2048-bit RSA key pair. Only the <strong>public key</strong> is shared.</li>
        <li>Client encrypts using <code>openssl_public_encrypt()</code> with OAEP padding.</li>
        <li>Only the server (holding the <strong>private key</strong>) can decrypt with <code>openssl_private_decrypt()</code>.</li>
        <li>OAEP padding is more secure than PKCS#1 v1.5 and resistant to chosen-ciphertext attacks.</li>
    </ul>
    <a href="index.php">← Back to Index</a>
</body>
</html>
