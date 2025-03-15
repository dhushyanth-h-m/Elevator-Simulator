# ElevatorControlSim Design Document

## Architecture Overview

ElevatorControlSim is designed as a multi-threaded application that simulates a building's elevator system. The system consists of several key components:

1. **Elevator**: Represents a single elevator car with its own thread of execution.
2. **ElevatorController**: Manages multiple elevators and dispatches requests.
3. **DatabaseLogger**: Handles logging of events to a PostgreSQL database.
4. **UserInterface**: Provides a command-line interface for interaction.

## Thread Model

The application uses multiple threads to simulate concurrent operation:

- **Main Thread**: Handles initialization and cleanup.
- **Elevator Threads**: Each elevator runs in its own thread, processing requests independently.
- **Dispatcher Thread**: Monitors the request queue and assigns requests to elevators.
- **UI Threads**: Separate threads for input handling and display updates.

## Elevator Scheduling Algorithm

The system uses a "Nearest Car" dispatch algorithm:

1. When a request comes in, the controller calculates the distance from each elevator to the requested floor.
2. The elevator with the shortest distance is assigned to handle the request.
3. If multiple elevators have the same distance, the one with the lower ID is chosen.
4. If all elevators are in emergency stop mode, the request is queued until the emergency is cleared.

## Database Schema

The system logs events to a PostgreSQL database with the following schema:

```sql
CREATE TABLE ElevatorLog (
    log_id       SERIAL PRIMARY KEY,
    elevator_id  INT,
    from_floor   INT,
    to_floor     INT,
    event_type   VARCHAR(50),
    event_time   TIMESTAMPTZ DEFAULT NOW()
);
```

Event types include:
- CALL_REQUEST
- ELEVATOR_DISPATCHED
- ELEVATOR_ARRIVED
- DOOR_OPENED
- DOOR_CLOSED
- EMERGENCY_STOP
- EMERGENCY_RELEASED
- SYSTEM_STARTED
- SYSTEM_STOPPED

## Synchronization

The application uses several synchronization primitives:

- **Mutexes**: Protect shared data structures from concurrent access.
- **Condition Variables**: Signal threads when new requests are available or when state changes.
- **Atomic Variables**: Used for flags and simple state that needs to be thread-safe.

## Emergency Stop Mechanism

The emergency stop feature works as follows:

1. When triggered, all elevators are immediately notified via an atomic flag.
2. Each elevator checks this flag regularly and stops movement when it's set.
3. New requests are rejected or queued while in emergency mode.
4. When the emergency is cleared, elevators return to normal operation. 