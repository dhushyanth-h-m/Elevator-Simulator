#include "Elevator.h"
#include <thread>
#include <iostream>
#include <chrono>

Elevator::Elevator(int elevatorId, int startFloor, int floors)
    : id(elevatorId),
      currentFloor(startFloor),
      destinationFloor(startFloor),
      direction(Direction::IDLE),
      status(ElevatorStatus::IDLE),
      emergencyStop(false),
      running(false),
      numFloors(floors) {
}

Elevator::~Elevator() {
    stop();
}

void Elevator::start() {
    if (running) {
        return;
    }
    
    running = true;
    processingThread = std::thread(&Elevator::processRequests, this);
}

void Elevator::stop() {
    if (!running) {
        return;
    }
    
    running = false;
    requestCV.notify_all();
    
    // Wait for the processing thread to finish
    if (processingThread.joinable()) {
        processingThread.join();
    }
}

void Elevator::emergencyStopActivate() {
    emergencyStop = true;
    status = ElevatorStatus::EMERGENCY;
    requestCV.notify_all();
}

void Elevator::emergencyStopRelease() {
    emergencyStop = false;
    status = ElevatorStatus::IDLE;
}

bool Elevator::addRequest(const Request& request) {
    if (emergencyStop) {
        return false;
    }
    
    {
        std::lock_guard<std::mutex> lock(requestMutex);
        requests.push(request);
    }
    
    requestCV.notify_one();
    return true;
}

void Elevator::processRequests() {
    while (running) {
        Request currentRequest{0, 0, Direction::IDLE};
        bool hasRequest = false;
        
        {
            std::unique_lock<std::mutex> lock(requestMutex);
            requestCV.wait(lock, [this] {
                return !running || emergencyStop || !requests.empty();
            });
            
            if (!running) {
                break;
            }
            
            if (emergencyStop) {
                // Wait until emergency is cleared
                continue;
            }
            
            if (!requests.empty()) {
                currentRequest = requests.front();
                requests.pop();
                hasRequest = true;
            }
        }
        
        if (hasRequest) {
            // First move to the pickup floor if it's a new request
            if (currentRequest.fromFloor != currentFloor) {
                moveToFloor(currentRequest.fromFloor);
            }
            
            // Only move to the destination floor if it's not 0 (which indicates a call request)
            if (currentRequest.toFloor != 0) {
                moveToFloor(currentRequest.toFloor);
            }
        } else {
            // No requests, set to idle
            direction = Direction::IDLE;
            status = ElevatorStatus::IDLE;
        }
    }
}

void Elevator::moveToFloor(int floor) {
    if (floor == currentFloor) {
        return;
    }
    
    // Set direction and status
    direction = (floor > currentFloor) ? Direction::UP : Direction::DOWN;
    status = ElevatorStatus::MOVING;
    destinationFloor = floor;
    
    // Simulate movement between floors
    while (currentFloor != floor && !emergencyStop && running) {
        // Wait for the time it takes to move between floors
        std::this_thread::sleep_for(std::chrono::milliseconds(FLOOR_TRAVEL_TIME_MS));
        
        // Move one floor in the current direction
        if (direction == Direction::UP) {
            currentFloor++;
        } else {
            currentFloor--;
        }
        
        // Check if we've reached the destination
        if (currentFloor == floor) {
            // Simulate doors opening and closing
            status = ElevatorStatus::STOPPED;
            std::this_thread::sleep_for(std::chrono::milliseconds(DOOR_OPERATION_TIME_MS * 2));
            
            // Reset direction if we're at the destination
            direction = Direction::IDLE;
            status = ElevatorStatus::IDLE;
        }
    }
}

int Elevator::getId() const {
    return id;
}

int Elevator::getCurrentFloor() const {
    return currentFloor;
}

int Elevator::getDestinationFloor() const {
    return destinationFloor;
}

Direction Elevator::getDirection() const {
    return direction;
}

ElevatorStatus Elevator::getStatus() const {
    return status;
}

bool Elevator::isIdle() const {
    return status == ElevatorStatus::IDLE;
}

bool Elevator::hasEmergencyStop() const {
    return emergencyStop;
}

int Elevator::calculateDistance(int floor) const {
    int distance = std::abs(currentFloor - floor);
    
    // If the elevator is moving, consider its direction
    if (status == ElevatorStatus::MOVING) {
        if ((direction == Direction::UP && floor < currentFloor) ||
            (direction == Direction::DOWN && floor > currentFloor)) {
            // Going in the opposite direction, add penalty
            distance += 2 * (numFloors - 1);
        }
    }
    
    return distance;
}