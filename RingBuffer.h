#include <array>
#include <atomic>
#include <complex>
#include <mutex>
#include <vector>

class RingBuffer {
    private:
        struct ComplexVector {
            std::vector<double> xs, ys;
        };
        struct Offsets {
            std::vector<std::complex<float>> chs;
            std::vector<float> phases_deg;
            bool flag = false;
        };
        struct SourceCh{
            float frequency = 0.0f;
            float amplitude = 0.0f;
            float phase = 0.0f;
            int func = 1; //funcSine
        };
        struct ScopeConfig{
            int nDaqChannel = 1;
            int nMultiChannel = 1;
        };
        struct Trigger {
            bool flag = false;
            bool readyFlag = false;
            bool countFlag = false;
            int nofm = 0;
            double level = 0.0;
        };
        struct Buffer {
            std::vector<double> times;
            std::vector<std::vector<double>> ys;
            std::vector<double> matrix, matrix2;
            int idxWrite = 0;
            int idxCurrent = 0;
            int nofm = 0;
        };
    public:
        bool pauseFlag = false;
        double dt = 0;
        double historySec = 0;
        int ch_multi = 0;
        const int RBF_K = 4;
        std::vector<ComplexVector> chs;
        Offsets offsets;
        std::vector<SourceCh> sourceChs;
        ScopeConfig scopeCfg;
        Trigger trigger;
        std::array<Buffer, 2> DoubleBuffers;
        std::atomic<int> plotActive = 0;
        std::mutex plotMutex;
        void initSource(const float frequency, const float ampCh1, const float ampCh2);
        void init(const double newRingDt, const double newHistorySec, const int n_daq_channel, const int n_multiplexer_channel);
        void init();
        void pop(const double xs[], const double ys[]);
        void update(const std::vector<std::vector<double>>& rawChs, const double rawDt);
    };