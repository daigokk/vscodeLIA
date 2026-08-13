#pragma once

#include <dwf.h>
#include "Device.h"

namespace Dwf {

class Dio {
public:
    void set_mode(Dwf::Device::Data* device_data, unsigned int fsOutputEnable=0xFFFF);
    void set_state(Dwf::Device::Data* device_data, unsigned int fsOutput);
    unsigned int get_state(Dwf::Device::Data *device_data);
};

} // namespace