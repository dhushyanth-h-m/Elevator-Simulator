# ElevatorControlSim

A sophisticated multi-elevator control system simulation featuring realistic elevator physics, multi-client support, and database synchronization.

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [System Architecture](#system-architecture)
- [Requirements](#requirements)
- [Installation](#installation)
- [Building from Source](#building-from-source)
- [Usage](#usage)
  - [Command Line Arguments](#command-line-arguments)
  - [Interactive Commands](#interactive-commands)
  - [Client Options](#client-options)
- [Using Docker](#using-docker)
- [PostgreSQL Setup](#postgresql-setup)
  - [Prerequisites](#prerequisites)
  - [Setup](#setup)
  - [Running Multiple Synchronized Instances](#running-multiple-synchronized-instances)
- [Testing](#testing)
- [Configuration](#configuration)
- [Technical Details](#technical-details)
  - [Elevator Physics](#elevator-physics)
  - [Scheduling Algorithm](#scheduling-algorithm)
  - [Database Integration](#database-integration)
- [Multi-Terminal Testing](#multi-terminal-testing)
- [Development](#development)
  - [Project Structure](#project-structure)
  - [Git Workflow](#git-workflow)
  - [CI/CD Pipeline](#ci/cd-pipeline)
  - [Docker Development](#docker-development)
  - [Key Classes](#key-classes)
- [Contributing](#contributing)
  - [Contribution Guidelines](#contribution-guidelines)
  - [Development Environment Setup](#development-environment-setup)
- [License](#license)

## Overview

The Elevator Control Simulation is a comprehensive C++ application that simulates a real-world elevator system. It models multiple elevators serving requests across various floors, with realistic timing for floor travel and door operations. The system implements intelligent request scheduling to optimize elevator efficiency and provides multiple interfaces for interaction including a terminal UI, network API, and database synchronization.

Perfect for:
- Educational purposes demonstrating concurrency concepts
- Testing elevator scheduling algorithms
- Architectural studies of distributed systems
- Computer science coursework on discrete-event simulations

## Features

- **Multi-elevator management**: Handle any number of elevators simultaneously
- **Realistic physics**: Configurable travel times, door operations, and acceleration
- **Intelligent scheduling**: Implementation of the nearest car dispatch algorithm for optimal elevator assignment
- **Emergency operations**: Support for emergency stops and system override
- **Network API**: TCP socket-based interface for remote control and monitoring
- **Database integration**: PostgreSQL logging and state persistence
- **Multi-terminal support**: Connect multiple clients simultaneously
- **Interactive UI**: Terminal-based visualization of elevator states
- **Automated testing**: Specialized testing tools and scripts

## System Architecture

The system is built on several key components:

- **ElevatorController**: Central coordination system that manages elevators and dispatches requests
- **Elevator**: Individual elevator units with independent state management
- **ElevatorServer**: Network interface providing TCP socket-based API
- **DatabaseLogger**: Persistence and synchronization via PostgreSQL
- **UserInterface**: Terminal-based visualization and interactive control
- **DemoRunner**: Automated demonstration capabilities

The architecture follows a multi-threaded design where each elevator operates in its own thread, controlled by a central dispatcher that optimizes request handling based on elevator position, direction, and load.

## Requirements

- C++17 compatible compiler (GCC 7+, Clang 5+, or MSVC 19.14+)
- CMake 3.10 or higher
- PostgreSQL 12+ with development libraries
- libpqxx (PostgreSQL C++ client library)
- POSIX-compatible operating system (Linux, macOS, or Windows with WSL)

## Installation

### Dependencies

- libpqxx (PostgreSQL C++ client library)
- GoogleTest (for testing)
- Standard POSIX networking libraries

Install dependencies:

```bash
# On macOS with Homebrew
brew install postgresql libpqxx cmake

# On Ubuntu/Debian
sudo apt-get install postgresql postgresql-contrib libpqxx-dev cmake
```

## Building from Source

```bash
# Clone the repository
git clone 
cd ElevatorControlSim

# Create build directory
mkdir -p build
cd build

# Configure
cmake ..

# Build
make

# Run tests (optional)
make test
```

## Usage

### Running the Server

```bash
# Start with default settings (3 elevators, 10 floors)
./elevator_sim

# Customize configuration
./elevator_sim --elevators 5 --floors 20 --port 8081
```

### Command Line Arguments

| Argument | Description | Default |
|----------|-------------|---------|
| `--elevators N` | Number of elevators to simulate | 3 |
| `--floors N` | Number of floors in the building | 10 |
| `--port N` | TCP port for the server | 8081 |
| `--demo` | Run automated demonstration | Off |
| `--no-server` | Disable network server | Server enabled |
| `--help` | Show help message | - |

### Interactive Commands

When running in interactive mode, the following commands are available:

| Command | Description | Example |
|---------|-------------|---------|
| `call <floor> <direction>` | Request an elevator to a floor | `call 5 up` |
| `go <floor>` | Set destination floor (when inside elevator) | `go 10` |
| `status` | Show current status of all elevators | `status` |
| `stop` | Emergency stop the current elevator | `stop` |
| `release` | Release from emergency state | `release` |
| `help` | Display help message | `help` |
| `exit` | Exit the simulation | `exit` |

### Client Options

Connect to the simulation using the provided client:

```bash
# Connect to local server with default settings
./elevator_client

# Connect to a specific server/port
./elevator_client --server 192.168.1.100 --port 8081
```

## Using Docker

```bash
# Build and run with Docker Compose
docker-compose up
```

## PostgreSQL Setup

This project uses PostgreSQL for database logging and synchronization between multiple instances.

### Prerequisites

- PostgreSQL 12 or higher
- libpqxx (PostgreSQL C++ client library)

### Setup

1. Install PostgreSQL:
   ```bash
   # On macOS with Homebrew
   brew install postgresql
   
   # On Ubuntu/Debian
   sudo apt-get install postgresql postgresql-contrib
   ```

2. Install libpqxx:
   ```bash
   # On macOS with Homebrew
   brew install libpqxx
   
   # On Ubuntu/Debian
   sudo apt-get install libpqxx-dev
   ```

3. Run the database setup script:
   ```bash
   cd db
   chmod +x setup.sh
   ./setup.sh
   ```

4. Start PostgreSQL service:
   ```bash
   # On macOS with Homebrew
   brew services start postgresql
   
   # On Ubuntu/Debian
   sudo service postgresql start
   ```

### Running Multiple Synchronized Instances

You can run multiple instances of the elevator simulation that will stay synchronized through the PostgreSQL database:

```bash
# Terminal 1
./elevator_sim --demo

# Terminal 2
./elevator_sim --demo

# Terminal 3
./elevator_sim
```

All instances will share the same elevator states and respond to requests from any instance.

## Testing

The system includes comprehensive testing capabilities:

### Unit Tests

Run the built-in unit tests to verify core functionality:

```bash
cd build
./tests/elevator_tests
```

### Multi-Terminal Testing

The multi-terminal test script simulates multiple clients making concurrent requests:

```bash
# Run 5 clients making 8 requests each
python tests/multi_terminal_test.py

# Customize testing parameters
python tests/multi_terminal_test.py --clients 10 --requests 20 --server 192.168.1.100 --port 8081
```

### Test Script Options

| Option | Description | Default |
|--------|-------------|---------|
| `--server` | Server address | 127.0.0.1 |
| `--port` | Server port | 8081 |
| `--clients` | Number of client terminals to simulate | 5 |
| `--requests` | Requests per client | 8 |
| `--floors` | Max floor number for random requests | 15 |

## Configuration

Key configuration parameters can be found in the following files:

1. `include/Elevator.h`:
   - `FLOOR_TRAVEL_TIME_MS`: Time in milliseconds to travel between floors (default: 1000ms)
   - `DOOR_OPERATION_TIME_MS`: Time for doors to open/close (default: 1000ms)

2. `include/ElevatorServer.h`:
   - Default server port (8081)

3. `src/DatabaseLogger.cpp`:
   - Database connection settings

4. `src/UserInterface.cpp`:
   - `DISPLAY_REFRESH_RATE_MS`: Status display refresh rate (synchronized to 1000ms)

### Elevator Physics

The simulation models realistic elevator behavior:

- **Travel Time**: Each elevator takes 1 second to travel between adjacent floors
- **Door Operation**: Door opening and closing operations take 1 second each
- **Acceleration**: Gradual speed changes when starting and stopping
- **Capacity Limits**: Configurable elevator capacity with weight distribution

### Display Synchronization

The elevator status display is synchronized with the elevator movement:

- **Server Updates**: The server updates elevator status information every 1 second
- **UI Refresh**: The terminal display refreshes every 1 second to match elevator movement
- **Client Polling**: Network clients receive updates every 1 second
- **Database Sync**: Database synchronization occurs with each status change

This synchronization ensures smooth visualization of elevator movement across all interfaces, with elevators visibly moving one floor per second in real-time.

### Scheduling Algorithm

The elevator dispatching algorithm uses the nearest car dispatch approach:

1. For each new request, calculate the estimated time for each elevator to serve it
2. Consider current position, direction, and existing queue of each elevator
3. Assign request to the elevator that can serve it with minimal delay
4. Handle special cases like emergency prioritization and building capacity limits

### Database Integration

The system connects to PostgreSQL for:

- Event logging (calls, movement, errors)
- State persistence across restarts
- Real-time synchronization between multiple instances
- Historical data analysis

## Multi-Terminal Testing

The included Python-based multi-terminal testing script (`multi_terminal_test.py`) allows comprehensive testing of the system under concurrent load. Each simulated client:

1. Connects to the elevator server
2. Makes a configurable number of random requests
3. Reports timing and responses with real-time updates every second to match elevator movement
4. Uses color-coded output for easy monitoring

Example output:
```
[23:16:03.099] Client 0 - Response:
Elevator Statuses:
ID | Current Floor | Destination | Direction | Status
----------------------------------------------------
0 | 1 | -- | Idle | Idle
1 | 1 | -- | Idle | Idle
2 | 1 | -- | Idle | Idle
```

## Development

### Project Structure

```
ElevatorControlSim/
├── include/              # Header files
├── src/                  # Source files
├── tests/                # Test code
├── build/                # Build artifacts (generated)
├── docs/                 # Documentation
├── db/                   # Database scripts
├── docker/               # Docker configuration
├── .github/              # CI/CD configuration
└── scripts/              # Utility scripts
```

### Git Workflow

This project uses Git for version control. Here's how to get started:

```bash
# Clone the repository
git clone 

# Create a feature branch
git checkout -b feature/new-feature

# Make changes and commit
git add .
git commit -m "Add my new feature"

# Push changes to remote repository
git push origin feature/new-feature

# Create a pull request
```

### CI/CD Pipeline

This project includes a continuous integration workflow that automatically:

1. Builds the project using CMake
2. Runs all tests
3. Builds Docker images (when merged to main)

The pipeline configuration can be found in the CI/CD configuration files.

### Docker Development

The project includes Docker support for consistent development environments:

```bash
# Build and start both application and database
docker-compose up

# Build only without starting
docker-compose build

# Run in background
docker-compose up -d

# View logs
docker-compose logs -f

# Stop services
docker-compose down
```

The Docker configuration uses a multi-stage build process to minimize image size and includes all dependencies needed to run the application.

### Key Classes

- `Elevator`: Models individual elevator behavior and state
- `ElevatorController`: Central coordinator managing multiple elevators
- `ElevatorServer`: Network interface for remote connections
- `DatabaseLogger`: Persistence and synchronization layer
- `UserInterface`: Terminal user interface
- `Request`: Data structure for elevator requests

## Contributing

Contributions to the Elevator Control Simulation project are welcome! Here's how you can contribute:

1. **Fork the repository**
2. **Create a branch** for your feature or bugfix (`git checkout -b feature/amazing-feature`)
3. **Commit your changes** (`git commit -am 'Add some amazing feature'`)
4. **Push to the branch** (`git push origin feature/amazing-feature`)
5. **Create a new Pull Request**

### Contribution Guidelines

- Follow the existing code style
- Write or update tests for your changes
- Update documentation as needed
- Make sure all tests pass before submitting your PR
- Include a clear description of your changes in the PR

### Development Environment Setup

For the best development experience, we recommend:

1. Using the Docker development environment
2. Installing the recommended extensions for your IDE:
   - For VSCode: C/C++ extension, CMake Tools
   - For CLion: Built-in CMake support
3. Setting up pre-commit hooks for code formatting

## License

This project is licensed under the MIT License - see the LICENSE file for details.

---

## About

This simulation was developed as a demonstration of concurrent systems programming using modern C++ techniques. It illustrates principles of real-time systems, resource scheduling, and distributed architecture.
