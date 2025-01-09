import socket
import threading

SERVER_IP = "0.0.0.0"
SERVER_PORT = 8080

def handle_client(client_socket, client_address):
    print(f"Connection from {client_address}")
    while True:
        data = client_socket.recv(1024)
        if not data:
            break
        print(data.decode("utf-8"))
    client_socket.close()

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.bind((SERVER_IP, SERVER_PORT))
server.listen(5)

print(f"Listening on {SERVER_IP}:{SERVER_PORT}")

while True:
    client_socket, client_address = server.accept()
    client_thread = threading.Thread(target=handle_client, args=(client_socket, client_address))
    client_thread.start()
