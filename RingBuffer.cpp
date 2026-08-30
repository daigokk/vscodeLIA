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
    chs.resize(nDaqChannel * nMultiplexerChannel);
    for(int i=0; i < chs.size(); ++i){
        chs[i].xs.resize(ringBufferSize, 0);
        chs[i].ys.resize(ringBufferSize, 0);
    }
    offsets.chs.resize(nDaqChannel * nMultiplexerChannel, 0);
    offsets.phases_deg.resize(nDaqChannel * nMultiplexerChannel, 0);

    std::lock_guard lock(plotMutex);
    for(auto& plotBuffer : DoubleBuffers){
        plotBuffer.times.resize(ringBufferSize);
        plotBuffer.ys.resize(chs.size());
        for(auto& values : plotBuffer.ys){
            values.resize(ringBufferSize);
        }
        plotBuffer.matrix.resize(nDaqChannel * nMultiplexerChannel * ringBufferSize);
        plotBuffer.matrix2.resize(RBF_K * nDaqChannel * nMultiplexerChannel * ringBufferSize);
        std::fill(plotBuffer.matrix.begin(), plotBuffer.matrix.end(), 0.0);
        std::fill(plotBuffer.matrix2.begin(), plotBuffer.matrix2.end(), 0.0);
        plotBuffer.idxWrite = 0;
        plotBuffer.idxCurrent = 0;
        plotBuffer.nofm = 0;
    }
    plotActive.store(0, std::memory_order_relaxed);

    ch_multi = 0;
}

void RingBuffer::init() {
    init(dt, historySec, scopeCfg.nDaqChannel, scopeCfg.nMultiChannel);
}

void RingBuffer::pop(const double xs[], const double ys[]){
    static std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();
    
    // ロック：アクティブバッファへの全書き込みを保護
    {
        std::lock_guard lock(plotMutex);
        
        const int activePlot = plotActive.load(std::memory_order_relaxed);
        auto& activeBuf = DoubleBuffers[activePlot];
        const int bufferSize = activeBuf.times.size();
        int idxWrite = activeBuf.idxWrite;
        int idxCurrent = activeBuf.idxCurrent;
        int nofm = activeBuf.nofm;
        
        // 値の更新
        activeBuf.times[idxWrite] = std::chrono::duration<double>(
            std::chrono::steady_clock::now() - start_time
        ).count();
        if (offsets.flag){
            for(int ch = 0; ch < chs.size(); ++ch){
                offsets.chs[ch].real(xs[ch]);
                offsets.chs[ch].imag(ys[ch]);
            }
            offsets.flag = false;
        }
        
        // RBF補間のトレーニングデータを格納する配列
        std::vector<double> x_train(chs.size()), y_train(chs.size());

        // 測定値のringBufferへの格納とコンター図用配列matrixの作成
        for(int ch = 0; ch < chs.size(); ++ch){
            chs[ch].xs[idxWrite] = xs[ch];
            chs[ch].ys[idxWrite] = ys[ch];
            const int idx = idxWrite * chs.size() + ch;
            activeBuf.matrix[idx] = chs[ch].ys[idxWrite];
            x_train[ch] = ch;
            y_train[ch] = chs[ch].ys[idxWrite];
        }

        // RBF補間 (epsilon = 0.8)
        Math::RBFInterpolation1D rbf;
        rbf.fit(x_train, y_train, 0.8, Math::RBFType::Multiquadric);
        for (int ch = 0; ch < x_train.size() * RBF_K; ++ch) {
            activeBuf.matrix2[idxWrite * x_train.size() * RBF_K + ch] = rbf.predict((double)ch/RBF_K);
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
                else if (trigger.nofm + bufferSize / 2 <= nofm) {
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
        if(idxWrite >= bufferSize) {idxWrite = 0;}

        activeBuf.times[idxCurrent] = activeBuf.times[idxCurrent];
        for(int ch = 0; ch < chs.size(); ++ch){
            activeBuf.ys[ch][idxCurrent] = chs[ch].ys[idxCurrent];
            activeBuf.matrix[idxCurrent * chs.size() + ch] = activeBuf.matrix[idxCurrent * chs.size() + ch];
        }
        for(int ch = 0; ch < chs.size() * RBF_K; ++ch){
            activeBuf.matrix2[idxCurrent * chs.size() * RBF_K + ch] = activeBuf.matrix2[idxCurrent * chs.size() * RBF_K + ch];
        }
        
        activeBuf.idxWrite = idxWrite;
        activeBuf.idxCurrent = idxCurrent;
        activeBuf.nofm = nofm;
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
