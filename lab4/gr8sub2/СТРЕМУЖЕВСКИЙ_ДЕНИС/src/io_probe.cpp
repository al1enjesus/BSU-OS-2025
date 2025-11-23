#include <bits/stdc++.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>

static double now_s() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

struct IoCounters {
    unsigned long long rbytes = 0, wbytes = 0;
};

static IoCounters read_self_io() {
    IoCounters c;
    std::ifstream f("/proc/self/io");
    std::string k;
    unsigned long long v;
    while (f >> k >> v) {
        if (k == "read_bytes:") c.rbytes = v;
        if (k == "write_bytes:") c.wbytes = v;
    }
    return c;
}

static void *alloc_aligned(size_t align, size_t sz) {
    void *p = nullptr;
    if (posix_memalign(&p, align, sz)) return nullptr;
    return p;
}

int main(int argc, char **argv) {
    std::string path = "io_test.bin", mode = "write";
    size_t size_mb = 100, bs = 1 << 20; // 1 MiB
    bool use_direct = false, use_sync = false;

    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];
        auto need = [&](const char *n) { return i + 1 < argc && a == n; };
        if (need("--file")) path = argv[++i];
        else if (need("--size-mb")) size_mb = strtoull(argv[++i], nullptr, 10);
        else if (need("--bs")) bs = strtoull(argv[++i], nullptr, 10);
        else if (need("--mode")) mode = argv[++i];
        else if (a == "--direct") use_direct = true;
        else if (a == "--sync") use_sync = true;
        else if (a == "--help") {
            fprintf(stderr,
                    "Usage: %s --file F --size-mb N --mode write|read [--bs B] [--direct] [--sync]\n"
                    " --direct  : O_DIRECT (обход page cache, нужен выровн. буфер/размер)\n"
                    " --sync    : fdatasync() (или O_SYNC для write)\n", argv[0]);
            return 0;
        }
    }

    const size_t total = size_mb * 1024ull * 1024ull;
    size_t align = use_direct ? 4096 : 64;
    if (use_direct && (bs % 4096)) {
        fprintf(stderr, "[warn] --bs округлён до кратного 4096\n");
        bs = (bs / 4096) * 4096;
        if (!bs) bs = 4096;
    }

    int flags = 0, fd = -1;
    if (mode == "write") {
        flags = O_CREAT | O_TRUNC | O_WRONLY;
        if (use_direct) flags |= O_DIRECT;
        if (use_sync) flags |= O_SYNC;
        fd = open(path.c_str(), flags, 0644);
    } else {
        flags = O_RDONLY;
        if (use_direct) flags |= O_DIRECT;
        fd = open(path.c_str(), flags);
    }
    if (fd < 0) {
        perror("open");
        return 1;
    }

    void *buf = use_direct ? alloc_aligned(align, bs) : malloc(bs);
    if (!buf) {
        perror("alloc");
        close(fd);
        return 1;
    }
    memset(buf, 0xA5, bs);


    IoCounters c0 = read_self_io();
    struct rusage u0;
    getrusage(RUSAGE_SELF, &u0);
    double t0 = now_s();

    size_t done = 0;
    if (mode == "write") {
        while (done < total) {
            size_t chunk = std::min(bs, total - done);
            ssize_t w = write(fd, buf, chunk);
            if (w < 0) {
                perror("write");
                break;
            }
            done += (size_t) w;
        }
        if (use_sync) { if (fdatasync(fd) != 0) perror("fdatasync"); }
    } else {
        while (done < total) {
            size_t chunk = std::min(bs, total - done);
            ssize_t r = read(fd, buf, chunk);
            if (r < 0) {
                perror("read");
                break;
            }
            if (r == 0) break;
            done += (size_t) r;
        }
    }

    double t1 = now_s();
    struct rusage u1;
    getrusage(RUSAGE_SELF, &u1);
    IoCounters c1 = read_self_io();

    close(fd);
    free(buf);

    double sec = std::max(1e-9, t1 - t0);
    double mb = done / (1024.0 * 1024.0);
    double mbps = mb / sec;

    printf("Mode: %s\nFile: %s\nSize: %.2f MB\nBlock: %zu bytes\n", mode.c_str(), path.c_str(), mb, bs);
    printf("Options: direct=%s, sync=%s\n", use_direct ? "yes" : "no", use_sync ? "yes" : "no");
    printf("Elapsed: %.3f s, Throughput: %.2f MB/s\n", sec, mbps);
    printf("rusage:  minflt=%ld majflt=%ld\n", u1.ru_minflt - u0.ru_minflt, u1.ru_majflt - u0.ru_majflt);
    printf("/proc/self/io delta: read_bytes=%llu write_bytes=%llu\n",
           (unsigned long long) (c1.rbytes - c0.rbytes),
           (unsigned long long) (c1.wbytes - c0.wbytes));
    return 0;
}
