#include <stdexcept>
#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <cmath>
#pragma once
using namespace std;
class BatteryDepletedError : public std::runtime_error {
public:
    BatteryDepletedError(const string& msg = "Battery is depleted")
        : std::runtime_error(msg) {}
};
class InvalidStateError : public std::runtime_error {
public:
    InvalidStateError(const string& msg = "Invalid state for operation")
        : std::runtime_error(msg) {}
};
class AltitudeError : public std::runtime_error {
public:
    AltitudeError(const string& msg = "Altitude exceeds maximum limit")
        : std::runtime_error(msg) {}
};