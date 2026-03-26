<?php
/**
 * Program 2: Password Hashing & Secure Login Verification
 * Uses PHP's built-in password_hash() with BCRYPT algorithm.
 */

// Simulated "database" of users (hash stored, never plaintext)
$users_db = [];

// In-memory registry across page load via session
session_start();
if (!isset($_SESSION['users_db'])) {
    $_SESSION['users_db'] = [];
}

$output = '';
$error  = '';

$action = $_POST['action'] ?? '';

if ($action === 'register') {
    $username = trim($_POST['username'] ?? '');
    $password = $_POST['password'] ?? '';

    if ($username === '' || $password === '') {
        $error = 'Username and password are required.';
    } elseif (isset($_SESSION['users_db'][$username])) {
        $error = "User '{$username}' already exists.";
    } else {
        // Hash the password with BCRYPT (cost factor 12)
        $hash = password_hash($password, PASSWORD_BCRYPT, ['cost' => 12]);
        $_SESSION['users_db'][$username] = $hash;
        $output = "User '{$username}' registered successfully.<br>"
                . "<strong>Stored hash:</strong> <code>" . htmlspecialchars($hash) . "</code>";
    }

} elseif ($action === 'login') {
    $username = trim($_POST['username'] ?? '');
    $password = $_POST['password'] ?? '';

    if (!isset($_SESSION['users_db'][$username])) {
        $error = 'User not found.';
    } else {
        $storedHash = $_SESSION['users_db'][$username];
        if (password_verify($password, $storedHash)) {
            $output = "Login <strong>successful</strong> for user '{$username}'.";
            // Check if rehash needed (e.g. cost factor changed)
            if (password_needs_rehash($storedHash, PASSWORD_BCRYPT, ['cost' => 12])) {
                $_SESSION['users_db'][$username] = password_hash($password, PASSWORD_BCRYPT, ['cost' => 12]);
                $output .= " (Hash was upgraded.)";
            }
        } else {
            $error = 'Invalid password.';
        }
    }
}
?>
<!DOCTYPE html>
<html>
<head>
    <title>2. Password Hashing & Login</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 700px; margin: 30px auto; padding: 0 15px; }
        input[type=text], input[type=password] { width: 100%; padding: 5px; margin: 4px 0; box-sizing: border-box; }
        button { padding: 6px 14px; margin-top: 8px; }
        .ok { color: green; } .error { color: red; }
        code { word-break: break-all; background: #f4f4f4; padding: 2px 4px; }
        a { display: block; margin-top: 20px; }
    </style>
</head>
<body>
    <h2>2. Password Hashing & Secure Login Verification</h2>

    <?php if ($output): ?><p class="ok"><?= $output ?></p><?php endif; ?>
    <?php if ($error):  ?><p class="error"><?= htmlspecialchars($error) ?></p><?php endif; ?>

    <h3>Register</h3>
    <form method="POST">
        <input type="text"     name="username" placeholder="Username" required>
        <input type="password" name="password" placeholder="Password" required>
        <button type="submit" name="action" value="register">Register</button>
    </form>

    <h3>Login</h3>
    <form method="POST">
        <input type="text"     name="username" placeholder="Username" required>
        <input type="password" name="password" placeholder="Password" required>
        <button type="submit" name="action" value="login">Login</button>
    </form>

    <?php if (!empty($_SESSION['users_db'])): ?>
    <h3>Registered Users (hashes stored)</h3>
    <table border="1" cellpadding="5">
        <tr><th>Username</th><th>Bcrypt Hash</th></tr>
        <?php foreach ($_SESSION['users_db'] as $u => $h): ?>
        <tr>
            <td><?= htmlspecialchars($u) ?></td>
            <td style="font-size:11px;word-break:break-all"><?= htmlspecialchars($h) ?></td>
        </tr>
        <?php endforeach; ?>
    </table>
    <?php endif; ?>

    <hr>
    <h3>How it works</h3>
    <ul>
        <li><code>password_hash()</code> uses <strong>BCRYPT</strong> with a random salt and a cost factor of 12.</li>
        <li><code>password_verify()</code> safely compares without timing attacks.</li>
        <li><code>password_needs_rehash()</code> upgrades old hashes when the algorithm or cost changes.</li>
    </ul>
    <a href="index.php">← Back to Index</a>
</body>
</html>
