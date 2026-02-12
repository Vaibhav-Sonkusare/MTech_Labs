<%@ page session="true" %>
<%
if(session.getAttribute("user") == null){
response.sendRedirect("login.jsp");
return;
}
%>
<html>
<head>
<link rel="stylesheet" href="style.css">
</head>
<body>
<div class="container">
<h2>Dashboard</h2>
Welcome <%=session.getAttribute("user")%>
<br><br>
<a href="startTest.jsp?testId=test1">General Knowledge Test</a><br>
<a href="startTest.jsp?testId=test2">Computer Science Test</a>
<br><br>
<a href="logout.jsp">Logout</a>
</div>
</body>
</html>