#pragma once

#include <vector>
#include <cmath>
#include <stdexcept>
#include <functional>

namespace Math {

enum class RBFType {
    Gaussian,
    Multiquadric,
    InverseMultiquadric,
    Linear,
    Cubic
};

class RBFInterpolation1D {
public:
    RBFInterpolation1D() = default;

    // データ点, 値, ガウス等のスケールパラメータepsilon, RBFの種類を指定して学習
    void fit(const std::vector<double>& x, const std::vector<double>& y, double epsilon = 1.0, RBFType type = RBFType::Gaussian) {
        if (x.size() != y.size() || x.empty()) {
            throw std::invalid_argument("Input vectors x and y must have the same non-zero size.");
        }

        x_ = x;
        y_ = y;
        epsilon_ = epsilon;
        type_ = type;
        size_t n = x_.size();

        // カーネル関数の設定
        setKernel();

        // A行列の作成
        std::vector<std::vector<double>> A(n, std::vector<double>(n, 0.0));
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = 0; j < n; ++j) {
                double r = std::abs(x_[i] - x_[j]);
                A[i][j] = kernel_(r, epsilon_);
            }
        }

        // ガウスの消去法（ピボット選択付き）で重み w (A * w = y) を算出
        weights_ = solveLinearSystem(A, y_);
    }

    // 補間値の計算
    double predict(double target_x) const {
        if (weights_.empty()) {
            throw std::runtime_error("Model has not been fitted yet.");
        }

        double result = 0.0;
        for (size_t i = 0; i < x_.size(); ++i) {
            double r = std::abs(target_x - x_[i]);
            result += weights_[i] * kernel_(r, epsilon_);
        }
        return result;
    }

private:
    std::vector<double> x_;
    std::vector<double> y_;
    std::vector<double> weights_;
    double epsilon_ = 1.0;
    RBFType type_ = RBFType::Gaussian;
    std::function<double(double, double)> kernel_;

    void setKernel() {
        switch (type_) {
            case RBFType::Gaussian:
                kernel_ = [](double r, double eps) { return std::exp(-std::pow(eps * r, 2)); };
                break;
            case RBFType::Multiquadric:
                kernel_ = [](double r, double eps) { return std::sqrt(1.0 + std::pow(eps * r, 2)); };
                break;
            case RBFType::InverseMultiquadric:
                kernel_ = [](double r, double eps) { return 1.0 / std::sqrt(1.0 + std::pow(eps * r, 2)); };
                break;
            case RBFType::Linear:
                kernel_ = [](double r, double) { return r; };
                break;
            case RBFType::Cubic:
                kernel_ = [](double r, double) { return std::pow(r, 3); };
                break;
        }
    }

    // 連立一次方程式 A * x = b を解く内部関数
    std::vector<double> solveLinearSystem(std::vector<std::vector<double>> A, const std::vector<double>& b) {
        size_t n = A.size();
        std::vector<double> x = b;

        for (size_t i = 0; i < n; ++i) {
            // ピボット選択
            size_t max_row = i;
            for (size_t k = i + 1; k < n; ++k) {
                if (std::abs(A[k][i]) > std::abs(A[max_row][i])) {
                    max_row = k;
                }
            }
            std::swap(A[i], A[max_row]);
            std::swap(x[i], x[max_row]);

            if (std::abs(A[i][i]) < 1e-12) {
                throw std::runtime_error("Matrix is singular or near-singular.");
            }

            // 前進消去
            for (size_t k = i + 1; k < n; ++k) {
                double c = -A[k][i] / A[i][i];
                for (size_t j = i; j < n; ++j) {
                    if (i == j) {
                        A[k][j] = 0;
                    } else {
                        A[k][j] += c * A[i][j];
                    }
                }
                x[k] += c * x[i];
            }
        }

        // 後退代入
        for (int i = static_cast<int>(n) - 1; i >= 0; --i) {
            x[i] /= A[i][i];
            for (int k = i - 1; k >= 0; --k) {
                x[k] -= A[k][i] * x[i];
            }
        }

        return x;
    }
};

} // namespace Math
