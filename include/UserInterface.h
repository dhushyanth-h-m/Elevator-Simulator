#pragma once

#include "ElevatorController.h"
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <functional>

class UserInterface {
private:
    ElevatorController& controller;
    std::thread inputThread;
    std::thread displayThread;
    std::atomic<bool> running;
    std::mutex displayMutex;
    
    void inputLoop();
    void displayLoop();
    void displayStatus();
    void displayHelp();
    void processCommand(const std::string& command);
    
public:
    UserInterface(ElevatorController& elevatorController);
    ~UserInterface();
    
    void start();
    void stop();
    
    bool isRunning() const {
        return running;
    }
}; 