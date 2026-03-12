<?php
header("Content-Type: application/json");
$method = $_SERVER['REQUEST_METHOD'];

$path = isset($_SERVER['PATH_INFO']) ? $_SERVER['PATH_INFO'] : '/';
if ($path === '/' || $path === '') {
    if (isset($_GET['endpoint'])) {
        $path = '/' . trim($_GET['endpoint'], '/');
    }
}

$xmlFile = 'data.xml';
$dom = new DOMDocument();
if (file_exists($xmlFile)) {
    $dom->load($xmlFile);
} else {
    echo json_encode(["error" => "Data file not found"]);
    exit;
}

if ($method == 'GET') {
    if ($path === '/courses' || $path === '/courses/') {
        $coursesList = [];
        $courses = $dom->getElementsByTagName('Course');
        foreach ($courses as $c) {
            $coursesList[] = [
                'id' => $c->getAttribute('id'),
                'title' => $c->getElementsByTagName('Title')->item(0)->nodeValue,
                'credits' => $c->getElementsByTagName('Credits')->item(0)->nodeValue
            ];
        }
        echo json_encode($coursesList);
    } elseif (preg_match('/^\/results\/([0-9a-zA-Z]+)$/', $path, $matches)) {
        $studentId = $matches[1];
        
        $xpath = new DOMXPath($dom);
        $enrolments = $xpath->query("//Enrolment[StudentId='$studentId']");
        
        $results = [];
        foreach ($enrolments as $e) {
            $courseId = $e->getElementsByTagName('CourseId')->item(0)->nodeValue;
            $grade = $e->getElementsByTagName('Grade')->item(0)->nodeValue;
            
            $courseQuery = $xpath->query("//Course[@id='$courseId']");
            $title = "Unknown";
            if ($courseQuery->length > 0) {
                $title = $courseQuery->item(0)->getElementsByTagName('Title')->item(0)->nodeValue;
            }
            
            $results[] = [
                'courseId' => $courseId,
                'title' => $title,
                'grade' => $grade
            ];
        }
        echo json_encode($results);
    } else {
        http_response_code(404);
        echo json_encode(["error" => "Endpoint not found: $path"]);
    }
} else {
    http_response_code(405);
    echo json_encode(["error" => "Method not allowed"]);
}
?>
