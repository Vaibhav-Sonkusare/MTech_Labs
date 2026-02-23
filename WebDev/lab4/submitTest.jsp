<%@ page session="true" %>
    <% String[][] questions=(String[][])session.getAttribute("questions"); java.util.HashMap<Integer,String> answers =
        (java.util.HashMap<Integer,String>)session.getAttribute("answers");

            if(questions == null || answers == null){
            response.sendRedirect("dashboard.jsp");
            return;
            }

            int score = 0;
            for(int i=0; i<questions.length; i++){ String correct=questions[i][5]; String given=answers.get(i);
                if(correct !=null && correct.equals(given)) { score++; } } session.setAttribute("score", score); Cookie
                resume=new Cookie("resume_test", "" ); resume.setMaxAge(0); response.addCookie(resume); /* Clear
                persistence */ String username=(String) session.getAttribute("user"); java.util.Map<String,
                java.util.Map<String, Object>> progressMap =
                (java.util.Map<String, java.util.Map<String, Object>>)
                    application.getAttribute("user_progress_map");

                    if(progressMap != null && username != null){
                    progressMap.remove(username);
                    }

                    // Clear session attributes
                    session.removeAttribute("testId");
                    session.removeAttribute("questions");
                    session.removeAttribute("currentQuestion");
                    session.removeAttribute("answers");
                    session.removeAttribute("markedQuestions");
                    session.removeAttribute("remainingTime");

                    response.sendRedirect("result.jsp");
                    %>