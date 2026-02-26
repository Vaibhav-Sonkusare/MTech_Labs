<?php
require_once 'functions.php';
require_login();
$username = $_SESSION['user_username'];

if (!isset($_SESSION['claim_data']['step1']) || !isset($_SESSION['claim_data']['step2'])) {
    header("Location: dashboard.php");
    exit();
}

$is_submitted = false;

// If "Confirm" button was clicked
if ($_SERVER["REQUEST_METHOD"] == "POST") {
    $is_submitted = true;
    
    // Save claim to file
    $claim = [
        'id' => uniqid(),
        'username' => $username,
        'step1' => $_SESSION['claim_data']['step1'],
        'step2' => $_SESSION['claim_data']['step2'],
        'status' => 'Submitted',
        'date' => date('Y-m-d H:i:s')
    ];
    
    $file = 'claims.json';
    $claims = file_exists($file) ? json_decode(file_get_contents($file), true) : [];
    $claims[] = $claim;
    file_put_contents($file, json_encode($claims, JSON_PRETTY_PRINT));
    
    // Clear the progress cookie
    setcookie("last_step_" . $username, "summary", time() + 86400, "/");

    // We keep session data for display, but mark as "done" internally if needed.
    // For this lab, just displaying it is enough.
} else {
    // Just visiting: Update cookie
    setcookie("last_step_" . $username, "summary", time() + 86400, "/");
}

$step1 = $_SESSION['claim_data']['step1'];
$step2 = $_SESSION['claim_data']['step2'];
?>
<?php include 'header.php'; ?>

<div class="card" style="max-width: 800px; margin: 0 auto;">
    <?php if (!$is_submitted): ?>
        <div class="step-indicator">
            <div class="step completed">1</div>
            <div class="step completed">2</div>
            <div class="step active">3</div>
        </div>
        <h2 style="text-align: center; margin-bottom: 2rem;">Review Your Claim</h2>
    <?php else: ?>
        <div class="success-msg" style="text-align: center;">
            <h2 style="color: #22c55e;">Claim Submitted Successfully!</h2>
            <p>Your claim has been recorded.</p>
        </div>
        <h3 style="text-align: center; margin-bottom: 2rem;">Final Summary</h3>
    <?php endif; ?>

    <div class="summary-section">
        <h3>Personal Details</h3>
        <table class="summary-table">
            <tr>
                <th>Full Name</th>
                <td><?php echo htmlspecialchars($step1['fullname']); ?></td>
            </tr>
            <tr>
                <th>Phone</th>
                <td><?php echo htmlspecialchars($step1['phone']); ?></td>
            </tr>
            <tr>
                <th>Policy Number</th>
                <td><?php echo htmlspecialchars($step1['policy_number']); ?></td>
            </tr>
        </table>
    </div>
    
    <div class="summary-section" style="margin-top: 2rem;">
        <h3>Incident Details</h3>
        <table class="summary-table">
            <tr>
                <th>Date</th>
                <td><?php echo htmlspecialchars($step2['date']); ?></td>
            </tr>
            <tr>
                <th>Location</th>
                <td><?php echo htmlspecialchars($step2['location']); ?></td>
            </tr>
            <tr>
                <th>Description</th>
                <td><?php echo htmlspecialchars($step2['description']); ?></td>
            </tr>
        </table>
    </div>

    <div style="margin-top: 2rem; display: flex; justify-content: space-between;">
        <?php if (!$is_submitted): ?>
            <a href="claim_step2.php" class="btn btn-secondary">&larr; Edit Details</a>
            <form action="claim_summary.php" method="POST" style="display:inline;">
                <button type="submit" class="btn">Confirm & Submit Claim</button>
            </form>
        <?php else: ?>
            <a href="dashboard.php" class="btn">Back to Dashboard</a>
            <a href="logout.php" class="btn btn-secondary">Logout</a>
        <?php endif; ?>
    </div>
</div>

<?php 
// Clean up session if submitted
if ($is_submitted) {
    unset($_SESSION['claim_data']);
    // Reset cookie
    setcookie("last_step_" . $username, "", time() - 3600, "/");
}
?>

<?php include 'footer.php'; ?>
