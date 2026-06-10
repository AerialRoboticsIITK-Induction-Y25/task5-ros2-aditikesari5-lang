#ifndef DRONE_NODE_HPP
#define DRONE_NODE_HPP

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <chrono>
#include <memory>
#include "header.hpp"

/**
 * @class DroneNode
 * @brief A ROS 2 node representing a single drone
 * 
 * This node:
 * - Publishes drone status every 1 second to /drone/<name>/status
 * - Publishes telemetry as JSON every 2 seconds
 * - Publishes alerts when battery is critical
 * - Accepts ROS 2 parameters: drone_name, initial_battery, mission_name
 * - Manages a MissionDrone object internally
 */
class DroneNode : public rclcpp::Node {
public:
    /**
     * @brief Constructor - initializes the drone node
     */
    DroneNode();

private:
    // ROS 2 publishers
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr status_publisher_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr telemetry_publisher_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr alert_publisher_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr mission_complete_publisher_;

    // ROS 2 timers
    rclcpp::TimerBase::SharedPtr status_timer_;
    rclcpp::TimerBase::SharedPtr telemetry_timer_;

    // Drone object (internal state)
    std::unique_ptr<MissionDrone> drone_;

    // Parameters
    std::string drone_name_;
    double initial_battery_;
    std::string mission_name_;

    // State tracking
    int publish_count_; // Count publishes to advance waypoint every 3 publishes
    bool mission_running_;
    bool was_critical_;

    // Callback functions (called by timers)
    void status_timer_callback();
    void telemetry_timer_callback();

    // Helper functions
    std::string generate_telemetry_json();
    void advance_mission();
};

#endif // DRONE_NODE_HPP
