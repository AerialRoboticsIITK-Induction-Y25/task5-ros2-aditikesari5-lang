#include <rclcpp/rclcpp.hpp>
#include "task5-ros2-aditikesari5-lang/health_monitor_node.hpp"

/**
 * Health Monitor Main Entry Point
 * 
 * Executed when you run:
 * ros2 run task5-ros2-aditikesari5-lang health_monitor_node
 */
int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<HealthMonitorNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
