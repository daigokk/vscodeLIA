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

void RingBuffer::init(const double newRingDt, const double newHistorySec, const int nDaqChannel, const int nMultiplexerChannel){

    dt = newRingDt;
    historySec = newHistorySec;
    scopeCfg.nDaqChannel = nDaqChannel;
    scopeCfg.nMultiChannel = nMultiplexerChannel;
    const int ringBufferSize = historySec / newRingDt / nMultiplexerChannel;
    meaBuffer.chs.resize(nDaqChannel * nMultiplexerChannel);
    for(int i=0; i < meaBuffer.chs.size(); ++i){
        meaBuffer.chs[i].xs.resize(ringBufferSize, 0);
        meaBuffer.chs[i].ys.resize(ringBufferSize, 0);
    }
    meaBuffer.times.resize(ringBufferSize, 0.0);
    meaBuffer.idxWrite = 0;
    meaBuffer.idxCurrent = 0;
    meaBuffer.nofm = 0;
    offsets.chs.resize(nDaqChannel * nMultiplexerChannel, 0);
    offsets.phases_deg.resize(nDaqChannel * nMultiplexerChannel, 0);

    {
        std::lock_guard lock(plotMutex);
        for(auto& plotBuffer : DoubleBuffers){
            plotBuffer.times.resize(ringBufferSize);
            plotBuffer.ys.resize(meaBuffer.chs.size());
            for(auto& values : plotBuffer.ys){
                values.resize(ringBufferSize);
            }
            plotBuffer.matrix.resize(nDaqChannel * nMultiplexerChannel * ringBufferSize);
            plotBuffer.matrixRBF.resize(RBF_K * nDaqChannel * nMultiplexerChannel * ringBufferSize);
            std::fill(plotBuffer.matrix.begin(), plotBuffer.matrix.end(), 0.0);
            std::fill(plotBuffer.matrixRBF.begin(), plotBuffer.matrixRBF.end(), 0.0);
            plotBuffer.idxWrite = 0;
            plotBuffer.idxCurrent = 0;
            plotBuffer.nofm = 0;
        }
    }
    plotActive.store(0, std::memory_order_relaxed);

    ch_multi = 0;
}

void RingBuffer::init() {
    init(dt, historySec, scopeCfg.nDaqChannel, scopeCfg.nMultiChannel);
}

