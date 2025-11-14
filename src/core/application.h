#pragma once

#include <memory>
#include <string>

namespace shotcut {

class Application {
public:
    Application();
    ~Application();
    
    // Disable copy constructor and assignment
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    
    // Initialization and main loop
    bool initialize();
    int run();
    void shutdown();
    
private:
    class Impl;
    std::unique_ptr<Impl> pimpl_;
};

} // namespace shotcut
