#include "task5-ros2-aditikesari5-lang/drone_node.hpp"
#include <sstream>

/**
 * DroneNode Constructor
 * 
 * What it does:
 * 1. Calls parent class (rclcpp::Node) constructor with node name
 * 2. Declares and reads ROS 2 parameters
 * 3. Creates 5 predefined waypoints
 * 4. Creates publishers for status, telemetry, alerts, mission_complete
 * 5. Creates two timers: one for status (1s), one for telemetry (2s)
 * 6. Initializes the MissionDrone object with parameters
 */
DroneNode::DroneNode() : Node("drone_node"), publish_count_(0), mission_running_(true), was_critical_(false) {
    // ========================================================================
    // Declare Parameters
    // ========================================================================
    // Parameters can be set from command line or launch file
    declare_parameter("drone_name", "Alpha");
    declare_parameter("initial_battery", 100.0);
    declare_parameter("mission_name", "default_mission");

    // Get parameter values
    drone_name_ = get_parameter("drone_name").as_string();
    initial_battery_ = get_parameter("initial_battery").as_double();
    mission_name_ = get_parameter("mission_name").as_string();

    // Log what parameters we received
    RCLCPP_INFO(get_logger(), "Drone Node started for: %s (Battery: %.1f%%)", 
                drone_name_.c_str(), initial_battery_);

    // ========================================================================
    // Create the MissionDrone object
    // ========================================================================
    // This is your existing C++ drone class, running inside ROS 2
    drone_ = std::make_unique<MissionDrone>(
        drone_name_,           // Name
        initial_battery_,      // Battery level
        25.0,                  // Speed (m/s)
       700.0,                 // Max altitude
        mission_name_          // Mission name
    );

    // ========================================================================
    // Add 5 predefined waypoints to the drone
    // ========================================================================
    auto now = std::chrono::system_clock::now();
    auto timestamp_0 = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();

    drone_->add_waypoint(37.7749, -122.4194, 50, std::to_string(timestamp_0));  // SF
    drone_->add_waypoint(34.0522, -118.2437, 75, std::to_string(timestamp_0 + 5000));  // LA
    drone_->add_waypoint(40.7128, -74.0060, 100, std::to_string(timestamp_0 + 10000)); // NYC
    drone_->add_waypoint(41.8781, -87.6298, 125, std::to_string(timestamp_0 + 15000)); // Chicago
    drone_->add_waypoint(39.7392, -104.9903, 150, std::to_string(timestamp_0 + 20000)); // Denver

    RCLCPP_INFO(get_logger(), "%s: Added 5 waypoints", drone_name_.c_str());

    // ========================================================================
    // Create Publishers
    // ========================================================================
    // Publishers broadcast messages to topics that anyone can subscribe to

    // Status topic - sends simple status string every 1 second
    status_publisher_ = create_publisher<std_msgs::msg::String>(
        "/drone/" + drone_name_ + "/status", 10);  // 10 = queue size

    // Telemetry topic - sends JSON with full drone state every 2 seconds
    telemetry_publisher_ = create_publisher<std_msgs::msg::String>(
        "/drone/" + drone_name_ + "/telemetry", 10);

    // Alert topic - sends warning when battery critical
    alert_publisher_ = create_publisher<std_msgs::msg::String>(
        "/drone/" + drone_name_ + "/alert", 10);

    // Mission complete topic - sent when all waypoints visited
    mission_complete_publisher_ = create_publisher<std_msgs::msg::String>(
        "/drone/" + drone_name_ + "/mission_complete", 10);

    // ========================================================================
    // Create Timers
    // ========================================================================
    // Timers trigger callbacks at regular intervals (in milliseconds)

    // Status timer: fires every 1000ms (1 second)
    status_timer_ = create_wall_timer(
        std::chrono::milliseconds(1000),
        std::bind(&DroneNode::status_timer_callback, this));

    // Telemetry timer: fires every 2000ms (2 seconds)
    telemetry_timer_ = create_wall_timer(
        std::chrono::milliseconds(2000),
        std::bind(&DroneNode::telemetry_timer_callback, this));

    RCLCPP_INFO(get_logger(), "%s: Node initialized with timers", drone_name_.c_str());
}

