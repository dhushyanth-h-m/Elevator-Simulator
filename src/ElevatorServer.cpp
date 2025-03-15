#include "ElevatorServer.h"
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <fcntl.h>
#include <limits>

ElevatorServer::ElevatorServer(ElevatorController& controller, int port)
    : controller(controller), running(false), port(port), serverSocket(-1) {
}

ElevatorServer::~ElevatorServer() {
    stop();
}

bool ElevatorServer::start() {
    if (running) {
        return true; // Already running
    }
    
    // Create socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Error opening socket: " << strerror(errno) << std::endl;
        return false;
    }
    
    // Set socket options to reuse address
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "Error setting socket options: " << strerror(errno) << std::endl;
        close(serverSocket);
        return false;
    }
    
    // Bind to the port
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);
    
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error binding socket: " << strerror(errno) << std::endl;
        close(serverSocket);
        return false;
    }
    
    // Set socket to listen with a backlog of 5 connections
    if (listen(serverSocket, 5) < 0) {
        std::cerr << "Error listening on socket: " << strerror(errno) << std::endl;
        close(serverSocket);
        return false;
    }
    
    // Start the server loop in a separate thread
    running = true;
    serverThread = std::thread(&ElevatorServer::serverLoop, this);
    
    std::cout << "Elevator server started on port " << port << std::endl;
    return true;
}

void ElevatorServer::stop() {
    if (!running) {
        return;
    }
    
    running = false;
    
    // Close the server socket to interrupt accept()
    if (serverSocket >= 0) {
        close(serverSocket);
        serverSocket = -1;
    }
    
    // Wait for the server thread to finish
    if (serverThread.joinable()) {
        serverThread.join();
    }
    
    // Close all client connections and join threads
    std::lock_guard<std::mutex> lock(clientsMutex);
    for (auto& client : activeClients) {
        close(client.first);
    }
    
    for (auto& thread : clientThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }
    
    activeClients.clear();
    clientThreads.clear();
    
    std::cout << "Elevator server stopped" << std::endl;
}

void ElevatorServer::serverLoop() {
    while (running) {
        // Set up for select() to allow non-blocking accept
        fd_set readFds;
        FD_ZERO(&readFds);
        FD_SET(serverSocket, &readFds);
        
        // Set timeout to 1 second to allow checking running flag periodically
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int ready = select(serverSocket + 1, &readFds, nullptr, nullptr, &timeout);
        
        if (!running) {
            break;
        }
        
        if (ready < 0) {
            if (errno != EINTR) {
                std::cerr << "Error in select(): " << strerror(errno) << std::endl;
            }
            continue;
        }
        
        if (ready == 0) {
            // Timeout, no connections waiting
            continue;
        }
        
        // Accept a new client connection
        struct sockaddr_in clientAddr;
        socklen_t clientLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientLen);
        
        if (clientSocket < 0) {
            std::cerr << "Error accepting connection: " << strerror(errno) << std::endl;
            continue;
        }
        
        // Get client info
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
        std::cout << "New client connected from " << clientIP << ":" << ntohs(clientAddr.sin_port) << std::endl;
        
        // Add client to active clients
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            activeClients[clientSocket] = true;
        }
        
        // Start a new thread to handle this client
        clientThreads.emplace_back(&ElevatorServer::handleClient, this, clientSocket);
    }
}

void ElevatorServer::handleClient(int clientSocket) {
    // Welcome message
    sendResponse(clientSocket, "Welcome to the Elevator Control System!\n"
                              "Available commands:\n"
                              "  call <floor> <direction>  - Request an elevator (direction: up/down)\n"
                              "  go <floor>                - Set destination floor\n"
                              "  stop                      - Trigger emergency stop\n"
                              "  release                   - Release emergency stop\n"
                              "  status                    - Get elevator statuses\n"
                              "  exit                      - Disconnect from server\n");
    
    char buffer[1024];
    bool clientActive = true;
    
    // Set socket to non-blocking mode
    int flags = fcntl(clientSocket, F_GETFL, 0);
    fcntl(clientSocket, F_SETFL, flags | O_NONBLOCK);
    
    while (running && clientActive) {
        fd_set readFds;
        FD_ZERO(&readFds);
        FD_SET(clientSocket, &readFds);
        
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int ready = select(clientSocket + 1, &readFds, nullptr, nullptr, &timeout);
        
        if (!running) {
            break;
        }
        
        if (ready < 0) {
            if (errno != EINTR) {
                std::cerr << "Error in client select(): " << strerror(errno) << std::endl;
                break;
            }
            continue;
        }
        
        if (ready == 0) {
            // Timeout, no data available
            continue;
        }
        
        // Read from client
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytesRead = read(clientSocket, buffer, sizeof(buffer) - 1);
        
        if (bytesRead <= 0) {
            // Client disconnected or error
            if (bytesRead < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
                // No data available, not an error
                continue;
            }
            clientActive = false;
            break;
        }
        
        // Process the command
        std::string command(buffer, bytesRead);
        // Remove newlines and carriage returns
        command.erase(std::remove(command.begin(), command.end(), '\n'), command.end());
        command.erase(std::remove(command.begin(), command.end(), '\r'), command.end());
        
        if (!command.empty()) {
            processCommand(clientSocket, command);
        }
    }
    
    // Close the socket and mark client as inactive
    close(clientSocket);
    
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        activeClients.erase(clientSocket);
    }
    
    std::cout << "Client disconnected" << std::endl;
}

