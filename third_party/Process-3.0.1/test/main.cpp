#include <process/process.hpp>

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

using namespace std::literals;
using namespace org::ttldtor::process;

int main() {
    auto now = [] {
        return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count();
    };
    std::cout << std::string(79, '=') << std::endl;
    std::cout << "Physical memory usage: " << Process::getPhysicalMemorySize() / 1024 << "KB" << std::endl;
    std::cout << "Total CPU time: " << Process::getTotalProcessorTime().count() << "ms" << std::endl;

    auto t1 = now();

    while (now() - t1 <= 5) {
        static std::size_t i = 0;
        (void)(i++);
    }

    std::cout << "Total CPU time (+ ≈5s): " << Process::getTotalProcessorTime().count() << "ms" << std::endl;
    std::cout << "Physical memory usage: " << Process::getPhysicalMemorySize() / 1024 << "KB" << std::endl;

    {
        std::vector<std::uint8_t> vb(10 * 1024 * 1024, 0);

        for (std::size_t i = 0; i < vb.size(); i++) {
            vb[i]++;
        }

        std::cout << "Physical memory usage (+10MB vector<byte>): " << Process::getPhysicalMemorySize() / 1024 << "KB"
                  << std::endl;
    }

    std::cout << "Physical memory usage (-10MB vector<byte>): " << Process::getPhysicalMemorySize() / 1024 << "KB"
              << std::endl;

    for (auto n = 0; n < 5; n++) {
        const std::size_t size = 5 * 1024 * 1024 * (n + 1);
        auto *arr = new std::uint8_t[size]{0};

        std::cout << "Physical memory usage (+" << 5 * (n + 1)
                  << "MB new byte[]): " << Process::getPhysicalMemorySize() / 1024 << "KB" << std::endl;

        for (auto i = 0; i < size; i++) {
            arr[i]++;
        }

        delete[] arr;

        std::this_thread::sleep_for(1s);

        std::cout << "Physical memory usage (-" << 5 * (n + 1)
                  << "MB new byte[]): " << Process::getPhysicalMemorySize() / 1024 << "KB" << std::endl;
    }
}
