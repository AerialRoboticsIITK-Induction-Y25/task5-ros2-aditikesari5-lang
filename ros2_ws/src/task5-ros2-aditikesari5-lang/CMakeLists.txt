cmake_minimum_required(VERSION 3.8)
project(task5-ros2-aditikesari5-lang)

# Set C++ standard
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

# Add compiler warnings
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ============================================================================
# PART #1: Find ROS 2 Dependencies
# ============================================================================
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(std_srvs REQUIRED)  # new for services
# ============================================================================
# Include directories (where to find header files)
# ============================================================================
include_directories(include)

# ============================================================================
# PART #2&3: Add the Executables and set dependencies
# ============================================================================
add_executable(drone_node src/drone_node.cpp src/drone_main.cpp)
ament_target_dependencies(drone_node rclcpp std_msgs)

# Executable 2: Fleet Manager Node
add_executable(fleet_manager_node src/fleet_manager_node.cpp  src/fleet_manager_main.cpp)
ament_target_dependencies(fleet_manager_node rclcpp std_msgs std_srvs)

# Executable 3: Health Monitor Node
add_executable(health_monitor_node src/health_monitor_node.cpp src/health_monitor_main.cpp)
ament_target_dependencies(health_monitor_node rclcpp std_msgs)


# ============================================================================
# PART #4: Install Rules
# ============================================================================
install(TARGETS 
  drone_node
  fleet_manager_node
  health_monitor_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION include/
)

install(DIRECTORY launch/
  DESTINATION share/${PROJECT_NAME}/launch
)

# ============================================================================
# PART #5: Package Registration
# ============================================================================
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()