void ElevatorServer::processCommand(int clientSocket, const std::string& command) {
    // Remove debug output for cleaner terminal
    // std::cout << "Processing command: '" << command << "' from client " << clientSocket << std::endl;
    
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
                // std::cout << "Invalid direction: " << dirStr << std::endl;
                sendResponse(clientSocket, "Invalid direction. Use 'up' or 'down'.");
                return;
            }
            
            // std::cout << "Adding request: floor " << floor << " direction " << dirStr << std::endl;
            if (floor < 1 || floor > controller.getNumFloors()) {
                sendResponse(clientSocket, "Invalid floor number. Floors must be between 1 and " + std::to_string(controller.getNumFloors()));
                return;
            }
            controller.addRequest(floor, 0, dir);
            sendResponse(clientSocket, "Elevator requested at floor " + std::to_string(floor) + 
                                      " going " + dirStr);
        } else {
            // std::cout << "Invalid call command format" << std::endl;
            sendResponse(clientSocket, "Invalid command format. Use 'call <floor> <direction>'");
        }
    } else if (cmd == "go") {
        int floor;
        
        if (iss >> floor) {
            if (floor < 1 || floor > controller.getNumFloors()) {
                sendResponse(clientSocket, "Invalid floor number. Floors must be between 1 and " + std::to_string(controller.getNumFloors()));
                return;
            }
            
            // Get all available elevators
            auto statuses = controller.getElevatorStatuses();
            
            // Find the best elevator to use - closest idle one
            int bestElevatorId = -1;
            int bestElevatorFloor = -1;
            int shortestDistance = std::numeric_limits<int>::max();
            
            for (const auto& [id, currentFloor, destFloor, direction, status] : statuses) {
                if (status == ElevatorStatus::IDLE || status == ElevatorStatus::STOPPED) {
                    int distance = std::abs(currentFloor - floor);
                    if (distance < shortestDistance) {
                        shortestDistance = distance;
                        bestElevatorId = id;
                        bestElevatorFloor = currentFloor;
                    }
                }
            }
            
            if (bestElevatorId >= 0) {
                // Use the best elevator
                controller.addRequest(bestElevatorFloor, floor, 
                    floor > bestElevatorFloor ? Direction::UP : Direction::DOWN);
                sendResponse(clientSocket, "Elevator #" + std::to_string(bestElevatorId) + 
                                         " will go to floor " + std::to_string(floor));
            } else {
                sendResponse(clientSocket, "No idle elevator available. Try again later.");
            }
        } else {
            sendResponse(clientSocket, "Invalid command format. Use 'go <floor>'");
        }
    } else if (cmd == "stop") {
        controller.emergencyStop();
        sendResponse(clientSocket, "EMERGENCY STOP activated for all elevators!");
    } else if (cmd == "release") {
        controller.releaseEmergencyStop();
        sendResponse(clientSocket, "Emergency stop released. Elevators returning to normal operation.");
    } else if (cmd == "status") {
        sendResponse(clientSocket, getElevatorStatusJson());
    } else if (cmd == "exit") {
        sendResponse(clientSocket, "Goodbye!");
        return;
    } else {
        sendResponse(clientSocket, "Unknown command '" + cmd + "'. Type 'help' for available commands.");
    }
}

void ElevatorServer::sendResponse(int clientSocket, const std::string& response) {
    std::string fullResponse = response + "\n";
    // Remove debug output for cleaner terminal
    // std::cout << "Sending response to client " << clientSocket << ": " << response << std::endl;
    ssize_t bytesSent = write(clientSocket, fullResponse.c_str(), fullResponse.length());
    if (bytesSent < 0) {
        std::cerr << "Error sending response: " << strerror(errno) << std::endl;
    } 
    // else {
    //     std::cout << "Sent " << bytesSent << " bytes to client " << clientSocket << std::endl;
    // }
}

std::string ElevatorServer::getElevatorStatusJson() const {
    std::ostringstream oss;
    auto statuses = controller.getElevatorStatuses();
    
    oss << "Elevator Statuses:\n";
    oss << "ID | Current Floor | Destination | Direction | Status\n";
    oss << "----------------------------------------------------\n";
    
    for (const auto& [id, currentFloor, destFloor, direction, status] : statuses) {
        std::string dirStr;
        switch (direction) {
            case Direction::IDLE: dirStr = "Idle"; break;
            case Direction::UP: dirStr = "Up"; break;
            case Direction::DOWN: dirStr = "Down"; break;
        }
        
        std::string statusStr;
        switch (status) {
            case ElevatorStatus::IDLE: statusStr = "Idle"; break;
            case ElevatorStatus::MOVING: statusStr = "Moving"; break;
            case ElevatorStatus::STOPPED: statusStr = "Stopped"; break;
            case ElevatorStatus::EMERGENCY: statusStr = "EMERGENCY"; break;
        }
        
        oss << id << " | " 
            << currentFloor << " | " 
            << (direction == Direction::IDLE ? "--" : std::to_string(destFloor)) << " | "
            << dirStr << " | " 
            << statusStr << "\n";
    }
    
    return oss.str();
} 