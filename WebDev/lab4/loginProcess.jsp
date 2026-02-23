<%@ page session="true" %>
<%
String username = request.getParameter("username");
String password = request.getParameter("password");
String remember = request.getParameter("remember");


java.util.HashMap<String,String> users =
(java.util.HashMap<String,String>)application.getAttribute("users");


if(users != null && password.equals(users.get(username))){


session.setAttribute("user", username);


if("yes".equals(remember)){
Cookie c = new Cookie("remember_user", username);
c.setMaxAge(7*24*60*60);
response.addCookie(c);
}


response.sendRedirect("dashboard.jsp");
}else{
out.println("Invalid Login");
}
%>