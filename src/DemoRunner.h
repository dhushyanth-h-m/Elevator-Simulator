#pragma once

#include "ElevatorController.h"
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <functional>
#include <atomic>

// A class to run automated demos of the elevator system
class DemoRunner {
public:
    DemoRunner(ElevatorController& controller);
    ~DemoRunner();
    
    // Start the demo
    void start();
    
    // Stop the demo
    void stop();
    
    // Check if the demo is running
    bool isRunning() const;
    
private:
    struct DemoStep {
        std::string description;
        std::function<void()> action;
        int delayAfterMs;
        
        DemoStep(const std::string& desc, std::function<void()> act, int delay)
            : description(desc), action(act), delayAfterMs(delay) {}
    };
    
    ElevatorController& controller;
    std::vector<DemoStep> demoSteps;
    std::thread demoThread;
    std::atomic<bool> running;
    
    // The main demo loop
    void runDemo();
    
    // Initialize the demo steps
    void initializeDemoSteps();
}; 