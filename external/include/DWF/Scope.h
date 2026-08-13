#pragma once

#include <dwf.h>
#include "Device.h"

namespace Dwf {

class Scope {
private:
    class Data {
        public:
            int sampling_frequency = 100e06;
            int buffer_size = 0;
            int max_buffer_size = 0;
            Data& operator=(const Data &data) {
                if (this != &data) {
                    sampling_frequency = data.sampling_frequency;
                    buffer_size = data.buffer_size;
                    max_buffer_size = data.max_buffer_size;
                }
                return *this;
            }
    } data;
    void open(Dwf::Device::Data *device_data, double sampling_frequency = 100e06, int buffer_size = 0, double offset = 0, double amplitude_range = 5);
    void trigger(Dwf::Device::Data *device_data, bool enable, const TRIGSRC source = trigsrcNone, int channel = 1, double timeout = 0, bool edge_rising = true, double level = 0);

public:
    void run(Dwf::Device::Data *device_data, const double sample_rate = 100e6, const int buffer_size = 10000, const double offset = 0.0, const double range = 5.0);
};

} // namespace