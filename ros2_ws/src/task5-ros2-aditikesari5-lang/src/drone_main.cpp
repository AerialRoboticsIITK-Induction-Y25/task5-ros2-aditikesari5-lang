#include <rclcpp/rclcpp.hpp>
#include "task5-ros2-aditikesari5-lang/drone_node.hpp"

/**
 * Main entry point for Drone Node
 * 
 * This is what runs when you execute:
 * ros2 run task5-ros2-aditikesari5-lang drone_node
 * 
 * What it does:
 * 1. Initialize ROS 2 environment
 * 2. Create a DroneNode instance
 * 3. Spin (loop forever, processing callbacks)
 * 4. Cleanup when user presses Ctrl+C
 */
int main(int argc, char * argv[]) {
    // Initialize ROS 2
    rclcpp::init(argc, argv);

    // Create the drone node
    auto node = std::make_shared<DroneNode>();

    // Spin the node (loop forever, processing callbacks)
    // When timers fire, callbacks execute
    rclcpp::spin(node);

    // Shutdown (runs when Ctrl+C is pressed)
    rclcpp::shutdown();

    return 0;
}
