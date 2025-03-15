#include "DatabaseLogger.h"
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fstream>

#ifndef ELEVATOR_TESTING
#include <pqxx/pqxx>
#endif

DatabaseLogger::DatabaseLogger(const std::string& connString)
    : connectionString(connString), connected(false)
#ifndef ELEVATOR_TESTING
    , conn(nullptr)
#endif
{
}

DatabaseLogger::~DatabaseLogger() {
    disconnect();
}

bool DatabaseLogger::connect() {
#ifndef ELEVATOR_TESTING
    try {
        // Create a new connection
        conn = std::make_unique<pqxx::connection>(connectionString);
        
        if (conn && conn->is_open()) {
            std::cout << "Connected to PostgreSQL database: " 
                      << conn->dbname() << " as " << conn->username() << std::endl;
            
            // Initialize database tables
            initializeDatabase();
            
            connected = true;
            return true;
        }
    } catch (const std::exception& e) {
        std::cerr << "Database connection error: " << e.what() << std::endl;
        conn = nullptr;
    }
    
    return false;
#else
    // Mock implementation for testing
    connected = true;
    return true;
#endif
}

void DatabaseLogger::disconnect() {
#ifndef ELEVATOR_TESTING
    std::lock_guard<std::mutex> lock(dbMutex);
    
    if (conn) {
        try {
            // The connection is automatically closed when destroyed
            conn.reset(); // This will destroy the connection object
        } catch (const std::exception& e) {
            std::cerr << "Error disconnecting from database: " << e.what() << std::endl;
        }
    }
    
    connected = false;
#else
    // Mock implementation for testing
    connected = false;
#endif
}

bool DatabaseLogger::isConnected() const {
    return connected;
}

#ifndef ELEVATOR_TESTING
void DatabaseLogger::initializeDatabase() {
    try {
        std::lock_guard<std::mutex> lock(dbMutex);
        
        if (!conn || !conn->is_open()) {
            return;
        }
        
        // Create a transaction
        pqxx::work txn(*conn);
        
        // Check if tables exist and create them if they don't
        txn.exec(
            "CREATE TABLE IF NOT EXISTS elevators ("
            "   id INTEGER PRIMARY KEY,"
            "   current_floor INTEGER NOT NULL,"
            "   destination_floor INTEGER NOT NULL,"
            "   direction INTEGER NOT NULL,"
            "   status INTEGER NOT NULL,"
            "   updated_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP"
            ");"
        );
        
        txn.exec(
            "CREATE TABLE IF NOT EXISTS elevator_logs ("
            "   id SERIAL PRIMARY KEY,"
            "   timestamp TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP,"
            "   event_type VARCHAR(50) NOT NULL,"
            "   elevator_id INTEGER,"
            "   from_floor INTEGER,"
            "   to_floor INTEGER"
            ");"
        );
        
        // Commit the transaction
        txn.commit();
        
    } catch (const std::exception& e) {
        std::cerr << "Error initializing database: " << e.what() << std::endl;
    }
}

std::string DatabaseLogger::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}
#endif

void DatabaseLogger::logEvent(LogEventType eventType, int elevatorId, int fromFloor, int toFloor) {
#ifndef ELEVATOR_TESTING
    if (!connected || !conn) {
        return;
    }
    
    // Map event type to string
    std::string eventTypeStr;
    switch (eventType) {
        case LogEventType::CALL_REQUEST: eventTypeStr = "CALL_REQUEST"; break;
        case LogEventType::ELEVATOR_DISPATCHED: eventTypeStr = "ELEVATOR_DISPATCHED"; break;
        case LogEventType::ELEVATOR_ARRIVED: eventTypeStr = "ELEVATOR_ARRIVED"; break;
        case LogEventType::DOOR_OPENED: eventTypeStr = "DOOR_OPENED"; break;
        case LogEventType::DOOR_CLOSED: eventTypeStr = "DOOR_CLOSED"; break;
        case LogEventType::EMERGENCY_STOP: eventTypeStr = "EMERGENCY_STOP"; break;
        case LogEventType::EMERGENCY_RELEASED: eventTypeStr = "EMERGENCY_RELEASED"; break;
        case LogEventType::SYSTEM_STARTED: eventTypeStr = "SYSTEM_STARTED"; break;
        case LogEventType::SYSTEM_STOPPED: eventTypeStr = "SYSTEM_STOPPED"; break;
        case LogEventType::SYNC_EVENT: eventTypeStr = "SYNC_EVENT"; break;
        default: eventTypeStr = "UNKNOWN"; break;
    }
    
    try {
        std::lock_guard<std::mutex> lock(dbMutex);
        
        if (!conn || !conn->is_open()) {
            return;
        }
        
        // Create a transaction
        pqxx::work txn(*conn);
        
        // Insert log record
        txn.exec_params(
            "INSERT INTO elevator_logs (event_type, elevator_id, from_floor, to_floor) "
            "VALUES ($1, $2, $3, $4)",
            eventTypeStr, elevatorId, fromFloor, toFloor
        );
        
        // Commit the transaction
        txn.commit();
        
        // Also log to console for debugging
        std::cout << getCurrentTimestamp() << " - " << eventTypeStr
                  << " - Elevator: " << elevatorId
                  << " - From: " << fromFloor
                  << " - To: " << toFloor << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error logging event to database: " << e.what() << std::endl;
    }
#endif
}

