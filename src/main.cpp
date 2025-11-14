#include <iostream>
#include <memory>
#include "core/application.h"

int main(int argc, char* argv[]) {
    try {
        auto app = std::make_unique<shotcut::Application>();
        
        if (!app->initialize()) {
            std::cerr << "Failed to initialize application" << std::endl;
            return -1;
        }
        
        return app->run();
    } catch (const std::exception& e) {
        std::cerr << "Application error: " << e.what() << std::endl;
        return -1;
    } catch (...) {
        std::cerr << "Unknown application error" << std::endl;
        return -1;
    }
}
