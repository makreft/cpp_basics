#pragma once
#include <cstdint>
#include <cstddef>

// =============================================================================
//  Mini-"RTP"-Parser fuer die Fuzzing-Demo
//  (stark vereinfacht - das ist NICHT echtes RTP, nur ein Lehrbeispiel!)
//
//  Pseudo-Header (9 Bytes):
//    [0]    Version / Flags
//    [1]    Payload-Type
//    [2..3] Sequenznummer
//    [4..7] Timestamp
//    [8]    Payload-Laenge  <-- behauptet, wie viele Payload-Bytes folgen.
//                               DAS ist das angreiferkontrollierte Feld.
//  danach: die Payload-Bytes.
// =============================================================================

// Verwundbar: vertraut dem Laengenfeld aus dem Paket blind.
void parse_rtp_vulnerable(const uint8_t* data, size_t size);

// Abgesichert: validiert gegen die tatsaechliche Paketgroesse UND die
// Zielpuffergroesse, bevor kopiert wird.
void parse_rtp_safe(const uint8_t* data, size_t size);
