#include "rtp_parser.hpp"

// =============================================================================
//  Die Fuzzing-Schnittstelle (libFuzzer-Konvention).
//  libFuzzer - oder der Standalone-Treiber (standalone_main.cpp) - ruft diese
//  Funktion mit JEDEM erzeugten Eingabe-Puffer auf. Rueckgabe 0 = OK.
//
//  Mit -DSAFE wird die abgesicherte Variante getestet (findet keinen Crash),
//  ohne -DSAFE die verwundbare (Fuzzer/ASan findet den Overflow).
// =============================================================================
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
#ifdef SAFE
    parse_rtp_safe(data, size);
#else
    parse_rtp_vulnerable(data, size);
#endif
    return 0;
}
