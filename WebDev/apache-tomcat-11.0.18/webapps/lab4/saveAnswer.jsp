<%@ page session="true" %>

<%
String ans = request.getParameter("answer");
Integer index = (Integer)session.getAttribute("currentQuestion");

java.util.HashMap<Integer,String> answers =
(java.util.HashMap<Integer,String>)session.getAttribute("answers");

if(ans != null){
    answers.put(index, ans);
}

// move to next question
index++;

String[][] questions =
(String[][])application.getAttribute("questions");

// if last question reached → end test
if(index >= questions.length){
    response.sendRedirect("submitTest.jsp");
    return;
}

session.setAttribute("currentQuestion", index);

// update resume cookie
Cookie resume = new Cookie("resume_test", String.valueOf(index));
resume.setMaxAge(60*60);
response.addCookie(resume);

response.sendRedirect("question.jsp");
%>
