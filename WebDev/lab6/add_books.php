<?php
session_start();

if (!isset($_SESSION['books'])) {
    $_SESSION['books'] = [];
}

$message = "";

if ($_SERVER["REQUEST_METHOD"] == "POST") {
    if (isset($_POST['add_book'])) {
        $book = [
            'id' => $_POST['book_id'],
            'title' => $_POST['title'],
            'author' => $_POST['author'],
            'year' => $_POST['year'],
            'isbn' => $_POST['isbn'],
            'status' => $_POST['status']
        ];
        $_SESSION['books'][] = $book;
        $message = "Book added to session. Total books in session: " . count($_SESSION['books']);
    } elseif (isset($_POST['save_xml'])) {
        if (count($_SESSION['books']) > 0) {
            $dom = new DOMDocument('1.0', 'UTF-8');
            $dom->formatOutput = true;
            
            $xslt = $dom->createProcessingInstruction('xml-stylesheet', 'type="text/xsl" href="library.xsl"');
            $dom->appendChild($xslt);

            $library = $dom->createElement('library');
            $dom->appendChild($library);

            foreach ($_SESSION['books'] as $b) {
                $bookNode = $dom->createElement('book');
                $bookNode->setAttribute('id', htmlspecialchars($b['id']));
                
                $titleNode = $dom->createElement('title', htmlspecialchars($b['title']));
                $bookNode->appendChild($titleNode);
                
                $authorNode = $dom->createElement('author', htmlspecialchars($b['author']));
                $bookNode->appendChild($authorNode);
                
                $yearNode = $dom->createElement('year', htmlspecialchars($b['year']));
                $bookNode->appendChild($yearNode);
                
                $isbnNode = $dom->createElement('isbn', htmlspecialchars($b['isbn']));
                $bookNode->appendChild($isbnNode);
                
                $statusNode = $dom->createElement('status', htmlspecialchars($b['status']));
                $bookNode->appendChild($statusNode);
                
                $library->appendChild($bookNode);
            }

            $dom->save('library.xml');
            $message = "XML Generated Successfully. <a href='view_catalog.php'>View Transformed Catalog</a>";
            $_SESSION['books'] = []; // Clear session after saving
        } else {
            $message = "No books in session to save.";
        }
    }
}
?>
<!DOCTYPE html>
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
    <title>Add Library Books</title>
</head>
<body>
    <h1>Add Library Books</h1>
    <a href="index.php">Back to Home</a>
    <br/><br/>
    
    <?php if ($message) echo "<p><strong>$message</strong></p>"; ?>

    <form method="post" action="">
        <label>Book ID:</label><br/>
        <input type="text" name="book_id" required="required" placeholder="e.g., LIB001" /><br/>
        
        <label>Title:</label><br/>
        <input type="text" name="title" required="required" /><br/>
        
        <label>Author:</label><br/>
        <input type="text" name="author" required="required" /><br/>
        
        <label>Year:</label><br/>
        <input type="number" name="year" required="required" /><br/>
        
        <label>ISBN:</label><br/>
        <input type="text" name="isbn" required="required" /><br/>
        
        <label>Status:</label><br/>
        <select name="status">
            <option value="Available">Available</option>
            <option value="Issued">Issued</option>
        </select><br/><br/>
        
        <input type="submit" name="add_book" value="Add Book" />
        <input type="submit" name="save_xml" value="Save &amp; Generate XML" />
    </form>
    
    <h2>Books Currently in Session</h2>
    <?php if (count($_SESSION['books']) > 0): ?>
        <table border="1">
            <tr>
                <th>ID</th>
                <th>Title</th>
                <th>Author</th>
                <th>Year</th>
                <th>ISBN</th>
                <th>Status</th>
            </tr>
            <?php foreach ($_SESSION['books'] as $b): ?>
            <tr>
                <td><?php echo htmlspecialchars($b['id']); ?></td>
                <td><?php echo htmlspecialchars($b['title']); ?></td>
                <td><?php echo htmlspecialchars($b['author']); ?></td>
                <td><?php echo htmlspecialchars($b['year']); ?></td>
                <td><?php echo htmlspecialchars($b['isbn']); ?></td>
                <td><?php echo htmlspecialchars($b['status']); ?></td>
            </tr>
            <?php endforeach; ?>
        </table>
    <?php else: ?>
        <p>No books added in this session yet.</p>
    <?php endif; ?>
</body>
</html>
