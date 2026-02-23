<?php
if (session_status() === PHP_SESSION_NONE) {
    session_start();
}

// Functions to manage users using a JSON file for persistence, 
// while also utilizing session as requested by the prompt.

define('USER_DB', 'users.json');

function get_users() {
    if (!file_exists(USER_DB)) {
        return [];
    }
    $json = file_get_contents(USER_DB);
    return json_decode($json, true) ?? [];
}

function save_user($user) {
    $users = get_users();
    $users[$user['username']] = $user;
    file_put_contents(USER_DB, json_encode($users, JSON_PRETTY_PRINT));
    
    // Also store in session as requested
    if (!isset($_SESSION['registered_users'])) {
        $_SESSION['registered_users'] = [];
    }
    $_SESSION['registered_users'][$user['username']] = $user;
}

function find_user($username) {
    // Check session first (as per requirement priority)
    if (isset($_SESSION['registered_users']) && isset($_SESSION['registered_users'][$username])) {
        return $_SESSION['registered_users'][$username];
    }
    
    // Fallback to file for persistence (Remember Me feature needs this)
    $users = get_users();
    return $users[$username] ?? null;
}

function is_logged_in() {
    return isset($_SESSION['user_modal_logged_in']) && $_SESSION['user_modal_logged_in'] === true;
}

function require_login() {
    if (!is_logged_in()) {
        header("Location: index.php");
        exit();
    }
}
?>
