#include "../include/Application.h"
#include "../include/Filesystem.h"

#include <iostream>

int main(int argc, char* argv[]) {
    try {
        const stockflow::fs::path base = argc > 1
            ? stockflow::fs::path(argv[1])
            : stockflow::fs::current_path();
        stockflow::Application application(base);
        return application.run(std::cin, std::cout);
    } catch (const std::exception& exception) {
        std::cerr << "Fatal error: " << exception.what() << '\n';
        return 1;
    }
}
