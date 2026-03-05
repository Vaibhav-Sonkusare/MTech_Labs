<?php

$db = new SQLite3('students.db');

$db->exec("CREATE TABLE IF NOT EXISTS students(
id INTEGER PRIMARY KEY AUTOINCREMENT,
name TEXT,
department TEXT
)");

if(isset($_GET['action'])){

$action=$_GET['action'];

if($action=="insert"){

$name=$_GET['name'];
$dept=$_GET['dept'];

$db->exec("INSERT INTO students(name,department) VALUES('$name','$dept')");

echo "Student Added";

}

if($action=="fetch"){

$result=$db->query("SELECT * FROM students");

echo "<table border=1 cellpadding=5>";
echo "<tr><th>ID</th><th>Name</th><th>Department</th></tr>";

while($row=$result->fetchArray()){

echo "<tr>";
echo "<td>".$row['id']."</td>";
echo "<td>".$row['name']."</td>";
echo "<td>".$row['department']."</td>";
echo "</tr>";

}

echo "</table>";

}

if($action=="search"){

$key=$_GET['key'];

$result=$db->query("SELECT * FROM students 
WHERE id LIKE '%$key%' 
OR name LIKE '%$key%' 
OR department LIKE '%$key%'");

while($row=$result->fetchArray()){

echo $row['id']." - ".$row['name']." - ".$row['department']."<br>";

}

}

if($action=="delete"){

$id=$_GET['id'];

$db->exec("DELETE FROM students WHERE id=$id");

echo "Student Deleted";

}

exit;

}

?>

<!DOCTYPE html>
<html>
<head>
<title>AJAX SQLite Student System</title>
</head>

<body>

<h1>AJAX SQLite Student System</h1>

<h2>Add Student</h2>

Name:
<input type="text" id="name">

Department:
<input type="text" id="dept">

<button onclick="addStudent()">Add</button>

<p id="status"></p>

<hr>

<h2>Student List</h2>

<button onclick="loadStudents()">Load Students</button>

<div id="students"></div>

<hr>

<h2>Search Student</h2>

<input type="text" id="search" onkeyup="searchStudent()" placeholder="Search by ID, Name or Department">

<div id="result"></div>

<hr>

<h2>Delete Student</h2>

Enter ID:
<input type="text" id="sid">

<button onclick="deleteStudent()">Delete</button>

<p id="deleteMsg"></p>

<script>

function addStudent(){

var name=document.getElementById("name").value;
var dept=document.getElementById("dept").value;

var xhttp=new XMLHttpRequest();

xhttp.onreadystatechange=function(){

if(this.readyState==4 && this.status==200){

document.getElementById("status").innerHTML=this.responseText;

document.getElementById("name").value="";
document.getElementById("dept").value="";

loadStudents();

}

};

xhttp.open("GET","index.php?action=insert&name="+name+"&dept="+dept,true);
xhttp.send();

}

function loadStudents(){

var xhttp=new XMLHttpRequest();

xhttp.onreadystatechange=function(){

if(this.readyState==4 && this.status==200){
document.getElementById("students").innerHTML=this.responseText;
}

};

xhttp.open("GET","index.php?action=fetch",true);
xhttp.send();

}

function searchStudent(){

var key=document.getElementById("search").value;

if(key==""){
document.getElementById("result").innerHTML="";
return;
}

var xhttp=new XMLHttpRequest();

xhttp.onreadystatechange=function(){

if(this.readyState==4 && this.status==200){
document.getElementById("result").innerHTML=this.responseText;
}

};

xhttp.open("GET","index.php?action=search&key="+key,true);
xhttp.send();

}

function deleteStudent(){

var id=document.getElementById("sid").value;

var xhttp=new XMLHttpRequest();

xhttp.onreadystatechange=function(){

if(this.readyState==4 && this.status==200){

document.getElementById("deleteMsg").innerHTML=this.responseText;

document.getElementById("sid").value="";

loadStudents();

}

};

xhttp.open("GET","index.php?action=delete&id="+id,true);
xhttp.send();

}

</script>

</body>
</html>