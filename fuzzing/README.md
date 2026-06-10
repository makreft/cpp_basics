# Fuzzing-Demo: einen Parser von außen kaputtmachen

Zeigt am Beispiel eines (vereinfachten) RTP-Parsers, wie man **untrusted Input**
mit Fuzzing absichert: eine **verwundbare** Version, die einem Längenfeld blind
vertraut, und die **abgesicherte** Version – sichtbar gemacht mit dem
AddressSanitizer (ASan).

## Der Bug (klassisch und realistisch)

Ein Paket trägt ein Längenfeld „es folgen N Payload-Bytes". Die verwundbare
Version kopiert N Bytes in einen festen 64-Byte-Puffer – **ohne** N gegen die
echte Paketgröße oder die Puffergröße zu prüfen. Ein Angreifer setzt N=200 →
**Stack Buffer Overflow** (Überschreiben benachbarten Speichers → potenziell
Codeausführung). Ein klassischer, real ausnutzbarer Bug durch ungeprüfte Längenfelder.

```c
uint8_t buf[64];
std::memcpy(buf, payload, declared_len);   // declared_len bis 255  -> Overflow
```

Der Fix begrenzt gegen alle drei Grenzen:

```c
size_t n = std::min(declared, std::min(avail, sizeof(buf)));   // sicher
std::memcpy(buf, payload, n);
```

## Dateien

| Datei | Rolle |
|---|---|
| `rtp_parser.hpp` / `.cpp` | der Parser – `parse_rtp_vulnerable` und `parse_rtp_safe` |
| `fuzz_target.cpp` | die Fuzzing-Schnittstelle `LLVMFuzzerTestOneInput` (libFuzzer-Konvention) |
| `standalone_main.cpp` | Treiber, der eine Eingabedatei „abspielt" – für g++/ASan ohne clang |
| `corpus/valid_packet` | ein gültiges Beispielpaket als Fuzzing-Startpunkt (Seed) |

> Hinweis: `rtp_parser.cpp` und `fuzz_target.cpp` haben **kein eigenes `main()`** –
> sie sind kein Ziel für „Strg+Shift+B" einzeln, sondern werden mit den unten
> stehenden Mehr-Datei-Kommandos gebaut.

---

## Weg A: Reproduzieren mit g++ / ASan (funktioniert sofort, ohne clang)

Verwundbare Version bauen und einen bösartigen Input abspielen:

```bash
# bösen Input erzeugen: 9 Byte Header (Byte[8]=200) + 200 Payload-Bytes
{ printf '\x80\x60\x00\x01\x00\x00\x00\x00\xc8'; head -c 200 /dev/zero; } > /tmp/crash_input

# verwundbar bauen (FORTIFY bewusst aus, siehe Hinweis unten) ...
g++ -std=c++17 -g -O1 -fsanitize=address -D_FORTIFY_SOURCE=0 \
    standalone_main.cpp fuzz_target.cpp rtp_parser.cpp -o repro_vuln

# ... und den Input abspielen -> ASan meldet stack-buffer-overflow + bricht ab
./repro_vuln /tmp/crash_input
```

Erwartete Ausgabe (gekürzt) – ASan zeigt direkt die Stelle:

```
==…==ERROR: AddressSanitizer: stack-buffer-overflow …
    #0 … in parse_rtp_vulnerable(...) rtp_parser.cpp:23
    [64, 128) 'buf' (line 17) <== Memory access at offset 128 overflows this variable
SUMMARY: AddressSanitizer: stack-buffer-overflow … in memcpy
```

Zum Vergleich die **abgesicherte** Version mit demselben Input (kein Crash):

```bash
g++ -std=c++17 -g -O1 -fsanitize=address -D_FORTIFY_SOURCE=0 -DSAFE \
    standalone_main.cpp fuzz_target.cpp rtp_parser.cpp -o repro_safe
./repro_safe /tmp/crash_input        # -> "[OK] ... ohne Crash verarbeitet"
```

> **Bonus – Defense in Depth:** Lässt man `-D_FORTIFY_SOURCE=0` weg, fängt auf
> Ubuntu schon glibc's **FORTIFY** (standardmäßig aktiv ab `-O1`) den Overflow ab –
> dann sieht man `*** buffer overflow detected ***` statt der ASan-Meldung. Wir
> schalten FORTIFY hier nur aus, um den detaillierten ASan-Stacktrace mit
> Zeilennummer zu zeigen. In Produktion will man **beide** Netze aktiv haben.

So nutzt man den Standalone-Treiber auch, um einen vom Fuzzer gefundenen Crash
später reproduzierbar nachzustellen.

---

## Weg B: Echtes coverage-guided Fuzzing mit libFuzzer (clang)

Braucht clang (`sudo apt install clang`). libFuzzer liefert dann selbst das
`main()` – `standalone_main.cpp` wird hier **nicht** mitgelinkt.

```bash
# verwundbar: Fuzzer + ASan
clang++ -std=c++17 -g -O1 -fsanitize=fuzzer,address \
    fuzz_target.cpp rtp_parser.cpp -o fuzz_vuln

# laufen lassen (mit Seed-Corpus). Findet den Crash typischerweise in Sekunden
# und legt den Auslöser als Datei 'crash-<hash>' ab:
./fuzz_vuln corpus/

# den gefundenen Crash gezielt nachspielen:
./fuzz_vuln crash-<hash>
```

Abgesicherte Version – läuft ohne Crash, bis man ihn per `-runs`/Zeit stoppt:

```bash
clang++ -std=c++17 -g -O1 -fsanitize=fuzzer,address -DSAFE \
    fuzz_target.cpp rtp_parser.cpp -o fuzz_safe
./fuzz_safe corpus/ -max_total_time=20
```

**Was libFuzzer besser kann als Weg A:** Es misst, welche Code-Pfade eine Eingabe
erreicht, und mutiert gezielt weiter, um *neue* Pfade zu finden. So entdeckt es den
Crash selbstständig, statt dass man den bösen Input von Hand bauen muss.

---

## Was man daraus mitnimmt

- **Input-Validierung an der Vertrauensgrenze** ist nicht optional: Längenfelder aus
  dem Netz immer gegen die echte Paket- *und* Puffergröße prüfen.
- **Fuzzing + ASan** macht genau diese Bugs automatisch und reproduzierbar sichtbar –
  die wirkungsvollste Kombination für Parser von untrusted Daten.
- Der **Standalone-Treiber** zeigt: dieselbe Harness reproduziert einen Crash auch
  ohne die volle Fuzzing-Infrastruktur (und lässt sich z. B. mit AFL++ kombinieren).

Erzählbar als: *„Ich hab einen RTP-Parser mit einem Längenfeld-Bug gebaut, ihn per
libFuzzer/ASan zum Absturz gebracht und mit einer dreifach begrenzten Kopie gefixt –
verifiziert durch erneutes Abspielen desselben Inputs."*
