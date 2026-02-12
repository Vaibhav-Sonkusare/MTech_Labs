<%@ page session="true" %>

<%
/* ------------------ SESSION CHECK ------------------ */
if(session.getAttribute("user") == null){
    response.sendRedirect("login.jsp");
    return;
}

/* ------------------ LOAD DATA ------------------ */
String[][] questions =
    (String[][]) application.getAttribute("questions");

Integer index =
    (Integer) session.getAttribute("currentQuestion");

java.util.HashMap<Integer,String> answers =
    (java.util.HashMap<Integer,String>)
    session.getAttribute("answers");

/* ----------- PER QUESTION TIMER VALUE ----------- */
/* change this value as needed */
int remaining = 15;   // 30 seconds per question
%>

<html>
<head>
    <title>Test</title>
    <link rel="stylesheet" href="style.css">
</head>

<body>

<div class="container">

    <!-- TIMER -->
    <div class="timer">
        Time Left: <span id="timer"><%=remaining%></span>
    </div>

    <!-- QUESTION FORM -->
    <form action="saveAnswer.jsp" method="post" id="testForm">

        <h3>Question <%=index + 1%></h3>

        <p><%=questions[index][0]%></p>

        <%
        String saved = answers.get(index);

        for(int i=1;i<=4;i++){
            char optionLetter = (char)('A'+i-1);
        %>

        <input type="radio"
               name="answer"
               value="<%=optionLetter%>"
               <%= (saved != null &&
                    saved.equals(String.valueOf(optionLetter)))
                    ? "checked" : "" %> >

        <%=questions[index][i]%>
        <br>

        <% } %>

        <br>
        <input type="submit" value="Save & Next">

    </form>

</div>

<script>

let time = <%=remaining%>;

setInterval(function(){

    time--;
    document.getElementById("timer").innerHTML = time;

    // timer expiry → auto next question
    if(time <= 0){
        document.getElementById("testForm").submit();
    }

},1000);

</script>

</body>
</html>
