// =============================================================================
//  Thema: Bildverarbeitung  ->  Motion Detection
// =============================================================================
//
//  Einfachste Bewegungserkennung = "Frame Differencing":
//  zwei aufeinanderfolgende Graustufenbilder voneinander abziehen, den Betrag
//  schwellwerten, und zaehlen, wie viele Pixel sich stark geaendert haben.
//  Use-Case: "nur aufzeichnen, wenn sich etwas bewegt".
//
//  Hier ohne OpenCV, nur mit einem kleinen 2D-Array, damit es ueberall baut.
//  In echt: cv::absdiff() + cv::threshold(), oder ein Background-Modell (MOG2).
//
//  Bauen:  g++ -std=c++17 -g -O0 -Wall -Wextra 09_motion_detection.cpp -o 09
// =============================================================================

#include <iostream>
#include <vector>
#include <cstdlib>   // std::abs

using Bild = std::vector<std::vector<int>>;   // Graustufen 0..255

// Hilfsausgabe: '.' = ruhig, '#' = Bewegung erkannt
void zeichne(const Bild& bewegung) {
    for (auto& zeile : bewegung) {
        std::cout << "   ";
        for (int v : zeile) std::cout << (v ? '#' : '.');
        std::cout << "\n";
    }
}

int main() {
    std::cout << "=== Motion Detection (Frame Differencing) ===\n\n";

    const int H = 6, B = 10;
    const int SCHWELLE = 30;   // ab welcher Helligkeitsaenderung gilt es als Bewegung

    // Frame 1: gleichmaessiger Hintergrund (Helligkeit 100)
    Bild frame1(H, std::vector<int>(B, 100));

    // Frame 2: gleicher Hintergrund, aber ein heller "Block" ist gewandert
    Bild frame2 = frame1;
    for (int y = 2; y < 4; ++y)
        for (int x = 4; x < 7; ++x)
            frame2[y][x] = 200;   // Objekt (z.B. Person) an neuer Stelle

    // Differenz bilden + schwellwerten
    Bild bewegung(H, std::vector<int>(B, 0));
    int bewegtePixel = 0;
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < B; ++x) {
            int diff = std::abs(frame2[y][x] - frame1[y][x]);
            if (diff > SCHWELLE) {
                bewegung[y][x] = 1;
                ++bewegtePixel;
            }
        }
    }

    std::cout << "Bewegungsmaske (Schwelle " << SCHWELLE << "):\n";
    zeichne(bewegung);

    std::cout << "\nGeaenderte Pixel: " << bewegtePixel << " von " << H * B << "\n";
    if (bewegtePixel > 0)
        std::cout << "-> Bewegung erkannt: Aufzeichnung starten / Event ausloesen.\n";
    else
        std::cout << "-> keine Bewegung: nichts tun (Speicher/Bandbreite sparen).\n";

    std::cout << "\nMerksatz: Frame-Differenz ist simpel, aber rausch- und\n"
                 "lichtempfindlich. Robuster: Background-Subtraction-Modell (MOG2).\n";
    return 0;
}
