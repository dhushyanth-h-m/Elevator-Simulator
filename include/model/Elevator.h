#pragma once

#include <atomic>
#include <thread>
#include <mutex>
#include <queue>
#include <string>
#include <condition_variable>
#include <functional>

namespace elevator {

// Direction of elevator movement
enum class Direction {`
    IDLE,
    UP,
    DOWN
};

// Status of the elevator
enum class ElevatorStatus {
    IDLE,
    MOVING,
    STOPPED,
    DOOR_OPEN,
    DOOR_CLOSED,
    EMERGENCY_STOPPED
};

// Request for elevator service
struct ElevatorRequest {
    int fromFloor;
    int toFloor;  // -1 if this is just a pickup request
    Direction direction;
    
    ElevatorRequest(int from, int to, Direction dir) :
        fromFloor(from), toFloor(to), direction(dir) {}
};

class Elevator {
public:
    Elevator(int id, int totalFloors);
    ~Elevator();
    
    // State getters
    int getId() const;
    int getCurrentFloor() const;
    int getDestinationFloor() const;
    Direction getDirection() const;
    ElevatorStatus getStatus() const;
    std::string getStatusString() const;
    
    // Operations
    void start();
    void stop();
    void emergencyStop();
    void reset();
    void addRequest(const ElevatorRequest& request);
    
    // Calculate distance to a floor based on current position and direction
    int calculateDistanceToFloor(int floor, Direction requestDirection) const;
    
private:
    // Core elevator properties
    int id_;
    int totalFloors_;
    std::atomic<int> currentFloor_;
    std::atomic<int> destinationFloor_;
    std::atomic<Direction> direction_;
    std::atomic<ElevatorStatus> status_;
    
    // Thread management
    std::thread controlThread_;
    std::atomic<bool> running_;
    std::atomic<bool> emergencyStopped_;
    
    // Request queue management
    std::queue<ElevatorRequest> requestQueue_;
    std::mutex queueMutex_;
    std::condition_variable queueCondition_;
    
    // Movement simulation
    static constexpr int FLOOR_TRAVEL_TIME_MS = 1000;  // Time to travel between floors
    static constexpr int DOOR_OPERATION_TIME_MS = 1500; // Time for doors to open/close
    
    // Private operations
    void controlLoop();
    void processNextRequest();
    void moveToFloor(int targetFloor);
    void openDoors();
    void closeDoors();
    void logStatus(const std::string& message);
};

} // namespace elevator