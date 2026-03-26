<?php
/**
 * Program 10: Secure Token Generation for Authentication
 * Demonstrates: CSRF tokens, API tokens, password-reset tokens, and OTPs
 * using cryptographically secure random functions.
 */

session_start();

$msg   = '';
$error = '';
$action = $_POST['action'] ?? '';

// -------- Token generation helpers --------

/** Cryptographically secure random token (hex) */
function generate_token(int $bytes = 32): string {
    return bin2hex(random_bytes($bytes));
}

/** Time-limited token: token + expiry stored in session */
function generate_timed_token(string $purpose, int $ttlSeconds = 30): string {
    $token = generate_token(24);
    $_SESSION['tokens'][$purpose] = [
        'value'   => $token,
        'expires' => time() + $ttlSeconds,
    ];
    return $token;
}

/** Verify a timed token */
function verify_timed_token(string $purpose, string $input): string {
    $entry = $_SESSION['tokens'][$purpose] ?? null;
    if (!$entry)                              return 'NOT_FOUND';
    if (time() > $entry['expires'])           return 'EXPIRED';
    if (!hash_equals($entry['value'], $input)) return 'INVALID';
    unset($_SESSION['tokens'][$purpose]);     // one-time use
    return 'VALID';
}

/** 6-digit OTP */
function generate_otp(): string {
    return str_pad((string)random_int(0, 999999), 6, '0', STR_PAD_LEFT);
}

/** JWT-like signed token (Header.Payload.Sig using HMAC-SHA256) */
function generate_signed_token(array $payload, string $secret): string {
    $header  = base64_encode(json_encode(['alg' => 'HS256', 'typ' => 'JWT-like']));
    $body    = base64_encode(json_encode($payload));
    $sig     = base64_encode(hash_hmac('sha256', $header . '.' . $body, $secret, true));
    return $header . '.' . $body . '.' . $sig;
}

function verify_signed_token(string $token, string $secret): array|false {
    $parts = explode('.', $token);
    if (count($parts) !== 3) return false;
    [$header, $body, $sig] = $parts;
    $expectedSig = base64_encode(hash_hmac('sha256', $header . '.' . $body, $secret, true));
    if (!hash_equals($expectedSig, $sig)) return false;
    $payload = json_decode(base64_decode($body), true);
    if (isset($payload['exp']) && time() > $payload['exp']) return false;
    return $payload;
}

define('SIGN_SECRET', 'jwtLikeSigningSecret_32bytes!!!!');

// -------- Actions --------
if (!isset($_SESSION['tokens'])) $_SESSION['tokens'] = [];

