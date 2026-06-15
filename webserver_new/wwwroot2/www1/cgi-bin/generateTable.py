#!/usr/bin/env python3

import cgi
import cgitb

cgitb.enable()
form = cgi.FieldStorage()

rows = int(form.getvalue("rows", 5))
columns = int(form.getvalue("columns", 5))
color = form.getvalue("color", "#FF0000")

print("Content-Type: text/html\n")

print("<!DOCTYPE html>")
print("<html>")
print("<head>")
print("<title>Generated Table</title>")
print('<link rel="stylesheet" href="CGI-style.css">')
print("<style>")
print("table { border-collapse: collapse; width: auto; }")
print("th, td { border: 1px solid black; padding: 0; text-align: center; width: 50px; height: 50px;}")
# print("td { background-color: " + color + "; }")
print(".oddRow .oddCol { background-color: " + color + "; }")
print(".oddRow .evenCol { background-color: white; }")
print(".evenRow .evenCol { background-color: " + color + "; }")
print(".evenRow .oddCol { background-color: white; }")
print("</style>")
print("</head>")
print("<body>")
print("<h1>Generated Table</h1>")


print("<table>")
for r in range(rows):
    if r % 2 == 1:
        print("<tr class=oddRow>")
    else:
        print("<tr class=evenRow>")
    for c in range(columns):
        if c % 2 == 1:
            print("<td class=oddCol></td>")
        else:
            print("<td class=evenCol></td>")
    print("</tr>")
print("</table>")

print("</body>")
print("</html>")