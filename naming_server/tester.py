import socket
import threading
import struct

# Configuration
NAMING_SERVER_IP = '127.0.0.1'
NAMING_SERVER_PORT = 5000

# Storage Server Stub
def mock_storage_server():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((NAMING_SERVER_IP, NAMING_SERVER_PORT))
        # Send connection type 'S'
        s.sendall(b'S')
        # Prepare StorageServer data
        ss_ip = '192.168.1.10'.encode('utf-8')
        ss_ip += b'\x00' * (16 - len(ss_ip))
        nm_port = 6000
        client_port = 7000
        data = ss_ip + struct.pack('ii', nm_port, client_port)
        s.sendall(data)
        # Receive response
        response = s.recv(1024)
        print("Storage Server Response:", response.decode())

# Client Stub
def mock_client():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((NAMING_SERVER_IP, NAMING_SERVER_PORT))
        # Send connection type 'C'
        s.sendall(b'C')
        # Prepare Message
        command = 1  # Example command
        data = struct.pack('i', command)
        s.sendall(data)
        # Receive response
        response = s.recv(1024)
        print("Client Response:", response.decode())

# Run mocks in separate threads
def run_tests():
    ss_thread = threading.Thread(target=mock_storage_server)
    client_thread = threading.Thread(target=mock_client)
    
    ss_thread.start()
    client_thread.start()
    
    ss_thread.join()
    client_thread.join()

if __name__ == "__main__":
    run_tests()