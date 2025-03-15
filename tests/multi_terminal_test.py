#!/usr/bin/env python3
import socket
import threading
import time
import random
import argparse
import sys
import select
import queue
from datetime import datetime

# Default server settings
DEFAULT_SERVER = "127.0.0.1"
DEFAULT_PORT = 8081
DEFAULT_CLIENTS = 5
DEFAULT_REQUESTS = 8

# Colors for terminal output
COLORS = [
    "\033[91m",  # Red
    "\033[92m",  # Green
    "\033[93m",  # Yellow
    "\033[94m",  # Blue
    "\033[95m",  # Magenta
    "\033[96m",  # Cyan
    "\033[97m",  # White
]
RESET_COLOR = "\033[0m"

class ElevatorClient:
    def __init__(self, server, port, client_id, floors, delay=0.5):
        self.server = server
        self.port = port
        self.client_id = client_id
        self.floors = floors
        self.delay = delay
        self.sock = None
        self.connected = False
        self.running = False
        self.response_queue = queue.Queue()
        self.color = COLORS[client_id % len(COLORS)]
        
    def connect(self):
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect((self.server, self.port))
            self.connected = True
            self.running = True
            
            # Start receiver thread
            self.receiver_thread = threading.Thread(target=self.receive_messages)
            self.receiver_thread.daemon = True
            self.receiver_thread.start()
            
            # Give the server a moment to process the connection
            time.sleep(1)
            
            # Skip the welcome message
            self.get_response()
            
            return True
        except Exception as e:
            print(f"Client {self.client_id} - Connection error: {e}")
            self.connected = False
            return False
            
    def disconnect(self):
        self.running = False
        if self.sock:
            self.sock.close()
        
    def send_command(self, command):
        if not self.connected:
            return False
            
        timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
        print(f"{self.color}[{timestamp}] Client {self.client_id} - Sending: {command}{RESET_COLOR}")
        
        try:
            # Ensure command ends with a newline
            if not command.endswith('\n'):
                command += '\n'
            self.sock.sendall(command.encode())
            return True
        except Exception as e:
            print(f"Client {self.client_id} - Send error: {e}")
            self.connected = False
            return False
            
    def receive_messages(self):
        while self.running:
            try:
                ready = select.select([self.sock], [], [], 0.5)
                if ready[0]:
                    data = self.sock.recv(4096)
                    if not data:
                        self.connected = False
                        break
                        
                    response = data.decode()
                    self.response_queue.put(response)
            except Exception as e:
                if self.running:
                    print(f"Client {self.client_id} - Receive error: {e}")
                self.connected = False
                break
                
    def get_response(self, timeout=10):
        try:
            start_time = time.time()
            while time.time() - start_time < timeout:
                try:
                    response = self.response_queue.get(block=False)
                    timestamp = datetime.now().strftime("%H:%M:%S.%f")[:-3]
                    print(f"{self.color}[{timestamp}] Client {self.client_id} - Response:\n{response}{RESET_COLOR}")
                    return response
                except queue.Empty:
                    time.sleep(0.1)
                    
            print(f"Client {self.client_id} - Timeout waiting for response")
            return None
        except Exception as e:
            print(f"Client {self.client_id} - Error getting response: {e}")
            return None
            
    def generate_random_request(self):
        floor_from = random.randint(1, self.floors)
        floor_to = random.randint(1, self.floors)
        while floor_to == floor_from:
            floor_to = random.randint(1, self.floors)
            
        direction = "up" if floor_to > floor_from else "down"
        
        # First call the elevator
        self.send_command(f"call {floor_from} {direction}")
        self.get_response()
        
        # Wait a moment for the elevator to arrive
        time.sleep(random.uniform(0.5, 1.5))
        
        # Then set the destination
        self.send_command(f"go {floor_to}")
        self.get_response()
        
        return floor_from, floor_to
        
    def run_random_requests(self, num_requests):
        for i in range(num_requests):
            if not self.connected or not self.running:
                break
                
            # Get and display elevator status
            self.send_command("status")
            self.get_response()
            
            # Wait a bit before the next command
            time.sleep(1)
            
            # Generate a random request
            from_floor, to_floor = self.generate_random_request()
            
            # Wait a bit before the next request
            wait_time = random.uniform(1, 3)
            time.sleep(wait_time)
            
def run_client(server, port, client_id, floors, num_requests):
    client = ElevatorClient(server, port, client_id, floors)
    
    if client.connect():
        print(f"Client {client_id} connected")
        client.run_random_requests(num_requests)
        client.disconnect()
        print(f"Client {client_id} finished")
    
def main():
    parser = argparse.ArgumentParser(description="Simulate multiple clients connecting to the elevator server")
    parser.add_argument("--server", default=DEFAULT_SERVER, help=f"Server IP address (default: {DEFAULT_SERVER})")
    parser.add_argument("--port", type=int, default=DEFAULT_PORT, help=f"Server port (default: {DEFAULT_PORT})")
    parser.add_argument("--clients", type=int, default=DEFAULT_CLIENTS, help=f"Number of clients (default: {DEFAULT_CLIENTS})")
    parser.add_argument("--floors", type=int, default=15, help="Number of floors in the elevator system (default: 15)")
    parser.add_argument("--requests", type=int, default=DEFAULT_REQUESTS, help=f"Requests per client (default: {DEFAULT_REQUESTS})")
    
    args = parser.parse_args()
    
    print(f"Starting {args.clients} clients connecting to {args.server}:{args.port}")
    print(f"Each client will make {args.requests} random elevator requests")
    
    # Create and start client threads
    threads = []
    for i in range(args.clients):
        thread = threading.Thread(
            target=run_client,
            args=(args.server, args.port, i, args.floors, args.requests)
        )
        thread.daemon = True
        threads.append(thread)
        
    # Start all threads with a slight delay between them
    for thread in threads:
        thread.start()
        time.sleep(1.5)  # Stagger the clients more to reduce server load
        
    # Wait for all threads to finish
    for thread in threads:
        thread.join()
        
    print("All clients have completed their requests")
    
if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nTest interrupted by user")
        sys.exit(0) 