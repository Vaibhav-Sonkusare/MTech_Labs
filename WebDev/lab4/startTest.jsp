<%@ page session="true" %>

    <% /* ------------------ LOGIN CHECK ------------------ */ if(session.getAttribute("user")==null){
        response.sendRedirect("login.jsp"); return; } /* ------------------ INITIALIZE TESTS (ONCE) ------------------
        */ if(application.getAttribute("tests")==null){ java.util.HashMap<String, String[][]> tests = new
        java.util.HashMap<>();

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


                    /* ------------------ CHECK FOR RESUME ------------------ */
                    String username = (String) session.getAttribute("user");
                    java.util.Map<String, java.util.Map<String, Object>> progressMap =
                        (java.util.Map<String, java.util.Map<String, Object>>)
                            application.getAttribute("user_progress_map");

                            if(progressMap == null){
                            progressMap = new java.util.HashMap<>();
                                application.setAttribute("user_progress_map", progressMap);
                                }

                                java.util.Map<String, Object> userProgress = progressMap.get(username);
                                    boolean resuming = false;

                                    if(userProgress != null){
                                    String storedTestId = (String) userProgress.get("testId");
                                    if(storedTestId != null && storedTestId.equals(testId)){
                                    resuming = true;

                                    // Load from persistence
                                    session.setAttribute("testId", testId);
                                    session.setAttribute("questions", questions);

                                    Integer currentQ = (Integer) userProgress.get("currentQuestion");
                                    session.setAttribute("currentQuestion", currentQ != null ? currentQ : 0);

                                    java.util.Map<Integer, String> storedAnswers =
                                        (java.util.Map<Integer, String>) userProgress.get("answers");
                                            session.setAttribute("answers", storedAnswers != null ? storedAnswers : new
                                            java.util.HashMap<Integer,String>());

                                                Integer remTime = (Integer) userProgress.get("remainingTime");
                                                session.setAttribute("remainingTime", remTime != null ? remTime : 15);

                                                java.util.Set<Integer> marked = (java.util.Set<Integer>)
                                                        userProgress.get("markedQuestions");
                                                        session.setAttribute("markedQuestions", marked != null ? marked
                                                        : new java.util.HashSet<Integer>());
                                                            }
                                                            }

                                                            if(!resuming){
                                                            /* ------------------ INITIALIZE NEW SESSION
                                                            ------------------ */
                                                            session.setAttribute("testId", testId);
                                                            session.setAttribute("questions", questions);
                                                            session.setAttribute("currentQuestion", 0);
                                                            session.setAttribute("answers", new java.util.HashMap
                                                            <Integer,String>());
                                                                session.setAttribute("remainingTime", 15);
                                                                session.setAttribute("markedQuestions", new
                                                                java.util.HashSet<Integer>());

                                                                    /* ------------------ CREATE PERSISTENT ENTRY
                                                                    ------------------ */
                                                                    java.util.Map<String, Object> newProgress = new
                                                                        java.util.HashMap<>();
                                                                            newProgress.put("testId", testId);
                                                                            newProgress.put("currentQuestion", 0);
                                                                            newProgress.put("answers", new
                                                                            java.util.HashMap<Integer, String>());
                                                                                newProgress.put("remainingTime", 15);
                                                                                newProgress.put("markedQuestions", new
                                                                                java.util.HashSet<Integer>());

                                                                                    progressMap.put(username,
                                                                                    newProgress);
                                                                                    }


                                                                                    /* ------------------ CREATE RESUME
                                                                                    COOKIE
                                                                                    ------------------ */
                                                                                    /* (simple version for now, full
                                                                                    resume later) */
                                                                                    Cookie resume = new
                                                                                    Cookie("resume_test", testId +
                                                                                    "|0");
                                                                                    resume.setMaxAge(60*60);
                                                                                    response.addCookie(resume);


                                                                                    /* ------------------ START TEST
                                                                                    ------------------ */
                                                                                    response.sendRedirect("question.jsp");
                                                                                    %>