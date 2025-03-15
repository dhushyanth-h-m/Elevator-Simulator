#pragma once

#include "Elevator.h"
#include <string>
#include <mutex>
#include <vector>
#include <memory>

// Only include PostgreSQL in non-test builds
#ifndef ELEVATOR_TESTING
#include <pqxx/pqxx>
#endif

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

class DatabaseLogger {
private:
    std::string connectionString;
    std::mutex dbMutex;
    bool connected;
    
    #ifndef ELEVATOR_TESTING
    std::unique_ptr<pqxx::connection> conn;
    
    // Initialize database tables if they don't exist
    void initializeDatabase();
    
    // Get a formatted timestamp for logging
    std::string getCurrentTimestamp() const;
    #endif
    
public:
    DatabaseLogger(const std::string& connString = "dbname=elevator_db user=elevator_user password=secret host=localhost");
    ~DatabaseLogger();
    
    bool connect();
    void disconnect();
    bool isConnected() const;
    
    // Logging methods
    void logEvent(LogEventType eventType, int elevatorId, int fromFloor, int toFloor);
    void logSystemEvent(LogEventType eventType);
    
    // Synchronization methods
    void syncElevatorState(int elevatorId, int currentFloor, int destFloor, int direction, int status);
    std::vector<std::tuple<int, int, int, Direction, ElevatorStatus>> getElevatorStates();
    
    // Retrieve logs
    std::vector<std::tuple<std::string, std::string, int, int, int>> getRecentLogs(int limit = 10);
};