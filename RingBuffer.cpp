#include "Config.h"
#include "Psd.h"
#include "Rbf.h"
#include <complex>
#include <cmath>
#include <chrono>

void Config::RingBuffer::init(const double rawSize, const double rawDt, const double newRingDt, const int ringSize, const int n_daq_channel, const int n_multiplexer_channel){
    dt = newRingDt;
    scheduleTime.resize(ringSize);
    for (int i = 0; i < scheduleTime.size(); ++i){
        scheduleTime[i] = i * dt * n_multiplexer_channel;
    }

    times.resize(ringSize);
    chs.resize(n_daq_channel * n_multiplexer_channel);
    for(int i=0; i < chs.size(); ++i){
        chs[i].xs.resize(ringSize, 0);
        chs[i].ys.resize(ringSize, 0);
    }
    matrix.resize(n_daq_channel * n_multiplexer_channel * ringSize);
    matrix2.resize(RBF_K * n_daq_channel * n_multiplexer_channel * ringSize);
    offsets.chs.resize(n_daq_channel * n_multiplexer_channel, 0);
    offsets.phases_deg.resize(n_daq_channel * n_multiplexer_channel, 0);
    sourceChs.resize(2);
    sourceChs[1].amplitude = 0;
    idxWrite = 0; idxCurrent = 0; nofm = 0; ch_multi = 0;
}

void Config::RingBuffer::pop(const double xs[], const double ys[]){
    static std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
    times[idxWrite] = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - start_time
    ).count();
    if (offsets.flag){
        for(int ch = 0; ch < chs.size(); ++ch){
            offsets.chs[ch].real(xs[ch]);
            offsets.chs[ch].imag(ys[ch]);
        }
        offsets.flag = false;
    }
    // RBF補間の準備
    std::vector<double> x_train(chs.size()), y_train(chs.size());

    // 測定値のringBufferへの格納とコンター図用配列matrixの作成
    for(int ch = 0; ch < chs.size(); ++ch){
        chs[ch].xs[idxWrite] = xs[ch];
        chs[ch].ys[idxWrite] = ys[ch];
        const int idx = ch * times.size() + idxWrite;
        matrix[idx] = chs[ch].ys[idxWrite];
        x_train[ch] = ch;
        y_train[ch] = chs[ch].ys[idxWrite];
    }

    // RBF補間 (epsilon = 0.8)
    Math::RBFInterpolation1D rbf;
    rbf.fit(x_train, y_train, 0.8, Math::RBFType::Multiquadric);
    for (int ch = 0; ch < x_train.size() * RBF_K; ++ch) {
        matrix2[ch * times.size() + idxWrite] = rbf.predict((double)ch/RBF_K);
    }

    // トリガー処理
    if (trigger.flag) {
        if (trigger.level >= 0) {
            // slope:+
            if (!trigger.readyFlag && !trigger.countFlag){
                bool flag = true;
                for(int ch = 0; ch < chs.size(); ++ch){
                    if(chs[ch].ys[idxCurrent] > trigger.level) {
                        flag = false;
                    }
                }
                if(flag){ trigger.readyFlag = true; }
            }
            if (trigger.readyFlag && !trigger.countFlag){
                bool flag = false;
                for(int ch = 0; ch < chs.size(); ++ch){
                    if(chs[ch].ys[idxCurrent] >= trigger.level) {
                        flag = true;
                        break;
                    }
                }
                if(flag){ trigger.countFlag = true; }
            }   
        }
        else {
            // slope: -
            if (!trigger.readyFlag && !trigger.countFlag){
                bool flag = true;
                for(int ch = 0; ch < chs.size(); ++ch){
                    if(chs[ch].ys[idxCurrent] < trigger.level) {
                        flag = false;
                    }
                }
                if(flag){ trigger.readyFlag = true; }
            }
            if (trigger.readyFlag && !trigger.countFlag){
                bool flag = false;
                for(int ch = 0; ch < chs.size(); ++ch){
                    if(chs[ch].ys[idxCurrent] <= trigger.level) {
                        flag = true;
                        break;
                    }
                }
                if(flag){ trigger.countFlag = true; }
            }
        }
        // Post trigger
        if(trigger.readyFlag && trigger.countFlag && trigger.countFlag){
            if (trigger.nofm == 0) {
                trigger.nofm = nofm;
            }
            else if (trigger.nofm + times.size() / 2 <= nofm) {
                pauseFlag = true;
                trigger.nofm = 0;
                trigger.readyFlag = false;
                trigger.countFlag = false;
            }
        }
    }
    else {
        trigger.nofm = 0;
    }
    idxCurrent = idxWrite;
    idxWrite++; nofm++;
    if(idxWrite >= times.size()) {idxWrite = 0;}
}

void Config::RingBuffer::update(const std::vector<std::vector<double>>& rawChs, const double rawDt){
    static Psd psd;
    if(psd.frequency != sourceChs[0].frequency || psd.getSize() != rawChs[0].size() || psd.dt != rawDt){
        psd.init(rawChs[0].size(), sourceChs[0].frequency, rawDt);
    }
    static double xs[N_DAQ_CHANNEL*N_MULTIPLEXER_CHANNEL];
    static double ys[N_DAQ_CHANNEL*N_MULTIPLEXER_CHANNEL];
    for(int i = 0; i < N_DAQ_CHANNEL; ++i){
        const int ch = i + ch_multi * N_DAQ_CHANNEL;
        auto const [x, y] = psd.calc(rawChs[ch].data());
        auto const [x2, y2] = psd.rotate(
            offsets.phases_deg[ch],
            x - offsets.chs[ch].real(),
            y - offsets.chs[ch].imag()
        );
        xs[ch] = x2;
        ys[ch] = y2;
    }
    ch_multi++;
    if(ch_multi >= N_MULTIPLEXER_CHANNEL){
        pop(xs, ys);
        ch_multi = 0;
    }
}