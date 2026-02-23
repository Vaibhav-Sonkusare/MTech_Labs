<%@ page session="true" %>
    <% /* ------------------ SESSION CHECK ------------------ */ if(session.getAttribute("user")==null){
        response.sendRedirect("login.jsp"); return; } String indexStr=request.getParameter("index"); if(indexStr==null){
        response.sendRedirect("question.jsp"); return; } try{ int index=Integer.parseInt(indexStr); String[][]
        questions=(String[][]) session.getAttribute("questions"); if(questions !=null && index>= 0 && index <
            questions.length){ session.setAttribute("currentQuestion", index); /* Also update persistence for current
            position */ String username=(String) session.getAttribute("user"); java.util.Map<String,
            java.util.Map<String, Object>> progressMap =
            (java.util.Map<String, java.util.Map<String, Object>>)
                application.getAttribute("user_progress_map");

                if(progressMap != null && username != null && progressMap.containsKey(username)){
                progressMap.get(username).put("currentQuestion", index);
                progressMap.get(username).put("remainingTime", 15);
                }

                session.setAttribute("remainingTime", 15);
                }
                }catch(Exception e){}

                response.sendRedirect("question.jsp");
                %>