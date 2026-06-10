#include "task5-ros2-aditikesari5-lang/fleet_manager_node.hpp"
#include <chrono>

/**
 * FleetManagerNode Constructor
 * 
 * What it does:
 * 1. Initialize subscriptions for each drone (Alpha, Beta, Gamma)
 * 2. Create a timer that fires every 5 seconds for reporting
 * 3. Create a service for on-demand status reports
 */
FleetManagerNode::FleetManagerNode() : Node("fleet_manager_node") {
    RCLCPP_INFO(get_logger(), "Fleet Manager Node started");

    // ========================================================================
    // Create Subscriptions for Status Topics
    // ========================================================================
    // These subscribe to drone status messages
    // The callback is called whenever a message arrives

    alpha_status_sub_ = create_subscription<std_msgs::msg::String>(
        "/drone/Alpha/status",
        10,  // Queue size
        [this](const std_msgs::msg::String &msg) {
            this->on_status_received(msg, "Alpha");
        });

    beta_status_sub_ = create_subscription<std_msgs::msg::String>(
        "/drone/Beta/status",
        10,
        [this](const std_msgs::msg::String &msg) {
            this->on_status_received(msg, "Beta");
        });

    gamma_status_sub_ = create_subscription<std_msgs::msg::String>(
        "/drone/Gamma/status",
        10,
        [this](const std_msgs::msg::String &msg) {
            this->on_status_received(msg, "Gamma");
        });

    // ========================================================================
    // Create Subscriptions for Telemetry Topics (JSON data)
    // ========================================================================
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
    // Create Subscription for Alerts
    // ========================================================================
    // This listens for critical battery alerts from any drone
    alert_sub_ = create_subscription<std_msgs::msg::String>(
        "/drone/+/alert",  // Wildcard - listens to all drone alerts
        10,
        std::bind(&FleetManagerNode::on_alert_received, this, std::placeholders::_1));

    // ========================================================================
    // Create a Timer for Periodic Reporting
    // ========================================================================
    // Fires every 5000 milliseconds (5 seconds)
    report_timer_ = create_wall_timer(
        std::chrono::milliseconds(5000),
        std::bind(&FleetManagerNode::report_timer_callback, this));

    // ========================================================================
    // Create a Service for On-Demand Reports
    // ========================================================================
    // Clients can call this service to get an immediate status report
    status_report_service_ = create_service<std_srvs::srv::Trigger>(
        "/fleet/status_report",
        std::bind(&FleetManagerNode::on_status_report_request, this, 
                  std::placeholders::_1, std::placeholders::_2));

    RCLCPP_INFO(get_logger(), "Fleet Manager: subscriptions and service created");
}

/**
 * Status Received Callback
 * 
 * Called whenever a drone publishes its status.
 * Parses the pipe-separated string and stores drone state.
 * 
 * Message format:
 * "name:Alpha|battery:87.3|altitude:15.2|status:flying|waypoint:2/5|speed:3.2"
 */
void FleetManagerNode::on_status_received(const std_msgs::msg::String &msg, 
                                          const std::string &drone_name) {
    RCLCPP_DEBUG(get_logger(), "Received status from %s: %s", 
                 drone_name.c_str(), msg.data.c_str());

    parse_status_string(msg.data, drone_name);
}

/**
 * Telemetry Received Callback
 * 
 * Called whenever a drone publishes telemetry (full JSON state).
 * Parses JSON manually and updates drone state.
 */
void FleetManagerNode::on_telemetry_received(const std_msgs::msg::String &msg,
                                             const std::string &drone_name) {
    RCLCPP_DEBUG(get_logger(), "Received telemetry from %s", drone_name.c_str());

    parse_telemetry_json(msg.data, drone_name);
}

/**
 * Alert Received Callback
 * 
 * Called when a drone publishes an alert (e.g., critical battery).
 * Prints warning with timestamp.
 */
void FleetManagerNode::on_alert_received(const std_msgs::msg::String &msg) {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    RCLCPP_WARN(get_logger(), "[ALERT] %s at %s", 
                msg.data.c_str(), std::ctime(&time));
}

/**
 * Report Timer Callback
 * 
 * Called every 5 seconds.
 * Prints a formatted table of all drone statuses.
 */
void FleetManagerNode::report_timer_callback() {
    std::string report = create_fleet_report();
    RCLCPP_INFO(get_logger(), "\n%s", report.c_str());
}

/**
 * Status Report Service Callback
 * 
 * Called when a client requests /fleet/status_report service.
 * Immediately returns the current fleet status.
 */
void FleetManagerNode::on_status_report_request(
    const std::shared_ptr<std_srvs::srv::Trigger::Request> request,
    std::shared_ptr<std_srvs::srv::Trigger::Response> response) {
    
    std::string report = create_fleet_report();
    RCLCPP_INFO(get_logger(), "Status report requested: %s", report.c_str());
    
    response->success = true;
    response->message = report;
}

/**
 * Parse Status String
 * 
 * Extracts fields from pipe-separated status string.
 * Format: "name:Alpha|battery:87.3|altitude:15.2|status:flying|waypoint:2/5|speed:3.2"
 */
