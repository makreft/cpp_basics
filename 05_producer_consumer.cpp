// =============================================================================
//  Thema: Producer-Consumer mit Condition Variable
//  (genau das Muster fuer "Kamera erzeugt Frames, Encoder verarbeitet sie")
// =============================================================================
//
//  Eine beschraenkte Warteschlange (bounded queue) entkoppelt zwei Threads:
//    - Producer  = Kamera-Capture: legt Frames rein
//    - Consumer  = Encoder/Writer: holt Frames raus
//  Eine condition_variable laesst Threads SCHLAFEN, statt aktiv zu warten
//  (kein "busy waiting", spart CPU).
//
//  Bauen:  g++ -std=c++17 -g -O0 -Wall -Wextra -pthread 05_producer_consumer.cpp -o 05
// =============================================================================

#include <iostream>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

struct Frame {
    int nummer;
};

class FrameQueue {
public:
    explicit FrameQueue(std::size_t maxGroesse) : maxGroesse_(maxGroesse) {}

    // Producer ruft das auf. Blockiert, wenn die Queue voll ist.
    void push(Frame f) {
        std::unique_lock<std::mutex> lock(m_);
        nichtVoll_.wait(lock, [&] { return q_.size() < maxGroesse_; });
        q_.push(f);
        nichtLeer_.notify_one();   // einen wartenden Consumer wecken
    }

    // Consumer ruft das auf. Blockiert, wenn die Queue leer ist.
    // Gibt false zurueck, wenn "fertig" und nichts mehr kommt.
    bool pop(Frame& out) {
        std::unique_lock<std::mutex> lock(m_);
        nichtLeer_.wait(lock, [&] { return !q_.empty() || fertig_; });
        if (q_.empty()) return false;   // fertig + leer
        out = q_.front();
        q_.pop();
        nichtVoll_.notify_one();   // einen wartenden Producer wecken
        return true;
    }

    void schliessen() {
        std::lock_guard<std::mutex> lock(m_);
        fertig_ = true;
        nichtLeer_.notify_all();   // alle Consumer aufwecken, damit sie enden
    }

private:
    std::queue<Frame>       q_;
    std::size_t             maxGroesse_;
    bool                    fertig_ = false;
    std::mutex              m_;
    std::condition_variable nichtLeer_;
    std::condition_variable nichtVoll_;
};

int main() {
    std::cout << "=== Producer-Consumer (Kamera -> Encoder) ===\n\n";

    FrameQueue queue(4);              // max. 4 Frames "in flight"
    constexpr int ANZAHL = 10;

    // Producer = Kamera
    std::thread kamera([&] {
        for (int i = 1; i <= ANZAHL; ++i) {
            queue.push(Frame{i});
            std::cout << "[Kamera ] Frame " << i << " aufgenommen\n";
        }
        queue.schliessen();          // signalisiert: keine Frames mehr
    });

    // Consumer = Encoder
    std::thread encoder([&] {
        Frame f;
        while (queue.pop(f)) {
            std::cout << "          [Encoder] Frame " << f.nummer << " kodiert\n";
        }
        std::cout << "          [Encoder] fertig, Queue leer\n";
    });

    kamera.join();
    encoder.join();
    std::cout << "\nAlle Frames verarbeitet.\n";
    return 0;
}
