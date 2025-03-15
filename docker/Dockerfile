FROM gcc:latest AS builder

# Install dependencies
RUN apt-get update && apt-get install -y \
    cmake \
    libpq-dev \
    libpqxx-dev \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Clean any previous build artifacts
RUN rm -rf build

# Build the application
RUN mkdir -p build && \
    cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    make elevator_sim elevator_client

# Runtime image
FROM debian:bullseye-slim

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libpq5 \
    libpqxx-6.4 \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy the built executable from the builder stage
COPY --from=builder /app/build/elevator_sim /app/

# Set environment variables
ENV DB_HOST=postgres
ENV DB_PORT=5432
ENV DB_NAME=elevator_db
ENV DB_USER=elevator_user
ENV DB_PASSWORD=secret
ENV CI=true

# Run the application
CMD ["./elevator_sim"] 