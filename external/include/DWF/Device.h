#pragma once

#include <vector>
#include <string>
#include <dwf.h>

namespace Dwf {

class Error {
    public:
        std::string message;
        std::string function;
        std::string instrument;
        Error& operator=(const Error& data) {
            if (this != &data) {
                message = data.message;
                function = data.function;
                instrument = data.instrument;
            }
            return *this;
        }
};

class Warning {
    public:
        std::string message;
        std::string function;
        std::string instrument;
        Warning& operator=(const Warning& data) {
            if (this != &data) {
                message = data.message;
                function = data.function;
                instrument = data.instrument;
            }
            return *this;
        }
};

class Device {
public:
    class Data {
    public:
        struct AnalogInput {
            int channel_count = 0;
            int max_buffer_size = 0;
            int max_resolution = 0;
            double min_range = 0;
            double max_range = 0;
            double steps_range = 0;
            double min_offset = 0;
            double max_offset = 0;
            double steps_offset = 0;
            AnalogInput& operator=(const AnalogInput&) = default;
        };

        struct AnalogOutput {
            int channel_count = 0;
            std::vector<int> node_count;
            std::vector<std::vector<std::string>> node_type;
            std::vector<std::vector<int>> max_buffer_size;
            std::vector<std::vector<double>> min_amplitude;
            std::vector<std::vector<double>> max_amplitude;
            std::vector<std::vector<double>> min_offset;
            std::vector<std::vector<double>> max_offset;
            std::vector<std::vector<double>> min_frequency;
            std::vector<std::vector<double>> max_frequency;
            AnalogOutput& operator=(const AnalogOutput&) = default;
        };

        struct AnalogIo {
            int channel_count = 0;
            std::vector<int> node_count;
            std::vector<std::string> channel_name;
            std::vector<std::string> channel_label;
            std::vector<std::vector<std::string>> node_name;
            std::vector<std::vector<std::string>> node_unit;
            std::vector<std::vector<double>> min_set_range;
            std::vector<std::vector<double>> max_set_range;
            std::vector<std::vector<double>> min_read_range;
            std::vector<std::vector<double>> max_read_range;
            std::vector<std::vector<int>> set_steps;
            std::vector<std::vector<int>> read_steps;
            AnalogIo& operator=(const AnalogIo&) = default;
        };

        struct AnalogInfo {
            AnalogInput input;
            AnalogOutput output;
            AnalogIo IO;
            AnalogInfo& operator=(const AnalogInfo&) = default;
        };

        struct DigitalInput {
            int channel_count = 0;
            int max_buffer_size = 0;
            DigitalInput& operator=(const DigitalInput&) = default;
        };

        struct DigitalOutput {
            int channel_count = 0;
            int max_buffer_size = 0;
            DigitalOutput& operator=(const DigitalOutput&) = default;
        };

        struct DigitalInfo {
            DigitalInput input;
            DigitalOutput output;
            DigitalInfo& operator=(const DigitalInfo&) = default;
        };

        HDWF handle = 0;
        std::string name = "";
        std::string version = "";
        std::string serial = "";
        Error error;
        Warning warning;
        AnalogInfo analog;
        DigitalInfo digital;

        Data& operator=(const Data&) = default;
    };

    // private function definitions
private:
    static void get_info(Data* device_data);

    // public function definitions
public:
    static Data* open(const std::string serial = "", const int config = 0);
    static void check_error(Data *device_data, const char *caller = __builtin_FUNCTION(), const char *file = __FILE__);
    static void close(Data *device_data);
    static double temperature(Data *device_data);
};


} // namespace Dwf