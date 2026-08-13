/* DEVICE CONTROL FUNCTIONS: open, check_error, close, temperature */

/* include the header */
#include "Device.h"

#include <functional>
#include <map>
#include <utility>

namespace {

using DeviceData = Dwf::Device::Data;

std::vector<std::string> collectNodeTypes(int nodeMask) {
    std::vector<std::string> nodeTypes;
    for (int nodeIndex = 0; nodeIndex < 3; ++nodeIndex) {
        if ((1 << nodeIndex) & nodeMask) {
            switch (nodeIndex) {
                case AnalogOutNodeCarrier: nodeTypes.push_back("carrier"); break;
                case AnalogOutNodeFM:      nodeTypes.push_back("FM"); break;
                case AnalogOutNodeAM:      nodeTypes.push_back("AM"); break;
                default: break;
            }
        }
    }
    return nodeTypes;
}

} // namespace

/* ----------------------------------------------------- */

Dwf::Device::Data* Dwf::Device::open(std::string serial) {
    /*
        open a specific device by serial number
        parameters: - serial number of the device (empty string = first connected device)
                    - config
        returns:    - device data
    */

    Data *device_data = new Data();

    // count devices
    int device_count = 0;
    FDwfEnum(enumfilterAll, &device_count);

    // check for connected devices
    if (device_count <= 0) {
        device_data->error.instrument = "device";
        device_data->error.function = "open";
        device_data->error.message = serial.empty() ? "There are no connected devices" : "There is no device with serial " + serial + " connected.";
        throw device_data->error;
    }

    // this is the device handle - it will be used by all functions to "address" the connected device
    device_data->handle = 0;

    // connect to the matching serial or, if empty, the first available device
    HDWF index = 0;
    bool serial_found = false;
    while (device_data->handle == 0 && index < device_count) {
        if (serial.empty()) {
            FDwfDeviceOpen(index, &device_data->handle);
        } else {
            char found_serial[32] = {0};
            if (FDwfEnumSN(index, found_serial) != 0 && serial == found_serial) {
                serial_found = true;
                FDwfDeviceOpen(index, &device_data->handle);
            }
        }
        index++;
    }

    if (!serial.empty() && !serial_found) {
        device_data->error.instrument = "device";
        device_data->error.function = "open";
        device_data->error.message = "There is no device with serial " + serial + " connected.";
        throw device_data->error;
    }

    // check connected device type
    device_data->name = "";
    if (device_data->handle != 0) {
        char deviceName[32] = {0};
        if (FDwfEnumDeviceName(0, deviceName) != 0) {
            device_data->name = std::string(deviceName);
        }
    }

    // check for errors
    // if the device handle is empty after a connection attempt
    if (device_data->handle == hdwfNone) {
        // check for errors
        int err_nr = 0; // variable for error number
        FDwfGetLastError(&err_nr);  // get error number

        // if there is an error
        if (err_nr != dwfercNoErc) {
            // check the error message
            check_error(device_data);
        }
    }

    // set global data
    get_info(device_data);
    return device_data;
}

/* ----------------------------------------------------- */

void Dwf::Device::check_error(Data *device_data, const char *caller, const char *file) {
    /*
        check for errors
    */
    char err_msg[512];  // variable for the error message
    FDwfGetLastErrorMsg(err_msg);  // get the error message
    device_data->error.message = err_msg;   // cast it to string
    if (device_data->error.message != "") {
        device_data->error.function = caller;
        device_data->error.instrument = file;
        // delete the extension
        size_t index = device_data->error.instrument.find('.');
        if (index != std::string::npos) {
            device_data->error.instrument = device_data->error.instrument.substr(0, index);
        }
        // delete the path
        device_data->error.instrument = std::string(device_data->error.instrument.rbegin(), device_data->error.instrument.rend());
        index = device_data->error.instrument.find('/');
        if (index != std::string::npos) {
            device_data->error.instrument = device_data->error.instrument.substr(0, index);
        }
        index = device_data->error.instrument.find('\\');
        if (index != std::string::npos) {
            device_data->error.instrument = device_data->error.instrument.substr(0, index);
        }
        device_data->error.instrument = std::string(device_data->error.instrument.rbegin(), device_data->error.instrument.rend());
        throw device_data->error;
    }
    return;
}

/* ----------------------------------------------------- */

void Dwf::Device::close(Data *device_data) {
    /*
        close a specific device
    */
    if (device_data->handle != 0) {
        FDwfDeviceClose(device_data->handle);
    }
    delete device_data;
    return;
}

/* ----------------------------------------------------- */

