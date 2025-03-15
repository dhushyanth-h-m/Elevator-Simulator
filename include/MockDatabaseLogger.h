#pragma once

#include "Elevator.h"
#include <string>
#include <mutex>
#include <vector>
#include <memory>
#include "DatabaseLogger.h"

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
class MockDatabaseLogger : public DatabaseLogger {
public:
    MockDatabaseLogger() : DatabaseLogger(false) {} // Initialize without connecting
    
    // Override methods that would normally connect to the database
    bool connect() override { return true; } // Pretend connection succeeded
    
    void logEvent(LogEventType eventType, int elevatorId, int fromFloor, int toFloor) override {} 
    void logSystemEvent(LogEventType eventType) override {}
    
    void syncElevatorState(int elevatorId, int currentFloor, int destFloor, int direction, int status) override {}
    std::vector<std::tuple<int, int, int, Direction, ElevatorStatus>> getElevatorStates() override { 
        return {}; 
    }
    
    std::vector<std::tuple<std::string, std::string, int, int, int>> getRecentLogs(int limit = 10) override {
        return {};
    }
};