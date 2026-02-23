<%@ page session="true" %>
<%
String savedUser = "";
Cookie[] cookies = request.getCookies();
if(cookies != null){
for(Cookie c : cookies){
if(c.getName().equals("remember_user")){
savedUser = c.getValue();
}
}
}
%>


<html>
<head>
<link rel="stylesheet" href="style.css">
</head>
<body>
<div class="container">
<h2>Login</h2>
<form action="loginProcess.jsp" method="post">
Username:
<input type="text" name="username" value="<%=savedUser%>" required>


Password:
<input type="password" name="password" required>


Remember Me <input type="checkbox" name="remember" value="yes"><br><br>


<input type="submit" value="Login">
</form>


<br>
<a href="signup.jsp">Signup</a> |
<a href="forgetPassword.jsp">Forget Password</a>
</div>
</body>
</html>