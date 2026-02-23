<?php
require_once 'functions.php';

require_login();

$username = $_SESSION['user_username'];
$last_step = isset($_COOKIE['last_step_' . $username]) ? $_COOKIE['last_step_' . $username] : null;

// Determine where "Resume Application" should point
$resume_link = 'claim_step1.php';
if ($last_step) {
    if ($last_step == '2') $resume_link = 'claim_step2.php';
    if ($last_step == 'summary') $resume_link = 'claim_summary.php';
}
?>
<?php include 'header.php'; ?>

    <h1>Dashboard</h1>

<div class="dashboard-grid">
    <!-- Apply for Insurance -->
    <div class="card stats-card">
        <h3>Apply</h3>
        <p>New policy application.</p>
        <a href="apply_insurance.php" class="btn">Apply Now</a>
    </div>

    <!-- Claim Insurance -->
    <div class="card stats-card">
        <h3>Claim</h3>
        <p>Report an incident.</p>
        
        <?php if ($last_step): ?>
            <a href="<?php echo htmlspecialchars($resume_link); ?>" class="btn">Resume</a>
            <a href="claim_step1.php?new=1" class="btn-secondary">New</a>
        <?php else: ?>
            <a href="claim_step1.php" class="btn">Start Claim</a>
        <?php endif; ?>
    </div>

    <!-- View Claim Summary -->
    <div class="card stats-card">
        <h3>History</h3>
        <p>Submitted claims.</p>
        <a href="view_claims.php" class="btn">View Claims</a>
    </div>
</div>

<?php include 'footer.php'; ?>