/**
 * Status Timer Callback
 * 
 * Called every 1 second. Publishes drone status in simple format.
 * Also drains battery by 0.5 each time and advances waypoint every 3 publishes.
 * 
 * Message format:
 * "name:Alpha|battery:87.3|altitude:15.2|status:flying|waypoint:2/5|speed:3.2"
 */
void DroneNode::status_timer_callback() {
    if (!mission_running_) return;

    try {
        // Drain battery (every publish costs 0.5%)
        drone_->drain_battery(0.5);

        // Increment publish counter
        publish_count_++;

        // Advance waypoint every 3 publishes
        if (publish_count_ >= 3 && !drone_->mission_complete()) {
            try {
                drone_->next_waypoint();
                publish_count_ = 0;
            } catch (const BatteryDepletedError&) {
                // If battery depleted while advancing, land
                drone_->land();
                mission_running_ = false;
                return;
            }
        }

        // Check if battery is critical
        if (drone_->is_critical() && !was_critical_) {
            was_critical_ = true;
            auto alert_msg = std_msgs::msg::String();
            alert_msg.data = "ALERT: " + drone_name_ + " battery critical at " + 
                           std::to_string(drone_->get_battery_level()) + "%";
            alert_publisher_->publish(alert_msg);
            RCLCPP_WARN(get_logger(), "%s", alert_msg.data.c_str());
            
            // Land when critical
            drone_->land();
        }

        // Create status message
        std::ostringstream status_stream;
        status_stream << "name:" << drone_name_
                      << "|battery:" << drone_->get_battery_level()
                      << "|altitude:" << drone_->get_altitude()
                      << "|status:" << drone_->get_status()
                      << "|waypoint:" << drone_->get_current_waypoint_index() 
                      << "/" << drone_->get_waypoints().size()
                      << "|speed:" << drone_->get_speed();

        auto msg = std_msgs::msg::String();
        msg.data = status_stream.str();
        status_publisher_->publish(msg);

        RCLCPP_DEBUG(get_logger(), "Published status: %s", msg.data.c_str());

        // Check if mission is complete
        if (drone_->mission_complete() && mission_running_) {
            mission_running_ = false;
            auto complete_msg = std_msgs::msg::String();
            complete_msg.data = drone_name_ + " mission completed!";
            mission_complete_publisher_->publish(complete_msg);
            RCLCPP_INFO(get_logger(), "%s", complete_msg.data.c_str());
        }

    } catch (const BatteryDepletedError& e) {
        RCLCPP_ERROR(get_logger(), "Battery depleted for %s: %s", 
                     drone_name_.c_str(), e.what());
        mission_running_ = false;
    } catch (const std::exception& e) {
        RCLCPP_ERROR(get_logger(), "Error in status callback: %s", e.what());
    }
}

/**
 * Telemetry Timer Callback
 * 
 * Called every 2 seconds. Publishes full drone state as JSON string.
 * 
 * Example JSON:
 * {
 *   "name": "Alpha",
 *   "battery": 85.5,
 *   "altitude": 50.0,
 *   "status": "flying",
 *   "waypoint_index": 2,
 *   "total_waypoints": 5,
 *   "speed": 25.0,
 *   "ai_mode": "manual"
 * }
 */
void DroneNode::telemetry_timer_callback() {
    if (!mission_running_) return;

    std::string json = generate_telemetry_json();
    auto msg = std_msgs::msg::String();
    msg.data = json;
    telemetry_publisher_->publish(msg);

    RCLCPP_DEBUG(get_logger(), "Published telemetry for %s", drone_name_.c_str());
}

/**
 * Helper: Generate Telemetry JSON
 * 
 * Creates a JSON string with all drone telemetry data.
 * Manual JSON generation (no external library needed for simple JSON).
 */
std::string DroneNode::generate_telemetry_json() {
    std::ostringstream json;
    json << "{"
         << "\"name\":\"" << drone_name_ << "\","
         << "\"battery\":" << drone_->get_battery_level() << ","
         << "\"altitude\":" << drone_->get_altitude() << ","
         << "\"status\":\"" << drone_->get_status() << "\","
         << "\"waypoint_index\":" << drone_->get_current_waypoint_index() << ","
         << "\"total_waypoints\":" << drone_->get_waypoints().size() << ","
         << "\"speed\":" << drone_->get_speed()
         << "}";
    return json.str();
}
