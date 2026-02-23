<?php
require_once 'functions.php';
require_login();
$username = $_SESSION['user_username'];

$claims = [];
$claims_file = 'claims.json';
if (file_exists($claims_file)) {
    $all_claims = json_decode(file_get_contents($claims_file), true);
    // Filter by username
    $claims = array_filter($all_claims, function($c) use ($username) {
        return $c['username'] === $username;
    });
}
?>
<?php include 'header.php'; ?>

<div class="card">
    <h2>Your Claims</h2>
    <?php if (empty($claims)): ?>
        <p>No claims found.</p>
        <a href="claim_step1.php" class="btn">File a Claim</a>
    <?php else: ?>
        <table class="summary-table">
            <thead>
                <tr>
                    <th>Date</th>
                    <th>Full Name</th>
                    <th>Policy Number</th>
                    <th>Incident Date</th>
                    <th>Status</th>
                </tr>
            </thead>
            <tbody>
                <?php foreach ($claims as $claim): ?>
                    <tr>
                        <td><?php echo htmlspecialchars($claim['date']); ?></td>
                        <td><?php echo htmlspecialchars($claim['step1']['fullname']); ?></td>
                        <td><?php echo htmlspecialchars($claim['step1']['policy_number']); ?></td>
                        <td><?php echo htmlspecialchars($claim['step2']['date']); ?></td>
                        <td><?php echo htmlspecialchars($claim['status']); ?></td>
                    </tr>
                <?php endforeach; ?>
            </tbody>
        </table>
    <?php endif; ?>
    <div style="margin-top: 1rem;">
        <a href="dashboard.php" class="btn btn-secondary">Back to Dashboard</a>
    </div>
</div>

<?php include 'footer.php'; ?>
