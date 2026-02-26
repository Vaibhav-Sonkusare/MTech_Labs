<?php
require_once 'functions.php';

$error = '';

if (is_logged_in()) {
    header("Location: dashboard.php");
    exit();
}

$pre_filled_username = '';
if (isset($_COOKIE['remember_username'])) {
    $pre_filled_username = $_COOKIE['remember_username'];
}

if ($_SERVER["REQUEST_METHOD"] == "POST") {
    $username = trim($_POST["username"]);
    $password = $_POST["password"];
    $remember = isset($_POST["remember_me"]) ? true : false;
    
    // Attempt login
    $user = find_user($username); // Checks session then file
    
    if ($user && $password === $user['password']) {
        // Success
        $_SESSION['user_modal_logged_in'] = true;
        $_SESSION['user_username'] = $user['username'];
        $_SESSION['user_full_name'] = $user['fullname'];
        
        if ($remember) {
            setcookie('remember_username', $username, time() + (86400 * 30), "/"); // 30 days
        } else {
            // Clear cookie if unchecked
            if (isset($_COOKIE['remember_username'])) {
                setcookie('remember_username', '', time() - 3600, "/");
            }
        }
        
        header("Location: dashboard.php");
        exit();
    } else {
        $error = "Invalid username or password.";
    }
}
?>
<?php include 'header.php'; ?>

<div class="auth-container card">
    <div class="auth-header">
        <h1>Welcome Back</h1>
        <p class="text-muted">Sign in to manage your claims</p>
    </div>

    <?php if ($error): ?>
        <div class="error-msg"><?php echo htmlspecialchars($error); ?></div>
    <?php endif; ?>

    <form action="index.php" method="POST">
        <div class="form-group">
            <label for="username">Username</label>
            <input type="text" id="username" name="username" value="<?php echo htmlspecialchars($pre_filled_username); ?>" required>
        </div>
        
        <div class="form-group">
            <label for="password">Password</label>
            <input type="password" id="password" name="password" required>
        </div>
        
        <div class="form-group" style="display: flex; align-items: center; justify-content: space-between;">
            <label style="margin-bottom: 0; display: flex; align-items: center; cursor: pointer;">
                <input type="checkbox" name="remember_me" style="width: auto; margin-right: 0.5rem;" <?php echo !empty($pre_filled_username) ? 'checked' : ''; ?>>
                Remember Me
            </label>
            <a href="forgot_password.php" style="font-size: 0.9rem;">Forgot Password?</a>
        </div>
        
        <button type="submit" class="btn" style="width: 100%;">Login</button>
    </form>
    
    <p style="text-align: center; margin-top: 1rem; color: #666;">
        Don't have an account? <a href="signup.php">Create Account</a>
    </p>
</div>

<?php include 'footer.php'; ?>
