<?php
session_start();

// Redirect if already logged in
if (isset($_SESSION['logged_in']) && $_SESSION['logged_in'] === true) {
    header('Location: dashboard.php');
    exit;
}

$error = '';
// Pre-fill username from cookie if Remember Me was used
$cookie_username = $_COOKIE['remember_username'] ?? '';

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $username  = trim($_POST['username'] ?? '');
    $password  = $_POST['password'] ?? '';
    $remember  = isset($_POST['remember_me']);

    $users = $_SESSION['registered_users'] ?? [];

    if (!$username || !$password) {
        $error = 'Username and password are required.';
    } elseif (!isset($users[$username])) {
        $error = 'Username not found.';
    } elseif ($users[$username]['password'] !== $password) {
        $error = 'Incorrect password.';
    } else {
        // Successful login
        $_SESSION['logged_in'] = true;
        $_SESSION['username']  = $username;
        $_SESSION['fullname']  = $users[$username]['fullname'];

        if ($remember) {
            // Set cookie for 30 days
            setcookie('remember_username', $username, time() + (30 * 24 * 3600), '/');
        } else {
            // Remove cookie if not checked
            setcookie('remember_username', '', time() - 3600, '/');
        }

        // If a last_step cookie exists, restore the step
        header('Location: dashboard.php');
        exit;
    }
}
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Login - Department Management System</title>
</head>
<body>
<h2>Login</h2>

<?php if ($error): ?>
    <p style="color:red;"><?= htmlspecialchars($error) ?></p>
<?php endif; ?>

<form method="post" action="login.php">
    <table>
        <tr>
            <td><label for="username">Username:</label></td>
            <td><input type="text" id="username" name="username" required
                       value="<?= htmlspecialchars(!empty($_POST['username']) ? $_POST['username'] : $cookie_username) ?>"></td>
        </tr>
        <tr>
            <td><label for="password">Password:</label></td>
            <td><input type="password" id="password" name="password" required></td>
        </tr>
        <tr>
            <td></td>
            <td>
                <input type="checkbox" id="remember_me" name="remember_me"
                    <?= $cookie_username ? 'checked' : '' ?>>
                <label for="remember_me">Remember Me</label>
            </td>
        </tr>
        <tr>
            <td></td>
            <td><input type="submit" value="Login"></td>
        </tr>
    </table>
</form>

<p>Don't have an account? <a href="register.php">Register</a></p>
</body>
</html>
