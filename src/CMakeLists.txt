# Collect all source files
file(GLOB SOURCES 
    "*.cpp"
)

# Create the main executable
add_executable(elevator_sim 
    ${SOURCES}
)

# Link with PostgreSQL and pqxx
target_link_libraries(elevator_sim 
    ${PostgreSQL_LIBRARIES}
    ${PQXX_LIBRARIES}
    pthread
)