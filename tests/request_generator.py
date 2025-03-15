#!/usr/bin/env python3
import random
import time
import os
import sys

# Parameters
floors = 15
elevators = 5
num_requests = 10
delay_between_requests = 2  # seconds

def generate_random_request():
    floor_from = random.randint(1, floors)
    floor_to = random.randint(1, floors)
    while floor_to == floor_from:
        floor_to = random.randint(1, floors)
    
    direction = "up" if floor_to > floor_from else "down"
    
    return f"call {floor_from} {direction}\ngo {floor_to}"

def main():
    print(f"Generating {num_requests} random elevator requests...")
    
    for i in range(num_requests):
        request = generate_random_request()
        print(f"\nRequest {i+1}/{num_requests}:")
        print(request)
        
        # Copy these commands to the clipboard for easy pasting
        if sys.platform == 'darwin':  # macOS
            os.system(f"echo '{request}' | pbcopy")
            print("(Commands copied to clipboard. Paste in the elevator simulation terminal)")
        
        time.sleep(delay_between_requests)
    
    print("\nAll requests generated!")

if __name__ == "__main__":
    main()
