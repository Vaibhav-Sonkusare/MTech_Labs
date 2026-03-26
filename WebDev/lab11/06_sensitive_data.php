<?php
/**
 * Program 6: Secure Storage & Retrieval of Sensitive User Data
 * Encrypts sensitive fields (e.g. phone, email, SSN) before storing in session.
 * Demonstrates how a database layer would handle encrypted PII.
 */

session_start();

define('DATA_KEY', 'sensitiveKey_32bytes_here!!!!!!'); // 32-byte key
define('CIPHER',   'AES-256-CBC');

function encrypt_data(string $value, string $key): string {
    $iv  = openssl_random_pseudo_bytes(openssl_cipher_iv_length(CIPHER));
    $enc = openssl_encrypt($value, CIPHER, $key, OPENSSL_RAW_DATA, $iv);
    return base64_encode($iv . $enc);
}

function decrypt_data(string $value, string $key): string {
    $raw   = base64_decode($value);
    $ivLen = openssl_cipher_iv_length(CIPHER);
    $iv    = substr($raw, 0, $ivLen);
    $enc   = substr($raw, $ivLen);
    return openssl_decrypt($enc, CIPHER, $key, OPENSSL_RAW_DATA, $iv) ?: '[decryption failed]';
}

$message = '';
$error   = '';
$action  = $_POST['action'] ?? '';

if ($action === 'store') {
    $name  = trim($_POST['name']  ?? '');
    $email = trim($_POST['email'] ?? '');
    $phone = trim($_POST['phone'] ?? '');
    $ssn   = trim($_POST['ssn']   ?? '');

    if ($name === '' || $email === '') {
        $error = 'Name and Email are required.';
    } else {
        if (!isset($_SESSION['sensitive_records'])) {
            $_SESSION['sensitive_records'] = [];
        }
        // Encrypt sensitive fields; store name as-is (less sensitive in this demo)
        $_SESSION['sensitive_records'][] = [
            'name'  => $name,
            'email' => encrypt_data($email, DATA_KEY),
            'phone' => encrypt_data($phone, DATA_KEY),
            'ssn'   => encrypt_data($ssn,   DATA_KEY),
        ];
        $message = "Record for '{$name}' stored with encrypted fields.";
    }
}

if ($action === 'clear') {
    unset($_SESSION['sensitive_records']);
    $message = 'All records cleared.';
}

$records = $_SESSION['sensitive_records'] ?? [];
?>
<!DOCTYPE html>
<html>
<head>
    <title>6. Sensitive Data Storage</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 800px; margin: 30px auto; padding: 0 15px; }
        input[type=text], input[type=email] { width: 100%; padding: 5px; margin: 4px 0; box-sizing: border-box; }
        button { padding: 6px 14px; margin: 4px 4px 4px 0; }
        .ok { color: green; } .error { color: red; }
        table { border-collapse: collapse; width: 100%; margin-top: 10px; font-size: 12px; }
        td, th { border: 1px solid #ccc; padding: 5px; word-break: break-all; }
        .enc { color: #888; font-size: 10px; }
        a { display: block; margin-top: 20px; }
    </style>
</head>
<body>
    <h2>6. Secure Storage & Retrieval of Sensitive User Data</h2>

    <?php if ($message): ?><p class="ok"><?= htmlspecialchars($message) ?></p><?php endif; ?>
    <?php if ($error):   ?><p class="error"><?= htmlspecialchars($error) ?></p><?php endif; ?>

    <h3>Add Sensitive Record</h3>
    <form method="POST">
        <input type="text"  name="name"  placeholder="Full Name (required)">
        <input type="email" name="email" placeholder="Email (will be encrypted)">
        <input type="text"  name="phone" placeholder="Phone (will be encrypted)">
        <input type="text"  name="ssn"   placeholder="SSN / ID (will be encrypted)">
        <button type="submit" name="action" value="store">Store Encrypted</button>
    </form>

    <?php if (!empty($records)): ?>
    <h3>Stored Records</h3>

    <h4>As stored in DB (encrypted view)</h4>
    <table>
        <tr><th>Name</th><th>Email (Encrypted)</th><th>Phone (Encrypted)</th><th>SSN (Encrypted)</th></tr>
        <?php foreach ($records as $r): ?>
        <tr>
            <td><?= htmlspecialchars($r['name']) ?></td>
            <td class="enc"><?= htmlspecialchars($r['email']) ?></td>
            <td class="enc"><?= htmlspecialchars($r['phone']) ?></td>
            <td class="enc"><?= htmlspecialchars($r['ssn']) ?></td>
        </tr>
        <?php endforeach; ?>
    </table>

    <h4>After decryption (application layer)</h4>
    <table>
        <tr><th>Name</th><th>Email</th><th>Phone</th><th>SSN</th></tr>
        <?php foreach ($records as $r): ?>
        <tr>
            <td><?= htmlspecialchars($r['name']) ?></td>
            <td><?= htmlspecialchars(decrypt_data($r['email'], DATA_KEY)) ?></td>
            <td><?= htmlspecialchars(decrypt_data($r['phone'], DATA_KEY)) ?></td>
            <td><?= htmlspecialchars(decrypt_data($r['ssn'],   DATA_KEY)) ?></td>
        </tr>
        <?php endforeach; ?>
    </table>

    <form method="POST" style="margin-top:10px">
        <button type="submit" name="action" value="clear">Clear All Records</button>
    </form>
    <?php endif; ?>

    <hr>
    <h3>How it works</h3>
    <ul>
        <li>Sensitive fields (email, phone, SSN) are encrypted with <strong>AES-256-CBC</strong> before storage.</li>
        <li>The encryption key is kept server-side (in a real app: environment variable or key vault).</li>
        <li>The "DB" view shows opaque ciphertext; the application decrypts on retrieval.</li>
        <li>Even if the database is leaked, sensitive fields remain unreadable without the key.</li>
    </ul>
    <a href="index.php">← Back to Index</a>
</body>
</html>
