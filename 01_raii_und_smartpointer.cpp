// =============================================================================
//  Thema: C/C++  ->  RAII & Smart Pointer
//  (mit EIGENER Implementierung von unique_ptr / shared_ptr / weak_ptr)
// =============================================================================
//
//  RAII = "Resource Acquisition Is Initialization":
//  Eine Ressource (Speicher, Datei, Mutex, Kamera-Handle) wird im KONSTRUKTOR
//  geholt und im DESTRUKTOR automatisch freigegeben. Smart Pointer wenden RAII
//  auf Heap-Speicher an. Hier bauen wir sie selbst nach, um zu verstehen, wie
//  Besitz und Referenzzaehlung intern funktionieren. (In echtem Code nimmt man
//  natuerlich std::unique_ptr / std::shared_ptr.)
//
//  Bauen:   g++ -std=c++17 -g -O0 -Wall -Wextra 01_raii_und_smartpointer.cpp -o 01
//  oder in VSCodium: Datei offen lassen -> Strg+Shift+B, debuggen mit F5.
// =============================================================================

#include <iostream>
#include <string>
#include <utility>

// -----------------------------------------------------------------------------
// 0) RAII von Hand: eine Klasse, die eine "Ressource" kapselt.
//    Stell dir statt des Namens ein Kamera-Handle oder File Descriptor vor.
// -----------------------------------------------------------------------------
class KameraVerbindung {
public:
    explicit KameraVerbindung(std::string name) : name_(std::move(name)) {
        std::cout << "  [+] Verbindung zu '" << name_ << "' geoeffnet\n";
    }
    ~KameraVerbindung() {
        std::cout << "  [-] Verbindung zu '" << name_ << "' geschlossen\n";
    }
private:
    std::string name_;
};

// =============================================================================
//  EIGENE unique_ptr-Implementierung
//  Idee: alleiniger Besitz. Nicht kopierbar (sonst wuerden zwei Objekte
//  denselben Zeiger loeschen -> double free). Nur verschiebbar: der Besitz
//  WANDERT. Groesse = genau ein Zeiger -> KEIN Overhead.
// =============================================================================
template <typename T>
class MyUniquePtr {
public:
    explicit MyUniquePtr(T* p = nullptr) noexcept : ptr_(p) {}
    ~MyUniquePtr() { delete ptr_; }   // RAII: Freigabe im Destruktor

    // nicht kopierbar (bewusst geloescht)
    MyUniquePtr(const MyUniquePtr&)            = delete;
    MyUniquePtr& operator=(const MyUniquePtr&) = delete;

