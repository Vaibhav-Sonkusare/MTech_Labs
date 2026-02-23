<%@ page session="true" %>
<%
String fullname = request.getParameter("fullname");
String username = request.getParameter("username");
String password = request.getParameter("password");


java.util.HashMap<String,String> users =
(java.util.HashMap<String,String>)application.getAttribute("users");


if(users == null){
users = new java.util.HashMap<>();
}


users.put(username, password);
application.setAttribute("users", users);


response.sendRedirect("login.jsp");
%>