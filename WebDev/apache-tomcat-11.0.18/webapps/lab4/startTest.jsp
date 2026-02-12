<%@ page session="true" %>

<%
/* ------------------ LOGIN CHECK ------------------ */
if(session.getAttribute("user") == null){
    response.sendRedirect("login.jsp");
    return;
}


/* ------------------ INITIALIZE TESTS (ONCE) ------------------ */
if(application.getAttribute("tests") == null){

    java.util.HashMap<String, String[][]> tests =
            new java.util.HashMap<>();

    /* -------- TEST 1 -------- */
    String[][] test1 = {
        {"Capital of India?","Delhi","Mumbai","Chennai","Kolkata","A"},
        {"2 + 2 = ?","3","4","5","6","B"},
        {"Sun rises in?","East","West","North","South","A"}
    };

    /* -------- TEST 2 -------- */
    String[][] test2 = {
        {"Linux creator?","Linus Torvalds","Bill Gates","Steve Jobs","Mark Zuckerberg","A"},
        {"HTTP default port?","21","22","80","443","C"},
        {"Which is a Linux distro?","Windows","Ubuntu","MacOS","DOS","B"}
    };

    tests.put("test1", test1);
    tests.put("test2", test2);

    application.setAttribute("tests", tests);
}


/* ------------------ GET SELECTED TEST ------------------ */
String testId = request.getParameter("testId");

if(testId == null){
    response.sendRedirect("dashboard.jsp");
    return;
}

java.util.HashMap<String, String[][]> tests =
    (java.util.HashMap<String, String[][]>)
    application.getAttribute("tests");

String[][] questions = tests.get(testId);

if(questions == null){
    response.sendRedirect("dashboard.jsp");
    return;
}


/* ------------------ INITIALIZE TEST SESSION ------------------ */
session.setAttribute("testId", testId);
session.setAttribute("questions", questions);
session.setAttribute("currentQuestion", 0);
session.setAttribute("answers",
        new java.util.HashMap<Integer,String>());


/* ------------------ CREATE RESUME COOKIE ------------------ */
/* (simple version for now, full resume later) */
Cookie resume = new Cookie("resume_test", testId + "|0");
resume.setMaxAge(60*60);
response.addCookie(resume);


/* ------------------ START TEST ------------------ */
response.sendRedirect("question.jsp");
%>
