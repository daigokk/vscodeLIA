#include "RingBuffer.h"
#include "Psd.h"
#include "Rbf.h"
#include <cmath>
#include <chrono>

void RingBuffer::initSource(const float frequency, const float ampCh1, const float ampCh2){
    sourceChs.resize(2);
    sourceChs[0].frequency = frequency;
    sourceChs[0].amplitude = ampCh1;
    sourceChs[1].frequency = sourceChs[0].frequency;
    sourceChs[1].amplitude = ampCh2;
}

void RingBuffer::init(const double newRingDt, const double newHistorySec, const int n_daq_channel, const int n_multiplexer_channel){
    dt = newRingDt;
    historySec = newHistorySec;
    scopeCfg.nDaqChannel = n_daq_channel;
    scopeCfg.nMultiChannel = n_multiplexer_channel;
    const int ringBufferSize = historySec / newRingDt / n_multiplexer_channel;
    times.resize(ringBufferSize);
    chs.resize(n_daq_channel * n_multiplexer_channel);
    for(int i=0; i < chs.size(); ++i){
        chs[i].xs.resize(ringBufferSize, 0);
        chs[i].ys.resize(ringBufferSize, 0);
    }
    matrix.resize(n_daq_channel * n_multiplexer_channel * ringBufferSize);
    matrix2.resize(RBF_K * n_daq_channel * n_multiplexer_channel * ringBufferSize);
    offsets.chs.resize(n_daq_channel * n_multiplexer_channel, 0);
    offsets.phases_deg.resize(n_daq_channel * n_multiplexer_channel, 0);

    for(auto& plotBuffer : plotBuffers){
        plotBuffer.times.resize(ringBufferSize, 0);
        plotBuffer.ys.resize(chs.size());
        for(auto& values : plotBuffer.ys){
            values.resize(ringBufferSize, 0);
        }
        plotBuffer.matrix.resize(matrix.size(), 0);
        plotBuffer.matrix2.resize(matrix2.size(), 0);
        plotBuffer.idxWrite = 0;
        plotBuffer.idxCurrent = 0;
        plotBuffer.nofm = 0;
    }
    plotActive.store(0, std::memory_order_relaxed);
    
    idxWrite = 0; idxCurrent = 0; nofm = 0; ch_multi = 0;
}

void RingBuffer::pop(const double xs[], const double ys[]){
    static std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
    const int activePlot = plotActive.load(std::memory_order_acquire);
    const int backPlot = 1 - activePlot;
    auto& plot = plotBuffers[backPlot];
    const int previousColumn = (idxWrite + times.size() - 1) % times.size();
    plot.times[previousColumn] = times[previousColumn];
    for(int ch = 0; ch < chs.size(); ++ch){
        plot.ys[ch][previousColumn] = chs[ch].ys[previousColumn];
        plot.matrix[previousColumn * chs.size() + ch] = matrix[previousColumn * chs.size() + ch];
    }
    for(int ch = 0; ch < chs.size() * RBF_K; ++ch){
        plot.matrix2[previousColumn * chs.size() * RBF_K + ch] = matrix2[previousColumn * chs.size() * RBF_K + ch];
    }
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
        const int idx = idxWrite * chs.size() + ch;
        matrix[idx] = chs[ch].ys[idxWrite];
        x_train[ch] = ch;
        y_train[ch] = chs[ch].ys[idxWrite];
    }

    // RBF補間 (epsilon = 0.8)
    Math::RBFInterpolation1D rbf;
    rbf.fit(x_train, y_train, 0.8, Math::RBFType::Multiquadric);
    for (int ch = 0; ch < x_train.size() * RBF_K; ++ch) {
        matrix2[idxWrite * x_train.size() * RBF_K + ch] = rbf.predict((double)ch/RBF_K);
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

    plot.times[idxCurrent] = times[idxCurrent];
    for(int ch = 0; ch < chs.size(); ++ch){
        plot.ys[ch][idxCurrent] = chs[ch].ys[idxCurrent];
        plot.matrix[idxCurrent * chs.size() + ch] = matrix[idxCurrent * chs.size() + ch];
    }
    for(int ch = 0; ch < chs.size() * RBF_K; ++ch){
        plot.matrix2[idxCurrent * chs.size() * RBF_K + ch] = matrix2[idxCurrent * chs.size() * RBF_K + ch];
    }
    plot.idxWrite = idxWrite;
    plot.idxCurrent = idxCurrent;
    plot.nofm = nofm;
    {
        std::lock_guard lock(plotMutex);
        plotActive.store(backPlot, std::memory_order_release);
    }
}

void RingBuffer::update(const std::vector<std::vector<double>>& rawChs, const double rawDt){
    static Psd psd;
    if(psd.frequency != sourceChs[0].frequency || psd.getSize() != rawChs[0].size() || psd.dt != rawDt){
        psd.init(rawChs[0].size(), sourceChs[0].frequency, rawDt);
    }
    static std::vector<double> xs(chs.size()), ys(chs.size());
    for(int i = 0; i < scopeCfg.nDaqChannel; ++i){
        const int ch = i + ch_multi * scopeCfg.nDaqChannel;
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
    if(ch_multi >= scopeCfg.nMultiChannel){
        pop(xs.data(), ys.data());
        ch_multi = 0;
    }
}
