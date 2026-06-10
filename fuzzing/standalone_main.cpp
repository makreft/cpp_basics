// =============================================================================
//  Standalone-Treiber (kein libFuzzer / kein clang noetig)
//
//  Ruft LLVMFuzzerTestOneInput() mit dem Inhalt jeder als Argument uebergebenen
//  Datei auf. Damit laesst sich das Fuzz-Target AUCH mit g++/ASan bauen und ein
//  konkreter (boeser) Input "abspielen" - ideal, um einen gefundenen Crash zu
//  REPRODUZIEREN, ohne clang installieren zu muessen.
//
//  Fuer echtes coverage-guided Fuzzing siehe README.md (Build mit clang
//  '-fsanitize=fuzzer,address' - dann liefert libFuzzer selbst das main()).
// =============================================================================
#include <cstdint>
#include <cstddef>
#include <cstdio>
#include <vector>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size);

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "Aufruf: %s <input-datei> [weitere ...]\n", argv[0]);
        return 2;
    }
    for (int i = 1; i < argc; ++i) {
        FILE* f = std::fopen(argv[i], "rb");
        if (!f) { std::perror(argv[i]); continue; }
        std::fseek(f, 0, SEEK_END);
        long n = std::ftell(f);
        std::fseek(f, 0, SEEK_SET);
        if (n < 0) { std::fclose(f); continue; }
        std::vector<uint8_t> buf(static_cast<size_t>(n));
        if (n > 0) {
            size_t got = std::fread(buf.data(), 1, buf.size(), f);
            buf.resize(got);
        }
        std::fclose(f);
        std::printf("[*] '%s' (%zu Bytes) -> Parser ...\n", argv[i], buf.size());
        LLVMFuzzerTestOneInput(buf.data(), buf.size());
        std::printf("[OK] '%s' ohne Crash verarbeitet\n", argv[i]);
    }
    return 0;
}
