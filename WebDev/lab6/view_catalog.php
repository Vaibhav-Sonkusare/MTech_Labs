<?php
$xmlFile = 'library.xml';
$fallbackXml = 'sample_library.xml';
$xslFile = 'library.xsl';

$xmlToLoad = "";

if (file_exists($xmlFile)) {
    $xmlToLoad = $xmlFile;
} elseif (file_exists($fallbackXml)) {
    $xmlToLoad = $fallbackXml;
}

?>
<!DOCTYPE html>
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
    <title>View Library Catalog</title>
</head>
<body>
    <h1>Library Catalog (Transformed)</h1>
    <a href="index.php">Back to Home</a>
    <br/><br/>

    <?php
    if ($xmlToLoad) {
        if (!file_exists($xslFile)) {
            echo "<p>XSL file missing.</p>";
        } else {
            $xml = new DOMDocument;
            $xml->load($xmlToLoad);

            $xsl = new DOMDocument;
            $xsl->load($xslFile);

            $proc = new XSLTProcessor;
            $proc->importStyleSheet($xsl);

            $htmlContent = $proc->transformToXML($xml);
            
            // Save as catalog.html
            file_put_contents('catalog.html', $htmlContent);

            echo "<h2>Transformed XHTML</h2>";
            echo "<div>" . $htmlContent . "</div>";
            
            echo "<h3>Links and Preview</h3>";
            echo "<p><a href='catalog.html' download>Download generated catalog.html</a></p>";
            
            $rawXml = htmlspecialchars(file_get_contents($xmlToLoad));
            echo "<h3>Raw XML Preview (" . htmlspecialchars($xmlToLoad) . ")</h3>";
            echo "<pre style='background:#f9f9f9; padding:10px; border:1px solid #ccc; max-width: 800px; overflow: auto;'>" . $rawXml . "</pre>";
        }
    } else {
        echo "<p>No books added yet. Add some books first.</p>";
    }
    ?>
</body>
</html>
