<%@ page session="true" %>
    <% String timeStr=request.getParameter("time"); if(timeStr !=null && session.getAttribute("user") !=null){ try{ int
        time=Integer.parseInt(timeStr); session.setAttribute("remainingTime", time); // Sync to persistent map String
        username=(String)session.getAttribute("user"); java.util.Map<String, java.util.Map<String, Object>> progressMap
        =
        (java.util.Map<String, java.util.Map<String, Object>>) application.getAttribute("user_progress_map");

            if(progressMap != null && progressMap.containsKey(username)){
            java.util.Map<String, Object> userProgress = progressMap.get(username);
                userProgress.put("remainingTime", time);
                }
                }catch(NumberFormatException e){}
                }
                %>