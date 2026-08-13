#include "Dio.h"

void Dwf::Dio::set_mode(Dwf::Device::Data* device_data, unsigned int fsOutputEnable) {
    /*
        set a DIO line as input, or as output
        parameters: - device data
                    - True means output, False means input
    */
    if (FDwfDigitalIOOutputEnableSet(device_data->handle, fsOutputEnable) == 0) {
        Dwf::Device::check_error(device_data);
    }
}

void Dwf::Dio::set_state(Dwf::Device::Data* device_data, unsigned int fsOutput) {
    // 設定
    if (FDwfDigitalIOOutputSet(device_data->handle, fsOutput) == 0) {
        Dwf::Device::check_error(device_data);
    }
    // 反映
    if (FDwfDigitalIOConfigure(device_data->handle) == 0) {
        Dwf::Device::check_error(device_data);
    }
}

unsigned int Dwf::Dio::get_state(Dwf::Device::Data *device_data){
    // load internal buffer with current state of the pins
    if (FDwfDigitalIOStatus(device_data->handle) == 0) {
        Dwf::Device::check_error(device_data);
    }
    
    // get the current state of the pins
    unsigned int data = 0;  // variable for this current state
    if (FDwfDigitalIOInputStatus(device_data->handle, &data) == 0) {
        Dwf::Device::check_error(device_data);
    }
    return data;
}