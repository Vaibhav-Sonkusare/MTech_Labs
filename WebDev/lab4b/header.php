<?php
if (session_status() === PHP_SESSION_NONE) {
    session_start();
}
?>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Insurance Claim System</title>
    <link rel="stylesheet" href="style.css">

</head>
<body>
    <header>
        <nav>
            <a href="index.php" class="brand">Insurance System</a>
            <div class="nav-links">
                <?php if (isset($_SESSION['user_full_name'])): ?>
                    <span>user: <?php echo htmlspecialchars($_SESSION['user_full_name']); ?></span>
                    <a href="dashboard.php">Dashboard</a>
                    <a href="logout.php">Logout</a>
                <?php else: ?>
                    <a href="index.php">Login</a>
                    <a href="signup.php">Sign Up</a>
                <?php endif; ?>
            </div>
        </nav>
    </header>
    <main class="container">
