<?php
$xml = new DOMDocument;
if (!file_exists('data.xml')) {
    die("XML data file not found.");
}
$xml->load('data.xml');

$xsl = new DOMDocument;
if (!file_exists('report.xsl')) {
    die("XSL template not found.");
}
$xsl->load('report.xsl');

$proc = new XSLTProcessor;
$proc->importStyleSheet($xsl);

echo $proc->transformToXML($xml);
?>
