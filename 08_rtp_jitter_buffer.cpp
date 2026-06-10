// =============================================================================
//  Thema: RTP-Sequenznummern & Jitter Buffer
// =============================================================================
//
//  RTP transportiert die Medien (meist ueber UDP). UDP liefert Pakete u.U.
//    - in falscher REIHENFOLGE
//    - gar NICHT (Verlust)
//  Deshalb hat jedes RTP-Paket eine SEQUENZNUMMER (+ Timestamp). Der Empfaenger
//  nutzt einen "Jitter Buffer": kurz puffern, nach Sequenznummer sortieren,
//  Luecken (= Verlust) erkennen.
//
//  Dieses Programm simuliert ankommende Pakete in der Reihenfolge 1,3,2,5
//  (Paket 4 fehlt) und gibt sie sortiert + mit Verlust-Erkennung aus.
//
//  Bauen:  g++ -std=c++17 -g -O0 -Wall -Wextra 08_rtp_jitter_buffer.cpp -o 08
// =============================================================================

#include <iostream>
#include <map>
#include <vector>

struct RtpPaket {
    int seq;        // Sequenznummer
    int timestamp;  // Medienzeit (hier nur symbolisch)
};

int main() {
    std::cout << "=== RTP Sequenznummern & Jitter Buffer ===\n\n";

    // So treffen die Pakete ein (out of order, Paket 4 fehlt komplett):
    std::vector<RtpPaket> ankunft = {
        {1, 1000}, {3, 1200}, {2, 1100}, {5, 1400}
    };

    std::cout << "1) Ankunftsreihenfolge (wie das Netz sie liefert): ";
    for (auto& p : ankunft) std::cout << p.seq << " ";
    std::cout << "\n\n";

    // Jitter Buffer: nach Sequenznummer sortiert ablegen.
    std::map<int, RtpPaket> buffer;
    for (auto& p : ankunft) buffer[p.seq] = p;

    // Geordnet "abspielen" und dabei Luecken erkennen.
    std::cout << "2) Geordnete Wiedergabe (mit Verlust-Erkennung):\n";
    int erwartet = buffer.begin()->first;
    int letzte   = buffer.rbegin()->first;
    for (int seq = erwartet; seq <= letzte; ++seq) {
        auto it = buffer.find(seq);
        if (it != buffer.end())
            std::cout << "   seq " << seq << "  ts=" << it->second.timestamp
                      << "  -> wiedergegeben\n";
        else
            std::cout << "   seq " << seq
                      << "        -> FEHLT (Paketverlust, Concealment noetig)\n";
    }

    std::cout << "\nMerksatz: UDP ist schnell, aber unzuverlaessig. Sequenznummern\n"
                 "ermoeglichen Re-Ordering und Verlust-Erkennung; der Jitter Buffer\n"
                 "glaettet Laufzeitschwankungen - auf Kosten etwas Latenz.\n";
    return 0;
}
