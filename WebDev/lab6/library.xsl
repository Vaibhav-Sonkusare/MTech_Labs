<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns="http://www.w3.org/1999/xhtml">
    <xsl:output method="html" encoding="UTF-8" indent="yes" />

    <xsl:template match="/">
        <table border="1" cellpadding="8" cellspacing="0" style="border-collapse: collapse; text-align: left;">
            <tr style="background-color: #f2f2f2;">
                <th>Book ID</th>
                <th>Title</th>
                <th>Author</th>
                <th>Year</th>
                <th>ISBN</th>
                <th>Status</th>
            </tr>
            <xsl:for-each select="library/book">
                <tr>
                    <td><xsl:value-of select="@id"/></td>
                    <td><xsl:value-of select="title"/></td>
                    <td><xsl:value-of select="author"/></td>
                    <td><xsl:value-of select="year"/></td>
                    <td><xsl:value-of select="isbn"/></td>
                    <td><xsl:value-of select="status"/></td>
                </tr>
            </xsl:for-each>
        </table>
    </xsl:template>
</xsl:stylesheet>
