<%@ page session="true" %>
    <% /* ------------------ SESSION CHECK ------------------ */ if(session.getAttribute("user")==null){
        response.sendRedirect("login.jsp"); return; } /* ------------------ LOAD DATA ------------------ */ String[][]
        questions=(String[][]) session.getAttribute("questions"); if(questions==null){
        response.sendRedirect("dashboard.jsp"); return; } Integer index=(Integer)
        session.getAttribute("currentQuestion"); java.util.HashMap<Integer,String> answers =
        (java.util.HashMap<Integer,String>) session.getAttribute("answers");

            if(index == null || answers == null){
            response.sendRedirect("dashboard.jsp");
            return;
            }

            /* ----------- PER QUESTION TIMER VALUE ----------- */
            Integer remainingObj = (Integer) session.getAttribute("remainingTime");
            int remaining = (remainingObj != null) ? remainingObj : 15;
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
                        Time Left: <span id="timer">
                            <%=remaining%>
                        </span>
                    </div>

                    <!-- QUESTION FORM -->
                    <form action="saveAnswer.jsp" method="post" id="testForm">

                        <h3>Question <%=index + 1%>
                        </h3>

                        <p>
                            <%=questions[index][0]%>
                        </p>

                        <% String saved=answers.get(index); for(int i=1; i<=4; i++){ char optionLetter=(char)('A' + i -
                            1); %>

                            <input type="radio" name="answer" value="<%=optionLetter%>" <%=(saved !=null &&
                                saved.equals(String.valueOf(optionLetter))) ? "checked" : "" %> >

                            <%=questions[index][i]%>
                                <br>

                                <% } %>

                                    <br>

                                    <% java.util.Set<Integer> marked = (java.util.Set<Integer>)
                                            session.getAttribute("markedQuestions");
                                            boolean isMarked = (marked != null && marked.contains(index));
                                            %>

                                            <label>
                                                <input type="checkbox" name="markForReview" <%=isMarked ? "checked" : ""
                                                    %>> Mark for Review
                                            </label>

                                            <br><br>
                                            <input type="submit" value="Save & Next">

                    </form>

                    <!-- MARKED QUESTIONS LIST -->
                    <% if(marked !=null && !marked.isEmpty()){ %>
                        <div class="marked-list" style="margin-top: 20px; padding: 10px; border-top: 1px solid #ccc;">
                            <h4>Marked for Review:</h4>
                            <% java.util.List<Integer> sortedMarked = new java.util.ArrayList<>(marked);
                                    java.util.Collections.sort(sortedMarked);
                                    for(Integer m : sortedMarked){
                                    %>
                                    <a href="jumpToQuestion.jsp?index=<%=m%>" style="margin-right: 10px;"
                                        onclick="return confirm('Jump to selected question? Your current selection on this page will NOT be saved.')">Q
                                        <%=m+1%></a>
                                    <% } %>
                        </div>
                        <% } %>

                </div>

                <script>
                    let time = <%=remaining%>;

                    setInterval(function () {
                        time--;
                        document.getElementById("timer").innerHTML = time;

                        // Sync timer to server
                        fetch('updateTimer.jsp?time=' + time);

                        // timer expiry -> auto next question
                        if (time <= 0) {
                            document.getElementById("testForm").submit();
                        }
                    }, 1000);
                </script>

            </body>

            </html>