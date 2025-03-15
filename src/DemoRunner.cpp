#include "DemoRunner.h"
#include <iostream>
#include <random>

DemoRunner::DemoRunner(ElevatorController& controller)
    : controller(controller), running(false) {
    initializeDemoSteps();
}

DemoRunner::~DemoRunner() {
    stop();
}

void DemoRunner::start() {
    if (running) {
        return;
    }
    
    running = true;
    demoThread = std::thread(&DemoRunner::runDemo, this);
}

void DemoRunner::stop() {
    if (!running) {
        return;
    }
    
    running = false;
    
    if (demoThread.joinable()) {
        demoThread.join();
    }
}

bool DemoRunner::isRunning() const {
    return running;
}

void DemoRunner::runDemo() {
    std::cout << "\n=== Starting Automated Demo ===" << std::endl;
    std::cout << "The system will automatically execute a series of elevator commands." << std::endl;
    std::cout << "Press Ctrl+C to exit the demo at any time." << std::endl;
    std::cout << std::endl;
    
    // Wait a moment before starting
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    // Run through all demo steps
    for (const auto& step : demoSteps) {
        if (!running) {
            break;
        }
        
        // Print step description
        std::cout << "\n[DEMO] " << step.description << std::endl;
        
        // Execute the action
        step.action();
        
        // Wait for the specified delay
        std::this_thread::sleep_for(std::chrono::milliseconds(step.delayAfterMs));
    }
    
    std::cout << "\n=== Demo Completed ===" << std::endl;
    std::cout << "You can now interact with the system manually or exit." << std::endl;
    
    running = false;
}

