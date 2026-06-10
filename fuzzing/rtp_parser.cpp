#include "rtp_parser.hpp"
#include <cstring>
#include <algorithm>

static const size_t HEADER = 9;   // Groesse des Pseudo-Headers

// -----------------------------------------------------------------------------
//  VERWUNDBAR
//  Der klassische Fehler: "dem Laengenfeld aus dem Netz blind glauben".
// -----------------------------------------------------------------------------
void parse_rtp_vulnerable(const uint8_t* data, size_t size) {
    if (size < HEADER) return;            // nur ein minimaler Header-Check

    uint8_t declared_len = data[8];       // angreiferkontrolliert: 0..255
    const uint8_t* payload = data + HEADER;

    uint8_t buf[64];                      // fester Puffer - nur 64 Bytes gross
    // BUG: declared_len kann bis 255 sein -> Stack Buffer Overflow (Schreiben).
    //      Ausserdem wird ueber das Paket hinaus GELESEN, wenn tatsaechlich
    //      weniger Payload da ist als behauptet. Kein Abgleich mit:
    //        - der echten Restgroesse (size - HEADER)
    //        - der Zielpuffergroesse (sizeof buf)
    std::memcpy(buf, payload, declared_len);

    // "Verarbeitung", damit buf wirklich benutzt und nicht wegoptimiert wird:
    volatile uint8_t sink = 0;
    for (size_t i = 0; i < sizeof(buf); ++i) sink ^= buf[i];
    (void)sink;
}

// -----------------------------------------------------------------------------
//  ABGESICHERT
//  Der Fix: gegen ALLE drei Grenzen begrenzen -
//    1) behauptete Laenge   (declared)
//    2) tatsaechlich vorhandene Bytes (avail)
//    3) Zielpuffergroesse   (sizeof buf)
// -----------------------------------------------------------------------------
void parse_rtp_safe(const uint8_t* data, size_t size) {
    if (size < HEADER) return;

    size_t declared = data[8];
    size_t avail    = size - HEADER;      // so viele Payload-Bytes sind WIRKLICH da
    const uint8_t* payload = data + HEADER;

    uint8_t buf[64];
    size_t n = std::min(declared, std::min(avail, sizeof(buf)));   // <-- der Fix
    std::memcpy(buf, payload, n);

    volatile uint8_t sink = 0;
    for (size_t i = 0; i < n; ++i) sink ^= buf[i];
    (void)sink;
}
