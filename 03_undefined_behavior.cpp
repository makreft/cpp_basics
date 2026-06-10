// =============================================================================
//  Thema: C/C++  ->  Undefined Behavior (UB)
// =============================================================================
//
//  UB = Verhalten, das der C++-Standard NICHT definiert. Der Compiler darf
//  dann alles tun. Klassiker: "laeuft im Test, crasht im Feld."
//  In sicherheitskritischer Software ist das der Endgegner und wird durch
//  Coding-Standards, statische Analyse und Sanitizer bekaempft.
//
//  Dieses Programm fuehrt KEIN UB aus - die gefaehrlichen Zeilen sind
//  auskommentiert. Es zeigt jeweils Problem + sichere Loesung.
//
//  Tipp zum Selbst-Erleben: eine der //-BUG-Zeilen aktivieren und neu bauen mit
//      g++ -std=c++17 -g -fsanitize=address,undefined 03_undefined_behavior.cpp -o 03
//  Der Sanitizer zeigt dir dann genau, wo es knallt.
// =============================================================================

#include <iostream>
#include <vector>
#include <limits>
#include <memory>

int main() {
    std::cout << "=== Undefined Behavior (und wie man es vermeidet) ===\n\n";

    // 1) UNINITIALISIERTE VARIABLE -------------------------------------------
    // int x;                  // BUG: Wert ist unbestimmt
    // std::cout << x;         //      Lesen davon ist UB
    int x = 0;                 // SICHER: immer initialisieren
    std::cout << "1) initialisiert: x = " << x << "\n";

    // 2) ZUGRIFF AUSSERHALB DES GUELTIGEN BEREICHS ---------------------------
    std::vector<int> v{10, 20, 30};
    // std::cout << v[5];      // BUG: out-of-bounds -> UB (kein Check bei [])
    try {
        std::cout << "2) v.at(5) wirft eine Exception statt UB: ";
        std::cout << v.at(5) << "\n";        // SICHER: at() prueft die Grenze
    } catch (const std::out_of_range& e) {
        std::cout << e.what() << "\n";
    }

    // 3) DANGLING POINTER (use-after-free) -----------------------------------
    // int* p = new int(42);
    // delete p;
    // std::cout << *p;        // BUG: Zugriff nach Freigabe -> UB
    auto sp = std::make_unique<int>(42);     // SICHER: Lebensdauer ist klar
    std::cout << "3) unique_ptr-Wert = " << *sp << "\n";

    // 4) SIGNED INTEGER OVERFLOW ---------------------------------------------
    int max = std::numeric_limits<int>::max();
    // int boom = max + 1;     // BUG: Ueberlauf von signed int ist UB
    long long sicher = static_cast<long long>(max) + 1;  // SICHER: groesserer Typ
    std::cout << "4) max(int)=" << max << ", +1 in long long = " << sicher << "\n";

    // 5) NULLPOINTER-DEREFERENZ ----------------------------------------------
    int* maybe = nullptr;
    // std::cout << *maybe;    // BUG: Dereferenzieren von nullptr -> UB
    if (maybe != nullptr)      // SICHER: vorher pruefen
        std::cout << *maybe;
    else
        std::cout << "5) Zeiger ist null -> sauber geprueft, kein Zugriff\n";

    std::cout << "\nMerksatz: UB nicht 'wegtesten', sondern\n"
                 "durch Tools (ASan/UBSan, valgrind, statische Analyse) und\n"
                 "defensives Programmieren systematisch verhindern.\n";
    return 0;
}
