#include <gtest/gtest.h>
#include "Elevator.h"
#include <thread>
#include <chrono>

class ElevatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up code
    }
    
    void TearDown() override {
        // Tear down code
    }
};

TEST_F(ElevatorTest, InitialState) {
    Elevator elevator(1, 5);
    
    EXPECT_EQ(elevator.getId(), 1);
    EXPECT_EQ(elevator.getCurrentFloor(), 5);
    EXPECT_EQ(elevator.getDirection(), Direction::IDLE);
    EXPECT_EQ(elevator.getStatus(), ElevatorStatus::IDLE);
    EXPECT_TRUE(elevator.isIdle());
    EXPECT_FALSE(elevator.hasEmergencyStop());
}

TEST_F(ElevatorTest, AddRequest) {
    Elevator elevator(1);
    elevator.start();
    
    // Add a request to move to floor 5
    Request request(1, 5, Direction::UP);
    bool result = elevator.addRequest(request);
    
    EXPECT_TRUE(result);
    
    // Wait for elevator to start moving
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_EQ(elevator.getDirection(), Direction::UP);
    EXPECT_EQ(elevator.getStatus(), ElevatorStatus::MOVING);
    
    // Wait for elevator to reach destination
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    EXPECT_EQ(elevator.getCurrentFloor(), 5);
    EXPECT_EQ(elevator.getDirection(), Direction::IDLE);
    EXPECT_EQ(elevator.getStatus(), ElevatorStatus::IDLE);
    
    elevator.stop();
}

TEST_F(ElevatorTest, EmergencyStop) {
    Elevator elevator(1);
    elevator.start();
    
    // Add a request to move to floor 10
    Request request(1, 10, Direction::UP);
    elevator.addRequest(request);
    
    // Wait for elevator to start moving
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Trigger emergency stop
    elevator.emergencyStopActivate();
    
    EXPECT_TRUE(elevator.hasEmergencyStop());
    EXPECT_EQ(elevator.getStatus(), ElevatorStatus::EMERGENCY);
    
    // Try to add another request during emergency
    Request emergencyRequest(1, 3, Direction::DOWN);
    bool result = elevator.addRequest(emergencyRequest);
    
    EXPECT_FALSE(result);
    
    // Release emergency stop
    elevator.emergencyStopRelease();
    
    EXPECT_FALSE(elevator.hasEmergencyStop());
    EXPECT_EQ(elevator.getStatus(), ElevatorStatus::IDLE);
    
    elevator.stop();
}

TEST_F(ElevatorTest, CalculateDistance) {
    Elevator elevator(1, 5);
    
    // Distance from floor 5 to 8
    EXPECT_EQ(elevator.calculateDistance(8), 3);
    
    // Distance from floor 5 to 2
    EXPECT_EQ(elevator.calculateDistance(2), 3);
}