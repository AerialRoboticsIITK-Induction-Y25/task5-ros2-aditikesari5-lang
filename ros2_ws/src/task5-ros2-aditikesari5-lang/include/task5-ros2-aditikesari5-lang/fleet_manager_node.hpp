#ifndef FLEET_MANAGER_NODE_HPP
#define FLEET_MANAGER_NODE_HPP

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <std_srvs/srv/trigger.hpp>
#include <map>
#include <string>
#include <sstream>
#include <iomanip>

/**
 * @class FleetManagerNode
 * @brief Central manager that monitors all drones
 * 
 * Responsibilities:
 * - Subscribe to all drone status/telemetry topics
 * - Parse drone data and maintain state
 * - Print fleet report every 5 seconds
 * - Handle alerts when drones have critical battery
 * - Provide on-demand status reports via service
 */
class FleetManagerNode : public rclcpp::Node {
public:
    /**
     * @brief Constructor - sets up subscriptions and timers
     */
    FleetManagerNode();

private:
    // Data structure to store info about each drone
    struct DroneStatus {
        std::string name;
        float battery;
        float altitude;
        std::string status;
        int current_waypoint;
        int total_waypoints;
        float speed;
        bool is_critical;
    };

    // Store status of each drone: key = drone name, value = DroneStatus
    std::map<std::string, DroneStatus> drone_states_;

    // ROS 2 Subscriptions (listen to drone topics)
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr alpha_status_sub_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr beta_status_sub_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr gamma_status_sub_;

    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr alpha_telemetry_sub_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr beta_telemetry_sub_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr gamma_telemetry_sub_;

    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr alert_sub_;

    // ROS 2 Timer (for periodic reporting)
    rclcpp::TimerBase::SharedPtr report_timer_;

    // ROS 2 Service
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr status_report_service_;

    // Callback functions for subscriptions
    void on_status_received(const std_msgs::msg::String &msg, const std::string &drone_name);
    void on_telemetry_received(const std_msgs::msg::String &msg, const std::string &drone_name);
    void on_alert_received(const std_msgs::msg::String &msg);

    // Callback for timer
    void report_timer_callback();

    // Service callback
    void on_status_report_request(
        const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
        std::shared_ptr<std_srvs::srv::Trigger::Response> response);

    // Helper functions
    std::string create_fleet_report();
    void parse_status_string(const std::string &status_str, const std::string &drone_name);
    void parse_telemetry_json(const std::string &json_str, const std::string &drone_name);
};

#endif // FLEET_MANAGER_NODE_HPP
