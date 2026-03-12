<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
    <xsl:template match="/">
        <html>
            <head>
                <title>University Report</title>
                <style>
                    body { font-family: sans-serif; }
                    table { border-collapse: collapse; width: 100%; margin-bottom: 20px; }
                    th, td { border: 1px solid #ccc; padding: 8px; text-align: left; }
                    th { background-color: #f4f4f4; }
                </style>
            </head>
            <body>
                <h1>University System Report</h1>
                <h2>Students</h2>
                <table>
                    <tr><th>ID</th><th>Name</th><th>Email</th></tr>
                    <xsl:for-each select="University/Students/Student">
                        <tr>
                            <td><xsl:value-of select="@id"/></td>
                            <td><xsl:value-of select="Name"/></td>
                            <td><xsl:value-of select="Email"/></td>
                        </tr>
                    </xsl:for-each>
                </table>
                <h2>Courses</h2>
                <table>
                    <tr><th>ID</th><th>Title</th><th>Credits</th></tr>
                    <xsl:for-each select="University/Courses/Course">
                        <tr>
                            <td><xsl:value-of select="@id"/></td>
                            <td><xsl:value-of select="Title"/></td>
                            <td><xsl:value-of select="Credits"/></td>
                        </tr>
                    </xsl:for-each>
                </table>
                <h2>Enrolments</h2>
                <table>
                    <tr><th>Student ID</th><th>Course ID</th><th>Grade</th></tr>
                    <xsl:for-each select="University/Enrolments/Enrolment">
                        <tr>
                            <td><xsl:value-of select="StudentId"/></td>
                            <td><xsl:value-of select="CourseId"/></td>
                            <td><xsl:value-of select="Grade"/></td>
                        </tr>
                    </xsl:for-each>
                </table>
            </body>
        </html>
    </xsl:template>
</xsl:stylesheet>
