<?php
require_once 'functions.php';
require_login();
$username = $_SESSION['user_username'];

$message = '';

if ($_SERVER["REQUEST_METHOD"] == "POST") {
    $policy_type = htmlspecialchars($_POST['policy_type']);
    
    $application = [
        'id' => uniqid(),
        'username' => $username,
        'policy_type' => $policy_type,
        'date' => date('Y-m-d H:i:s'),
        'status' => 'Pending'
    ];
    
    $file = 'applications.json';
    if (file_exists($file)) {
        $data = json_decode(file_get_contents($file), true);
    } else {
        $data = [];
    }
    
    $data[] = $application;
    file_put_contents($file, json_encode($data, JSON_PRETTY_PRINT));
    
    $message = "Application submitted successfully.";
}
?>
<?php include 'header.php'; ?>

<div class="card" style="max-width: 600px; margin: 0 auto;">
    <h2>Apply for Insurance</h2>
    
    <?php if ($message): ?>
        <div class="success-msg"><?php echo $message; ?></div>
    <?php endif; ?>
    
    <form action="apply_insurance.php" method="POST">
        <div class="form-group">
            <label for="policy_type">Select Policy Type</label>
            <select name="policy_type" id="policy_type" required>
                <option value="Health Insurance">Health Insurance</option>
                <option value="Car Insurance">Car Insurance</option>
                <option value="Life Insurance">Life Insurance</option>
                <option value="Home Insurance">Home Insurance</option>
            </select>
        </div>
        
        <button type="submit" class="btn">Submit Application</button>
        <a href="dashboard.php" class="btn btn-secondary">Cancel</a>
    </form>
</div>

<?php include 'footer.php'; ?>
