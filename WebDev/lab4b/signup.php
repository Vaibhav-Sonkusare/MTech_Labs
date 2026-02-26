<?php
require_once 'functions.php';

$error = '';
$success = '';

if ($_SERVER["REQUEST_METHOD"] == "POST") {
    $fullname = trim($_POST["fullname"]);
    $username = trim($_POST["username"]);
    $email = trim($_POST["email"]);
    $password = $_POST["password"];
    $confirm_password = $_POST["confirm_password"];

    if (empty($fullname) || empty($username) || empty($email) || empty($password)) {
        $error = "All fields are required.";
    } elseif ($password !== $confirm_password) {
        $error = "Passwords do not match.";
    } elseif (find_user($username)) {
        $error = "Username already exists.";
    } else {
        // Register user
        $new_user = [
            'fullname' => $fullname,
            'username' => $username,
            'email' => $email,
            'password' => $password, // Plain text password per request
        ];
        
        save_user($new_user);
        
        $success = "Registration successful! You can now login.";

    }
}
?>
<?php include 'header.php'; ?>

<div class="auth-container card">
    <div class="auth-header">
        <h1>Create Account</h1>
        <p class="text-muted">Join Insurance System today</p>
    </div>

    <?php if ($error): ?>
        <div class="error-msg"><?php echo htmlspecialchars($error); ?></div>
    <?php endif; ?>
    
    <?php if ($success): ?>
        <div class="success-msg">
            <?php echo htmlspecialchars($success); ?> 
            <br><a href="index.php" class="btn" style="margin-top: 1rem;">Go to Login</a>
        </div>
    <?php else: ?>
        <form action="signup.php" method="POST">
            <div class="form-group">
                <label for="fullname">Full Name</label>
                <input type="text" id="fullname" name="fullname" required>
            </div>
            
            <div class="form-group">
                <label for="username">Username</label>
                <input type="text" id="username" name="username" required>
            </div>
            
            <div class="form-group">
                <label for="email">Email Address</label>
                <input type="email" id="email" name="email" required>
            </div>
            
            <div class="form-group">
                <label for="password">Password</label>
                <input type="password" id="password" name="password" required>
            </div>
            
            <div class="form-group">
                <label for="confirm_password">Confirm Password</label>
                <input type="password" id="confirm_password" name="confirm_password" required>
            </div>
            
            <button type="submit" class="btn" style="width: 100%;">Sign Up</button>
        </form>
        <p style="text-align: center; margin-top: 1rem; color: var(--text-muted);">
            Already have an account? <a href="index.php">Log In</a>
        </p>
    <?php endif; ?>
</div>

<?php include 'footer.php'; ?>