void FleetManagerNode::parse_status_string(const std::string &status_str,
                                           const std::string &drone_name) {
    DroneStatus &drone = drone_states_[drone_name];
    drone.name = drone_name;

    // Split by pipe character
    std::istringstream stream(status_str);
    std::string field;

    while (std::getline(stream, field, '|')) {
        size_t colon_pos = field.find(':');
        if (colon_pos == std::string::npos) continue;

        std::string key = field.substr(0, colon_pos);
        std::string value = field.substr(colon_pos + 1);

        // Parse each field
        if (key == "battery") {
            drone.battery = std::stof(value);
            drone.is_critical = (drone.battery < 20.0);
        } else if (key == "altitude") {
            drone.altitude = std::stof(value);
        } else if (key == "status") {
            drone.status = value;
        } else if (key == "waypoint") {
            // Parse "2/5" format
            size_t slash_pos = value.find('/');
            if (slash_pos != std::string::npos) {
                drone.current_waypoint = std::stoi(value.substr(0, slash_pos));
                drone.total_waypoints = std::stoi(value.substr(slash_pos + 1));
            }
        } else if (key == "speed") {
            drone.speed = std::stof(value);
        }
    }
}

/**
 * Parse Telemetry JSON
 * 
 * Manually parses JSON (no external library).
 * Extracts fields from JSON string.
 * 
 * Example:
 * {"name":"Alpha","battery":85.5,"altitude":50.0,"status":"flying",...}
 */
void FleetManagerNode::parse_telemetry_json(const std::string &json_str,
                                            const std::string &drone_name) {
    DroneStatus &drone = drone_states_[drone_name];
    drone.name = drone_name;

    // Manual JSON parsing (simple key:value extraction)
    auto extract_float = [&json_str](const std::string &key) -> float {
        std::string search = "\"" + key + "\":";
        size_t pos = json_str.find(search);
        if (pos == std::string::npos) return 0.0f;
        
        pos += search.length();
        size_t end = json_str.find(',', pos);
        if (end == std::string::npos) {
            end = json_str.find('}', pos);
        }
        
        std::string value_str = json_str.substr(pos, end - pos);
        try {
            return std::stof(value_str);
        } catch (...) {
            return 0.0f;
        }
    };

    // Extract numeric fields
    drone.battery = extract_float("battery");
    drone.altitude = extract_float("altitude");
    drone.current_waypoint = (int)extract_float("waypoint_index");
    drone.total_waypoints = (int)extract_float("total_waypoints");
    drone.speed = extract_float("speed");
    drone.is_critical = (drone.battery < 20.0);

    // Extract string field (status)
    std::string search = "\"status\":\"";
    size_t pos = json_str.find(search);
    if (pos != std::string::npos) {
        pos += search.length();
        size_t end = json_str.find('"', pos);
        drone.status = json_str.substr(pos, end - pos);
    }
}

/**
 * Create Fleet Report
 * 
 * Generates a formatted ASCII table with all drone statuses.
 * 
 * Example output:
 * ╔════════════════════════════════════════════════════════════════╗
 * ║                    FLEET STATUS REPORT                         ║
 * ╠════════════════════════════════════════════════════════════════╣
 * ║ Drone  │ Battery │ Altitude │ Status  │ Waypoint │ Speed       ║
 * ╠════════════════════════════════════════════════════════════════╣
 * ║ Alpha  │  87.3%  │  50.0 m  │ flying  │  2 / 5   │  25.0 m/s   ║
 * ║ Beta   │  62.1%  │  75.0 m  │ idle    │  0 / 5   │  20.0 m/s   ║
 * ║ Gamma  │  18.5%  │   0.0 m  │ charging│  0 / 5   │   0.0 m/s   ║
 * ╚════════════════════════════════════════════════════════════════╝
 */
std::string FleetManagerNode::create_fleet_report() {
    std::ostringstream report;
    
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);

    report << "\n"
           << "╔═══════════════════════════════════════════════════════════════════╗\n"
           << "║                    FLEET STATUS REPORT                            ║\n"
           << "║  Time: " << std::put_time(std::localtime(&time), "%H:%M:%S") << "                                  ║\n"
           << "╠═══════════════════════════════════════════════════════════════════╣\n"
           << "║ Drone   │ Battery  │ Altitude │ Status    │ Waypoint  │ Speed     ║\n"
           << "╠═══════════════════════════════════════════════════════════════════╣\n";

    // Add row for each drone
    for (const auto &[name, status] : drone_states_) {
        std::string critical_marker = status.is_critical ? " ⚠" : "  ";
        
        report << "║ " << std::setw(6) << std::left << (name + critical_marker)
               << " │ " << std::setw(7) << std::fixed << std::setprecision(1) 
               << status.battery << "% │ " << std::setw(8) << std::fixed 
               << std::setprecision(1) << status.altitude << "m │ "
               << std::setw(9) << std::left << status.status << " │ "
               << std::setw(2) << status.current_waypoint << " / "
               << std::setw(2) << status.total_waypoints << " │ "
               << std::setw(7) << std::fixed << std::setprecision(1) 
               << status.speed << " m/s ║\n";
    }

    report << "╚═══════════════════════════════════════════════════════════════════╝\n";

    return report.str();
}
