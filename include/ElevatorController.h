#pragma once

#include "Elevator.h"
#include "DatabaseLogger.h"
#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>

class ElevatorController {
private:
    std::vector<std::unique_ptr<Elevator>> elevators;
    std::vector<std::thread> elevatorThreads;
    std::queue<Request> pendingRequests;
    std::mutex requestMutex;
    std::condition_variable requestCV;
    std::atomic<bool> running;
    std::thread dispatcherThread;
    std::unique_ptr<DatabaseLogger> dbLogger;
    
    // Configuration
    int numElevators;
    int numFloors;
    
    std::thread syncThread;
    std::atomic<bool> syncRunning;
    void startSyncThread();
    void syncWithDatabase();
    
    void dispatcherLoop();
    Elevator* findBestElevator(const Request& request);
    
public:
    ElevatorController(int elevators = 3, int floors = 10);
    ~ElevatorController();
    
    void start();
    void stop();
    void emergencyStop();
    void releaseEmergencyStop();
    void addRequest(int fromFloor, int toFloor, Direction direction);
    
    // Status information
    std::vector<std::tuple<int, int, int, Direction, ElevatorStatus>> getElevatorStatuses() const;
    
    // Configuration getters
    int getNumElevators() const;
    int getNumFloors() const;
    
    void syncElevatorStates();
}; 