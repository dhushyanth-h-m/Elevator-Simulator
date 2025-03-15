#include "ElevatorController.h"
#include <algorithm>
#include <iostream>
#include <limits>
#include <chrono>
#include <thread>
#include <functional>

ElevatorController::ElevatorController(int numElevators, int numFloors)
    : numFloors(numFloors), running(false), syncRunning(false) {
    
    // Initialize database logger
    dbLogger = std::make_unique<DatabaseLogger>();
    
    // Create elevators
    for (int i = 0; i < numElevators; i++) {
        elevators.push_back(std::make_unique<Elevator>(i, 1, numFloors));
    }
    
    // Connect to the database
#ifndef ELEVATOR_TESTING
    dbLogger->connect();
#endif
}

ElevatorController::~ElevatorController() {
    stop();
}

void ElevatorController::start() {
    if (running) {
        return;
    }
    
    running = true;
    
    // Start all elevators
    for (auto& elevator : elevators) {
        elevator->start();
    }
    
    // Start dispatcher thread
    dispatcherThread = std::thread(&ElevatorController::dispatcherLoop, this);
    
    // Log system start
    dbLogger->logSystemEvent(LogEventType::SYSTEM_STARTED);
    
    // Start sync thread
    startSyncThread();
}

void ElevatorController::stop() {
    if (!running) {
        return;
    }
    
    running = false;
    syncRunning = false;
    
    // Log system stop
    dbLogger->logSystemEvent(LogEventType::SYSTEM_STOPPED);
    
    // Stop all elevators
    for (auto& elevator : elevators) {
        if (elevator) {  // Check pointer is valid
            elevator->stop();
        }
    }
    
    // Notify dispatcher thread to exit
    {
        std::unique_lock<std::mutex> lock(requestMutex);
        requestCV.notify_all();
    }
    
    // Wait for dispatcher thread to finish
    if (dispatcherThread.joinable()) {
        dispatcherThread.join();
    }
    
    // Wait for sync thread to finish
    if (syncThread.joinable()) {
        syncThread.join();
    }
    
    // Disconnect from database
#ifndef ELEVATOR_TESTING
    if (dbLogger) {
        dbLogger->disconnect();
    }
#endif
}

void ElevatorController::emergencyStop() {
    for (auto& elevator : elevators) {
        elevator->emergencyStopActivate();
    }
    
    // Log emergency stop
    if (dbLogger->isConnected()) {
        dbLogger->logSystemEvent(LogEventType::EMERGENCY_STOP);
    }
}

void ElevatorController::releaseEmergencyStop() {
    for (auto& elevator : elevators) {
        elevator->emergencyStopRelease();
    }
    
    // Log emergency release
    if (dbLogger->isConnected()) {
        dbLogger->logSystemEvent(LogEventType::EMERGENCY_RELEASED);
    }
}

void ElevatorController::addRequest(int fromFloor, int toFloor, Direction direction) {
    // Validate the fromFloor
    if (fromFloor < 1 || fromFloor > numFloors) {
        std::cerr << "Invalid source floor number. Floors must be between 1 and " << numFloors << std::endl;
        return;
    }
    
    // Validate the toFloor - 0 is a special case used for "call" commands
    if (toFloor != 0 && (toFloor < 1 || toFloor > numFloors)) {
        std::cerr << "Invalid destination floor number. Floors must be between 1 and " << numFloors << std::endl;
        return;
    }
    
    Request request(fromFloor, toFloor, direction);
    
    {
        std::lock_guard<std::mutex> lock(requestMutex);
        pendingRequests.push(request);
    }
    
    // Log the request
    if (dbLogger->isConnected()) {
        dbLogger->logEvent(LogEventType::CALL_REQUEST, 0, fromFloor, toFloor);
    }
    
    requestCV.notify_one();
}

