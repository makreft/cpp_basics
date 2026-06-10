// =============================================================================
//  Thema: Video & Streaming  ->  I/P/B-Frames und GOP
// =============================================================================
//
//  H.264/H.265 komprimieren Video mit drei Frame-Typen:
//    I-Frame (Intra)   : komplettes Bild, unabhaengig. Gross. = "Keyframe"
//    P-Frame (Predicted): nur Differenz zum vorherigen Bild. Klein.
//    B-Frame (Bi-pred) : Differenz aus Vergangenheit UND Zukunft. Am kleinsten.
//
//  GOP (Group of Pictures) = Abstand zwischen zwei I-Frames.
//  Wichtig fuer Aufzeichnung und Wiedergabe:
//    - Schneiden/Seeken geht nur sauber an einem I-Frame.
//    - Nach Paketverlust kann der Decoder erst am naechsten I-Frame wieder
//      ein korrektes Bild zeigen.
//
//  Bauen:  g++ -std=c++17 -g -O0 -Wall -Wextra 07_video_frames_gop.cpp -o 07
// =============================================================================

#include <iostream>
#include <vector>
#include <string>

enum class Typ { I, P, B };

struct VideoFrame {
    int  index;
    Typ  typ;
    int  groesseKB;   // grobe, typische Groessenordnung
};

char buchstabe(Typ t) { return t == Typ::I ? 'I' : (t == Typ::P ? 'P' : 'B'); }

// Erzeugt eine simple Frame-Folge mit fester GOP-Laenge: I P P P I P P P ...
std::vector<VideoFrame> erzeugeStream(int anzahl, int gop) {
    std::vector<VideoFrame> stream;
    for (int i = 0; i < anzahl; ++i) {
        if (i % gop == 0)
            stream.push_back({i, Typ::I, 30});   // I-Frame: gross
        else
            stream.push_back({i, Typ::P, 4});    // P-Frame: klein
    }
    return stream;
}

int main() {
    std::cout << "=== I/P/B-Frames und GOP ===\n\n";

    const int GOP = 4;
    auto stream = erzeugeStream(12, GOP);

    // 1) Stream anzeigen + Bandbreite vergleichen
    std::cout << "1) Stream (GOP=" << GOP << "):  ";
    int summe = 0;
    for (auto& f : stream) { std::cout << buchstabe(f.typ); summe += f.groesseKB; }
    std::cout << "\n   Gesamtgroesse = " << summe << " KB"
              << " (haetten wir nur I-Frames: "
              << static_cast<int>(stream.size()) * 30 << " KB)\n\n";

    // 2) Seeking: zu Frame 9 springen -> wir muessen beim vorherigen I-Frame
    //    anfangen zu dekodieren, nicht mitten in den P-Frames.
    int ziel = 9;
    int startI = (ziel / GOP) * GOP;
    std::cout << "2) Seek zu Frame " << ziel
              << ": Dekodierung muss bei I-Frame " << startI << " starten,\n"
              << "   dann P-Frames bis " << ziel << " anwenden.\n\n";

    // 3) Paketverlust: Frame 5 (ein P-Frame) geht verloren.
    int verloren = 5;
    std::cout << "3) Frame " << verloren << " (" << buchstabe(stream[verloren].typ)
              << ") geht verloren:\n";
    std::cout << "   Bild gestoert ab Frame " << verloren << " ";
    int erholung = ((verloren / GOP) + 1) * GOP;
    std::cout << "bis zum naechsten I-Frame (" << erholung << "),\n"
              << "   ab dort wieder sauberes Bild.\n\n";

    std::cout << "Merksatz: kurze GOP = robuster + besseres Seeking, aber mehr\n"
                 "Bandbreite. Je nach Anwendung bewusst abwaegen.\n";
    return 0;
}
