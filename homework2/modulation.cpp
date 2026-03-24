#include <cmath>
#include <cstdlib>

static const double PI = 3.14159265358979323846;

int generate_cover_signal(double* cover, const int size) {
    if (!cover || size <= 0) return -1;

    // 用 100Hz 采样，载波频率 5Hz。
    const double fs = 100.0;
    const double fc = 5.0;

    for (int i = 0; i < size; ++i) {
        double t = i / fs;
        cover[i] = std::sin(2.0 * PI * fc * t);
    }
    return size;
}

int simulate_digital_modulation_signal(unsigned char* message, const int size) {
    if (!message || size <= 0) return -1;

    for (int i = 0; i < size; ++i) {
        message[i] = (unsigned char)(std::rand() % 2);
    }
    return size;
}

int simulate_analog_modulation_signal(double* message, const int size) {
    if (!message || size <= 0) return -1;

    // 模拟低频模拟信号: 范围大约在 [-1, 1]
    const double fs = 100.0;
    const double fm = 1.0;

    for (int i = 0; i < size; ++i) {
        double t = i / fs;
        message[i] = std::sin(2.0 * PI * fm * t);
    }
    return size;
}

int modulate_digital_frequency(double* cover, const int cover_len, const unsigned char* message, const int msg_len) {
    if (!cover || !message || cover_len <= 0 || msg_len <= 0) return -1;

    const double fs = 100.0;
    const double f0 = 3.0;
    const double f1 = 8.0;
    int n = (cover_len < msg_len) ? cover_len : msg_len;

    for (int i = 0; i < n; ++i) {
        double t = i / fs;
        double f = message[i] ? f1 : f0;
        cover[i] = std::sin(2.0 * PI * f * t);
    }
    return n;
}

int modulate_analog_frequency(double* cover, const int cover_len, const double* message, const int msg_len) {
    if (!cover || !message || cover_len <= 0 || msg_len <= 0) return -1;

    const double fs = 100.0;
    const double fc = 5.0;
    const double kf = 2.0;
    int n = (cover_len < msg_len) ? cover_len : msg_len;

    double phase = 0.0;
    for (int i = 0; i < n; ++i) {
        double inst_f = fc + kf * message[i];
        phase += 2.0 * PI * inst_f / fs;
        cover[i] = std::sin(phase);
    }
    return n;
}

int modulate_digital_amplitude(double* cover, const int cover_len, const unsigned char* message, const int msg_len) {
    if (!cover || !message || cover_len <= 0 || msg_len <= 0) return -1;

    const double fs = 100.0;
    const double fc = 5.0;
    int n = (cover_len < msg_len) ? cover_len : msg_len;

    for (int i = 0; i < n; ++i) {
        double t = i / fs;
        double amp = message[i] ? 1.0 : 0.3;
        cover[i] = amp * std::sin(2.0 * PI * fc * t);
    }
    return n;
}

int modulate_analog_amplitude(double* cover, const int cover_len, const double* message, const int msg_len) {
    if (!cover || !message || cover_len <= 0 || msg_len <= 0) return -1;

    const double fs = 100.0;
    const double fc = 5.0;
    const double ka = 0.5;
    int n = (cover_len < msg_len) ? cover_len : msg_len;

    for (int i = 0; i < n; ++i) {
        double t = i / fs;
        double amp = 1.0 + ka * message[i];
        cover[i] = amp * std::sin(2.0 * PI * fc * t);
    }
    return n;
}

int modulate_digital_phase(double* cover, const int cover_len, const unsigned char* message, const int msg_len) {
    if (!cover || !message || cover_len <= 0 || msg_len <= 0) return -1;

    const double fs = 100.0;
    const double fc = 5.0;
    int n = (cover_len < msg_len) ? cover_len : msg_len;

    for (int i = 0; i < n; ++i) {
        double t = i / fs;
        double phase = message[i] ? PI : 0.0;
        cover[i] = std::sin(2.0 * PI * fc * t + phase);
    }
    return n;
}

int modulate_analog_phase(double* cover, const int cover_len, const double* message, const int msg_len) {
    if (!cover || !message || cover_len <= 0 || msg_len <= 0) return -1;

    const double fs = 100.0;
    const double fc = 5.0;
    const double kp = PI / 2.0;
    int n = (cover_len < msg_len) ? cover_len : msg_len;

    for (int i = 0; i < n; ++i) {
        double t = i / fs;
        double phase = kp * message[i];
        cover[i] = std::sin(2.0 * PI * fc * t + phase);
    }
    return n;
}