void ElevatorController::dispatcherLoop() {
    while (running) {
        Request currentRequest{0, 0, Direction::IDLE};
        bool hasRequest = false;
        
        {
            std::unique_lock<std::mutex> lock(requestMutex);
            requestCV.wait(lock, [this] {
                return !running || !pendingRequests.empty();
            });
            
            if (!running) {
                break;
            }
            
            if (!pendingRequests.empty()) {
                currentRequest = pendingRequests.front();
                pendingRequests.pop();
                hasRequest = true;
            }
        }
        
        if (hasRequest) {
            Elevator* bestElevator = findBestElevator(currentRequest);
            
            if (bestElevator) {
                bestElevator->addRequest(currentRequest);
                
                // Log elevator dispatch
                if (dbLogger->isConnected()) {
                    dbLogger->logEvent(LogEventType::ELEVATOR_DISPATCHED, 
                                      bestElevator->getId(), 
                                      currentRequest.fromFloor, 
                                      currentRequest.toFloor);
                }
            } else {
                // If no elevator is available, put the request back in the queue
                std::lock_guard<std::mutex> lock(requestMutex);
                pendingRequests.push(currentRequest);
            }
        }
    }
}

Elevator* ElevatorController::findBestElevator(const Request& request) {
    Elevator* bestElevator = nullptr;
    int shortestDistance = std::numeric_limits<int>::max();
    
    for (auto& elevator : elevators) {
        // Skip elevators in emergency stop
        if (elevator->hasEmergencyStop()) {
            continue;
        }
        
        int distance = elevator->calculateDistance(request.fromFloor);
        
        // Prioritize idle elevators
        if (elevator->isIdle()) {
            distance -= numFloors; // Give idle elevators a significant advantage
        }
        
        // If this elevator is better than our current best, update
        if (distance < shortestDistance) {
            shortestDistance = distance;
            bestElevator = elevator.get();
        }
    }
    
    return bestElevator;
}

std::vector<std::tuple<int, int, int, Direction, ElevatorStatus>> ElevatorController::getElevatorStatuses() const {
    std::vector<std::tuple<int, int, int, Direction, ElevatorStatus>> statuses;
    
    for (const auto& elevator : elevators) {
        statuses.emplace_back(
            elevator->getId(),
            elevator->getCurrentFloor(),
            elevator->getDestinationFloor(),
            elevator->getDirection(),
            elevator->getStatus()
        );
    }
    
    return statuses;
}

int ElevatorController::getNumElevators() const {
    return elevators.size();
}

int ElevatorController::getNumFloors() const {
    return numFloors;
}

void ElevatorController::startSyncThread() {
#ifndef ELEVATOR_TESTING
    syncRunning = true;
    syncThread = std::thread(&ElevatorController::syncWithDatabase, this);
#endif
}

void ElevatorController::syncWithDatabase() {
#ifndef ELEVATOR_TESTING
    if (!dbLogger->isConnected()) {
        return;
    }
    
    while (syncRunning) {
        // First, sync our elevator states to the database
        for (const auto& elevator : elevators) {
            int id = elevator->getId();
            int currentFloor = elevator->getCurrentFloor();
            int destFloor = elevator->getDestinationFloor();
            int direction = static_cast<int>(elevator->getDirection());
            int status = static_cast<int>(elevator->getStatus());
            
            dbLogger->syncElevatorState(id, currentFloor, destFloor, direction, status);
        }
        
        // Then, check if there are any states in the database we need to sync with
        auto dbStates = dbLogger->getElevatorStates();
        
        // If we have fewer elevators than in the database, we need to add more
        if (dbStates.size() > elevators.size()) {
            for (const auto& [id, currentFloor, destFloor, direction, status] : dbStates) {
                // Check if this elevator exists in our system
                bool found = false;
                for (const auto& elevator : elevators) {
                    if (elevator->getId() == id) {
                        found = true;
                        break;
                    }
                }
                
                // If not found, create a new elevator with the state from the database
                if (!found) {
                    auto newElevator = std::make_unique<Elevator>(id, currentFloor, numFloors);
                    newElevator->start();
                    elevators.push_back(std::move(newElevator));
                }
            }
        }
        
        // Sleep for a while before next sync - exactly 1 second to match floor travel time
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
#endif
}