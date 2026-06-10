# C++ Basics

Kleine, lauffähige und ausführlich kommentierte C++-Programme zu Kernthemen
für systemnahe Linux-Entwicklung (RAII, Smart Pointer, Threads, IPC, Video-
Grundlagen). Jede Datei ist eigenständig (hat ein eigenes `main()`) und lässt
sich einzeln bauen und im Debugger durchsteppen.

## Bauen & Debuggen in VSCodium

1. Datei öffnen (z. B. `01_raii_und_smartpointer.cpp`).
2. **Strg+Shift+B** baut *die gerade offene Datei* (Debug-Symbole, `-pthread`).
3. Breakpoint setzen (links neben die Zeilennummer) und **F5** → Schritt für
   Schritt durchsteppen (F10 = über, F11 = hinein).

Oder im Terminal:

```bash
g++ -std=c++17 -g -O0 -Wall -Wextra -pthread DATEI.cpp -o programm && ./programm
```

## Übersicht

| Datei | Thema |
|---|---|
| `01_raii_und_smartpointer.cpp` | RAII + eigene `unique_ptr`/`shared_ptr`/`weak_ptr`-Implementierung, Zyklen |
| `02_move_semantik.cpp` | Move vs. Copy, Rule of Five (am Frame-Buffer) |
| `03_undefined_behavior.cpp` | UB-Klassiker + sichere Alternativen |
| `04_threads_mutex_deadlock.cpp` | Race Condition, Mutex, Deadlock-Vermeidung |
| `05_producer_consumer.cpp` | Producer-Consumer-Queue mit Condition Variable |
| `06_prozesse_und_ipc.cpp` | `fork()` + Shared Memory (zero-copy Datenübergabe) |
| `07_video_frames_gop.cpp` | I/P/B-Frames, GOP, Seeking, Paketverlust |
| `08_rtp_jitter_buffer.cpp` | RTP-Sequenznummern, Re-Ordering, Verlust |
| `09_motion_detection.cpp` | Frame-Differencing (Bewegungserkennung) |

## Bonus: Fuzzing-Demo (`fuzzing/`)

Ein Mini-RTP-Parser mit einem realistischen „Längenfeld blind vertrauen"-Bug,
einer libFuzzer-Harness und einer abgesicherten Variante – der Buffer Overflow
wird mit dem AddressSanitizer sichtbar gemacht. Funktioniert sofort per g++/ASan
(Standalone-Treiber), echtes coverage-guided Fuzzing per clang/libFuzzer.
Details in [`fuzzing/README.md`](fuzzing/README.md).

## Extra-Tipp: Fehler sichtbar machen

`03_undefined_behavior.cpp` führt absichtlich **kein** UB aus. Wenn du eine der
`// BUG`-Zeilen aktivierst und mit den Sanitizern baust, zeigt dir das Tool genau,
wo es knallt:

```bash
g++ -std=c++17 -g -fsanitize=address,undefined 03_undefined_behavior.cpp -o 03 && ./03
```
