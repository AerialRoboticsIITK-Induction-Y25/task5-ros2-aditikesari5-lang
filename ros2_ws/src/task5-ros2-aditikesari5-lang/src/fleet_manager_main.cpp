#include <rclcpp/rclcpp.hpp>
#include "task5-ros2-aditikesari5-lang/fleet_manager_node.hpp"

/**
 * Fleet Manager Main Entry Point
 * 
 * Executed when you run:
 * ros2 run task5-ros2-aditikesari5-lang fleet_manager_node
 */
int main(int argc, char * argv[]) {
    // Initialize ROS 2
    rclcpp::init(argc, argv);

    // Create fleet manager node
    auto node = std::make_shared<FleetManagerNode>();

    // Spin (loop forever, processing callbacks)
    rclcpp::spin(node);

    // Shutdown
    rclcpp::shutdown();

    return 0;
}
