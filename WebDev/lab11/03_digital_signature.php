<?php
/**
 * Program 3: Digital Signatures using Public/Private Keys (RSA + SHA256)
 * Generates a key pair, signs a message, and verifies the signature.
 */

session_start();

$output  = '';
$error   = '';
$action  = $_POST['action'] ?? '';

// Generate RSA key pair and store in session
if ($action === 'generate_keys') {
    $config = [
        'digest_alg'       => 'sha256',
        'private_key_bits' => 2048,
        'private_key_type' => OPENSSL_KEYTYPE_RSA,
    ];
    $res = openssl_pkey_new($config);
    openssl_pkey_export($res, $privateKey);
    $pubKeyDetails = openssl_pkey_get_details($res);
    $publicKey = $pubKeyDetails['key'];

    $_SESSION['ds_private'] = $privateKey;
    $_SESSION['ds_public']  = $publicKey;
    $output = 'Key pair generated successfully.';
}

// Sign a message
if ($action === 'sign') {
    $message = $_POST['message'] ?? '';
    if (empty($_SESSION['ds_private'])) {
        $error = 'Generate a key pair first.';
    } else {
        openssl_sign($message, $signature, $_SESSION['ds_private'], OPENSSL_ALGO_SHA256);
        $_SESSION['ds_signature'] = base64_encode($signature);
        $_SESSION['ds_message']   = $message;
        $output = 'Message signed. Signature (Base64):<br><code>' . htmlspecialchars(base64_encode($signature)) . '</code>';
    }
}

// Verify signature
if ($action === 'verify') {
    $message   = $_POST['message'] ?? '';
    $sigB64    = trim($_POST['signature'] ?? '');
    if (empty($_SESSION['ds_public'])) {
        $error = 'Generate a key pair first.';
    } else {
        $signature = base64_decode($sigB64);
        $result    = openssl_verify($message, $signature, $_SESSION['ds_public'], OPENSSL_ALGO_SHA256);
        if ($result === 1)      $output = '<strong style="color:green">Signature VALID ✓</strong>';
        elseif ($result === 0)  $output = '<strong style="color:red">Signature INVALID ✗</strong>';
        else                    $error  = 'Verification error: ' . openssl_error_string();
    }
}
?>
<!DOCTYPE html>
<html>
<head>
    <title>3. Digital Signatures</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 750px; margin: 30px auto; padding: 0 15px; }
        textarea { width: 100%; height: 70px; box-sizing: border-box; margin: 4px 0; }
        button { padding: 6px 14px; margin: 4px 4px 4px 0; }
        pre { background: #f4f4f4; padding: 8px; font-size: 11px; word-break: break-all; white-space: pre-wrap; }
        .error { color: red; }
        code { word-break: break-all; }
        a { display: block; margin-top: 20px; }
    </style>
</head>
<body>
    <h2>3. Digital Signatures (RSA-SHA256)</h2>

    <!-- Step 1: Generate Keys -->
    <form method="POST">
        <button type="submit" name="action" value="generate_keys">1. Generate RSA Key Pair</button>
    </form>

    <?php if (!empty($_SESSION['ds_private'])): ?>
    <details>
        <summary>Private Key</summary>
        <pre><?= htmlspecialchars($_SESSION['ds_private']) ?></pre>
    </details>
    <details>
        <summary>Public Key</summary>
        <pre><?= htmlspecialchars($_SESSION['ds_public']) ?></pre>
    </details>

    <!-- Step 2: Sign -->
    <h3>2. Sign a Message</h3>
    <form method="POST">
        <label>Message:</label>
        <textarea name="message"><?= htmlspecialchars($_SESSION['ds_message'] ?? '') ?></textarea>
        <button type="submit" name="action" value="sign">Sign</button>
    </form>

    <!-- Step 3: Verify -->
    <h3>3. Verify Signature</h3>
    <form method="POST">
        <label>Message:</label>
        <textarea name="message"><?= htmlspecialchars($_SESSION['ds_message'] ?? '') ?></textarea>
        <label>Signature (Base64):</label>
        <textarea name="signature"><?= htmlspecialchars($_SESSION['ds_signature'] ?? '') ?></textarea>
        <button type="submit" name="action" value="verify">Verify</button>
    </form>
    <?php else: ?>
        <p>Please generate a key pair first.</p>
    <?php endif; ?>

    <?php if ($output): ?><p><?= $output ?></p><?php endif; ?>
    <?php if ($error):  ?><p class="error"><?= htmlspecialchars($error) ?></p><?php endif; ?>

    <hr>
    <h3>How it works</h3>
    <ul>
        <li>RSA 2048-bit key pair generated with <code>openssl_pkey_new()</code>.</li>
        <li>Message is signed with the <strong>private key</strong> using SHA-256 (<code>openssl_sign()</code>).</li>
        <li>Anyone with the <strong>public key</strong> can verify via <code>openssl_verify()</code>.</li>
        <li>Even a single character change in the message invalidates the signature.</li>
    </ul>
    <a href="index.php">← Back to Index</a>
</body>
</html>