double Dwf::Device::temperature(Data *device_data) {
    /*
        return the board temperature
    */
    // find the system monitor
    int channel = -1;
    for (int channel_index = 0; channel_index < device_data->analog.IO.channel_count; channel_index++) {
        if (device_data->analog.IO.channel_label[channel_index] == "System") {
            channel = channel_index;
            break;
        }
    }
    if (channel < 0) {
        return 0;
    }

    // find the temperature node
    int node = -1;
    for (int node_index = 0; node_index < device_data->analog.IO.node_count[channel]; node_index++) {
        if (device_data->analog.IO.node_name[channel][node_index] == "Temp") {
            node = node_index;
            break;
        }
    }
    if (node < 0) {
        return 0;
    }

    // read the temperature
    if (FDwfAnalogIOStatus(device_data->handle) == 0) {
        check_error(device_data);
    }
    double temperature = 0;
    if (FDwfAnalogIOChannelNodeStatus(device_data->handle, channel, node, &temperature) == 0) {
        check_error(device_data);
    }
    return temperature;
}

/* ----------------------------------------------------- */

void Dwf::Device::get_info(Data* device_data) {
    /*
        get device information
    */

    // check WaveForms version
    char version[16];
    if (FDwfGetVersion(version) == 0) {
        check_error(device_data);
    }
    device_data->version = std::string(version);

    // analog input information
    // channel count
    int handle = device_data->handle;
    if (FDwfAnalogInChannelCount(handle, &device_data->analog.input.channel_count) == 0) {
        check_error(device_data);
    }
    // buffer size
    if (FDwfAnalogInBufferSizeInfo(handle, 0, &device_data->analog.input.max_buffer_size) == 0) {
        check_error(device_data);
    }
    // ADC resolution
    if (FDwfAnalogInBitsInfo(handle, &device_data->analog.input.max_resolution) == 0) {
        check_error(device_data);
    }
    // range information
    if (FDwfAnalogInChannelRangeInfo(handle, &device_data->analog.input.min_range, &device_data->analog.input.max_range, &device_data->analog.input.steps_range) == 0) {
        check_error(device_data);
    }
    // offset information
    if (FDwfAnalogInChannelOffsetInfo(handle, &device_data->analog.input.min_offset, &device_data->analog.input.max_offset, &device_data->analog.input.steps_offset) == 0) {
        check_error(device_data);
    }

    // analog output information
    // channel count
    if (FDwfAnalogOutCount(handle, &device_data->analog.output.channel_count) == 0) {
        check_error(device_data);
    }
    for (int channel_index = 0; channel_index < device_data->analog.output.channel_count; ++channel_index) {
        int nodeMask = 0;
        if (FDwfAnalogOutNodeInfo(handle, channel_index, &nodeMask) == 0) {
            check_error(device_data);
        }

        auto nodeTypes = collectNodeTypes(nodeMask);
        device_data->analog.output.node_type.push_back(nodeTypes);
        device_data->analog.output.node_count.push_back(static_cast<int>(nodeTypes.size()));

        std::vector<int> maxBufferSizes;
        maxBufferSizes.reserve(device_data->analog.output.node_count.back());
        for (int node_index = 0; node_index < device_data->analog.output.node_count.back(); ++node_index) {
            int maxBufferSize = 0;
            if (FDwfAnalogOutNodeDataInfo(handle, channel_index, node_index, 0, &maxBufferSize) == 0) {
                check_error(device_data);
            }
            maxBufferSizes.push_back(maxBufferSize);
        }
        device_data->analog.output.max_buffer_size.push_back(maxBufferSizes);

        std::vector<double> minAmplitudes, maxAmplitudes;
        minAmplitudes.reserve(device_data->analog.output.node_count.back());
        maxAmplitudes.reserve(device_data->analog.output.node_count.back());
        for (int node_index = 0; node_index < device_data->analog.output.node_count.back(); ++node_index) {
            double minAmplitude = 0.0;
            double maxAmplitude = 0.0;
            if (FDwfAnalogOutNodeAmplitudeInfo(handle, channel_index, node_index, &minAmplitude, &maxAmplitude) == 0) {
                check_error(device_data);
            }
            minAmplitudes.push_back(minAmplitude);
            maxAmplitudes.push_back(maxAmplitude);
        }
        device_data->analog.output.min_amplitude.push_back(minAmplitudes);
        device_data->analog.output.max_amplitude.push_back(maxAmplitudes);

        std::vector<double> minOffsets, maxOffsets;
        for (int node_index = 0; node_index < device_data->analog.output.node_count.back(); ++node_index) {
            double minOffset = 0.0;
            double maxOffset = 0.0;
            if (FDwfAnalogOutNodeOffsetInfo(handle, channel_index, node_index, &minOffset, &maxOffset) == 0) {
                check_error(device_data);
            }
            minOffsets.push_back(minOffset);
            maxOffsets.push_back(maxOffset);
        }
        device_data->analog.output.min_offset.push_back(minOffsets);
        device_data->analog.output.max_offset.push_back(maxOffsets);

        std::vector<double> minFrequencies, maxFrequencies;
        for (int node_index = 0; node_index < device_data->analog.output.node_count.back(); ++node_index) {
            double minFrequency = 0.0;
            double maxFrequency = 0.0;
            if (FDwfAnalogOutNodeFrequencyInfo(handle, channel_index, node_index, &minFrequency, &maxFrequency) == 0) {
                check_error(device_data);
            }
            minFrequencies.push_back(minFrequency);
            maxFrequencies.push_back(maxFrequency);
        }
        device_data->analog.output.min_frequency.push_back(minFrequencies);
        device_data->analog.output.max_frequency.push_back(maxFrequencies);
    }

    // analog IO information
    // channel count
    if (FDwfAnalogIOChannelCount(handle, &device_data->analog.IO.channel_count) == 0) {
        check_error(device_data);
    }
    for (int channel_index = 0; channel_index < device_data->analog.IO.channel_count; channel_index++) {
        // channel names and labels
        char temp1[256];
        char temp2[256];
        if (FDwfAnalogIOChannelName(handle, channel_index, temp1, temp2) == 0) {
            check_error(device_data);
        }
        device_data->analog.IO.channel_name.insert(device_data->analog.IO.channel_name.end(), std::string(temp1));
        device_data->analog.IO.channel_label.insert(device_data->analog.IO.channel_label.end(), std::string(temp2));
        // node count
        int temp3;
        if (FDwfAnalogIOChannelInfo(handle, channel_index, &temp3) == 0) {
            check_error(device_data);
        }
        device_data->analog.IO.node_count.insert(device_data->analog.IO.node_count.end(), temp3);
        // node names and units
        std::vector<std::string> templist1, templist2;
        for (int node_index = 0; node_index < device_data->analog.IO.node_count[channel_index]; node_index++) {
            if (FDwfAnalogIOChannelNodeName(handle, channel_index, node_index, temp1, temp2) == 0) {
                check_error(device_data);
            }
            templist1.insert(templist1.end(), temp1);
            templist2.insert(templist2.end(), temp2);
        }
        device_data->analog.IO.node_name.insert(device_data->analog.IO.node_name.end(), templist1);
        device_data->analog.IO.node_unit.insert(device_data->analog.IO.node_unit.end(), templist2);
        // node write info
        double temp4, temp5;
        std::vector<int> templist3;
        std::vector<double> templist4, templist5;
        for (int node_index = 0; node_index < device_data->analog.IO.node_count[channel_index]; node_index++) {
            if (FDwfAnalogIOChannelNodeSetInfo(handle, channel_index, node_index, &temp4, &temp5, &temp3) == 0) {
                check_error(device_data);
            }
            templist3.insert(templist3.end(), temp3);
            templist4.insert(templist4.end(), temp4);
            templist5.insert(templist5.end(), temp5);
        }
        device_data->analog.IO.min_set_range.insert(device_data->analog.IO.min_set_range.end(), templist4);
        device_data->analog.IO.max_set_range.insert(device_data->analog.IO.max_set_range.end(), templist5);
        device_data->analog.IO.set_steps.insert(device_data->analog.IO.set_steps.end(), templist3);
        // node read info
        templist3.clear();
        templist4.clear();
        templist5.clear();
        for (int node_index = 0; node_index < device_data->analog.IO.node_count[channel_index]; node_index++) {
            if (FDwfAnalogIOChannelNodeStatusInfo(handle, channel_index, node_index, &temp4, &temp5, &temp3) == 0) {
                check_error(device_data);
            }
            templist3.insert(templist3.end(), temp3);
            templist4.insert(templist4.end(), temp4);
            templist5.insert(templist5.end(), temp5);
        }
        device_data->analog.IO.min_read_range.insert(device_data->analog.IO.min_read_range.end(), templist4);
        device_data->analog.IO.max_read_range.insert(device_data->analog.IO.max_read_range.end(), templist5);
        device_data->analog.IO.read_steps.insert(device_data->analog.IO.read_steps.end(), templist3);
    }

    // digital input information
    // channel count
    if (FDwfDigitalInBitsInfo(handle, &device_data->digital.input.channel_count) == 0) {
        check_error(device_data);
    }
    // buffer size
    if (FDwfDigitalInBufferSizeInfo(handle, &device_data->digital.input.max_buffer_size) == 0) {
        check_error(device_data);
    }

    // digital output information
    // channel count
    if (FDwfDigitalOutCount(handle, &device_data->digital.output.channel_count) == 0) {
        check_error(device_data);
    }
    // buffer size
    unsigned int temp;
    if (FDwfDigitalOutDataInfo(handle, 0, &temp) == 0) {
        check_error(device_data);
    }
    device_data->digital.output.max_buffer_size = (int)temp;

    return;
}
