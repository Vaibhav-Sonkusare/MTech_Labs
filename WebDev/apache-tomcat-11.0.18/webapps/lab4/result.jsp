<%@ page session="true" %>
<html>
<head>
<link rel="stylesheet" href="style.css">
</head>
<body>
<div class="container">
<h2>Result</h2>
Score: <%=session.getAttribute("score")%>
<br><br>
<a href="dashboard.jsp">Back to Dashboard</a>
</div>
</body>
</html>