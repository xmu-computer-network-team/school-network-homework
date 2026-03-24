#include <algorithm>
#include <cstring>

enum MultiplexMode {
    MODE_STAT_TDM = 1,
    MODE_SYNC_TDM = 2,
    MODE_FDM = 3,
    MODE_CDM = 4
};

static int g_multiplex_mode = MODE_SYNC_TDM;

int set_multiplex_mode(int mode) {
    if (mode < MODE_STAT_TDM || mode > MODE_CDM) return -1;
    g_multiplex_mode = mode;
    return 0;
}

int get_multiplex_mode() {
    return g_multiplex_mode;
}

static unsigned char to_bit(unsigned char x) {
    return x ? 1 : 0;
}

static void write_int_le(unsigned char* p, int v) {
    p[0] = (unsigned char)(v & 0xFF);
    p[1] = (unsigned char)((v >> 8) & 0xFF);
    p[2] = (unsigned char)((v >> 16) & 0xFF);
    p[3] = (unsigned char)((v >> 24) & 0xFF);
}

static int read_int_le(const unsigned char* p) {
    return (int)p[0]
        | ((int)p[1] << 8)
        | ((int)p[2] << 16)
        | ((int)p[3] << 24);
}

static int do_sync_tdm(unsigned char* out, int out_cap, const unsigned char* a, int a_len, const unsigned char* b, int b_len) {
    int max_len = std::max(a_len, b_len);
    if (out_cap < max_len * 2) return -1;

    int k = 0;
    for (int i = 0; i < max_len; ++i) {
        out[k++] = (i < a_len) ? to_bit(a[i]) : 0;
        out[k++] = (i < b_len) ? to_bit(b[i]) : 0;
    }
    return k;
}

// 统计时分: 只发送值为 1 的位置，降低发送量。
// token 编码: [src(0/1), idx_low, idx_high]
static int do_stat_tdm(unsigned char* out, int out_cap, const unsigned char* a, int a_len, const unsigned char* b, int b_len) {
    int k = 0;
    for (int i = 0; i < a_len; ++i) {
        if (to_bit(a[i]) == 1) {
            if (k + 3 > out_cap || i > 65535) return -1;
            out[k++] = 0;
            out[k++] = (unsigned char)(i & 0xFF);
            out[k++] = (unsigned char)((i >> 8) & 0xFF);
        }
    }
    for (int i = 0; i < b_len; ++i) {
        if (to_bit(b[i]) == 1) {
            if (k + 3 > out_cap || i > 65535) return -1;
            out[k++] = 1;
            out[k++] = (unsigned char)(i & 0xFF);
            out[k++] = (unsigned char)((i >> 8) & 0xFF);
        }
    }
    return k;
}

// 频分: 同一时刻 a/b 各占不同子载波，这里用一个 2bit 符号模拟。
static int do_fdm(unsigned char* out, int out_cap, const unsigned char* a, int a_len, const unsigned char* b, int b_len) {
    int max_len = std::max(a_len, b_len);
    if (out_cap < max_len) return -1;

    for (int i = 0; i < max_len; ++i) {
        unsigned char abit = (i < a_len) ? to_bit(a[i]) : 0;
        unsigned char bbit = (i < b_len) ? to_bit(b[i]) : 0;
        out[i] = (unsigned char)(abit | (unsigned char)(bbit << 1));
    }
    return max_len;
}

// 码分: A 用 [1,1], B 用 [1,-1]，每个时隙输出两个 chip（偏移 +2 后存 unsigned char）。
static int do_cdm(unsigned char* out, int out_cap, const unsigned char* a, int a_len, const unsigned char* b, int b_len) {
    int max_len = std::max(a_len, b_len);
    if (out_cap < max_len * 2) return -1;

    int k = 0;
    for (int i = 0; i < max_len; ++i) {
        int abit = (i < a_len && to_bit(a[i])) ? 1 : -1;
        int bbit = (i < b_len && to_bit(b[i])) ? 1 : -1;

        int chip0 = abit * 1 + bbit * 1;
        int chip1 = abit * 1 + bbit * (-1);

        out[k++] = (unsigned char)(chip0 + 2);
        out[k++] = (unsigned char)(chip1 + 2);
    }
    return k;
}

int multiplex(unsigned char* c, const int c_size, const unsigned char* a, const int a_len, const unsigned char* b, const int b_len) {
    if (!c || !a || !b || c_size <= 0 || a_len < 0 || b_len < 0) return -1;

    int mode = g_multiplex_mode;
    int header = 9;
    if (c_size < header) return -1;

    c[0] = (unsigned char)mode;
    write_int_le(c + 1, a_len);
    write_int_le(c + 5, b_len);

    int payload = -1;
    if (mode == MODE_STAT_TDM) {
        payload = do_stat_tdm(c + header, c_size - header, a, a_len, b, b_len);
    } else if (mode == MODE_SYNC_TDM) {
        payload = do_sync_tdm(c + header, c_size - header, a, a_len, b, b_len);
    } else if (mode == MODE_FDM) {
        payload = do_fdm(c + header, c_size - header, a, a_len, b, b_len);
    } else if (mode == MODE_CDM) {
        payload = do_cdm(c + header, c_size - header, a, a_len, b, b_len);
    }

    if (payload < 0) return -1;
    return header + payload;
}

int demultiplex(unsigned char* a, const int a_size, unsigned char* b, const int b_size, const unsigned char* c, const int c_len) {
    if (!a || !b || !c || a_size < 0 || b_size < 0 || c_len < 9) return -1;

    std::memset(a, 0, (size_t)a_size);
    std::memset(b, 0, (size_t)b_size);

    int mode = c[0];
    int a_len = read_int_le(c + 1);
    int b_len = read_int_le(c + 5);
    if (a_len < 0 || b_len < 0) return -1;

    int real_a = std::min(a_size, a_len);
    int real_b = std::min(b_size, b_len);

    const unsigned char* p = c + 9;
    int payload_len = c_len - 9;

    if (mode == MODE_STAT_TDM) {
        if (payload_len % 3 != 0) return -1;
        for (int i = 0; i < payload_len; i += 3) {
            int src = p[i];
            int idx = (int)p[i + 1] | ((int)p[i + 2] << 8);
            if (src == 0 && idx >= 0 && idx < real_a) a[idx] = 1;
            if (src == 1 && idx >= 0 && idx < real_b) b[idx] = 1;
        }
    } else if (mode == MODE_SYNC_TDM) {
        if (payload_len % 2 != 0) return -1;
        int slots = payload_len / 2;
        for (int i = 0; i < slots; ++i) {
            if (i < real_a) a[i] = to_bit(p[2 * i]);
            if (i < real_b) b[i] = to_bit(p[2 * i + 1]);
        }
    } else if (mode == MODE_FDM) {
        int slots = payload_len;
        for (int i = 0; i < slots; ++i) {
            if (i < real_a) a[i] = (unsigned char)(p[i] & 1);
            if (i < real_b) b[i] = (unsigned char)((p[i] >> 1) & 1);
        }
    } else if (mode == MODE_CDM) {
        if (payload_len % 2 != 0) return -1;
        int slots = payload_len / 2;
        for (int i = 0; i < slots; ++i) {
            int chip0 = (int)p[2 * i] - 2;
            int chip1 = (int)p[2 * i + 1] - 2;

            int ra = chip0 + chip1;
            int rb = chip0 - chip1;

            if (i < real_a) a[i] = (ra > 0) ? 1 : 0;
            if (i < real_b) b[i] = (rb > 0) ? 1 : 0;
        }
    } else {
        return -1;
    }

    return real_a + real_b;
}