void RingBuffer::pop(const double xs[], const double ys[]){
    static std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();

    const double sampleTime = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - start_time
    ).count();

    if (offsets.flag){
        for(int ch = 0; ch < meaBuffer.chs.size(); ++ch){
            offsets.chs[ch].real(xs[ch]);
            offsets.chs[ch].imag(ys[ch]);
        }
        offsets.flag = false;
    }

    // RBF補間用トレーニングデータ格納配列
    std::vector<double> x_train(meaBuffer.chs.size()), y_train(meaBuffer.chs.size());

    meaBuffer.times[meaBuffer.idxWrite] = sampleTime;
    for(int ch = 0; ch < meaBuffer.chs.size(); ++ch){
        meaBuffer.chs[ch].xs[meaBuffer.idxWrite] = xs[ch];
        meaBuffer.chs[ch].ys[meaBuffer.idxWrite] = ys[ch];
        x_train[ch] = ch;
        y_train[ch] = ys[ch];
    }

    // RBF補間の学習
    Math::RBFInterpolation1D rbf;
    rbf.fit(x_train, y_train, 0.8, Math::RBFType::Multiquadric);

    // トリガー処理
    if (trigger.flag) {
        if (trigger.level >= 0) {
            if (!trigger.readyFlag && !trigger.countFlag){
                bool flag = true;
                for(int ch = 0; ch < meaBuffer.chs.size(); ++ch){
                    if(meaBuffer.chs[ch].ys[meaBuffer.idxCurrent] > trigger.level) {
                        flag = false;
                    }
                }
                if(flag){ trigger.readyFlag = true; }
            }
            if (trigger.readyFlag && !trigger.countFlag){
                bool flag = false;
                for(int ch = 0; ch < meaBuffer.chs.size(); ++ch){
                    if(meaBuffer.chs[ch].ys[meaBuffer.idxCurrent] >= trigger.level) {
                        flag = true;
                        break;
                    }
                }
                if(flag){ trigger.countFlag = true; }
            }
        }
        else {
            if (!trigger.readyFlag && !trigger.countFlag){
                bool flag = true;
                for(int ch = 0; ch < meaBuffer.chs.size(); ++ch){
                    if(meaBuffer.chs[ch].ys[meaBuffer.idxCurrent] < trigger.level) {
                        flag = false;
                    }
                }
                if(flag){ trigger.readyFlag = true; }
            }
            if (trigger.readyFlag && !trigger.countFlag){
                bool flag = false;
                for(int ch = 0; ch < meaBuffer.chs.size(); ++ch){
                    if(meaBuffer.chs[ch].ys[meaBuffer.idxCurrent] <= trigger.level) {
                        flag = true;
                        break;
                    }
                }
                if(flag){ trigger.countFlag = true; }
            }
        }

        if(trigger.readyFlag && trigger.countFlag && trigger.countFlag){
            if (trigger.nofm == 0) {
                trigger.nofm = meaBuffer.nofm;
            }
            else if (trigger.nofm + meaBuffer.times.size() / 2 <= meaBuffer.nofm) {
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

    // indexの更新
    meaBuffer.idxCurrent = meaBuffer.idxWrite;
    meaBuffer.idxWrite++; meaBuffer.nofm++;
    if(meaBuffer.idxWrite >= meaBuffer.times.size()) { meaBuffer.idxWrite = 0; }

    // plot用ダブルバッファの更新
    const int activePlot = plotActive.load(std::memory_order_relaxed);
    {
        std::lock_guard lock(plotMutex);
        auto& activeBuf = DoubleBuffers[activePlot];
        const int idxEnd = activeBuf.idxWrite <= meaBuffer.idxCurrent ? meaBuffer.idxCurrent : activeBuf.times.size()-1;
        for(int idx = activeBuf.idxWrite; idx <= idxEnd; ++idx){
            activeBuf.times[idx] = meaBuffer.times[idx];
            for(int ch = 0; ch < meaBuffer.chs.size(); ++ch){
                activeBuf.ys[ch][idx] = meaBuffer.chs[ch].ys[idx];
                activeBuf.matrix[idx * meaBuffer.chs.size() + ch] = meaBuffer.chs[ch].ys[idx];
                const int rbfIdx = idx * meaBuffer.chs.size() * RBF_K + ch * RBF_K;
                for (int k = 0; k < RBF_K; ++k) {
                    activeBuf.matrixRBF[rbfIdx + k] = rbf.predict((double)(ch * RBF_K + k) / RBF_K);
                }
            }
        }
        if (idxEnd != meaBuffer.idxCurrent) {
            for(int idx = 0; idx <= meaBuffer.idxCurrent; ++idx){
                activeBuf.times[idx] = meaBuffer.times[idx];
                for(int ch = 0; ch < meaBuffer.chs.size(); ++ch){
                    activeBuf.ys[ch][idx] = meaBuffer.chs[ch].ys[idx];
                    activeBuf.matrix[idx * meaBuffer.chs.size() + ch] = meaBuffer.chs[ch].ys[idx];
                    const int rbfIdx = idx * meaBuffer.chs.size() * RBF_K + ch * RBF_K;
                    for (int k = 0; k < RBF_K; ++k) {
                        activeBuf.matrixRBF[rbfIdx + k] = rbf.predict((double)(ch * RBF_K + k) / RBF_K);
                    }
                }
            }
        }
        activeBuf.idxWrite = meaBuffer.idxWrite;
        activeBuf.idxCurrent = meaBuffer.idxCurrent;
        activeBuf.nofm = meaBuffer.nofm;

        int nextPlot = (activePlot == 0) ? 1 : 0;
        plotActive.store(nextPlot, std::memory_order_release);
    }
}

void RingBuffer::update(const std::vector<std::vector<double>>& rawChs, const double rawDt){
    static Psd psd;
    if(psd.frequency != sourceChs[0].frequency || psd.getSize() != rawChs[0].size() || psd.dt != rawDt){
        psd.init(rawChs[0].size(), sourceChs[0].frequency, rawDt);
    }
    static std::vector<double> xs(meaBuffer.chs.size()), ys(meaBuffer.chs.size());
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