void DatabaseLogger::logSystemEvent(LogEventType eventType) {
    logEvent(eventType, -1, -1, -1);
}

void DatabaseLogger::syncElevatorState(int elevatorId, int currentFloor, int destFloor, int direction, int status) {
#ifndef ELEVATOR_TESTING
    if (!connected || !conn) {
        return;
    }
    
    try {
        std::lock_guard<std::mutex> lock(dbMutex);
        
        if (!conn || !conn->is_open()) {
            return;
        }
        
        // Create a transaction
        pqxx::work txn(*conn);
        
        // Insert or update elevator state
        txn.exec_params(
            "INSERT INTO elevators (id, current_floor, destination_floor, direction, status, updated_at) "
            "VALUES ($1, $2, $3, $4, $5, CURRENT_TIMESTAMP) "
            "ON CONFLICT (id) DO UPDATE SET "
            "current_floor = $2, destination_floor = $3, direction = $4, status = $5, "
            "updated_at = CURRENT_TIMESTAMP",
            elevatorId, currentFloor, destFloor, direction, status
        );
        
        // Commit the transaction
        txn.commit();
        
    } catch (const std::exception& e) {
        std::cerr << "Error syncing elevator state to database: " << e.what() << std::endl;
    }
#endif
}

std::vector<std::tuple<int, int, int, Direction, ElevatorStatus>> DatabaseLogger::getElevatorStates() {
    std::vector<std::tuple<int, int, int, Direction, ElevatorStatus>> states;
    
#ifndef ELEVATOR_TESTING
    if (!connected || !conn) {
        return states;
    }
    
    try {
        std::lock_guard<std::mutex> lock(dbMutex);
        
        if (!conn || !conn->is_open()) {
            return states;
        }
        
        // Create a transaction
        pqxx::work txn(*conn);
        
        // Query elevator states
        pqxx::result result = txn.exec(
            "SELECT id, current_floor, destination_floor, direction, status FROM elevators "
            "ORDER BY id"
        );
        
        // Iterate through results
        for (const auto& row : result) {
            int id = row[0].as<int>();
            int currentFloor = row[1].as<int>();
            int destFloor = row[2].as<int>();
            Direction direction = static_cast<Direction>(row[3].as<int>());
            ElevatorStatus status = static_cast<ElevatorStatus>(row[4].as<int>());
            
            states.emplace_back(id, currentFloor, destFloor, direction, status);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error retrieving elevator states from database: " << e.what() << std::endl;
    }
#endif
    
    return states;
}

std::vector<std::tuple<std::string, std::string, int, int, int>> DatabaseLogger::getRecentLogs(int limit) {
    std::vector<std::tuple<std::string, std::string, int, int, int>> logs;
    
#ifndef ELEVATOR_TESTING
    if (!connected || !conn) {
        return logs;
    }
    
    try {
        std::lock_guard<std::mutex> lock(dbMutex);
        
        if (!conn || !conn->is_open()) {
            return logs;
        }
        
        // Create a transaction
        pqxx::work txn(*conn);
        
        // Query recent logs
        pqxx::result result = txn.exec_params(
            "SELECT timestamp, event_type, elevator_id, from_floor, to_floor FROM elevator_logs "
            "ORDER BY timestamp DESC LIMIT $1",
            limit
        );
        
        // Iterate through results
        for (const auto& row : result) {
            std::string timestamp = row[0].as<std::string>();
            std::string eventType = row[1].as<std::string>();
            int elevatorId = row[2].as<int>();
            int fromFloor = row[3].as<int>();
            int toFloor = row[4].as<int>();
            
            logs.emplace_back(timestamp, eventType, elevatorId, fromFloor, toFloor);
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error retrieving logs from database: " << e.what() << std::endl;
    }
#endif
    
    return logs;
}