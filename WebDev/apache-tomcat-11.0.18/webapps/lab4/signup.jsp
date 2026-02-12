<%@ page session="true" %>
<html>
<head>
<link rel="stylesheet" href="style.css">
</head>
<body>
<div class="container">
<h2>Signup</h2>
<form action="signupProcess.jsp" method="post">
Full Name:
<input type="text" name="fullname" required>


Username:
<input type="text" name="username" required>


Password:
<input type="password" name="password" required>


<input type="submit" value="Signup">
</form>
<br>
<a href="login.jsp">Already have account? Login</a>
</div>
</body>
</html>