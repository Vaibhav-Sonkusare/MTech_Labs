<%@ page session="true" %>
    <% if(session.getAttribute("user")==null){ response.sendRedirect("login.jsp"); return; } %>
        <html>

        <head>
            <link rel="stylesheet" href="style.css">
        </head>

        <body>
            <div class="container">
                <h2>Dashboard</h2>
                Welcome <%=session.getAttribute("user")%>
                    <br><br>
                    <% String activeTest=null; java.util.Map<String, java.util.Map<String, Object>> pMap =
                        (java.util.Map<String, java.util.Map<String, Object>>)
                            application.getAttribute("user_progress_map");
                            if(pMap != null && session.getAttribute("user") != null){
                            String u = (String) session.getAttribute("user");
                            if(pMap.containsKey(u)){
                            activeTest = (String) pMap.get(u).get("testId");
                            }
                            }
                            %>
                            <a href="startTest.jsp?testId=test1">General Knowledge Test <%= "test1" .equals(activeTest)
                                    ? "(Resume)" : "" %></a><br>
                            <a href="startTest.jsp?testId=test2">Computer Science Test <%= "test2" .equals(activeTest)
                                    ? "(Resume)" : "" %></a>
                            <br><br>
                            <a href="logout.jsp">Logout</a>
            </div>
        </body>

        </html>