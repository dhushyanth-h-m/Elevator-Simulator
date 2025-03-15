#!/bin/bash

# PostgreSQL setup script for ElevatorControlSim (macOS version)

# Configuration
DB_NAME="elevator_db"
DB_USER="elevator_user"
DB_PASSWORD="secret"

# Check if PostgreSQL is installed
if ! command -v psql &> /dev/null; then
    echo "PostgreSQL is not installed. Please install PostgreSQL first with:"
    echo "brew install postgresql"
    exit 1
fi

# Check if PostgreSQL service is running
if ! pg_isready > /dev/null 2>&1; then
    echo "PostgreSQL service is not running. Starting it now..."
    brew services start postgresql
    # Wait for PostgreSQL to start
    sleep 5
fi

# Create user if it doesn't exist
echo "Creating database user..."
createuser --createdb $DB_USER 2>/dev/null || echo "User already exists"

# Set password for user
psql postgres -c "ALTER USER $DB_USER WITH PASSWORD '$DB_PASSWORD';" 2>/dev/null

# Create database if it doesn't exist
echo "Creating database..."
createdb -O $DB_USER $DB_NAME 2>/dev/null || echo "Database already exists"

# Connect to the database and create tables
echo "Setting up database schema..."
psql -d $DB_NAME << EOF
CREATE TABLE IF NOT EXISTS ElevatorLog (
    log_id       SERIAL PRIMARY KEY,
    elevator_id  INT,
    from_floor   INT,
    to_floor     INT,
    event_type   VARCHAR(50),
    event_time   TIMESTAMPTZ DEFAULT NOW()
);

CREATE TABLE IF NOT EXISTS ElevatorState (
    elevator_id    INT PRIMARY KEY,
    current_floor  INT NOT NULL,
    dest_floor     INT,
    direction      INT NOT NULL,
    status         INT NOT NULL,
    last_updated   TIMESTAMPTZ DEFAULT NOW()
);

CREATE INDEX IF NOT EXISTS idx_elevator_log_time ON ElevatorLog(event_time);
CREATE INDEX IF NOT EXISTS idx_elevator_log_id ON ElevatorLog(elevator_id);

GRANT ALL PRIVILEGES ON ALL TABLES IN SCHEMA public TO $DB_USER;
GRANT ALL PRIVILEGES ON ALL SEQUENCES IN SCHEMA public TO $DB_USER;
EOF

echo "PostgreSQL setup completed successfully!"
echo "Connection string: dbname=$DB_NAME user=$DB_USER password=$DB_PASSWORD host=localhost" 