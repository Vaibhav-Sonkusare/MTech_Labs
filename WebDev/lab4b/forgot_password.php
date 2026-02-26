<?php
require_once 'functions.php';

// Simulate forgot password
$message = "";

if ($_SERVER["REQUEST_METHOD"] == "POST") {
    $email = trim($_POST["email"]);
    // Logic to send email (simulated)
    $message = "If an account exists for $email, password reset instructions have been sent.";
}
?>
<?php include 'header.php'; ?>

<div class="auth-container card">
    <div class="auth-header">
        <h1>Forgot Password</h1>
        <p class="text-muted">Enter your email to reset your password</p>
    </div>

    <?php if ($message): ?>
        <div class="success-msg"><?php echo htmlspecialchars($message); ?></div>
        <p style="text-align: center;"><a href="index.php" class="btn">Back to Login</a></p>
    <?php else: ?>
        <form action="forgot_password.php" method="POST">
            <div class="form-group">
                <label for="email">Email Address</label>
                <input type="email" id="email" name="email" required>
            </div>
            <button type="submit" class="btn" style="width: 100%;">Reset Password</button>
        </form>
        <p style="text-align: center; margin-top: 1rem;">
            <a href="index.php">Back to Login</a>
        </p>
    <?php endif; ?>
</div>

<?php include 'footer.php'; ?>
