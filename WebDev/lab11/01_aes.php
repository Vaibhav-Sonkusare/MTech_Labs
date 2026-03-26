<?php
/**
 * Program 1: AES Encryption and Decryption in PHP
 * Uses AES-256-CBC for secure message transmission.
 */

define('AES_KEY', 'my_secret_key_32_bytes_long!!!!!'); // 32 bytes = 256-bit key
define('AES_CIPHER', 'AES-256-CBC');

function aes_encrypt(string $plaintext, string $key): string {
    $ivLength  = openssl_cipher_iv_length(AES_CIPHER);
    $iv        = openssl_random_pseudo_bytes($ivLength);
    $encrypted = openssl_encrypt($plaintext, AES_CIPHER, $key, OPENSSL_RAW_DATA, $iv);
    // Prepend IV to ciphertext so it can be used during decryption
    return base64_encode($iv . $encrypted);
}

function aes_decrypt(string $ciphertext, string $key): string|false {
    $data      = base64_decode($ciphertext);
    $ivLength  = openssl_cipher_iv_length(AES_CIPHER);
    $iv        = substr($data, 0, $ivLength);
    $encrypted = substr($data, $ivLength);
    return openssl_decrypt($encrypted, AES_CIPHER, $key, OPENSSL_RAW_DATA, $iv);
}

$message   = '';
$encrypted = '';
$decrypted = '';
$error     = '';

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $message = trim($_POST['message'] ?? '');
    $action  = $_POST['action'] ?? '';

    if ($action === 'encrypt' && $message !== '') {
        $encrypted = aes_encrypt($message, AES_KEY);
    } elseif ($action === 'decrypt' && $message !== '') {
        $result = aes_decrypt($message, AES_KEY);
        if ($result === false) {
            $error = 'Decryption failed. Invalid ciphertext or key.';
        } else {
            $decrypted = $result;
        }
    }
}
?>
<!DOCTYPE html>
<html>
<head>
    <title>1. AES Encryption & Decryption</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 700px; margin: 30px auto; padding: 0 15px; }
        textarea { width: 100%; height: 80px; margin: 5px 0; box-sizing: border-box; }
        button { margin-right: 10px; padding: 6px 14px; }
        pre { background: #f4f4f4; padding: 10px; word-break: break-all; white-space: pre-wrap; }
        .error { color: red; }
        a { display: block; margin-top: 20px; }
    </style>
</head>
<body>
    <h2>1. AES-256-CBC Encryption & Decryption</h2>
    <p><strong>Key (32 bytes):</strong> <code><?= htmlspecialchars(AES_KEY) ?></code></p>

    <form method="POST">
        <label>Input (plaintext to encrypt / ciphertext to decrypt):</label><br>
        <textarea name="message"><?= htmlspecialchars($message) ?></textarea><br>
        <button type="submit" name="action" value="encrypt">Encrypt</button>
        <button type="submit" name="action" value="decrypt">Decrypt</button>
    </form>

    <?php if ($error): ?>
        <p class="error"><?= htmlspecialchars($error) ?></p>
    <?php endif; ?>

    <?php if ($encrypted): ?>
        <h3>Encrypted (Base64):</h3>
        <pre><?= htmlspecialchars($encrypted) ?></pre>
    <?php endif; ?>

    <?php if ($decrypted): ?>
        <h3>Decrypted Plaintext:</h3>
        <pre><?= htmlspecialchars($decrypted) ?></pre>
    <?php endif; ?>

    <hr>
    <h3>How it works</h3>
    <ul>
        <li>Cipher: <strong>AES-256-CBC</strong></li>
        <li>A random 16-byte <strong>IV</strong> is generated per encryption.</li>
        <li>IV is prepended to the ciphertext before Base64 encoding.</li>
        <li>On decryption, the IV is extracted from the beginning of the data.</li>
    </ul>
    <a href="index.php">← Back to Index</a>
</body>
</html>
