#pragma once

#include "ElevatorController.h"
#include <string>
#include <thread>
#include <mutex>
#include <vector>
#include <atomic>
#include <functional>
#include <unordered_map>
#include <netinet/in.h>

class ElevatorServer {
private:
    ElevatorController& controller;
    std::atomic<bool> running;
    int port;
    int serverSocket;
    std::thread serverThread;
    std::mutex clientsMutex;
    std::vector<std::thread> clientThreads;
    std::unordered_map<int, bool> activeClients;  // socket fd -> active status
    
    void serverLoop();
    void handleClient(int clientSocket);
    void processCommand(int clientSocket, const std::string& command);
    void sendResponse(int clientSocket, const std::string& response);
    std::string getElevatorStatusJson() const;
    
public:
    ElevatorServer(ElevatorController& controller, int port = 8081);
    ~ElevatorServer();
    
    bool start();
    void stop();
    bool isRunning() const { return running; }
}; 