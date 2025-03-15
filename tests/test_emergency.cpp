#include <gtest/gtest.h>
#include "ElevatorController.h"
#include <thread>
#include <chrono>

class EmergencyTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up code
    }
    
    void TearDown() override {
        // Tear down code
    }
};

TEST_F(EmergencyTest, EmergencyStopAllElevators) {
    ElevatorController controller(3, 10);
    controller.start();
    
    // Add requests to all elevators
    controller.addRequest(1, 10, Direction::UP);
    controller.addRequest(1, 8, Direction::UP);
    controller.addRequest(1, 6, Direction::UP);
    
    // Wait for elevators to start moving
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Trigger emergency stop
    controller.emergencyStop();
    
    // Wait for emergency stop to take effect
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Check that all elevators are in emergency state
    auto statuses = controller.getElevatorStatuses();
    
    for (const auto& [id, currentFloor, destFloor, direction, status] : statuses) {
        EXPECT_EQ(status, ElevatorStatus::EMERGENCY);
    }
    
    controller.stop();
}

TEST_F(EmergencyTest, NoRequestsDuringEmergency) {
    // Create controller with 1 elevator and 10 floors
    ElevatorController controller(1, 10);
    
    // Start the controller
    controller.start();
    
    // Wait for controller to initialize
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Trigger emergency stop
    controller.emergencyStop();
    
    // Wait for emergency stop to take effect
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Add a request during emergency
    controller.addRequest(1, 5, Direction::UP);
    
    // Wait a bit to ensure request is processed
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Check that elevator is still in emergency state and hasn't moved
    auto statuses = controller.getElevatorStatuses();
    
    // Make sure we have at least one elevator status
    ASSERT_GT(statuses.size(), 0);
    
    auto [id, currentFloor, destFloor, direction, status] = statuses[0];
    
    EXPECT_EQ(status, ElevatorStatus::EMERGENCY);
    EXPECT_EQ(currentFloor, 1);
    
    // Stop the controller
    controller.stop();
    
    // Wait for controller to stop
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
} 