#!/usr/bin/env python3

import cgi
import os
import http.cookies
import uuid
import cgitb
from datetime import datetime, timedelta, timezone

cgitb.enable()

def ensure_tmp_dir():
    tmp_dir = "/tmp"
    if not os.path.exists(tmp_dir):
        os.makedirs(tmp_dir, exist_ok=True)
ensure_tmp_dir()

def get_or_create_session():
    cookies = http.cookies.SimpleCookie(os.environ.get("HTTP_COOKIE"))
    session_id = cookies.get("session_id")
    if session_id is None:
        session_id = str(uuid.uuid4())
        expire_time = (datetime.now(timezone.utc) + timedelta(minutes=5)).strftime("%a, %d %b %Y %H:%M:%S GMT")
        cookies["session_id"] = session_id
        cookies["session_id"]["path"] = "/"
        cookies["session_id"]["secure"] = True
        cookies["session_id"]["httponly"] = True
        cookies["session_id"]["expires"] = expire_time
        print(cookies.output())
    else:
        session_id = session_id.value
    return session_id

def load_session_data(session_id):
    session_file = f"/tmp/session_{session_id}.txt"
    try:
        with open(session_file, "r") as f:
            data = f.read().splitlines()
            return data
    except FileNotFoundError:
        print(f"Session file not found: {session_file}, returning default values.")
        return ["Guest", "duck.jpg"]

def save_session_data(session_id, player_name, avatar):
    session_file = f"/tmp/session_{session_id}.txt"
    try:
        with open(session_file, "w") as f:
            f.write(f"{player_name}\n{avatar}")
    except Exception as e:
        print(f"Error saving session data: {e}")

session_id = get_or_create_session()

form = cgi.FieldStorage()
player_name = form.getvalue("playerName", None)
avatar = form.getvalue("avatar", None)

if player_name and avatar:
    save_session_data(session_id, player_name, avatar)
else:
    player_name, avatar = load_session_data(session_id)

is_session_active = player_name != "Guest" and avatar != "duck.jpg"

print("Content-Type: text/html\n")
print(f"""
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Session Test</title>
    <link rel="stylesheet" href="CGI-style.css">
</head>
<body>   
""")

if is_session_active:
    print(f"""
        <div class="welcome">
            <h1>Welcome, {player_name}!</h1>
            <img src="assets/{avatar}" alt="Avatar" style="width: 150px; height: 150px; border-radius: 50%;">
            <form action="sessionTest.py" method="POST">
                <input type="hidden" name="playerName" value="Guest">
                <input type="hidden" name="avatar" value="avatar1.jpg">
                <button type="submit">Change player</button>
            </form>
        </div>
    """)
else:
    print(f"""
        <div class="form-container">
        <h1>Player Registration</h1>
        <form action="sessionTest.py" method="POST">
            <label for="playerName">Player Name:</label>
            <input type="text" id="playerName" name="playerName" required 
                   placeholder="Enter your player name" minlength="2" maxlength="20">

            <h3>Choose Your Avatar</h3>
            <div class="avatar-options">
                <div class="avatar-option">
                    <input type="radio" id="avatar1" name="avatar" value="avatar1.jpg" checked>
                    <label for="avatar1">
                        <img src="assets/avatar1.jpg" alt="Avatar 1">
                    </label>
                </div>
                <div class="avatar-option">
                    <input type="radio" id="avatar2" name="avatar" value="avatar2.jpg">
                    <label for="avatar2">
                        <img src="assets/avatar2.jpg" alt="Avatar 2">
                    </label>
                </div>
                <div class="avatar-option">
                    <input type="radio" id="avatar3" name="avatar" value="avatar3.jpg">
                    <label for="avatar3">
                        <img src="assets/avatar3.jpg" alt="Avatar 3">
                    </label>
                </div>
            </div>
            <input type="submit" value="Register Player">
        </form>
        </div>
    """)

print("""
    
</body>
</html>
""")