#include "UserInterface.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>
#include <chrono>

UserInterface::UserInterface(ElevatorController& elevatorController)
    : controller(elevatorController), running(false) {
}

UserInterface::~UserInterface() {
    stop();
}

void UserInterface::start() {
    if (running) {
        return;
    }
    
    running = true;
    
    // Start input thread
    inputThread = std::thread(&UserInterface::inputLoop, this);
    
    // Start display thread
    displayThread = std::thread(&UserInterface::displayLoop, this);
    
    // Display help information
    displayHelp();
}

void UserInterface::stop() {
    if (!running) {
        return;
    }
    
    running = false;
    
    // Wait for threads to finish
    if (inputThread.joinable()) {
        inputThread.join();
    }
    
    if (displayThread.joinable()) {
        displayThread.join();
    }
}

void UserInterface::inputLoop() {
    std::string command;
    
    while (running) {
        std::cout << "> ";
        std::getline(std::cin, command);
        
        if (!running) {
            break;
        }
        
        // Process the command in a way that doesn't get immediately overwritten
        {
            std::lock_guard<std::mutex> lock(displayMutex);
            processCommand(command);
            // Add a small delay to make sure the response is visible
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
}

void UserInterface::displayLoop() {
    // Initial display
    displayStatus();
    
    while (running) {
        // Update the display every 1 second to match the elevator speed
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        if (!running) {
            break;
        }
        
        {
            std::lock_guard<std::mutex> lock(displayMutex);
            displayStatus();
        }
    }
}

void UserInterface::displayStatus() {
    // Clear screen (platform-dependent)
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
    
    // ANSI color codes
    const std::string RESET = "\033[0m";
    const std::string BOLD = "\033[1m";
    const std::string BLUE = "\033[34m";
    const std::string GREEN = "\033[32m";
    const std::string YELLOW = "\033[33m";
    const std::string RED = "\033[31m";
    
    // Get current time
    auto now = std::chrono::system_clock::now();
    auto now_time = std::chrono::system_clock::to_time_t(now);
    std::string time_str = std::ctime(&now_time);
    // Remove newline character
    time_str.pop_back();
    
    std::cout << BOLD << "=== Elevator Control Simulation === " << RESET << "(" << time_str << ")" << std::endl;
    std::cout << std::endl;
    
    // Display elevator statuses
    std::cout << BOLD << std::setw(10) << "Elevator" << " | "
              << std::setw(14) << "Current Floor" << " | "
              << std::setw(12) << "Destination" << " | "
              << std::setw(10) << "Direction" << " | "
              << std::setw(10) << "Status" << RESET << std::endl;
    
    std::cout << std::string(65, '-') << std::endl;
    
    auto statuses = controller.getElevatorStatuses();
    
    for (const auto& [id, currentFloor, destFloor, direction, status] : statuses) {
        std::string dirStr;
        std::string colorCode;
        
        switch (direction) {
            case Direction::IDLE: 
                dirStr = "Idle"; 
                colorCode = RESET;
                break;
            case Direction::UP: 
                dirStr = "Up"; 
                colorCode = GREEN;
                break;
            case Direction::DOWN: 
                dirStr = "Down"; 
                colorCode = YELLOW;
                break;
        }
        
        std::string statusStr;
        std::string statusColor;
        
        switch (status) {
            case ElevatorStatus::IDLE: 
                statusStr = "Idle"; 
                statusColor = RESET;
                break;
            case ElevatorStatus::MOVING: 
                statusStr = "Moving"; 
                statusColor = GREEN;
                break;
            case ElevatorStatus::STOPPED: 
                statusStr = "Stopped"; 
                statusColor = BLUE;
                break;
            case ElevatorStatus::EMERGENCY: 
                statusStr = "EMERGENCY"; 
                statusColor = RED;
                break;
        }
        
        std::cout << BLUE << std::setw(10) << "#" + std::to_string(id) << RESET << " | "
                  << std::setw(14) << currentFloor << " | "
                  << std::setw(12) << (direction == Direction::IDLE ? "--" : std::to_string(destFloor)) << " | "
                  << colorCode << std::setw(10) << dirStr << RESET << " | "
                  << statusColor << std::setw(10) << statusStr << RESET << std::endl;
    }
    
    std::cout << std::endl;
    std::cout << "Type 'help' for available commands" << std::endl;
}

void UserInterface::displayHelp() {
    std::lock_guard<std::mutex> lock(displayMutex);
    
    std::cout << "\n=== Available Commands ===" << std::endl;
    std::cout << "call <floor> <direction>  - Request an elevator to a floor (direction: up/down)" << std::endl;
    std::cout << "go <floor>                - Set destination floor once inside elevator" << std::endl;
    std::cout << "stop                      - Trigger emergency stop for all elevators" << std::endl;
    std::cout << "release                   - Release emergency stop" << std::endl;
    std::cout << "help                      - Display this help message" << std::endl;
    std::cout << "exit                      - Exit the simulation" << std::endl;
    std::cout << std::endl;
}

void UserInterface::processCommand(const std::string& command) {
    std::istringstream iss(command);
    std::string cmd;
    iss >> cmd;
    
    if (cmd == "call") {
        int floor;
        std::string dirStr;
        
        if (iss >> floor >> dirStr) {
            Direction dir = Direction::IDLE;
            
            if (dirStr == "up") {
                dir = Direction::UP;
            } else if (dirStr == "down") {
                dir = Direction::DOWN;
            } else {
                std::cout << "Invalid direction. Use 'up' or 'down'." << std::endl;
                return;
            }
            
            controller.addRequest(floor, 0, dir);
            std::cout << "Elevator requested at floor " << floor << " going " << dirStr << std::endl;
        } else {
            std::cout << "Invalid command format. Use 'call <floor> <direction>'" << std::endl;
        }
    } else if (cmd == "go") {
        int floor;
        
        if (iss >> floor) {
            // Assume we're on the current floor of the first idle elevator
            auto statuses = controller.getElevatorStatuses();
            bool requestSent = false;
            
            for (const auto& [id, currentFloor, destFloor, direction, status] : statuses) {
                if (status == ElevatorStatus::IDLE || status == ElevatorStatus::STOPPED) {
                    controller.addRequest(currentFloor, floor, 
                        floor > currentFloor ? Direction::UP : Direction::DOWN);
                    std::cout << "Elevator #" << id << " will go to floor " << floor << std::endl;
                    requestSent = true;
                    break;
                }
            }
            
            if (!requestSent) {
                std::cout << "No idle elevator available. Try again later." << std::endl;
            }
        } else {
            std::cout << "Invalid command format. Use 'go <floor>'" << std::endl;
        }
    } else if (cmd == "stop") {
        controller.emergencyStop();
        std::cout << "EMERGENCY STOP activated for all elevators!" << std::endl;
    } else if (cmd == "release") {
        controller.releaseEmergencyStop();
        std::cout << "Emergency stop released. Elevators returning to normal operation." << std::endl;
    } else if (cmd == "help") {
        displayHelp();
    } else if (cmd == "exit") {
        std::cout << "Exiting simulation..." << std::endl;
        running = false;
        controller.stop();
    } else if (!cmd.empty()) {
        std::cout << "Unknown command '" << cmd << "'. Type 'help' for available commands." << std::endl;
    }
}
