#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <cmath>
#include "task5-ros2-aditikesari5-lang/exceptions.hpp"
#include <tuple>
using namespace std;
class Vehicle{
    string status;
    vector<string> flight_log;
     float battery_level;
public:
    string name;
    Vehicle(string n, float battery){
        name = n;
        battery_level = battery;
        status = "Idle";
    }
    virtual ~Vehicle() = default;
    virtual void get_info() const {
        cout << "Vehicle: " << name << "\n"
             << "Status: " << status << "\n"
             << "Battery: " << battery_level << "%\n";
    }
    void drain_battery(float amount){
        if(amount > battery_level){
            throw BatteryDepletedError();
        }
        battery_level -= amount;
        cout << "Battery drained by " << amount << "%. Remaining: " << battery_level << "%\n";
    }
    void charge_battery(float amount){
        if(status != "charging"){
            throw InvalidStateError("Cannot charge while in flight");
        }
        battery_level += amount;
        if(battery_level > 100) battery_level = 100;
        cout << "Battery charged by " << amount << "%. Current level: " << battery_level << "%\n";
    }
    bool is_critical() const {
        return battery_level < 20.0;
    }
    void get_flight_log() const {
        cout << "Flight Log for " << name << ":\n";
        for(const auto& entry : flight_log){
            cout << " - " << entry << "\n";
        }
    }
    void set_status(const string& new_status){
        status = new_status;
        flight_log.push_back("Status changed to: " + new_status);
    }
    void change_battery_by(float amount){
        battery_level += amount;
        if(battery_level > 100) battery_level = 100;
        flight_log.push_back("Battery level changed to: " + to_string(battery_level));
    }
};
class Drone : public Vehicle{
float speed;
protected: 
    float altitude;
    float max_altitude;
public:
    Drone(string n, float battery, float spd, float max_alt)
        : Vehicle(n, battery), speed(spd), altitude(0), max_altitude(max_alt) {}
    void take_off(float target_altitude){
        if(target_altitude > max_altitude){
            throw AltitudeError("Target altitude exceeds maximum limit");
        }
        altitude = target_altitude;
        set_status("In Flight");
        cout << name << " took off to " << altitude << " meters.\n";
    }
    void get_info() const override {
        Vehicle::get_info();
        cout << "Speed: " << speed << " m/s\n"
             << "Altitude: " << altitude << " m\n";
    }
    void land(){
        altitude = 0;
        set_status("idle");
        cout << name << " has landed.\n";
    }
    void emergency_stop(){
        altitude = 0;

        set_status("idle");
        cout << name << " has performed an emergency stop!\n";
        change_battery_by(-30); // Emergency stop consumes more battery
    }
};
class MissionDrone : public Drone{
    vector<std::pair<std::tuple<float,float,float>, std::string >> visited_waypoints;
public:    
    string mission_name;
    vector<std::tuple<float, float, float>> waypoints; // (latitude, longitude, altitude)
    int current_waypoint_index;
    MissionDrone(string n, float battery, float spd, float max_alt, string mission)
        : Drone(n, battery, spd, max_alt), mission_name(mission), current_waypoint_index(0) {}

    void add_waypoint(float lat, float lon, float alt,  string timestamp){
        waypoints.emplace_back(lat, lon, alt);
        visited_waypoints.emplace_back(std::make_tuple(lat, lon, alt), timestamp);
    }
    void get_info() const override {
        Drone::get_info();
        cout << "Mission: " << mission_name << "\n"
             << "Waypoints: " << waypoints.size() << "\n"
             // << "Timestamps:" << timestamp 
             ;
    }
    tuple<float, float, float> next_waypoint() {
        drain_battery(1.5);

        return waypoints[current_waypoint_index++];

    }
    void skip_waypoint(const string& reason){
        if(current_waypoint_index < waypoints.size()){
            cout << "Skipping waypoint " << current_waypoint_index
                 << " due to: " << reason << "\n";
            current_waypoint_index++;
        }
    }
    bool mission_complete(){
        if(current_waypoint_index >= waypoints.size()){
            cout << "Mission " << mission_name << " completed!\n";
            return true;
        }
        return false;
    }
    string mission_summary(){
        string summary = "Mission Summary for " + mission_name + ":\n";
        for(const auto& wp : visited_waypoints){
            auto [lat, lon, alt] = wp.first;
            const string& timestamp = wp.second;
            summary += " - Waypoint: (" + to_string(lat) + ", " + to_string(lon) + ", " + to_string(alt) + ") at " + timestamp + "\n";
        }
        return summary;
        
    }

};
class AutonomousDrone : public MissionDrone{
    vector<string> obstacle_log;
public:
    string ai_mode;
    tuple<float,float,float> home_position;
    AutonomousDrone(string n, float battery, float spd, float max_alt, string mission)
        : MissionDrone(n, battery, spd, max_alt, mission), ai_mode("Standard") {
        home_position = make_tuple(0.0f, 0.0f, 0.0f);
    }
    void get_info() const override {
        MissionDrone::get_info();
        cout << "AI Mode: " << ai_mode << "\n";
    }
    void set_ai_mode(const string& mode){
        ai_mode = mode;
        cout << name << " AI mode set to: " << ai_mode << "\n";
        if (ai_mode == "return_home"){
            // schedule return to home by adding home_position as next waypoint
            waypoints.push_back(home_position);
            cout << name << " returning to home position.\n";
        }
    }
    void detect_obstacle(tuple<float,float,float> position, const string& severity){
        obstacle_log.push_back("Obstacle detected at (" + to_string(get<0>(position)) + ", " + to_string(get<1>(position)) + ", " + to_string(get<2>(position)) + ") with severity: " + severity);
        cout << name << " detected an obstacle at (" << get<0>(position) << ", " << get<1>(position) << ", " << get<2>(position) << ")\n";
        if(severity == "high"){
            emergency_stop();
        }
        // obstacle handling completed
    }

    vector<tuple<float,float,float>> auto_replan(const vector<std::tuple<float,float,float>>& obstacles){
        vector<tuple<float,float,float>> new_path;
        for(const auto& wp : waypoints){
            bool safe = true;
            for(const auto& obs : obstacles){
                float dist = sqrt(pow(get<0>(wp) - get<0>(obs), 2) + pow(get<1>(wp) - get<1>(obs), 2) + pow(get<2>(wp) - get<2>(obs), 2));
                if(dist < 5.0f){ 
                    safe = false;
                    break;
                }
            }
            if(safe){
                new_path.push_back(wp);
            }
        }
        waypoints = new_path;
        cout << name << " has replanned its path to avoid obstacles.\n";
        return waypoints;
    }

};
    