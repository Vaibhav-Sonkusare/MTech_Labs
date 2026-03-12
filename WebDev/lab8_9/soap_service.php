<?php
class StudentService {
    private $xmlFile = 'data.xml';

    private function getDOM() {
        $dom = new DOMDocument();
        $dom->preserveWhiteSpace = false;
        $dom->formatOutput = true;
        if (file_exists($this->xmlFile)) {
            $dom->load($this->xmlFile);
        } else {
            $dom->loadXML('<?xml version="1.0" encoding="UTF-8"?><University><Students></Students><Courses></Courses><Enrolments></Enrolments></University>');
        }
        return $dom;
    }

    private function saveDOM($dom) {
        $dom->save($this->xmlFile);
    }

    public function addStudent($id, $name, $email) {
        $dom = $this->getDOM();
        $students = $dom->getElementsByTagName('Students')->item(0);
        
        // Check if student exists
        $xpath = new DOMXPath($dom);
        $existing = $xpath->query("//Student[@id='$id']");
        if ($existing->length > 0) {
            return "Student with ID $id already exists.";
        }

        $student = $dom->createElement('Student');
        $student->setAttribute('id', $id);
        
        $nameNode = $dom->createElement('Name', htmlspecialchars($name));
        $emailNode = $dom->createElement('Email', htmlspecialchars($email));
        
        $student->appendChild($nameNode);
        $student->appendChild($emailNode);
        
        $students->appendChild($student);
        $this->saveDOM($dom);
        return "Student added successfully.";
    }

    public function updateStudent($id, $name, $email) {
        $dom = $this->getDOM();
        $xpath = new DOMXPath($dom);
        $students = $xpath->query("//Student[@id='$id']");
        
        if ($students->length > 0) {
            $student = $students->item(0);
            $student->getElementsByTagName('Name')->item(0)->nodeValue = htmlspecialchars($name);
            $student->getElementsByTagName('Email')->item(0)->nodeValue = htmlspecialchars($email);
            $this->saveDOM($dom);
            return "Student updated successfully.";
        }
        return "Student not found.";
    }

    public function deleteStudent($id) {
        $dom = $this->getDOM();
        $xpath = new DOMXPath($dom);
        $students = $xpath->query("//Student[@id='$id']");
        
        if ($students->length > 0) {
            $student = $students->item(0);
            $student->parentNode->removeChild($student);
            $this->saveDOM($dom);
            return "Student deleted successfully.";
        }
        return "Student not found.";
    }
}

$options = array('uri' => 'http://localhost/student_service');
$server = new SoapServer(null, $options);
$server->setClass('StudentService');
$server->handle();
?>
