<?php
/**
 * Program 4: Secure File Upload with Encrypted Storage
 * Files are AES-256-CBC encrypted before saving to disk.
 */

define('UPLOAD_DIR',  __DIR__ . '/uploads_encrypted/');
define('FILE_KEY',    'fileEncKey_32bytes_exactly!!!!!!'); // 32 bytes
define('FILE_CIPHER', 'AES-256-CBC');

function encrypt_file(string $data, string $key): string {
    $iv  = openssl_random_pseudo_bytes(openssl_cipher_iv_length(FILE_CIPHER));
    $enc = openssl_encrypt($data, FILE_CIPHER, $key, OPENSSL_RAW_DATA, $iv);
    return base64_encode($iv . $enc);
}

function decrypt_file(string $data, string $key): string {
    $raw      = base64_decode($data);
    $ivLen    = openssl_cipher_iv_length(FILE_CIPHER);
    $iv       = substr($raw, 0, $ivLen);
    $enc      = substr($raw, $ivLen);
    return openssl_decrypt($enc, FILE_CIPHER, $key, OPENSSL_RAW_DATA, $iv);
}

$message = '';
$error   = '';

// Allowed extensions (whitelist)
$allowed = ['txt', 'pdf', 'png', 'jpg', 'jpeg', 'gif', 'csv'];

// Upload
if ($_SERVER['REQUEST_METHOD'] === 'POST' && isset($_FILES['file'])) {
    $file = $_FILES['file'];
    if ($file['error'] !== UPLOAD_ERR_OK) {
        $error = 'Upload error code: ' . $file['error'];
    } else {
        $ext = strtolower(pathinfo($file['name'], PATHINFO_EXTENSION));
        if (!in_array($ext, $allowed)) {
            $error = 'File type not allowed. Allowed: ' . implode(', ', $allowed);
        } elseif ($file['size'] > 2 * 1024 * 1024) {  // 2 MB limit
            $error = 'File too large (max 2 MB).';
        } else {
            $plaintext = file_get_contents($file['tmp_name']);
            $encrypted = encrypt_file($plaintext, FILE_KEY);
            // Store with .enc extension; original name saved in filename
            $safeName  = preg_replace('/[^a-zA-Z0-9._-]/', '_', $file['name']);
            $savePath  = UPLOAD_DIR . $safeName . '.enc';
            file_put_contents($savePath, $encrypted);
            $message = "File '{$safeName}' uploaded and encrypted successfully.";
        }
    }
}

// Download / decrypt
if (isset($_GET['download'])) {
    $fn = basename($_GET['download']); // sanitize
    $fp = UPLOAD_DIR . $fn;
    if (file_exists($fp) && preg_match('/\.enc$/', $fn)) {
        $enc   = file_get_contents($fp);
        $plain = decrypt_file($enc, FILE_KEY);
        // Original name = strip .enc
        $origName = preg_replace('/\.enc$/', '', $fn);
        header('Content-Description: File Transfer');
        header('Content-Type: application/octet-stream');
        header('Content-Disposition: attachment; filename="' . $origName . '"');
        header('Content-Length: ' . strlen($plain));
        echo $plain;
        exit;
    }
}

// List encrypted files
$files = glob(UPLOAD_DIR . '*.enc') ?: [];
?>
<!DOCTYPE html>
<html>
<head>
    <title>4. Secure File Upload</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 700px; margin: 30px auto; padding: 0 15px; }
        .ok { color: green; } .error { color: red; }
        table { border-collapse: collapse; width: 100%; margin-top: 10px; }
        td, th { border: 1px solid #ccc; padding: 6px; text-align: left; }
        a { color: #0066cc; }
        a.back { display: block; margin-top: 20px; }
    </style>
</head>
<body>
    <h2>4. Secure File Upload with Encrypted Storage</h2>

    <?php if ($message): ?><p class="ok"><?= htmlspecialchars($message) ?></p><?php endif; ?>
    <?php if ($error):   ?><p class="error"><?= htmlspecialchars($error) ?></p><?php endif; ?>

    <form method="POST" enctype="multipart/form-data">
        <input type="file" name="file" required>
        <button type="submit">Upload & Encrypt</button>
        <br><small>Allowed: <?= implode(', ', $allowed) ?> | Max: 2 MB</small>
    </form>

    <h3>Stored Encrypted Files</h3>
    <?php if (empty($files)): ?>
        <p>No files uploaded yet.</p>
    <?php else: ?>
        <table>
            <tr><th>Encrypted Filename</th><th>Size (bytes)</th><th>Action</th></tr>
            <?php foreach ($files as $f): $bn = basename($f); ?>
            <tr>
                <td><?= htmlspecialchars($bn) ?></td>
                <td><?= filesize($f) ?></td>
                <td><a href="?download=<?= urlencode($bn) ?>">Decrypt & Download</a></td>
            </tr>
            <?php endforeach; ?>
        </table>
    <?php endif; ?>

    <hr>
    <h3>How it works</h3>
    <ul>
        <li>Only whitelisted file extensions are accepted.</li>
        <li>File size is limited to 2 MB.</li>
        <li>File content is encrypted with <strong>AES-256-CBC</strong> before saving to disk.</li>
        <li>Stored files carry a <code>.enc</code> extension; original content is never written in plaintext.</li>
        <li>On download, the file is decrypted in-memory and streamed to the browser.</li>
    </ul>
    <a class="back" href="index.php">← Back to Index</a>
</body>
</html>