    // verschiebbar: Besitz wandert, Quelle wird leer
    MyUniquePtr(MyUniquePtr&& other) noexcept : ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }
    MyUniquePtr& operator=(MyUniquePtr&& other) noexcept {
        if (this != &other) {
            delete ptr_;            // alte Ressource freigeben
            ptr_       = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    // Zugriff wie ein roher Zeiger
    T*   get()        const noexcept { return ptr_; }
    T&   operator*()  const          { return *ptr_; }
    T*   operator->() const noexcept { return ptr_; }
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

    T*   release() noexcept { T* tmp = ptr_; ptr_ = nullptr; return tmp; }
    void reset(T* p = nullptr) noexcept { delete ptr_; ptr_ = p; }

private:
    T* ptr_;
};

// =============================================================================
//  EIGENE shared_ptr / weak_ptr-Implementierung
//
//  Mehrere shared_ptr teilen sich EIN Objekt. Ein gemeinsamer "Control Block"
//  zaehlt:
//    strong = wie viele shared_ptr zeigen drauf  -> bei 0: Objekt loeschen
//    weak   = wie viele weak_ptr zeigen drauf     -> Block erst loeschen,
//                                                    wenn strong UND weak 0 sind
// =============================================================================
struct ControlBlock {
    long strong;   // Anzahl shared_ptr -> bei 0 wird das OBJEKT geloescht
    long weak;     // Anzahl weak_ptr + 1, solange ueberhaupt shared_ptr leben.
                   // Dieser "+1"-Trick ist entscheidend: die shared_ptr-Gruppe
                   // haelt GEMEINSAM einen weak-Zaehler. Dadurch lebt der Control
                   // Block garantiert so lange, wie ein shared_ptr ihn anfasst,
                   // und wird erst geloescht, wenn strong UND weak bei 0 sind.
                   // (Ohne diesen Trick gibt es ein use-after-free bei Zyklen
                   //  mit weak_ptr - genau das hatte der AddressSanitizer hier
                   //  in einer ersten Version aufgedeckt.)
    // WICHTIG: das echte std::shared_ptr nutzt hier ATOMARE Zaehler
    // (std::atomic<long>), damit Zaehlen ueber Threads hinweg sicher ist.
    // Genau diese Atomar-Operationen sind der Overhead gegenueber unique_ptr.
};

template <typename T> class MyWeakPtr;   // Vorwaerts-Deklaration

template <typename T>
class MySharedPtr {
public:
    MySharedPtr() noexcept : ptr_(nullptr), cb_(nullptr) {}

    // strong=1 (dieser shared_ptr), weak=1 (die shared-Gruppe haelt einen
    // weak-Zaehler auf sich selbst -> haelt den Control Block am Leben)
    explicit MySharedPtr(T* p) : ptr_(p), cb_(new ControlBlock{1, 1}) {}

    // Kopieren: zeigt aufs gleiche Objekt, strong++
    MySharedPtr(const MySharedPtr& other) noexcept
        : ptr_(other.ptr_), cb_(other.cb_) {
        if (cb_) ++cb_->strong;
    }
    MySharedPtr& operator=(const MySharedPtr& other) noexcept {
        if (this != &other) {
            release();
            ptr_ = other.ptr_;
            cb_  = other.cb_;
            if (cb_) ++cb_->strong;
        }
        return *this;
    }

    // Verschieben: KEINE Zaehleraenderung, Quelle wird nur geleert
    MySharedPtr(MySharedPtr&& other) noexcept
        : ptr_(other.ptr_), cb_(other.cb_) {
        other.ptr_ = nullptr;
        other.cb_  = nullptr;
    }
    MySharedPtr& operator=(MySharedPtr&& other) noexcept {
        if (this != &other) {
            release();
            ptr_       = other.ptr_;
            cb_        = other.cb_;
            other.ptr_ = nullptr;
            other.cb_  = nullptr;
        }
        return *this;
    }

    ~MySharedPtr() { release(); }

    long use_count()  const noexcept { return cb_ ? cb_->strong : 0; }
    T*   get()        const noexcept { return ptr_; }
    T&   operator*()  const          { return *ptr_; }
    T*   operator->() const noexcept { return ptr_; }
    explicit operator bool() const noexcept { return ptr_ != nullptr; }

private:
    void release() noexcept {
        if (!cb_) return;
        if (--cb_->strong == 0) {        // letzter shared_ptr ist weg
            delete ptr_;                 // -> Objekt SOFORT freigeben
            // Jetzt den weak-Zaehler zurueckgeben, den die shared-Gruppe hielt.
            // Erst wenn auch kein weak_ptr mehr zeigt (weak==0), faellt der Block.
            // Wichtig: wir LESEN cb_ nicht mehr nach delete ptr_, sondern
            // dekrementieren nur - das ist auch bei Zyklen sicher.
            if (--cb_->weak == 0)
                delete cb_;
        }
        ptr_ = nullptr;
        cb_  = nullptr;
    }

    T*            ptr_;
    ControlBlock* cb_;

    friend class MyWeakPtr<T>;   // weak_ptr darf in den Control Block schauen
};

template <typename T>
class MyWeakPtr {
public:
    MyWeakPtr() noexcept : ptr_(nullptr), cb_(nullptr) {}

    // aus einem shared_ptr erzeugen: weak++ (KEIN strong!)
    MyWeakPtr(const MySharedPtr<T>& sp) noexcept
        : ptr_(sp.ptr_), cb_(sp.cb_) {
        if (cb_) ++cb_->weak;
    }
    MyWeakPtr(const MyWeakPtr& other) noexcept
        : ptr_(other.ptr_), cb_(other.cb_) {
        if (cb_) ++cb_->weak;
    }
    MyWeakPtr& operator=(const MySharedPtr<T>& sp) noexcept {
        release();
        ptr_ = sp.ptr_;
        cb_  = sp.cb_;
        if (cb_) ++cb_->weak;
        return *this;
    }
    MyWeakPtr& operator=(const MyWeakPtr& other) noexcept {
        if (this != &other) {
            release();
            ptr_ = other.ptr_;
            cb_  = other.cb_;
            if (cb_) ++cb_->weak;
        }
        return *this;
    }

    ~MyWeakPtr() { release(); }

    // lock(): nur wenn das Objekt noch LEBT (strong > 0), bekommt man einen
    // gueltigen shared_ptr zurueck - sonst einen leeren.
    MySharedPtr<T> lock() const noexcept {
        MySharedPtr<T> sp;
        if (cb_ && cb_->strong > 0) {
            sp.ptr_ = ptr_;
            sp.cb_  = cb_;
            ++cb_->strong;
        }
        return sp;
    }

private:
    void release() noexcept {
        if (!cb_) return;
        // weak==0 kann erst eintreten, wenn auch strong schon 0 ist: solange ein
        // shared_ptr lebt, haelt die Gruppe selbst einen weak-Zaehler (>=1).
        if (--cb_->weak == 0)
            delete cb_;          // Objekt war schon weg, jetzt auch der Block
        ptr_ = nullptr;
        cb_  = nullptr;
    }

    T*            ptr_;
    ControlBlock* cb_;

    friend class MySharedPtr<T>;
};

// -----------------------------------------------------------------------------
//  Demos
// -----------------------------------------------------------------------------
void raii_demo() {
    std::cout << "raii_demo(): betrete Block\n";
    {
        KameraVerbindung cam("Tuerkamera-1");
        // Egal wie wir den Block verlassen (return, Exception): der Destruktor
        // raeumt automatisch auf, sobald 'cam' aus dem Scope faellt.
    } // <-- hier wird ~KameraVerbindung() automatisch aufgerufen
    std::cout << "raii_demo(): Block verlassen\n\n";
}

void unique_ptr_demo() {
    std::cout << "unique_ptr_demo() (MyUniquePtr):\n";
    MyUniquePtr<KameraVerbindung> cam(new KameraVerbindung("Aussenkamera"));

    // auto cam2 = cam;             // FEHLER: nicht kopierbar (gewollt!)
    MyUniquePtr<KameraVerbindung> cam2 = std::move(cam);  // Besitz wandert
    std::cout << "  cam ist jetzt " << (cam ? "gueltig" : "leer (verschoben)")
              << ", cam2 haelt die Verbindung\n";
    std::cout << "  sizeof(MyUniquePtr) = " << sizeof(cam)
              << " bytes  (= ein Zeiger, kein Overhead)\n\n";
    // ~MyUniquePtr von cam2 schliesst die Verbindung automatisch.
}

struct Knoten {
    std::string         name;
    MySharedPtr<Knoten> naechster;   // STARKER Verweis nach vorne
    MyWeakPtr<Knoten>   vorheriger;  // SCHWACHER Verweis zurueck -> kein Zyklus!
    explicit Knoten(std::string n) : name(std::move(n)) {
        std::cout << "  [+] Knoten '" << name << "'\n";
    }
    ~Knoten() { std::cout << "  [-] Knoten '" << name << "'\n"; }
};

void shared_ptr_demo() {
    std::cout << "shared_ptr_demo() (MySharedPtr / MyWeakPtr):\n";
    MySharedPtr<Knoten> a(new Knoten("A"));
    MySharedPtr<Knoten> b(new Knoten("B"));

    a->naechster = b;          // A haelt B STARK  -> strong von B steigt
    b->vorheriger = a;         // B haelt A nur SCHWACH (weak_ptr)

    std::cout << "  use_count von B = " << b.use_count()
              << "  (b selbst + a->naechster)\n";

    // weak_ptr muss man "locken", um ans Objekt zu kommen:
    if (auto p = b->vorheriger.lock())
        std::cout << "  B kennt seinen Vorgaenger: " << p->name << "\n";

    // Waere 'vorheriger' ein MySharedPtr, haetten A und B sich gegenseitig STARK
    // gehalten -> strong nie 0 -> SPEICHERLECK. Der weak_ptr bricht den Zyklus.
    std::cout << "  (beide werden beim Verlassen korrekt freigegeben:)\n";
    std::cout << "  sizeof(MySharedPtr) = " << sizeof(a)
              << " bytes  (= zwei Zeiger: Objekt + Control Block)\n";
}

int main() {
    std::cout << "=== RAII & Smart Pointer (eigene Implementierung) ===\n\n";
    raii_demo();
    unique_ptr_demo();
    shared_ptr_demo();
    std::cout << "\nFertig.\n";
    return 0;
}
