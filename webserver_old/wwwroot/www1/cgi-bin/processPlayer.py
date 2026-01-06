#!/usr/bin/env python

import cgi
import cgitb
import os
import shutil
from pathlib import Path

cgitb.enable()

print("Content-Type: text/html\n")

form = cgi.FieldStorage()

playerName = form.getvalue('playerName', 'Unknown')
selectedAvatar = form.getvalue('avatar', 'avatar1.jpg')

avatarPath = os.path.join(os.path.dirname(__file__), 'assets', selectedAvatar)
if not os.path.exists(avatarPath):
    selectedAvatar = 'avatar1.jpg'

print("""
<!DOCTYPE html>
<html>
<head>
    <title>Registration Complete</title>
    <link rel="stylesheet" href="CGI-style.css">
</head>
<body>
    <div class="success">
        <h2>Registration Successful!</h2>
""")
print("<p>Welcome, " + playerName + "</p>")
print('<div class="avatar-option">')
print("<img src='assets/" + selectedAvatar + "' alt=" + selectedAvatar + "> </div>")
print("""    </div>
</body>
</html>
""")
