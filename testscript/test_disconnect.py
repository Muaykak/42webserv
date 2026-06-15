import socket
import time

SERVER_HOST = "localhost"
SERVER_PORT = 4343

def test_midway_disconnect():
    # 1. Create a TCP socket and connect to your webserv
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((SERVER_HOST, SERVER_PORT))
    print("[+] Connected to server.")

    # 2. Construct a POST request with a Content-Length of 100 bytes
    headers = (
        "POST /test.php HTTP/1.1\r\n"
        f"Host: {SERVER_HOST}:{SERVER_PORT}\r\n"
        "Content-Length: 100\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
    )
    
    # 3. Send headers
    s.sendall(headers.encode())
    print("[+] Sent headers.")

    # 4. Send only 30 bytes of the body (midway)
    partial_body = "A" * 30
    s.sendall(partial_body.encode())
    print("[+] Sent 30/100 bytes of the body.")

    # 5. Wait a moment, then kill the socket abruptly
    time.sleep(0.5)
    print("[-] Abruptly disconnecting now...")
    s.close() # This closes the file descriptor on the client side

if __name__ == "__main__":
    test_midway_disconnect()