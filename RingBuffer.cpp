#include "RingBuffer.h"
#include "Psd.h"
#include "Rbf.h"
#include <cmath>

// ============================================================
// 初期化処理
// ============================================================

void RingBuffer::initSource(const float frequency, const float ampCh1, const float ampCh2){
    sourceChs.resize(2);
    sourceChs[0].frequency = frequency;
    sourceChs[0].amplitude = ampCh1;
    sourceChs[1].frequency = sourceChs[0].frequency;
    sourceChs[1].amplitude = ampCh2;
}

void RingBuffer::init(const double newRingDt, const double newHistorySec, const int nDaqChannel, const int nMultiplexerChannel){
    // ============ パラメータ設定 ============
    dt = newRingDt;
    historySec = newHistorySec;
    scopeCfg.nDaqChannel = nDaqChannel;
    scopeCfg.nMultiChannel = nMultiplexerChannel;
    const int ringBufferSize = historySec / newRingDt / nMultiplexerChannel;
    
    // ============ 計測バッファの初期化 ============
    meaBuffer.chs.resize(nDaqChannel * nMultiplexerChannel);
    for(int i=0; i < meaBuffer.chs.size(); ++i){
        meaBuffer.chs[i].xs.resize(ringBufferSize, 0);
        meaBuffer.chs[i].ys.resize(ringBufferSize, 0);
    }
    meaBuffer.times.resize(ringBufferSize, 0.0);
    meaBuffer.idxWrite = 0;
    meaBuffer.idxCurrent = 0;
    meaBuffer.nofm = 0;
    
    // ============ オフセット・位相の初期化 ============
    offsets.chs.resize(nDaqChannel * nMultiplexerChannel, 0);
    offsets.phases_deg.resize(nDaqChannel * nMultiplexerChannel, 0);

    // ============ ダブルバッファの初期化 ============
    {
        std::lock_guard lock(plotMutex);
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
    ch_multi = 0;
}

void RingBuffer::init() {
    init(dt, historySec, scopeCfg.nDaqChannel, scopeCfg.nMultiChannel);
}

void RingBuffer::pop(const double xs[], const double ys[], const double sampleTime){
    static std::chrono::steady_clock::time_point start_time = std::chrono::steady_clock::now();



    // ============ オフセット適用 ============
    if (offsets.flag){
        for(int ch = 0; ch < meaBuffer.chs.size(); ++ch){
            offsets.chs[ch].real(xs[ch]);
            offsets.chs[ch].imag(ys[ch]);
        }
        offsets.flag = false;
    }

    // ============ 計測バッファに書き込み ============
    meaBuffer.times[meaBuffer.idxWrite] = sampleTime;
    for(int ch = 0; ch < meaBuffer.chs.size(); ++ch){
        meaBuffer.chs[ch].xs[meaBuffer.idxWrite] = xs[ch];
        meaBuffer.chs[ch].ys[meaBuffer.idxWrite] = ys[ch];
    }

    // ============ トリガー処理 ============
    updateTrigger();

    // ============ インデックス更新とリングバッファラップ ============
    meaBuffer.idxCurrent = meaBuffer.idxWrite;
    meaBuffer.idxWrite++;
    meaBuffer.nofm++;
    if(meaBuffer.idxWrite >= meaBuffer.times.size()) { 
        meaBuffer.idxWrite = 0; 
    }

    // ============ ダブルバッファ更新（GUI用） ============
    updatePlotBuffer();
}

// ============================================================
// トリガー処理（条件判定と状態更新）
// ============================================================

void RingBuffer::updateTrigger(){
    if (!trigger.flag) {
        trigger.nofm = 0;
        return;
    }

    const double currentValue = meaBuffer.chs[0].ys[meaBuffer.idxCurrent];
    const bool isAboveLevel = currentValue >= trigger.level;
    const bool isBelowLevel = currentValue <= trigger.level;

    // ============ 準備フェーズ ============
    if (!trigger.readyFlag && !trigger.countFlag) {
        bool allAboveOrEqual = true;
        for(int ch = 0; ch < meaBuffer.chs.size(); ++ch){
            const double val = meaBuffer.chs[ch].ys[meaBuffer.idxCurrent];
            if (trigger.level >= 0 && val <= trigger.level) {
                allAboveOrEqual = false;
                break;
            }
            if (trigger.level < 0 && val >= trigger.level) {
                allAboveOrEqual = false;
                break;
            }
        }
        if (allAboveOrEqual) {
            trigger.readyFlag = true;
        }
    }

    // ============ カウントフェーズ（トリガー発火） ============
    if (trigger.readyFlag && !trigger.countFlag) {
        bool triggered = false;
        for(int ch = 0; ch < meaBuffer.chs.size(); ++ch){
            const double val = meaBuffer.chs[ch].ys[meaBuffer.idxCurrent];
            if ((trigger.level >= 0 && val >= trigger.level) ||
                (trigger.level < 0 && val <= trigger.level)) {
                triggered = true;
                break;
            }
        }
        if (triggered) {
            trigger.countFlag = true;
        }
    }

    // ============ 完了フェーズ ============
    if (trigger.readyFlag && trigger.countFlag) {
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

// ============================================================
// ダブルバッファ更新（GPU用にデータを準備）
// ============================================================

void RingBuffer::updatePlotBuffer(){
    // ============ RBF補間モデルの学習 ============
    Math::RBFInterpolation1D rbf;
    std::vector<double> x_train(meaBuffer.chs.size()), y_train(meaBuffer.chs.size());
    for(int ch = 0; ch < meaBuffer.chs.size(); ++ch){
        x_train[ch] = ch + 0.5;
        y_train[ch] = meaBuffer.chs[ch].ys[meaBuffer.idxCurrent];
    }
    rbf.fit(x_train, y_train, 0.8, Math::RBFType::Multiquadric);

    {
        std::lock_guard lock(plotMutex);
        const int idxEnd = plotBuffer.idxWrite <= meaBuffer.idxCurrent 
                            ? meaBuffer.idxCurrent 
                            : plotBuffer.times.size() - 1;
        
        // 1要素分のデータコピーおよびRBF書き込みを行う共通関数
        auto copyIndexData = [&](int idx) {
            plotBuffer.times[idx] = meaBuffer.times[idx];

            for (size_t ch = 0; ch < meaBuffer.chs.size(); ++ch) {
                const double yVal = meaBuffer.chs[ch].ys[idx];

                plotBuffer.ys[ch][idx] = yVal;
                plotBuffer.matrix[idx * meaBuffer.chs.size() + ch] = yVal;

                // 事前計算したRBF補間値をコピー
                const size_t rbfBaseIdx = idx * meaBuffer.chs.size() * RBF_K + ch * RBF_K;
                for (int k = 0; k < RBF_K; ++k) {
                    plotBuffer.matrixRBF[rbfBaseIdx + k] = rbf.predict((double)(ch * RBF_K + k) / RBF_K);
                }
            }
        };

        // 書き込み範囲の算出
        const int idxStart = plotBuffer.idxWrite;
        const int idxCurrent = meaBuffer.idxCurrent;
        const int bufSize = static_cast<int>(plotBuffer.times.size());

        if (idxStart <= idxCurrent) {
            // ============ 通常ラップ処理 ============
            // 区間: [idxStart, idxCurrent]
            for (int idx = idxStart; idx <= idxCurrent; ++idx) {
                copyIndexData(idx);
            }
        } else {
            // ============ ラップアラウンド時の処理 ============
            // 区間1: [idxStart, bufSize - 1]
            for (int idx = idxStart; idx < bufSize; ++idx) {
                copyIndexData(idx);
            }
            // 区間2: [0, idxCurrent]
            for (int idx = 0; idx <= idxCurrent; ++idx) {
                copyIndexData(idx);
            }
        }
        
        // ============ メタデータ更新 ============
        plotBuffer.idxWrite = meaBuffer.idxWrite;
        plotBuffer.idxCurrent = meaBuffer.idxCurrent;
        plotBuffer.nofm = meaBuffer.nofm;
    }
}

void RingBuffer::update(const std::vector<std::vector<double>>& rawChs, const double rawDt, const double sampleTime){
    // ============ PSD 解析（初回は初期化） ============
    static Psd psd;
    if(psd.frequency != sourceChs[0].frequency || psd.getSize() != rawChs[0].size() || psd.dt != rawDt){
        psd.init(rawChs[0].size(), sourceChs[0].frequency, rawDt);
    }
    
    // ============ 各チャネルの PSD 計算 + 位相補正 ============
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
    
    // ============ マルチプレクサの次チャネルへ、または pop() を実行 ============
    ch_multi++;
    if(ch_multi >= scopeCfg.nMultiChannel){
        pop(xs.data(), ys.data(), sampleTime);
        ch_multi = 0;
    }
}
