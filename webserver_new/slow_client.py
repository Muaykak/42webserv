import socket
import time

SERVER_HOST = "localhost"
SERVER_PORT = 4343

def test_slow_trickle():
    # 1. Open a raw TCP socket connection
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    try:
        s.connect((SERVER_HOST, SERVER_PORT))
        print(f"[+] Connected to server at {SERVER_HOST}:{SERVER_PORT}")
        
        # 2. Send standard headers promising a 50-byte body
        headers = (
            "POST /upload/testSlowTxt.txt HTTP/1.1\r\n"
            f"Host: {SERVER_HOST}:{SERVER_PORT}\r\n"
            "Content-Length: 50\r\n"
            "Content-Type: text/plain\r\n"
            "\r\n"
        )
        s.sendall(headers.encode())
        print("[+] HTTP Headers sent. Commencing slow body trickle...")

        # 3. Trickle the body: 1 byte every 2 seconds
        for i in range(1, 51):
            try:
                s.sendall(b"A")
                print(f"[+] Sent byte {i}/50...")
                time.sleep(2)  # Change this delay to test different thresholds
            except socket.error as e:
                print(f"\n[-] Connection severed by server at byte {i}: {e}")
                break
        
        # 4. Attempt to read the server's response if the loop finished or broke
        print("[*] Listening for server response...")
        s.settimeout(3)
        try:
            response = s.recv(4096)
            if response:
                print("\n[+] Server Response Received:")
                print(response.decode(errors='ignore'))
            else:
                print("\n[-] Server disconnected cleanly without sending data.")
        except socket.timeout:
            print("\n[-] Timed out waiting for server response packet.")

    except Exception as e:
        print(f"[-] Connection error: {e}")
    finally:
        s.close()
        print("[-] Socket destroyed.")

if __name__ == "__main__":
    test_slow_trickle()