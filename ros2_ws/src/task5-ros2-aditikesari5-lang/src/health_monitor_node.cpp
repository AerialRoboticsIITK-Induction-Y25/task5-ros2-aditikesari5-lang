#include "task5-ros2-aditikesari5-lang/health_monitor_node.hpp"
#include <sstream>
#include <iomanip>
#include <cmath>

/**
 * HealthMonitorNode Constructor
 * 
 * What it does:
 * 1. Subscribe to telemetry from all three drones
 * 2. Create publishers for health warnings and summaries
 * 3. Create timer for periodic health checks (every 10 seconds)
 */
HealthMonitorNode::HealthMonitorNode() : Node("health_monitor_node") {
    RCLCPP_INFO(get_logger(), "Health Monitor Node started");

    // ========================================================================
    // Create Subscriptions for Telemetry Topics
    // ========================================================================
    // Listen for detailed drone state (including battery level)

    alpha_telemetry_sub_ = create_subscription<std_msgs::msg::String>(
        "/drone/Alpha/telemetry",
        10,
        [this](const std_msgs::msg::String &msg) {
            this->on_telemetry_received(msg, "Alpha");
        });

    beta_telemetry_sub_ = create_subscription<std_msgs::msg::String>(
        "/drone/Beta/telemetry",
        10,
        [this](const std_msgs::msg::String &msg) {
            this->on_telemetry_received(msg, "Beta");
        });

    gamma_telemetry_sub_ = create_subscription<std_msgs::msg::String>(
        "/drone/Gamma/telemetry",
        10,
        [this](const std_msgs::msg::String &msg) {
            this->on_telemetry_received(msg, "Gamma");
        });

    // ========================================================================
    // Create Publishers
    // ========================================================================
    // Publish warnings if drain rate is too high
    health_warning_pub_ = create_publisher<std_msgs::msg::String>(
        "/fleet/health_warning", 10);

    // Publish health summary every 10 seconds
    health_summary_pub_ = create_publisher<std_msgs::msg::String>(
        "/fleet/health_summary", 10);

    // ========================================================================
    // Create Timer for Periodic Health Checks
    // ========================================================================
    // Fire every 10000 milliseconds (10 seconds)
    health_timer_ = create_wall_timer(
        std::chrono::milliseconds(10000),
        std::bind(&HealthMonitorNode::health_timer_callback, this));

    RCLCPP_INFO(get_logger(), "Health Monitor: subscriptions and timers created");
}

/**
 * Telemetry Received Callback
 * 
 * Called when any drone publishes telemetry.
 * Extracts battery level and updates history.
 */
void HealthMonitorNode::on_telemetry_received(const std_msgs::msg::String &msg,
                                              const std::string &drone_name) {
    try {
        // Extract battery level from JSON
        std::string battery_str = extract_battery_from_json(msg.data);
        float battery = std::stof(battery_str);

        // Update history (keeps last 10 samples)
        update_battery_history(drone_name, battery);

        // Calculate new drain rate
        calculate_drain_rate(drone_name);

        // Check if drain rate is too high
        check_drain_warnings();

    } catch (const std::exception &e) {
        RCLCPP_ERROR(get_logger(), "Error processing telemetry: %s", e.what());
    }
}

/**
 * Health Timer Callback
 * 
 * Called every 10 seconds.
 * Prints diagnostics table and publishes health summary.
 */
void HealthMonitorNode::health_timer_callback() {
    std::string summary = create_health_summary();
    RCLCPP_INFO(get_logger(), "\n%s", summary.c_str());

    // Publish health summary as JSON
    auto msg = std_msgs::msg::String();
    msg.data = summary;
    health_summary_pub_->publish(msg);
}

/**
 * Update Battery History
 * 
 * Adds new battery sample to circular buffer (keeps only last 10).
 * Also stores timestamp for drain rate calculation.
 */
void HealthMonitorNode::update_battery_history(const std::string &drone_name, 
                                               float battery) {
    BatteryHistory &history = drone_battery_history_[drone_name];

    // Add new sample
    history.battery_levels.push_back(battery);
    history.timestamps.push_back(rclcpp::Clock().now().seconds());

    // Keep only last 10 samples (circular buffer)
    if (history.battery_levels.size() > 10) {
        history.battery_levels.pop_front();
        history.timestamps.pop_front();
    }

    RCLCPP_DEBUG(get_logger(), "%s battery: %.1f%% (samples: %lu)", 
                 drone_name.c_str(), battery, history.battery_levels.size());
}

/**
 * Calculate Drain Rate
 * 
 * Calculates battery drain rate in % per second.
 * Uses linear regression on last 10 samples (or fewer if available).
 * 
 * Formula: drain_rate = (first_battery - current_battery) / time_elapsed
 * Negative drain rate means charging.
 */
