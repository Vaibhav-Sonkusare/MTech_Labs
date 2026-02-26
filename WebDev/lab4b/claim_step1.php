<?php
require_once 'functions.php';

require_login();

$username = $_SESSION['user_username'];

// Handle "New Claim" request or reset
if (isset($_GET['new'])) {
    unset($_SESSION['claim_data']);
    setcookie("last_step_" . $username, "1", time() + 86400, "/"); 
}

// Initialize data array if not exists
if (!isset($_SESSION['claim_data'])) {
    $_SESSION['claim_data'] = [];
}

// Default values from session or user profile
$fullname = $_SESSION['claim_data']['step1']['fullname'] ?? $_SESSION['user_full_name'] ?? '';
$phone = $_SESSION['claim_data']['step1']['phone'] ?? '';
$policy_number = $_SESSION['claim_data']['step1']['policy_number'] ?? '';

// Update progress cookie
setcookie("last_step_" . $username, "1", time() + 86400, "/");

if ($_SERVER["REQUEST_METHOD"] == "POST") {
    $fullname = htmlspecialchars($_POST['fullname']);
    $phone = htmlspecialchars($_POST['phone']);
    $policy_number = htmlspecialchars($_POST['policy_number']);
    
    // Server-side validation (basic)
    if ($fullname && $phone && $policy_number) {
        $_SESSION['claim_data']['step1'] = [
            'fullname' => $fullname,
            'phone' => $phone,
            'policy_number' => $policy_number
        ];
        
        // Move to step 2
        header("Location: claim_step2.php");
        exit();
    }
}
?>
<?php include 'header.php'; ?>

<div class="card" style="max-width: 800px; margin: 0 auto;">
    <div class="step-indicator">
        <div class="step active">1</div>
        <div class="step">2</div>
        <div class="step">3</div>
    </div>

    <h2 style="text-align: center; margin-bottom: 2rem;">Step 1: Personal Details</h2>

    <form action="claim_step1.php" method="POST">
        <div class="form-group">
            <label for="fullname">Full Name</label>
            <input type="text" id="fullname" name="fullname" value="<?php echo htmlspecialchars($fullname); ?>" required>
        </div>

        <div class="form-group">
            <label for="phone">Phone Number</label>
            <input type="tel" id="phone" name="phone" value="<?php echo htmlspecialchars($phone); ?>" required>
        </div>

        <div class="form-group">
            <label for="policy_number">Policy Number</label>
            <input type="text" id="policy_number" name="policy_number" value="<?php echo htmlspecialchars($policy_number); ?>" required>
        </div>

        <div style="display: flex; justify-content: flex-end;">
            <button type="submit" class="btn">Next: Incident Details &rarr;</button>
        </div>
    </form>
</div>

<?php include 'footer.php'; ?>
