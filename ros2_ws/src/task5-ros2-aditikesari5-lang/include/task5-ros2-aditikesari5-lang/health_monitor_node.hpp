#ifndef HEALTH_MONITOR_NODE_HPP
#define HEALTH_MONITOR_NODE_HPP

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <deque>
#include <map>
#include <string>

/**
 * @class HealthMonitorNode
 * @brief Monitors drone battery health and drain rates
 * 
 * Responsibilities:
 * - Subscribe to all drone telemetry topics
 * - Track last 10 battery samples per drone
 * - Calculate battery drain rate (% per second)
 * - Warn if drain rate exceeds 1.5% per second
 * - Publish health summary every 10 seconds
 * - Estimate time to critical battery
 * - Estimate time to complete depletion
 */
class HealthMonitorNode : public rclcpp::Node {
public:
    HealthMonitorNode();

private:
    // Data structure for tracking battery history
    struct BatteryHistory {
        std::deque<float> battery_levels;      // Last 10 samples
        std::deque<double> timestamps;         // Timestamps for each sample
        float drain_rate;                      // % per second
        float time_to_critical;                // Seconds until 20%
        float time_to_depletion;               // Seconds until 0%
        bool high_drain_warning;               // True if drain_rate > 1.5
    };

    // Store battery history for each drone
    std::map<std::string, BatteryHistory> drone_battery_history_;

    // ROS 2 Subscriptions
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr alpha_telemetry_sub_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr beta_telemetry_sub_;
    rclcpp::Subscription<std_msgs::msg::String>::SharedPtr gamma_telemetry_sub_;

    // ROS 2 Publishers
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr health_warning_pub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr health_summary_pub_;

    // ROS 2 Timer
    rclcpp::TimerBase::SharedPtr health_timer_;

    // Callback functions
    void on_telemetry_received(const std_msgs::msg::String &msg, 
                               const std::string &drone_name);
    void health_timer_callback();

    // Helper functions
    void update_battery_history(const std::string &drone_name, float battery);
    void calculate_drain_rate(const std::string &drone_name);
    void check_drain_warnings();
    std::string create_health_summary();
    std::string extract_battery_from_json(const std::string &json_str);
};

#endif // HEALTH_MONITOR_NODE_HPP
