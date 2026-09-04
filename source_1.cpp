#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <string>
#include <cstring>
#include <windows.h>

std::atomic<int> clientCount{ 0 };
std::atomic<bool> generationDone{ false };

std::memory_order order = std::memory_order_seq_cst;

void clientGenerator(int maxClients) {
    for (int i = 0; i < maxClients; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        clientCount.fetch_add(1, order);
        std::cout << "[Генератор] Клиент добавлен. Очередь: "
            << clientCount.load(order) << std::endl;
    }

    std::memory_order storeOrder = (order == std::memory_order_acquire ||
        order == std::memory_order_release ||
        order == std::memory_order_acq_rel)
        ? std::memory_order_release
        : order;
    generationDone.store(true, storeOrder);
    std::cout << "[Генератор] Все клиенты сгенерированы." << std::endl;
}

void operatorWorker() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(2));

        std::memory_order loadOrder = (order == std::memory_order_release ||
            order == std::memory_order_acq_rel ||
            order == std::memory_order_seq_cst)
            ? std::memory_order_acquire
            : order;
        int current = clientCount.load(loadOrder);

        if (current > 0) {
            clientCount.fetch_sub(1, order);
            std::cout << "[Операционист] Клиент обслужен. Очередь: "
                << clientCount.load(order) << std::endl;
        }
        else {
            bool done = generationDone.load(loadOrder);
            if (done) {
                break; 
            }
        }
    }
    std::cout << "[Операционист] Завершил работу." << std::endl;
}

int main(int argc, char* argv[]) {
    SetConsoleOutputCP(1251);
    SetConsoleCP(1251);
    if (argc > 2) {
        std::string mode = argv[1];
        if (mode == "relaxed") {
            order = std::memory_order_relaxed;
        }
        else if (mode == "release" || mode == "acquire" || mode == "acq_rel") {
            order = std::memory_order_acq_rel;

        }
        else if (mode == "seq_cst") {
            order = std::memory_order_seq_cst;
        }
        else {
            std::cerr << "Неизвестный режим. Используйте: relaxed, acq_rel, seq_cst (по умолчанию)" << std::endl;
            return 1;
        }
    }

    int maxClients = 10;
    if (argc > 2) {
        maxClients = std::stoi(argv[2]);
        if (maxClients < 0) maxClients = 0;
    }
    else if (argc == 2) {
        maxClients = std::stoi(argv[1]);
        if (maxClients < 0) maxClients = 0;
    }

    std::cout << "Максимальное количество клиентов: " << maxClients << std::endl;
    std::cout << "Порядок доступа к памяти: ";
    switch (order) {
    case std::memory_order_relaxed: std::cout << "relaxed"; break;
    case std::memory_order_acq_rel: std::cout << "acquire-release"; break;
    case std::memory_order_seq_cst: std::cout << "seq_cst"; break;
    default: std::cout << "unknown"; break;
    }
    std::cout << std::endl << std::endl;

    std::thread generator(clientGenerator, maxClients);
    std::thread operatorThread(operatorWorker);

    generator.join();
    operatorThread.join();

    std::cout << "Программа завершена." << std::endl;
    return 0;
}
