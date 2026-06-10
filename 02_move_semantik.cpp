// =============================================================================
//  Thema: C/C++  ->  Move Semantics & Rule of Five
// =============================================================================
//
//  Ein Videoframe (z.B. 1080p) ist gross. Ihn zu kopieren ist teuer.
//  Move Semantics "stiehlt" stattdessen nur den Zeiger auf den Speicher -> billig.
//
//  Rule of Five: Wer einen von {Destruktor, Copy-Ctor, Copy-Assign,
//  Move-Ctor, Move-Assign} selbst schreibt, muss meist alle bedenken.
//  Rule of Zero: am besten gar keinen schreiben und RAII-Member (z.B.
//  std::vector / unique_ptr) die Arbeit machen lassen.
// =============================================================================

#include <iostream>
#include <cstring>
#include <utility>

// Ein bewusst "von Hand" verwalteter Frame-Puffer, um Kopie vs. Move zu zeigen.
// (In echtem Code wuerde man std::vector<unsigned char> nehmen -> Rule of Zero.)
class FrameBuffer {
public:
    explicit FrameBuffer(std::size_t groesse)
        : groesse_(groesse), daten_(new unsigned char[groesse]) {
        std::memset(daten_, 0, groesse_);
        std::cout << "  ctor      (" << groesse_ << " bytes)\n";
    }

    ~FrameBuffer() {
        std::cout << "  dtor      (" << groesse_ << " bytes)\n";
        delete[] daten_;
    }

    // --- copy constructor: TIEFE Kopie, teuer -------------------------------
    FrameBuffer(const FrameBuffer& other)
        : groesse_(other.groesse_), daten_(new unsigned char[other.groesse_]) {
        std::memcpy(daten_, other.daten_, groesse_);
        std::cout << "  COPY ctor   (" << groesse_ << " bytes dupliziert)\n";
    }
    // --- copy assignment ----------------------------------------------------
    FrameBuffer& operator=(const FrameBuffer& other) {
        if (this != &other) {
            delete[] daten_;
            groesse_ = other.groesse_;
            daten_   = new unsigned char[groesse_];
            std::memcpy(daten_, other.daten_, groesse_);
            std::cout << "  COPY assignment\n";
        }
        return *this;
    }

    // --- move constructor: STIEHLT den Zeiger, billig -----------------------
    FrameBuffer(FrameBuffer&& other) noexcept
        : groesse_(other.groesse_), daten_(other.daten_) {
        other.daten_   = nullptr;   // Quelle "leeren", damit sie nicht freigibt
        other.groesse_ = 0;
        std::cout << "  MOVE ctor   (nur Zeiger uebernommen, keine Kopie)\n";
    }
    // --- move assignment ----------------------------------------------------
    FrameBuffer& operator=(FrameBuffer&& other) noexcept {
        if (this != &other) {
            delete[] daten_;
            daten_         = other.daten_;
            groesse_       = other.groesse_;
            other.daten_   = nullptr;
            other.groesse_ = 0;
            std::cout << "  MOVE assignment\n";
        }
        return *this;
    }

    std::size_t groesse() const { return groesse_; }

private:
    std::size_t    groesse_;
    unsigned char* daten_;
};

// Eine Funktion, die einen Frame "erzeugt" und per Wert zurueckgibt.
FrameBuffer erzeugeFrame() {
    FrameBuffer f(1920 * 1080 * 3);  // ~6 MB
    return f;  // wird ge-move-t (oder vom Compiler weg-optimiert: RVO)
}

int main() {
    std::cout << "=== Move Semantics & Rule of Five ===\n\n";

    std::cout << "1) Copy (teuer):\n";
    FrameBuffer a(1024);
    FrameBuffer b = a;            // ruft copy constructor -> "COPY ctor"

    std::cout << "\n2) Move per std::move (billig):\n";
    FrameBuffer c = std::move(a); // ruft move constructor -> "MOVE ctor", a danach leer

    std::cout << "\n3) Rueckgabe per Wert aus Funktion:\n";
    FrameBuffer d = erzeugeFrame();
    std::cout << "  d hat " << d.groesse() << " bytes\n";

    std::cout << "\n(Destruktoren laufen jetzt automatisch:)\n";
    return 0;
}
