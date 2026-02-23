<?php
require_once 'functions.php';

require_login();
$username = $_SESSION['user_username'];

// Ensure Step 1 is done
if (empty($_SESSION['claim_data']['step1'])) {
    header("Location: claim_step1.php");
    exit();
}

// Update progress cookie
setcookie("last_step_" . $username, "2", time() + 86400, "/");

// Values
$date = $_SESSION['claim_data']['step2']['date'] ?? '';
$location = $_SESSION['claim_data']['step2']['location'] ?? '';
$description = $_SESSION['claim_data']['step2']['description'] ?? '';

if ($_SERVER["REQUEST_METHOD"] == "POST") {
    $date = htmlspecialchars($_POST['date']);
    $location = htmlspecialchars($_POST['location']);
    $description = htmlspecialchars($_POST['description']);
    
    if ($date && $location && $description) {
        $_SESSION['claim_data']['step2'] = [
            'date' => $date,
            'location' => $location,
            'description' => $description
        ];
        
        // Move to Summary
        header("Location: claim_summary.php");
        exit();
    }
}
?>
<?php include 'header.php'; ?>

<div class="card" style="max-width: 800px; margin: 0 auto;">
    <div class="step-indicator">
        <div class="step completed">1</div>
        <div class="step active">2</div>
        <div class="step">3</div>
    </div>

    <h2 style="text-align: center; margin-bottom: 2rem;">Step 2: Incident Details</h2>

    <form action="claim_step2.php" method="POST">
        <div class="form-group">
            <label for="date">Date of Incident</label>
            <input type="date" id="date" name="date" value="<?php echo htmlspecialchars($date); ?>" required>
        </div>

        <div class="form-group">
            <label for="location">Location</label>
            <input type="text" id="location" name="location" value="<?php echo htmlspecialchars($location); ?>" required>
        </div>

        <div class="form-group">
            <label for="description">Description of Incident</label>
            <textarea id="description" name="description" rows="5" required><?php echo htmlspecialchars($description); ?></textarea>
        </div>

        <div style="display: flex; justify-content: space-between;">
            <a href="claim_step1.php" class="btn btn-secondary">&larr; Back</a>
            <button type="submit" class="btn">Next: Review & Submit &rarr;</button>
        </div>
    </form>
</div>

<?php include 'footer.php'; ?>
