// =============================================================================
//  Thema: Prozesse, fork() und IPC (Shared Memory)
// =============================================================================
//
//  Typische Frage: "Frames von einem Capture-Prozess an einen Encoder-Prozess
//  uebergeben - welche IPC-Methode?"  Antwort: Shared Memory, weil Frames gross
//  sind und man sie so OHNE Kopieren teilt.
//
//  Dieses Programm:
//    - legt anonymes Shared Memory an (von Eltern UND Kind sichtbar)
//    - fork()  -> Kind = "Kamera", Eltern = "Encoder"
//    - Kind schreibt einen Frame in den geteilten Speicher (kein Kopieren)
//    - Eltern liest ihn nach einfacher Synchronisation (waitpid)
//
//  Hinweis: fork()/mmap() sind POSIX -> laeuft unter Linux, nicht unter Windows.
//  Bauen:  g++ -std=c++17 -g -O0 -Wall -Wextra 06_prozesse_und_ipc.cpp -o 06
// =============================================================================

#include <iostream>
#include <cstring>
#include <unistd.h>      // fork, getpid
#include <sys/wait.h>    // waitpid
#include <sys/mman.h>    // mmap, munmap

// Struktur, die im geteilten Speicher liegt (stark vereinfachter Frame).
struct SharedFrame {
    bool          bereit;
    int           breite;
    int           hoehe;
    unsigned char daten[16];   // in echt: Millionen Bytes - hier nur ein Mini-Beispiel
};

int main() {
    std::cout << "=== Prozesse & IPC via Shared Memory ===\n\n";

    // Anonymes Shared Memory: MAP_SHARED -> Aenderungen sind in beiden Prozessen
    // sichtbar; MAP_ANONYMOUS -> keine Datei noetig.
    void* region = mmap(nullptr, sizeof(SharedFrame),
                        PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (region == MAP_FAILED) {
        std::perror("mmap");
        return 1;
    }
    auto* frame = static_cast<SharedFrame*>(region);
    frame->bereit = false;

    pid_t pid = fork();   // ab hier laufen ZWEI Prozesse weiter
    if (pid < 0) {
        std::perror("fork");
        return 1;
    }

    if (pid == 0) {
        // ---- KIND-Prozess = "Kamera" ----
        frame->breite = 1920;
        frame->hoehe  = 1080;
        std::memset(frame->daten, 42, sizeof(frame->daten));
        frame->bereit = true;   // erst NACH dem Schreiben "bereit" setzen
        std::cout << "[Kind  " << getpid()
                  << "] Frame in Shared Memory geschrieben (ohne Kopieren)\n";
        _exit(0);               // Kind beendet sich
    } else {
        // ---- ELTERN-Prozess = "Encoder" ----
        waitpid(pid, nullptr, 0);   // einfache Synchronisation: auf Kind warten
        if (frame->bereit) {
            std::cout << "[Eltern " << getpid() << "] Frame gelesen: "
                      << frame->breite << "x" << frame->hoehe
                      << ", erstes Byte = " << static_cast<int>(frame->daten[0])
                      << "\n";
        }
        munmap(region, sizeof(SharedFrame));   // Shared Memory freigeben
    }

    std::cout << "\nMerksatz: Grosse Daten (Frames) -> Shared Memory (kein Kopieren),\n"
                 "kleine Kontrollnachrichten -> Pipe/Socket. Synchronisation nicht\n"
                 "vergessen (hier waitpid; sonst z.B. POSIX-Semaphore).\n";
    return 0;
}
