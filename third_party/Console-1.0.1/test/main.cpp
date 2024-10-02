#include <iostream>

#include <console/console.hpp>

using namespace org::ttldtor::console;

int main() {
    auto consoleSize = Console::getSize();

    std::cout << "Console size: " << consoleSize.first << " x " << consoleSize.second << std::endl;

    return 0;
}