<%@ page session="true" %>
    <% String ans=request.getParameter("answer"); Integer index=(Integer)session.getAttribute("currentQuestion");
        java.util.HashMap<Integer,String> answers =
        (java.util.HashMap<Integer,String>)session.getAttribute("answers");

            if(index == null || answers == null){
            response.sendRedirect("dashboard.jsp");
            return;
            }

            if(ans != null){
            answers.put(index, ans);
            }

            /* ------------------ HANDLE MARK FOR REVIEW ------------------ */
            String mark = request.getParameter("markForReview");
            java.util.Set<Integer> marked = (java.util.Set<Integer>) session.getAttribute("markedQuestions");
                    if(marked == null) {
                    marked = new java.util.HashSet<>();
                        session.setAttribute("markedQuestions", marked);
                        }

                        if("on".equals(mark)){
                        marked.add(index);
                        } else {
                        marked.remove(index);
                        }

                        // move to next question
                        index++;

                        String[][] questions = (String[][])session.getAttribute("questions");

                        if(questions == null){
                        response.sendRedirect("dashboard.jsp");
                        return;
                        }

                        // if last question reached -> end test
                        if(index >= questions.length){
                        response.sendRedirect("submitTest.jsp");
                        return;
                        }

                        // Update persistence
                        String username = (String) session.getAttribute("user");
                        java.util.Map<String, java.util.Map<String, Object>> progressMap =
                            (java.util.Map<String, java.util.Map<String, Object>>)
                                application.getAttribute("user_progress_map");

                                if(progressMap != null && progressMap.containsKey(username)){
                                java.util.Map<String, Object> userProgress = progressMap.get(username);
                                    userProgress.put("currentQuestion", index);
                                    userProgress.put("remainingTime", 15); // Reset to 15 for new question
                                    userProgress.put("markedQuestions", marked);
                                    }

                                    session.setAttribute("currentQuestion", index);
                                    session.setAttribute("remainingTime", 15); // Reset session timer

                                    // update resume cookie
                                    Cookie resume = new Cookie("resume_test", String.valueOf(index));
                                    resume.setMaxAge(60*60);
                                    response.addCookie(resume);

                                    response.sendRedirect("question.jsp");
                                    %>