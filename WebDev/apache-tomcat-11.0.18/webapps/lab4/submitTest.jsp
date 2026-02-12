<%@ page session="true" %>
<%
String[][] questions = (String[][])application.getAttribute("questions");
java.util.HashMap<Integer,String> answers =
(java.util.HashMap<Integer,String>)session.getAttribute("answers");


int score = 0;
for(int i=0;i<questions.length;i++){
String correct = questions[i][5];
String given = answers.get(i);
if(correct.equals(given)) score++;
}


session.setAttribute("score", score);


Cookie resume = new Cookie("resume_test", "");
resume.setMaxAge(0);
response.addCookie(resume);


response.sendRedirect("result.jsp");
%>