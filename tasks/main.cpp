#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <ctime>
#include <cmath>
#include <chrono>
#include "task5-ros2-aditikesari5-lang/exceptions.hpp"
#include "task5-ros2-aditikesari5-lang/header.hpp"
using namespace std;
int main() {

    Drone SkyRider("SkyRider", 100.0, 15.0, 500.0);
    MissionDrone MissionMaster("MissionMaster", 100.0, 20.0, 600.0, "Surveillance");
    AutonomousDrone AutoPilot("AutoPilot", 100.0, 25.0, 700.0, "Delivery");
    vector<Vehicle*> fleet;
    fleet.push_back(&SkyRider);
    fleet.push_back(&MissionMaster);
    fleet.push_back(&AutoPilot);
    for (auto* v : fleet) {
        v->get_info();
        cout << "\n";
    }
    SkyRider.get_info();
    MissionMaster.get_info();
    AutoPilot.get_info();
    /*Here the same function get_info is called for each class however the output for each case is different this is an example of
    compile time polymorphism.  The function is resolved at compile time based on the type of the object. SkyRider.get_info() 
    prints only the drone's information, while MissionMaster.get_info() prints the mission drone's information(including mission 
    name, waypoints etc)*/
    //If we do:
    cout << SkyRider.name << "\n";
    // It prints name but if we do:
    // cout << SkyRider.status << "\n";
    // It gives an error because status is a private member of Vehicle class and cannot be accessed directly from main function. 
    // We can only access it through public member functions of the class.
    
    // EXECUTING A MISSION:
    auto start = std::chrono::steady_clock::now();
            AutoPilot.take_off(50);
    AutoPilot.add_waypoint(37.7749, -122.4194, 100, to_string(start.time_since_epoch().count()));
    AutoPilot.add_waypoint(34.0522, -118.2437, 150, to_string((start + std::chrono::seconds(10)).time_since_epoch().count()));
    AutoPilot.add_waypoint(40.7128, -74.0060, 200, to_string((start + std::chrono::seconds(20)).time_since_epoch().count()));
    while (!AutoPilot.mission_complete()) {
        auto [lat, lon, alt] = AutoPilot.next_waypoint();
        cout << "Heading to waypoint: (" << lat << ", " << lon << ", " << alt << ")\n";
        if (AutoPilot.is_critical()) {
            cout << "Battery critical! Returning to base.\n";
            AutoPilot.set_ai_mode("return_home");
            break;
        }
        }
        AutoPilot.detect_obstacle(make_tuple(40.7128, -74.0060, 200), "high");
        AutoPilot.auto_replan({make_tuple(40.7128, -74.0060, 200)});
        AutoPilot.land();
        string a = AutoPilot.mission_summary();
        cout << a;
    return 0;
    }