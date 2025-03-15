#pragma once

#include "Elevator.h"
#include <string>
#include <mutex>
#include <vector>
#include <memory>

// Same enum as in the real DatabaseLogger
enum class LogEventType {
    CALL_REQUEST,
    ELEVATOR_DISPATCHED,
    ELEVATOR_ARRIVED,
    DOOR_OPENED,
    DOOR_CLOSED,
    EMERGENCY_STOP,
    EMERGENCY_RELEASED,
    SYSTEM_STARTED,
    SYSTEM_STOPPED,
    SYNC_EVENT
};

// Mock implementation that doesn't require PostgreSQL
class DatabaseLogger {
private:
    bool connected;
    
public:
    DatabaseLogger(const std::string& connString = "") : connected(false) {}
    ~DatabaseLogger() {}
    
    bool connect() { connected = true; return true; }
    void disconnect() { connected = false; }
    bool isConnected() const { return connected; }
    
    // Empty implementations for test
    void logEvent(LogEventType eventType, int elevatorId, int fromFloor, int toFloor) {}
    void logSystemEvent(LogEventType eventType) {}
    
    void syncElevatorState(int elevatorId, int currentFloor, int destFloor, int direction, int status) {}
    std::vector<std::tuple<int, int, int, Direction, ElevatorStatus>> getElevatorStates() { 
        return {}; 
    }
    
    std::vector<std::tuple<std::string, std::string, int, int, int>> getRecentLogs(int limit = 10) {
        return {};
    }
};