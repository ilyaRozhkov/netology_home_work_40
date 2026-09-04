#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <mutex>
#include <random>
#include <iomanip>
#include <sstream>
#include <atomic>
#include <windows.h> 

const int NUM_THREADS = 5;           
const int TOTAL_ITERATIONS = 50;     
const int BAR_LENGTH = 50;           
const double ERROR_PROBABILITY = 0.1; 
const int SLEEP_MS = 150;            

std::mutex cout_mutex;               

struct ThreadState {
    int index;                        
    std::thread::id id;             
    std::atomic<int> progress{ 0 };    
    bool finished = false;           
    std::chrono::duration<double> elapsed; 
    std::string bar;                 
    std::atomic<bool> error_on_last{ false }; 
};

std::vector<ThreadState> states(NUM_THREADS);

void update_display(int idx) {
    const auto& s = states[idx];
    std::lock_guard<std::mutex> lock(cout_mutex);

    std::cout << "\033[" << (idx + 1) << ";1H";
    std::cout << "\033[K";

    std::ostringstream oss;
    oss << "Поток #" << (idx + 1) << " (id:" << s.id << ") ";

    for (char c : s.bar) {
        if (c == '#') {
            oss << "\033[32m" << c << "\033[0m"; 
        }
        else if (c == 'E') {
            oss << "\033[31m" << c << "\033[0m"; 
        }
        else {
            oss << "\033[90m" << c << "\033[0m"; 
        }
    }

    if (s.finished) {
        oss << "  время: " << std::fixed << std::setprecision(3) << s.elapsed.count() << " с";
    }
    else {
        int percent = (s.progress * 100) / TOTAL_ITERATIONS;
        oss << " " << percent << "%";
    }

    std::cout << oss.str() << std::flush;
}

void worker(int idx) {
    ThreadState& st = states[idx];
    st.index = idx;
    st.id = std::this_thread::get_id();
    st.bar = std::string(BAR_LENGTH, '-');
    st.progress = 0;
    st.finished = false;

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);

    auto start = std::chrono::steady_clock::now();

    for (int i = 0; i < TOTAL_ITERATIONS; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_MS));

        bool error = false;
        try {
            if (dis(gen) < ERROR_PROBABILITY) {
                error = true;
                throw std::runtime_error("Симуляция ошибки");
            }
        }
        catch (const std::exception& e) {
            error = true;
        }

        if (i < BAR_LENGTH) {
            st.bar[i] = error ? 'E' : '#';
        }

        st.progress = i + 1;

        update_display(idx);
    }

    auto end = std::chrono::steady_clock::now();
    st.elapsed = end - start;
    st.finished = true;

    update_display(idx);
}

int main() {
    SetConsoleOutputCP(1251);
    SetConsoleCP(1251);
    if (BAR_LENGTH < TOTAL_ITERATIONS) {
        std::cerr << "Внимание: длина бара меньше числа итераций, прогресс-бар будет неполным." << std::endl;
    }

    std::cout << "\033[2J\033[1;1H\033[?25l";

    std::cout << "Многопоточный расчёт (потоков: " << NUM_THREADS
        << ", итераций: " << TOTAL_ITERATIONS << ")" << std::endl;

    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; ++i) {
        threads.emplace_back(worker, i);
    }

    for (auto& th : threads) {
        th.join();
    }

    std::cout << "\033[" << (NUM_THREADS + 2) << ";1H";
    std::cout << "\033[?25h";
    std::cout << "Все потоки завершены. Нажмите Enter для выхода...";
    std::cin.get();

    return 0;
}
