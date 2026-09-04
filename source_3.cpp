#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>
#include <windows.h> 

class Data {
private:
    int value;                 
    mutable std::mutex mtx;    

public:
    explicit Data(int val = 0) : value(val) {}

    int get() const {
        std::lock_guard<std::mutex> lock(mtx);
        return value;
    }

    void set(int val) {
        std::lock_guard<std::mutex> lock(mtx);
        value = val;
    }

    friend void swap_lock(Data& a, Data& b);
    friend void swap_scoped_lock(Data& a, Data& b);
    friend void swap_unique_lock(Data& a, Data& b);

    friend std::ostream& operator<<(std::ostream& os, const Data& d) {
        os << d.get();
        return os;
    }
};

void swap_lock(Data& a, Data& b) {
    if (&a == &b) return;

    std::lock(a.mtx, b.mtx);
    std::lock_guard<std::mutex> lock_a(a.mtx, std::adopt_lock);
    std::lock_guard<std::mutex> lock_b(b.mtx, std::adopt_lock);

    std::swap(a.value, b.value);
}

void swap_scoped_lock(Data& a, Data& b) {
    if (&a == &b) return;
    std::scoped_lock lock(a.mtx, b.mtx);
    std::swap(a.value, b.value);
}

void swap_unique_lock(Data& a, Data& b) {
    if (&a == &b) return;
    std::unique_lock<std::mutex> lock_a(a.mtx, std::defer_lock);
    std::unique_lock<std::mutex> lock_b(b.mtx, std::defer_lock);
    std::lock(lock_a, lock_b);
    std::swap(a.value, b.value);
}

int main() {
    SetConsoleOutputCP(1251);
    SetConsoleCP(1251);
    Data d1(42), d2(100);

    std::cout << "До обмена: d1 = " << d1 << ", d2 = " << d2 << std::endl;

    swap_lock(d1, d2);
    std::cout << "После swap_lock:   d1 = " << d1 << ", d2 = " << d2 << std::endl;

    swap_scoped_lock(d1, d2);
    std::cout << "После swap_scoped_lock: d1 = " << d1 << ", d2 = " << d2 << std::endl;

    swap_unique_lock(d1, d2);
    std::cout << "После swap_unique_lock: d1 = " << d1 << ", d2 = " << d2 << std::endl;

    std::thread t1([&]() {
        for (int i = 0; i < 1000; ++i) {
            swap_lock(d1, d2);
        }
        });
    std::thread t2([&]() {
        for (int i = 0; i < 1000; ++i) {
            swap_unique_lock(d1, d2);
        }
        });
    t1.join();
    t2.join();
    std::cout << "После многопоточных обменов: d1 = " << d1 << ", d2 = " << d2 << std::endl;

    return 0;
}
