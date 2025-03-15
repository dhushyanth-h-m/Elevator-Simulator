#include "ElevatorController.h"
#include "UserInterface.h"
#include "DemoRunner.h"
#include "ElevatorServer.h"
#include <iostream>
#include <string>
#include <csignal>

// Global controller for signal handling
ElevatorController* globalController = nullptr;
ElevatorServer* globalServer = nullptr;

// Signal handler for graceful shutdown
void signalHandler(int signal) {
    std::cout << "\nReceived signal " << signal << ". Shutting down..." << std::endl;
    
    if (globalServer) {
        globalServer->stop();
    }
    
    if (globalController) {
        globalController->stop();
    }
    
    exit(signal);
}

int main(int argc, char* argv[]) {
    int numElevators = 3;
    int numFloors = 10;
    bool runDemo = false;
    bool enableServer = true;  // Enable server by default
    int serverPort = 8081;      // Default server port
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "--elevators" && i + 1 < argc) {
            numElevators = std::stoi(argv[++i]);
        } else if (arg == "--floors" && i + 1 < argc) {
            numFloors = std::stoi(argv[++i]);
        } else if (arg == "--demo") {
            runDemo = true;
        } else if (arg == "--no-server") {
            enableServer = false;
        } else if (arg == "--port" && i + 1 < argc) {
            serverPort = std::stoi(argv[++i]);
        } else if (arg == "--help") {
            std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
            std::cout << "Options:" << std::endl;
            std::cout << "  --elevators N    Set number of elevators (default: 3)" << std::endl;
            std::cout << "  --floors N       Set number of floors (default: 10)" << std::endl;
            std::cout << "  --demo           Run automated demo instead of interactive mode" << std::endl;
            std::cout << "  --no-server      Disable the network server" << std::endl;
            std::cout << "  --port N         Set server port (default: 8081)" << std::endl;
            std::cout << "  --help           Display this help message" << std::endl;
            return 0;
        }
    }
    
    // Validate input
    if (numElevators < 1) {
        std::cerr << "Error: Number of elevators must be at least 1" << std::endl;
        return 1;
    }
    
    if (numFloors < 2) {
        std::cerr << "Error: Number of floors must be at least 2" << std::endl;
        return 1;
    }
    
    // Set up signal handlers
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    try {
        // Create elevator controller
        ElevatorController controller(numElevators, numFloors);
        globalController = &controller;
        
        // Start the controller
        controller.start();
        
        // Start the network server if enabled
        ElevatorServer* server = nullptr;
        if (enableServer) {
            server = new ElevatorServer(controller, serverPort);
            globalServer = server;
            
            if (!server->start()) {
                std::cerr << "Failed to start elevator server on port " << serverPort << std::endl;
                return 1;
            }
            
            std::cout << "Elevator server started on port " << serverPort << std::endl;
            std::cout << "Connect with: ./elevator_client --port " << serverPort << std::endl;
        }
        
        if (runDemo) {
            // Run in demo mode
            std::cout << "Starting elevator simulation in DEMO mode with " << numElevators 
                      << " elevators and " << numFloors << " floors..." << std::endl;
            
            // Create user interface (for display only)
            UserInterface ui(controller);
            ui.start();
            
            // Create and start demo runner
            DemoRunner demo(controller);
            demo.start();
            
            // Wait for demo to complete or user to exit
            while (ui.isRunning() || demo.isRunning()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        } else {
            // Run in interactive mode
            std::cout << "Starting elevator simulation with " << numElevators 
                      << " elevators and " << numFloors << " floors..." << std::endl;
            
            // Create user interface
            UserInterface ui(controller);
            ui.start();
            
            // Wait for UI to exit
            while (ui.isRunning()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        
        // Clean up
        if (server) {
            server->stop();
            delete server;
            globalServer = nullptr;
        }
        
        controller.stop();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
} 