// =============================================================================
//  Thema: Threads, Mutex, Race Condition, Deadlock
// =============================================================================
//
//  Mehrere Kamerastreams -> mehrere Threads -> gemeinsame Daten -> Gefahr von
//  Race Conditions (gleichzeitiger Zugriff ohne Synchronisation) und Deadlocks.
//
//  Bauen (mit -pthread!):
//      g++ -std=c++17 -g -O0 -Wall -Wextra -pthread 04_threads_mutex_deadlock.cpp -o 04
// =============================================================================

#include <iostream>
#include <thread>
#include <mutex>
#include <vector>

// -----------------------------------------------------------------------------
// 1) RACE CONDITION: zwei Threads erhoehen denselben Zaehler.
//    Ohne Schutz ist "++" NICHT atomar (lesen-erhoehen-schreiben) -> Datenverlust.
// -----------------------------------------------------------------------------
void race_demo() {
    constexpr int N = 200000;

    long ohne_schutz = 0;
    {
        auto inc = [&] { for (int i = 0; i < N; ++i) ++ohne_schutz; };
        std::thread t1(inc), t2(inc);
        t1.join(); t2.join();
    }

    long mit_schutz = 0;
    std::mutex m;
    {
        auto inc = [&] {
            for (int i = 0; i < N; ++i) {
                std::lock_guard<std::mutex> lock(m);  // RAII: sperrt + gibt frei
                ++mit_schutz;
            }
        };
        std::thread t1(inc), t2(inc);
        t1.join(); t2.join();
    }

    std::cout << "1) Race Condition:\n";
    std::cout << "   erwartet         = " << 2L * N << "\n";
    std::cout << "   OHNE Mutex       = " << ohne_schutz
              << "  <- meist zu klein, weil Updates verloren gehen\n";
    std::cout << "   MIT  Mutex       = " << mit_schutz << "  <- korrekt\n\n";
}

// -----------------------------------------------------------------------------
// 2) DEADLOCK vermeiden: zwei Mutexe immer in DERSELBEN Reihenfolge sperren,
//    oder std::scoped_lock nehmen, das mehrere Mutexe atomar (ohne Deadlock)
//    sperrt.
// -----------------------------------------------------------------------------
std::mutex mA, mB;

void deadlock_demo() {
    std::cout << "2) Zwei Mutexe sicher sperren (kein Deadlock dank scoped_lock):\n";

    auto job1 = [] {
        std::scoped_lock lock(mA, mB);   // sperrt BEIDE atomar
        std::cout << "   Thread 1 hat A und B\n";
    };
    auto job2 = [] {
        std::scoped_lock lock(mA, mB);   // gleiche Mutexe, garantiert kein Deadlock
        std::cout << "   Thread 2 hat A und B\n";
    };

    std::thread t1(job1), t2(job2);
    t1.join(); t2.join();

    // FALSCH waere:
    //   Thread 1: lock(mA); lock(mB);
    //   Thread 2: lock(mB); lock(mA);   <- umgekehrte Reihenfolge -> Deadlock moeglich
    std::cout << "\nMerksatz: feste Lock-Reihenfolge ODER std::scoped_lock/std::lock.\n";
}

int main() {
    std::cout << "=== Threads, Race Condition, Deadlock ===\n\n";
    race_demo();
    deadlock_demo();
    return 0;
}
