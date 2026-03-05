<?php
session_start();

// Handle Logout
if (isset($_GET['logout'])) {
    session_unset();
    session_destroy();
    header("Location: index.php");
    exit();
}

// Database Connection & Table Creation
$db = new SQLite3('grocery.db');
$db->exec("CREATE TABLE IF NOT EXISTS grocery_items(
    item_id TEXT PRIMARY KEY,
    name TEXT,
    category TEXT,
    quantity INTEGER,
    price REAL,
    status TEXT
)");

// Handle AJAX Requests
if (isset($_GET['action']) || isset($_POST['action'])) {
    $action = $_GET['action'] ?? $_POST['action'];

    if ($action == 'add') {
        $item_id = $db->escapeString($_POST['item_id']);
        $name = $db->escapeString($_POST['name']);
        $category = $db->escapeString($_POST['category']);
        $quantity = (int)$_POST['quantity'];
        $price = (float)$_POST['price'];
        $status = $db->escapeString($_POST['status']);

        // Check if item_id already exists
        $check = $db->querySingle("SELECT COUNT(*) FROM grocery_items WHERE item_id='$item_id'");
        if ($check > 0) {
            echo "Error: Item ID already exists.";
        } else {
            $stmt = $db->prepare("INSERT INTO grocery_items (item_id, name, category, quantity, price, status) VALUES (:item_id, :name, :category, :quantity, :price, :status)");
            $stmt->bindValue(':item_id', $item_id, SQLITE3_TEXT);
            $stmt->bindValue(':name', $name, SQLITE3_TEXT);
            $stmt->bindValue(':category', $category, SQLITE3_TEXT);
            $stmt->bindValue(':quantity', $quantity, SQLITE3_INTEGER);
            $stmt->bindValue(':price', $price, SQLITE3_FLOAT);
            $stmt->bindValue(':status', $status, SQLITE3_TEXT);
            
            if ($stmt->execute()) {
                echo "Grocery Item Added Successfully!";
            } else {
                echo "Error adding item.";
            }
        }
        exit;
    }

    if ($action == 'fetch') {
        $search = isset($_GET['search']) ? $db->escapeString($_GET['search']) : '';
        
        if ($search !== '') {
            $query = "SELECT * FROM grocery_items WHERE item_id LIKE '%$search%' OR name LIKE '%$search%' OR category LIKE '%$search%'";
        } else {
            $query = "SELECT * FROM grocery_items";
        }
        
        $result = $db->query($query);
        $items = [];
        while ($row = $result->fetchArray(SQLITE3_ASSOC)) {
            $items[] = $row;
        }
        
        // Return JSON response for easier manipulation on client side
        echo json_encode(['count' => count($items), 'items' => $items]);
        exit;
    }

    if ($action == 'delete') {
        $item_id = isset($_POST['item_id']) ? $db->escapeString($_POST['item_id']) : '';
        if ($item_id) {
            $db->exec("DELETE FROM grocery_items WHERE item_id='$item_id'");
            echo "Item Deleted successfully.";
        } else {
            echo "Error: Missing Item ID.";
        }
        exit;
    }
}
?>
<!DOCTYPE html>
<html>
<head>
    <title>Grocery Management System</title>
    <style>
        body { font-family: sans-serif; margin: 20px; }
        .nav { margin-bottom: 20px; padding-bottom: 10px; border-bottom: 1px solid #ccc; }
        .nav button, .nav a { 
            margin-right: 10px; 
            padding: 8px 12px; 
            text-decoration: none; 
            border: 1px solid #ccc; 
            background: #eee; 
            color: #333; 
            cursor: pointer; 
            font-size: 14px;
        }
        .section { display: none; margin-top: 20px; }
        .active { display: block; }
        table { border-collapse: collapse; width: 100%; margin-top: 15px; }
        th, td { border: 1px solid #aaa; padding: 8px; text-align: left; }
        th { background-color: #f9f9f9; }
        .form-group { margin-bottom: 15px; }
        .form-group label { display: inline-block; width: 120px; font-weight: bold; }
        .controls { margin-bottom: 15px; }
    </style>
</head>
<body>

<h1>Grocery Management System</h1>

<div class="nav">
    <button onclick="showSection('add-section')">Add Grocery Items</button>
    <button onclick="showSection('view-section'); loadCatalog();">View Grocery Catalog</button>
    <a href="index.php?logout=true">Logout</a>
</div>

<!-- ================= ADD GROCERY ITEMS SECTION ================= -->
<div id="add-section" class="section active">
    <h2>Add Grocery Item</h2>
    <form id="addForm" onsubmit="addItem(event)">
        <div class="form-group">
            <label>Item ID:</label>
            <input type="text" id="item_id" required placeholder="e.g., GR001">
        </div>
        <div class="form-group">
            <label>Item Name:</label>
            <input type="text" id="name" required>
        </div>
        <div class="form-group">
            <label>Category:</label>
            <input type="text" id="category" required>
        </div>
        <div class="form-group">
            <label>Quantity:</label>
            <input type="number" id="quantity" required min="0">
        </div>
        <div class="form-group">
            <label>Price:</label>
            <input type="number" id="price" required step="0.01" min="0">
        </div>
        <div class="form-group">
            <label>Status:</label>
            <select id="status" required>
                <option value="Available">Available</option>
                <option value="Issued">Issued</option>
            </select>
        </div>
        <button type="submit" style="padding: 8px 15px; cursor:pointer;">Submit</button>
    </form>
    <p id="add-msg" style="color: green; font-weight: bold;"></p>
</div>

<!-- ================= VIEW GROCERY CATALOG SECTION ================= -->
<div id="view-section" class="section">
    <h2>Grocery Catalog</h2>
    
    <div class="controls">
        <label>Search:</label>
        <input type="text" id="searchBox" placeholder="By ID, Name or Category" onkeyup="loadCatalog()">
        <button onclick="loadCatalog()" style="margin-left:10px; cursor:pointer;">Refresh Catalog</button>
        <span style="float: right; font-size: 16px;">Total Available Items: <strong id="total-count">0</strong></span>
    </div>

    <div id="catalog-container"></div>
</div>

<script>
// Switch between sections
function showSection(id) {
    document.getElementById('add-section').classList.remove('active');
    document.getElementById('view-section').classList.remove('active');
    document.getElementById(id).classList.add('active');
    
    // Clear messages
    document.getElementById('add-msg').innerText = '';
}

// Add Item using AJAX
function addItem(e) {
    e.preventDefault();
    
    var formData = new FormData();
    formData.append('action', 'add');
    formData.append('item_id', document.getElementById('item_id').value);
    formData.append('name', document.getElementById('name').value);
    formData.append('category', document.getElementById('category').value);
    formData.append('quantity', document.getElementById('quantity').value);
    formData.append('price', document.getElementById('price').value);
    formData.append('status', document.getElementById('status').value);

    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            document.getElementById('add-msg').innerText = this.responseText;
            
            if (this.responseText.includes('Successfully')) {
                document.getElementById('add-msg').style.color = 'green';
                document.getElementById('addForm').reset();
            } else {
                document.getElementById('add-msg').style.color = 'red';
            }
        }
    };
    xhttp.open("POST", "index.php", true);
    xhttp.send(formData);
}

// Load Catalog using AJAX
function loadCatalog() {
    var search = document.getElementById('searchBox').value;
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            var response = JSON.parse(this.responseText);
            document.getElementById('total-count').innerText = response.count;
            
            var html = '';
            if (response.count == 0) {
                html = '<p style="color:red; margin-top: 20px;">No grocery items available. Please add items first.</p>';
            } else {
                html += '<table>';
                html += '<tr><th>Item ID</th><th>Name</th><th>Category</th><th>Quantity</th><th>Price</th><th>Status</th><th>Action</th></tr>';
                for (var i = 0; i < response.items.length; i++) {
                    var item = response.items[i];
                    html += '<tr>';
                    html += '<td>' + item.item_id + '</td>';
                    html += '<td>' + item.name + '</td>';
                    html += '<td>' + item.category + '</td>';
                    html += '<td>' + item.quantity + '</td>';
                    html += '<td>$' + parseFloat(item.price).toFixed(2) + '</td>';
                    html += '<td>' + item.status + '</td>';
                    html += '<td><button style="cursor:pointer;" onclick="deleteItem(\'' + item.item_id + '\')">Delete</button></td>';
                    html += '</tr>';
                }
                html += '</table>';
            }
            document.getElementById('catalog-container').innerHTML = html;
        }
    };
    // Proper URL encoding for the search term
    xhttp.open("GET", "index.php?action=fetch&search=" + encodeURIComponent(search), true);
    xhttp.send();
}

// Delete Item using AJAX
function deleteItem(item_id) {
    if (!confirm('Are you sure you want to delete item ' + item_id + '?')) {
        return;
    }
    
    var formData = new FormData();
    formData.append('action', 'delete');
    formData.append('item_id', item_id);

    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            // Reload catalog automatically after delete
            loadCatalog();
        }
    };
    xhttp.open("POST", "index.php", true);
    xhttp.send(formData);
}
</script>

</body>
</html>