switch ($action) {
    case 'gen_csrf':
        $_SESSION['csrf_token'] = generate_token();
        $msg = 'CSRF token generated.';
        break;
    case 'gen_api':
        $_SESSION['api_token'] = generate_token(32);
        $msg = 'API token generated.';
        break;
    case 'gen_otp':
        $_SESSION['otp'] = generate_otp();
        $msg = 'OTP generated (valid for demo only).';
        break;
    case 'gen_reset':
        $_SESSION['reset_token_display'] = generate_timed_token('password_reset', 300);
        $msg = 'Password-reset token generated (expires in 300s, one-time use).';
        break;
    case 'verify_reset':
        $input  = trim($_POST['reset_input'] ?? '');
        $result = verify_timed_token('password_reset', $input);
        if ($result === 'VALID')     $msg   = 'Reset token VALID ✓ (now invalidated).';
        elseif ($result === 'EXPIRED') $error = 'Token EXPIRED.';
        elseif ($result === 'INVALID') $error = 'Token INVALID ✗.';
        else                           $error = 'No token found. Generate one first.';
        break;
    case 'gen_jwt':
        $payload   = ['sub' => 'user42', 'role' => 'student', 'iat' => time(), 'exp' => time() + 3600];
        $_SESSION['jwt_token'] = generate_signed_token($payload, SIGN_SECRET);
        $msg = 'Signed token (JWT-like) generated.';
        break;
    case 'verify_jwt':
        $t = trim($_POST['jwt_input'] ?? '');
        $p = verify_signed_token($t, SIGN_SECRET);
        if ($p === false) $error = 'Signature INVALID or token expired ✗';
        else              $msg   = 'Token VALID ✓ Payload: ' . htmlspecialchars(json_encode($p));
        break;
}
?>
<!DOCTYPE html>
<html>
<head>
    <title>10. Secure Token Generation</title>
    <style>
        body { font-family: Arial, sans-serif; max-width: 800px; margin: 30px auto; padding: 0 15px; }
        input[type=text] { width: 100%; padding: 5px; margin: 4px 0; box-sizing: border-box; }
        button { padding: 6px 14px; margin: 4px 4px 4px 0; }
        .ok { color: green; } .error { color: red; }
        code { background: #f4f4f4; padding: 2px 4px; word-break: break-all; font-size: 12px; }
        section { border: 1px solid #ddd; padding: 10px; margin-bottom: 15px; }
        h3 { margin-top: 0; }
        a { display: block; margin-top: 20px; }
    </style>
</head>
<body>
    <h2>10. Secure Token Generation for Authentication</h2>

    <?php if ($msg):   ?><p class="ok"><?= $msg ?></p><?php endif; ?>
    <?php if ($error): ?><p class="error"><?= htmlspecialchars($error) ?></p><?php endif; ?>

    <!-- CSRF Token -->
    <section>
        <h3>CSRF Token</h3>
        <form method="POST">
            <button type="submit" name="action" value="gen_csrf">Generate CSRF Token</button>
        </form>
        <?php if (!empty($_SESSION['csrf_token'])): ?>
        <p>Token: <code><?= htmlspecialchars($_SESSION['csrf_token']) ?></code> (64 hex chars = 256 bits)</p>
        <?php endif; ?>
    </section>

    <!-- API Token -->
    <section>
        <h3>API Bearer Token</h3>
        <form method="POST">
            <button type="submit" name="action" value="gen_api">Generate API Token</button>
        </form>
        <?php if (!empty($_SESSION['api_token'])): ?>
        <p>Token: <code><?= htmlspecialchars($_SESSION['api_token']) ?></code></p>
        <?php endif; ?>
    </section>

    <!-- OTP -->
    <section>
        <h3>6-Digit OTP</h3>
        <form method="POST">
            <button type="submit" name="action" value="gen_otp">Generate OTP</button>
        </form>
        <?php if (!empty($_SESSION['otp'])): ?>
        <p>OTP: <strong><?= htmlspecialchars($_SESSION['otp']) ?></strong></p>
        <?php endif; ?>
    </section>

    <!-- Password Reset Token -->
    <section>
        <h3>Timed Password-Reset Token (TTL: 300s, one-time use)</h3>
        <form method="POST">
            <button type="submit" name="action" value="gen_reset">Generate Reset Token</button>
        </form>
        <?php if (!empty($_SESSION['reset_token_display'])): ?>
        <p>Token: <code><?= htmlspecialchars($_SESSION['reset_token_display']) ?></code></p>
        <form method="POST">
            <input type="text" name="reset_input" placeholder="Paste token to verify">
            <button type="submit" name="action" value="verify_reset">Verify</button>
        </form>
        <?php endif; ?>
    </section>

    <!-- Signed JWT-like Token -->
    <section>
        <h3>Signed Token (JWT-like, HMAC-SHA256, 1hr expiry)</h3>
        <form method="POST">
            <button type="submit" name="action" value="gen_jwt">Generate Signed Token</button>
        </form>
        <?php if (!empty($_SESSION['jwt_token'])): ?>
        <p style="font-size:11px;word-break:break-all"><code><?= htmlspecialchars($_SESSION['jwt_token']) ?></code></p>
        <form method="POST">
            <input type="text" name="jwt_input" value="<?= htmlspecialchars($_SESSION['jwt_token']) ?>">
            <button type="submit" name="action" value="verify_jwt">Verify Signature</button>
        </form>
        <?php endif; ?>
    </section>

    <hr>
    <h3>Key Principles</h3>
    <ul>
        <li><code>random_bytes()</code> / <code>random_int()</code> are <strong>CSPRNG</strong>-based; never use <code>rand()</code> or <code>mt_rand()</code> for security tokens.</li>
        <li>CSRF tokens are per-session and validated server-side before any state-changing action.</li>
        <li>Timed tokens expire and are invalidated after first use to prevent replay.</li>
        <li><code>hash_equals()</code> is used for constant-time comparison to prevent timing attacks.</li>
        <li>JWT-like tokens carry a signed payload; the server verifies the HMAC before trusting any claim.</li>
    </ul>
    <a href="index.php">← Back to Index</a>
</body>
</html>
