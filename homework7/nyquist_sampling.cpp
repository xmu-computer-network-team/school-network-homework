#include <cmath>
#include <iostream>
#include <vector>

static const double PI = 3.14159265358979323846;

// 连续信号: 由 50Hz 和 120Hz 两个分量组成，最高有效频率为 120Hz。
static double analog_signal(double t) {
    return 1.0 * std::sin(2.0 * PI * 50.0 * t)
         + 0.6 * std::sin(2.0 * PI * 120.0 * t);
}

// 在 [0, duration] 内按采样频率 fs 采样。
static int sample_signal(std::vector<double>& samples, double fs, double duration) {
    if (fs <= 0.0 || duration <= 0.0) return -1;

    int n = (int)(duration * fs) + 1;
    samples.resize(n);

    for (int i = 0; i < n; ++i) {
        double t = i / fs;
        samples[i] = analog_signal(t);
    }
    return n;
}

// 用理想 sinc 插值重建信号值。
static double sinc_interpolate(const std::vector<double>& samples, double fs, double t) {
    double sum = 0.0;
    int n = (int)samples.size();

    for (int k = 0; k < n; ++k) {
        double tk = k / fs;
        double x = fs * (t - tk);

        double sinc_val;
        if (std::fabs(x) < 1e-12) {
            sinc_val = 1.0;
        } else {
            sinc_val = std::sin(PI * x) / (PI * x);
        }

        sum += samples[k] * sinc_val;
    }

    return sum;
}

// 评估重建误差（均方根误差 RMSE）
static double evaluate_rmse(const std::vector<double>& samples, double fs, double duration, int eval_points) {
    if (samples.empty() || fs <= 0.0 || duration <= 0.0 || eval_points <= 1) return -1.0;

    double mse = 0.0;
    for (int i = 0; i < eval_points; ++i) {
        double t = duration * i / (eval_points - 1);
        double y_true = analog_signal(t);
        double y_rec = sinc_interpolate(samples, fs, t);
        double err = y_true - y_rec;
        mse += err * err;
    }
    mse /= eval_points;

    return std::sqrt(mse);
}

static void run_case(double fs, double duration, double max_freq) {
    std::vector<double> samples;
    int n = sample_signal(samples, fs, duration);

    if (n <= 0) {
        std::cout << "采样失败，fs=" << fs << std::endl;
        return;
    }

    double rmse = evaluate_rmse(samples, fs, duration, 1000);

    std::cout << "\n===== 采样频率 fs = " << fs << " Hz =====" << std::endl;
    std::cout << "最高有效频率 fmax = " << max_freq << " Hz, 2*fmax = " << 2.0 * max_freq << " Hz" << std::endl;
    std::cout << "采样点数: " << n << std::endl;
    std::cout << "重建 RMSE: " << rmse << std::endl;

    if (fs > 2.0 * max_freq) {
        std::cout << "结论: fs > 2*fmax，重建误差较小，可较好重建。" << std::endl;
    } else {
        std::cout << "结论: fs <= 2*fmax，出现混叠，无法准确重建。" << std::endl;
    }

    // 打印少量样例点便于观察。
    std::cout << "部分采样值: ";
    int show_n = (n < 6) ? n : 6;
    for (int i = 0; i < show_n; ++i) {
        std::cout << samples[i] << " ";
    }
    std::cout << std::endl;
}

int main() {
    // 信号最高有效频率 120Hz。
    const double max_freq = 120.0;
    const double duration = 0.1;  // 模拟 0.1s。

    std::cout << "Nyquist 采样定理模拟" << std::endl;
    std::cout << "信号: sin(2*pi*50*t) + 0.6*sin(2*pi*120*t)" << std::endl;

    // 情况1: 大于两倍最高频率（满足Nyquist）
    run_case(400.0, duration, max_freq);

    // 情况2: 小于两倍最高频率（不满足Nyquist）
    run_case(180.0, duration, max_freq);

    return 0;
}
