<?php
session_start();

$error = '';
$success = '';

if ($_SERVER['REQUEST_METHOD'] === 'POST') {
    $fullname = trim($_POST['fullname'] ?? '');
    $username = trim($_POST['username'] ?? '');
    $email    = trim($_POST['email'] ?? '');
    $password = $_POST['password'] ?? '';
    $confirm  = $_POST['confirm_password'] ?? '';

    if (!$fullname || !$username || !$email || !$password || !$confirm) {
        $error = 'All fields are required.';
    } elseif (!filter_var($email, FILTER_VALIDATE_EMAIL)) {
        $error = 'Invalid email address.';
    } elseif ($password !== $confirm) {
        $error = 'Passwords do not match.';
    } else {
        // Store user in session (acts as our "database")
        $_SESSION['registered_users'][$username] = [
            'fullname' => $fullname,
            'username' => $username,
            'email'    => $email,
            'password' => $password,
        ];
        $success = 'Registration successful! <a href="login.php">Login here</a>.';
    }
}
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Register - Department Management System</title>
</head>
<body>
<h2>Register</h2>

<?php if ($error): ?>
    <p style="color:red;"><?= htmlspecialchars($error) ?></p>
<?php endif; ?>
<?php if ($success): ?>
    <p style="color:green;"><?= $success ?></p>
<?php endif; ?>

<form method="post" action="register.php">
    <table>
        <tr>
            <td><label for="fullname">Full Name:</label></td>
            <td><input type="text" id="fullname" name="fullname" required
                       value="<?= htmlspecialchars($_POST['fullname'] ?? '') ?>"></td>
        </tr>
        <tr>
            <td><label for="username">Username:</label></td>
            <td><input type="text" id="username" name="username" required
                       value="<?= htmlspecialchars($_POST['username'] ?? '') ?>"></td>
        </tr>
        <tr>
            <td><label for="email">Email ID:</label></td>
            <td><input type="email" id="email" name="email" required
                       value="<?= htmlspecialchars($_POST['email'] ?? '') ?>"></td>
        </tr>
        <tr>
            <td><label for="password">Password:</label></td>
            <td><input type="password" id="password" name="password" required></td>
        </tr>
        <tr>
            <td><label for="confirm_password">Confirm Password:</label></td>
            <td><input type="password" id="confirm_password" name="confirm_password" required></td>
        </tr>
        <tr>
            <td></td>
            <td><input type="submit" value="Register"></td>
        </tr>
    </table>
</form>

<p>Already have an account? <a href="login.php">Login</a></p>
</body>
</html>
