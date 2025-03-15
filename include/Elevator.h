#pragma once

#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <string>
#include <thread>

enum class Direction {
    IDLE,
    UP,
    DOWN
};

enum class ElevatorStatus {
    IDLE,
    MOVING,
    STOPPED,
    EMERGENCY
};

struct Request {
    int fromFloor;
    int toFloor;
    Direction direction;
    std::chrono::system_clock::time_point timestamp;
    
    Request(int from, int to, Direction dir) 
        : fromFloor(from), toFloor(to), direction(dir), 
          timestamp(std::chrono::system_clock::now()) {}
};

class Elevator {
private:
    int id;
    std::atomic<int> currentFloor;
    std::atomic<int> destinationFloor;
    std::atomic<Direction> direction;
    std::atomic<ElevatorStatus> status;
    std::mutex requestMutex;
    std::condition_variable requestCV;
    std::queue<Request> requests;
    std::atomic<bool> emergencyStop;
    std::atomic<bool> running;
    
    int numFloors;
    
    // Time it takes to move between floors (in milliseconds)
    const int FLOOR_TRAVEL_TIME_MS = 1000;
    // Time it takes for doors to open/close (in milliseconds)
    const int DOOR_OPERATION_TIME_MS = 1000;
    
    // Thread for processing requests
    std::thread processingThread;
    
    void processRequests();
    void moveToFloor(int floor);
    
public:
    Elevator(int elevatorId, int startFloor = 1, int floors = 10);
    ~Elevator();
    
    void start();
    void stop();
    void emergencyStopActivate();
    void emergencyStopRelease();
    bool addRequest(const Request& request);
    
    // Getters
    int getId() const;
    int getCurrentFloor() const;
    int getDestinationFloor() const;
    Direction getDirection() const;
    ElevatorStatus getStatus() const;
    bool isIdle() const;
    bool hasEmergencyStop() const;
    
    // For calculating distance to a floor
    int calculateDistance(int floor) const;
};