void DemoRunner::initializeDemoSteps() {
    // Get the number of elevators and floors
    int numElevators = controller.getNumElevators();
    int numFloors = controller.getNumFloors();
    
    // Create a random number generator for more varied demonstrations
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> floorDist(1, numFloors);
    
    // PHASE 1: Initial demonstration of individual elevator movements
    // Step 1: Call elevator to floor 3 going up
    demoSteps.emplace_back(
        "Calling elevator to floor 3 (going up)",
        [this]() { controller.addRequest(3, 0, Direction::UP); },
        5000  // Wait 5 seconds for elevator to arrive
    );
    
    // Step 2: From floor 3, go to floor 8
    demoSteps.emplace_back(
        "Setting destination to floor 8",
        [this]() { controller.addRequest(3, 8, Direction::UP); },
        8000  // Wait 8 seconds for elevator to arrive
    );
    
    // PHASE 2: Multiple elevators working simultaneously
    // Step 3: Dispatch multiple elevators to different floors simultaneously
    demoSteps.emplace_back(
        "Dispatching all elevators to different floors simultaneously",
        [this, numElevators, numFloors, floorDist, gen]() mutable {
            // Create a set of random floor requests, one for each elevator
            for (int i = 0; i < numElevators; i++) {
                int targetFloor = floorDist(gen);
                Direction dir = (targetFloor > 1) ? Direction::DOWN : Direction::UP;
                controller.addRequest(targetFloor, 0, dir);
                std::cout << "  - Calling elevator to floor " << targetFloor << std::endl;
            }
        },
        10000  // Wait 10 seconds for elevators to start moving
    );
    
    // Step 4: Set destinations for all elevators
    demoSteps.emplace_back(
        "Setting destinations for all elevators",
        [this, floorDist, gen]() mutable {
            auto statuses = controller.getElevatorStatuses();
            for (const auto& [id, currentFloor, destFloor, direction, status] : statuses) {
                if (status == ElevatorStatus::IDLE || status == ElevatorStatus::STOPPED) {
                    int targetFloor = floorDist(gen);
                    // Make sure target is different from current floor
                    while (targetFloor == currentFloor) {
                        targetFloor = floorDist(gen);
                    }
                    Direction dir = (targetFloor > currentFloor) ? Direction::UP : Direction::DOWN;
                    controller.addRequest(currentFloor, targetFloor, dir);
                    std::cout << "  - Elevator #" << id << " at floor " << currentFloor 
                              << " going to floor " << targetFloor << std::endl;
                }
            }
        },
        15000  // Wait 15 seconds for elevators to complete their journeys
    );
    
    // PHASE 3: Demonstrate emergency stop and release
    // Step 5: Trigger emergency stop while elevators are moving
    demoSteps.emplace_back(
        "Triggering EMERGENCY STOP for all elevators",
        [this, numFloors]() { 
            // First, make sure elevators are moving
            auto statuses = controller.getElevatorStatuses();
            for (const auto& [id, currentFloor, destFloor, direction, status] : statuses) {
                if (status == ElevatorStatus::IDLE) {
                    int targetFloor = (currentFloor < numFloors) ? currentFloor + 1 : currentFloor - 1;
                    Direction dir = (targetFloor > currentFloor) ? Direction::UP : Direction::DOWN;
                    controller.addRequest(currentFloor, targetFloor, dir);
                }
            }
            
            // Wait a moment for elevators to start moving
            std::this_thread::sleep_for(std::chrono::seconds(2));
            
            // Now trigger emergency stop
            controller.emergencyStop();
        },
        5000  // Wait 5 seconds in emergency state
    );
    
    // Step 6: Release emergency stop
    demoSteps.emplace_back(
        "Releasing emergency stop",
        [this]() { controller.releaseEmergencyStop(); },
        5000  // Wait 5 seconds after release
    );
    
    // PHASE 4: Complex traffic patterns
    // Step 7: Simulate morning up-peak traffic (everyone going up)
    demoSteps.emplace_back(
        "Simulating morning up-peak traffic (ground floor to upper floors)",
        [this, numElevators, numFloors, floorDist, gen]() mutable {
            // Multiple people calling elevators from ground floor
            for (int i = 0; i < numElevators * 2; i++) {
                controller.addRequest(1, 0, Direction::UP);
                std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Stagger requests
            }
            
            // Wait for elevators to arrive
            std::this_thread::sleep_for(std::chrono::seconds(5));
            
            // Now set destinations to upper floors
            auto statuses = controller.getElevatorStatuses();
            for (const auto& [id, currentFloor, destFloor, direction, status] : statuses) {
                if (currentFloor == 1) {
                    // Distribute passengers to different upper floors
                    std::uniform_int_distribution<> upperFloorDist(numFloors/2, numFloors);
                    int targetFloor = upperFloorDist(gen);
                    controller.addRequest(1, targetFloor, Direction::UP);
                    std::cout << "  - Passenger in elevator #" << id << " going to floor " << targetFloor << std::endl;
                }
            }
        },
        20000  // Wait 20 seconds for this complex pattern
    );
    
    // Step 8: Simulate evening down-peak traffic (everyone going down)
    demoSteps.emplace_back(
        "Simulating evening down-peak traffic (upper floors to ground floor)",
        [this, numElevators, numFloors, floorDist, gen]() mutable {
            // Multiple people calling elevators from upper floors
            for (int i = 0; i < numElevators * 2; i++) {
                std::uniform_int_distribution<> upperFloorDist(numFloors/2, numFloors);
                int fromFloor = upperFloorDist(gen);
                controller.addRequest(fromFloor, 0, Direction::DOWN);
                std::cout << "  - Calling elevator to floor " << fromFloor << " (going down)" << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Stagger requests
            }
            
            // Wait for elevators to arrive
            std::this_thread::sleep_for(std::chrono::seconds(8));
            
            // Now set destinations to ground floor
            auto statuses = controller.getElevatorStatuses();
            for (const auto& [id, currentFloor, destFloor, direction, status] : statuses) {
                if (currentFloor > numFloors/2) {
                    controller.addRequest(currentFloor, 1, Direction::DOWN);
                    std::cout << "  - Passenger in elevator #" << id << " going to ground floor" << std::endl;
                }
            }
        },
        20000  // Wait 20 seconds for this complex pattern
    );
    
    // PHASE 5: Stress test with random requests
    // Step 9: Generate many random requests to stress test the system
    demoSteps.emplace_back(
        "Stress testing with many random requests",
        [this, numFloors, floorDist, gen]() mutable {
            // Generate a bunch of random requests
            for (int i = 0; i < 10; i++) {
                int fromFloor = floorDist(gen);
                int toFloor;
                do {
                    toFloor = floorDist(gen);
                } while (toFloor == fromFloor);
                
                Direction dir = (toFloor > fromFloor) ? Direction::UP : Direction::DOWN;
                
                // First call the elevator
                controller.addRequest(fromFloor, 0, dir);
                std::cout << "  - Request " << (i+1) << ": Calling elevator to floor " << fromFloor << std::endl;
                
                // Wait a short random time between requests
                std::this_thread::sleep_for(std::chrono::milliseconds(300 + (gen() % 700)));
            }
        },
        30000  // Wait 30 seconds for the system to handle all these requests
    );
    
    // Step 10: Final reset - send all elevators back to ground floor
    demoSteps.emplace_back(
        "Resetting all elevators to ground floor",
        [this]() {
            auto statuses = controller.getElevatorStatuses();
            for (const auto& [id, currentFloor, destFloor, direction, status] : statuses) {
                if (currentFloor != 1) {
                    controller.addRequest(currentFloor, 1, Direction::DOWN);
                    std::cout << "  - Sending elevator #" << id << " back to ground floor" << std::endl;
                }
            }
        },
        15000  // Wait 15 seconds for elevators to return to ground floor
    );
}