void HealthMonitorNode::calculate_drain_rate(const std::string &drone_name) {
    BatteryHistory &history = drone_battery_history_[drone_name];

    if (history.battery_levels.size() < 2) {
        history.drain_rate = 0.0f;
        return;
    }

    // Get first and last samples
    float first_battery = history.battery_levels.front();
    float last_battery = history.battery_levels.back();
    double first_time = history.timestamps.front();
    double last_time = history.timestamps.back();

    double time_elapsed = last_time - first_time;
    if (time_elapsed <= 0.0) {
        history.drain_rate = 0.0f;
        return;
    }

    // Calculate drain rate (positive = draining, negative = charging)
    history.drain_rate = (first_battery - last_battery) / static_cast<float>(time_elapsed);

    // Calculate time to critical (20%) and depletion (0%)
    float current_battery = last_battery;

    if (history.drain_rate > 0.01f) {  // Only if actively draining
        float battery_to_critical = current_battery - 20.0f;
        float battery_to_depleted = current_battery - 0.0f;

        history.time_to_critical = battery_to_critical / history.drain_rate;
        history.time_to_depletion = battery_to_depleted / history.drain_rate;
    } else if (history.drain_rate < -0.01f) {  // Charging
        history.time_to_critical = -1.0f;      // N/A while charging
        history.time_to_depletion = -1.0f;
    } else {
        history.time_to_critical = -1.0f;
        history.time_to_depletion = -1.0f;
    }
}

/**
 * Check Drain Warnings
 * 
 * If any drone's drain rate exceeds 1.5% per second,
 * publish a warning and log it.
 */
void HealthMonitorNode::check_drain_warnings() {
    for (auto &[drone_name, history] : drone_battery_history_) {
        // Check if drain rate is excessive (> 1.5% per second)
        if (history.drain_rate > 1.5f && !history.high_drain_warning) {
            history.high_drain_warning = true;

            std::ostringstream warning;
            warning << "HIGH DRAIN WARNING: " << drone_name 
                    << " draining at " << std::fixed << std::setprecision(2) 
                    << history.drain_rate << "% per second!";

            auto msg = std_msgs::msg::String();
            msg.data = warning.str();
            health_warning_pub_->publish(msg);

            RCLCPP_WARN(get_logger(), "%s", warning.str().c_str());

        } else if (history.drain_rate <= 1.5f && history.high_drain_warning) {
            // Drain rate returned to normal
            history.high_drain_warning = false;
            RCLCPP_INFO(get_logger(), "%s drain rate normalized", drone_name.c_str());
        }
    }
}

/**
 * Create Health Summary
 * 
 * Generates formatted diagnostics table with:
 * - Drain rate (% per second)
 * - Current battery
 * - Time to critical
 * - Time to depletion
 */
std::string HealthMonitorNode::create_health_summary() {
    std::ostringstream summary;

    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    summary << "\n"
            << "╔════════════════════════════════════════════════════════════════════╗\n"
            << "║                   HEALTH DIAGNOSTICS REPORT                        ║\n"
            << "║  Time: " << std::put_time(std::localtime(&time), "%H:%M:%S") << "                                    ║\n"
            << "╠════════════════════════════════════════════════════════════════════╣\n"
            << "║ Drone │ Drain Rate │ Battery │ Time to Critical │ Time to Empty ║\n"
            << "║       │  (%/sec)   │   (%)   │     (seconds)    │  (seconds)    ║\n"
            << "╠════════════════════════════════════════════════════════════════════╣\n";

    for (const auto &[drone_name, history] : drone_battery_history_) {
        float battery = history.battery_levels.empty() ? 0.0f : history.battery_levels.back();

        std::string critical_time_str = (history.time_to_critical < 0) ? "N/A" : 
            std::to_string((int)history.time_to_critical) + "s";
        std::string depletion_time_str = (history.time_to_depletion < 0) ? "N/A" :
            std::to_string((int)history.time_to_depletion) + "s";

        summary << "║ " << std::setw(5) << std::left << drone_name
                << " │ " << std::setw(10) << std::fixed << std::setprecision(2) 
                << history.drain_rate
                << " │ " << std::setw(7) << std::fixed << std::setprecision(1) 
                << battery
                << " │ " << std::setw(16) << critical_time_str
                << " │ " << std::setw(12) << depletion_time_str << " ║\n";
    }

    summary << "╚════════════════════════════════════════════════════════════════════╝\n";

    return summary.str();
}

/**
 * Extract Battery from JSON
 * 
 * Manually parses JSON to find battery value.
 * Example: {"name":"Alpha","battery":85.5,...}
 */
std::string HealthMonitorNode::extract_battery_from_json(const std::string &json_str) {
    std::string search = "\"battery\":";
    size_t pos = json_str.find(search);

    if (pos == std::string::npos) {
        throw std::runtime_error("Battery field not found in JSON");
    }

    pos += search.length();

    // Find end of number (either , or })
    size_t end = json_str.find(',', pos);
    if (end == std::string::npos) {
        end = json_str.find('}', pos);
    }

    std::string battery_str = json_str.substr(pos, end - pos);
    
    // Trim whitespace
    size_t start = battery_str.find_first_not_of(" \t\n\r");
    if (start != std::string::npos) {
        battery_str = battery_str.substr(start);
    }
    size_t end_trim = battery_str.find_last_not_of(" \t\n\r");
    if (end_trim != std::string::npos) {
        battery_str = battery_str.substr(0, end_trim + 1);
    }

    return battery_str;
}
