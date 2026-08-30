#include "Scope.h"

void Dwf::Scope::open(Dwf::Device::Data *device_data, double sampling_frequency, int buffer_size, double offset, double amplitude_range){
    /*
        initialize the oscilloscope

        parameters: - device data
                    - sampling frequency in Hz, default is 20MHz
                    - buffer size, default is 0 (maximum)
                    - offset voltage in Volts, default is 0V
                    - amplitude range in Volts, default is ±5V
    */
    // set global variables
    data.sampling_frequency = sampling_frequency;
    data.max_buffer_size = device_data->analog.input.max_buffer_size;
    // enable all channels
    if (FDwfAnalogInChannelEnableSet(device_data->handle, -1, true) == 0) {
        Dwf::Device::check_error(device_data);
    }
    
    // set offset voltage (in Volts)
    if (FDwfAnalogInChannelOffsetSet(device_data->handle, -1, offset) == 0) {
        Dwf::Device::check_error(device_data);
    }
    
    // set range (maximum signal amplitude in Volts)
    if (FDwfAnalogInChannelRangeSet(device_data->handle, -1, amplitude_range) == 0) {
        Dwf::Device::check_error(device_data);
    }
    
    // set the buffer size (data point in a recording)
    if (buffer_size == 0 || buffer_size > data.max_buffer_size) {
        buffer_size = data.max_buffer_size;
    }
    data.buffer_size = buffer_size;
    if (FDwfAnalogInBufferSizeSet(device_data->handle, buffer_size) == 0) {
        Dwf::Device::check_error(device_data);
    }
    
    // set the acquisition frequency (in Hz)
    if (FDwfAnalogInFrequencySet(device_data->handle, sampling_frequency) == 0) {
        Dwf::Device::check_error(device_data);
    }
    
    // disable averaging (for more info check the documentation)
    if (FDwfAnalogInChannelFilterSet(device_data->handle, -1, filterDecimate) == 0) {
        Dwf::Device::check_error(device_data);
    }
    return;
}

void Dwf::Scope::trigger(Dwf::Device::Data *device_data, bool enable, const TRIGSRC source, int channel, double timeout, bool edge_rising, double level) {
    /*
        set up triggering

        parameters: - device handle
                    - enable / disable triggering with True/False
                    - trigger source - possible: none, analog, digital, external[1-4]
                    - trigger channel - possible options: 1-4 for analog, or 0-15 for digital
                    - auto trigger timeout in seconds, default is 0
                    - trigger edge rising - True means rising, False means falling, default is rising
                    - trigger level in Volts, default is 0V
    */
    if (enable && source != trigsrcNone) {
        // enable/disable auto triggering
        if (FDwfAnalogInTriggerAutoTimeoutSet(device_data->handle, timeout) == 0) {
            Dwf::Device::check_error(device_data);
        }

        // set trigger source
        if (FDwfAnalogInTriggerSourceSet(device_data->handle, source) == 0) {
            Dwf::Device::check_error(device_data);
        }

        // set trigger channel
        if (source == trigsrcDetectorAnalogIn) {
            if (FDwfAnalogInTriggerChannelSet(device_data->handle, channel) == 0) {
                Dwf::Device::check_error(device_data);
            }
        }

        // set trigger type
        if (FDwfAnalogInTriggerTypeSet(device_data->handle, trigtypeEdge) == 0) {
            Dwf::Device::check_error(device_data);
        }

        // set trigger level
        if (FDwfAnalogInTriggerLevelSet(device_data->handle, level) == 0) {
            Dwf::Device::check_error(device_data);
        }
        
        // バッファの先頭にトリガー検出位置を合わせる
        double bufferSec = (data.buffer_size / 2.0) / data.sampling_frequency;
        if (FDwfAnalogInTriggerPositionSet(device_data->handle, bufferSec) == 0) {
            Dwf::Device::check_error(device_data);
        }

        // set trigger edge
        if (edge_rising) {
            // rising edge
            if (FDwfAnalogInTriggerConditionSet(device_data->handle, trigcondRisingPositive) == 0) {
                Dwf::Device::check_error(device_data);
            }
        }
        else {
            // falling edge
            if (FDwfAnalogInTriggerConditionSet(device_data->handle, trigcondFallingNegative) == 0) {
                Dwf::Device::check_error(device_data);
            }
        }
    }
    else {
        // turn off the trigger
        if (FDwfAnalogInTriggerSourceSet(device_data->handle, trigsrcNone) == 0) {
            Dwf::Device::check_error(device_data);
        }
    }
    return;
}

void Dwf::Scope::run(Dwf::Device::Data *device_data, const double sample_rate, const int buffer_size, const double offset, const double range) {
    /**
    * @brief スコープを設定する
    * @param sample_rate サンプルレート
    * @param buffer_size バッファサイズ
    * @param offset オフセット
    * @param range ダイナミックレンジ (2.5: ±2.5V, 25: ±25V)
    */
    open(device_data, sample_rate, buffer_size, offset, range*2);
    trigger(device_data, true, trigsrcAnalogOut1, 0, 0);
    if (FDwfAnalogInConfigure(device_data->handle, true, true) == 0) {
        Dwf::Device::check_error(device_data);
    }
}
