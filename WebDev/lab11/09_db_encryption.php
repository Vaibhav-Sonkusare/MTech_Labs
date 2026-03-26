<?php
/**
 * Program 9: Encrypt & Decrypt Database Records
 * Uses SQLite3 + AES-256-CBC to store and retrieve encrypted records.
 * Demonstrates field-level encryption for a "users" table.
 */

define('DB_PATH', __DIR__ . '/lab11_records.db');
define('DB_KEY',  'dbEncryptionKey_32bytes_exactly!'); // 32 bytes
define('CIPHER',  'AES-256-CBC');

function db_encrypt(string $value): string {
    $iv  = openssl_random_pseudo_bytes(openssl_cipher_iv_length(CIPHER));
    $enc = openssl_encrypt($value, CIPHER, DB_KEY, OPENSSL_RAW_DATA, $iv);
    return base64_encode($iv . $enc);
}

function db_decrypt(string $value): string {
    $raw   = base64_decode($value);
    $ivLen = openssl_cipher_iv_length(CIPHER);
    $dec   = openssl_decrypt(substr($raw, $ivLen), CIPHER, DB_KEY, OPENSSL_RAW_DATA, substr($raw, 0, $ivLen));
    return $dec !== false ? $dec : '[error]';
}

// Init SQLite DB
$db = new SQLite3(DB_PATH);
$db->exec("CREATE TABLE IF NOT EXISTS records (
    id      INTEGER PRIMARY KEY AUTOINCREMENT,
    name    TEXT NOT NULL,
    email   TEXT NOT NULL,
    salary  TEXT NOT NULL
)");

$msg   = '';
$error = '';
$action = $_POST['action'] ?? '';

if ($action === 'insert') {
    $name   = trim($_POST['name']   ?? '');
    $email  = trim($_POST['email']  ?? '');
    $salary = trim($_POST['salary'] ?? '');
    if ($name === '' || $email === '' || $salary === '') {
        $error = 'All fields required.';
    } else {
        $stmt = $db->prepare("INSERT INTO records (name, email, salary) VALUES (:n, :e, :s)");
        $stmt->bindValue(':n', $name);
        $stmt->bindValue(':e', db_encrypt($email));
        $stmt->bindValue(':s', db_encrypt($salary));
        $stmt->execute();
        $msg = "Record for '{$name}' inserted with encrypted email & salary.";
    }
}

if ($action === 'delete' && isset($_POST['id'])) {
    $id = (int)$_POST['id'];
    $db->exec("DELETE FROM records WHERE id = {$id}");
    $msg = "Record #{$id} deleted.";
}

// Fetch all records
$rows = [];
$res  = $db->query("SELECT * FROM records");
while ($row = $res->fetchArray(SQLITE3_ASSOC)) {
    $rows[] = $row;
}
?>
<!DOCTYPE html>
<html>
<head>
    <title>9. DB Record Encryption</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 900px; margin: 30px auto; padding: 0 15px; }
        input[type=text], input[type=email], input[type=number] {
            width: 100%; padding: 5px; margin: 4px 0; box-sizing: border-box;
        }
        button { padding: 6px 14px; margin: 4px 4px 4px 0; }
        .ok { color: green; } .error { color: red; }
        table { border-collapse: collapse; width: 100%; margin-top: 8px; font-size: 12px; }
        td, th { border: 1px solid #ccc; padding: 5px; }
        .raw { color: #888; font-size: 10px; word-break: break-all; max-width: 220px; }
        a { display: block; margin-top: 20px; }
    </style>
</head>
<body>
    <h2>9. Encrypt & Decrypt Database Records</h2>
    <p><em>Database:</em> SQLite at <code><?= htmlspecialchars(DB_PATH) ?></code></p>

    <?php if ($msg):   ?><p class="ok"><?= htmlspecialchars($msg) ?></p><?php endif; ?>
    <?php if ($error): ?><p class="error"><?= htmlspecialchars($error) ?></p><?php endif; ?>

    <h3>Insert Encrypted Record</h3>
    <form method="POST">
        <input type="text"   name="name"   placeholder="Full Name">
        <input type="email"  name="email"  placeholder="Email">
        <input type="text"   name="salary" placeholder="Salary">
        <button type="submit" name="action" value="insert">Insert</button>
    </form>

    <h3>Records</h3>
    <?php if (empty($rows)): ?>
        <p>No records yet.</p>
    <?php else: ?>
    <!-- Raw DB view -->
    <h4>Raw (as stored in DB — encrypted)</h4>
    <table>
        <tr><th>ID</th><th>Name</th><th>Email (enc)</th><th>Salary (enc)</th><th>Action</th></tr>
        <?php foreach ($rows as $r): ?>
        <tr>
            <td><?= $r['id'] ?></td>
            <td><?= htmlspecialchars($r['name']) ?></td>
            <td class="raw"><?= htmlspecialchars($r['email']) ?></td>
            <td class="raw"><?= htmlspecialchars($r['salary']) ?></td>
            <td>
                <form method="POST">
                    <input type="hidden" name="id" value="<?= $r['id'] ?>">
                    <button type="submit" name="action" value="delete">Delete</button>
                </form>
            </td>
        </tr>
        <?php endforeach; ?>
    </table>

    <!-- Decrypted view -->
    <h4>Decrypted (application layer)</h4>
    <table>
        <tr><th>ID</th><th>Name</th><th>Email</th><th>Salary</th></tr>
        <?php foreach ($rows as $r): ?>
        <tr>
            <td><?= $r['id'] ?></td>
            <td><?= htmlspecialchars($r['name']) ?></td>
            <td><?= htmlspecialchars(db_decrypt($r['email'])) ?></td>
            <td><?= htmlspecialchars(db_decrypt($r['salary'])) ?></td>
        </tr>
        <?php endforeach; ?>
    </table>
    <?php endif; ?>

    <hr>
    <h3>How it works</h3>
    <ul>
        <li>Sensitive columns (<em>email, salary</em>) are AES-256-CBC encrypted before INSERT.</li>
        <li>The <strong>name</strong> column is stored in plaintext (for searchability demo).</li>
        <li>Decryption happens in PHP, not the database — SQL never sees plaintext for sensitive fields.</li>
        <li>In production: use an HSM or secrets manager for the encryption key.</li>
    </ul>
    <a href="index.php">← Back to Index</a>
</body>
</html>
