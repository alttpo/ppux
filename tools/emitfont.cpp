#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

#include <chrono>
#include <thread>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../drawlist_fwd.hpp"

using namespace DrawList;

void sleep(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

int main(int argc, char **argv) {
    uint32_t sent = 0;

#ifdef _WIN32
    setmode(fileno(stdout),O_BINARY);
    setmode(fileno(stdin),O_BINARY);
#endif

    // font:
    uint32_t s = 0;
    FILE* f = fopen(argv[1], "rb");

    uint32_t a = 0xFF220000 + 1;
    // write size of font:
    fseek(f, 0, SEEK_END);
    s = ftell(f);
    sent = 0;
    sent += printf("WRITE_CORE_MEMORY %lX", a);
    sent += printf(" %02X %02X %02X", s & 0xFF, (s >> 8) & 0xFF, (s >> 16) & 0xFF);
    sent += printf("\n");
    fseek(f, 0, SEEK_SET);
    a += 3;

    // write chunks of font data:
    for (;;) {
        const size_t N = 256;
        uint8_t buf[N];
        size_t n = fread(buf, 1, N, f);
        sent += printf("WRITE_CORE_MEMORY %lX", a);
        for (size_t i = 0; i < n; i++) {
            auto c = buf[i];
            sent += printf(" %02X", c);
        }
        sent += printf("\n");
        if (sent >= 16384) {
            sleep(17);
            sent = 0;
        }

        a += n;
        if (n < N) {
            break;
        }
    }

    // enable font loading now:
    printf("WRITE_CORE_MEMORY %lX 01\n", 0xFF220000);

    return 0;
}