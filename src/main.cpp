#include <atomic>
#include <csignal>
#include <memory>
#include <thread>

#include "acceptor.h"
#include "event_loop.h"
#include "logger.h"
#include "reactor.h"
#include "config_manager.h"
#include "router.h"
#include "backend_pool.h"
#include "event_loop_factory.h"
#include <connection.h>
#include "ILogger.h"

static std::atomic<bool> g_Stop{false};
static void handleSignal(int) { g_Stop.store(true, std::memory_order_relaxed); }


int main(int argc, char* argv[]) {
    std::signal(SIGINT,  handleSignal);
    std::signal(SIGTERM, handleSignal);
    try{
        const std::string configPath = (argc > 1) ? argv[1] : "config/config.json";
        auto configManager = ConfigManager(configPath);
        const LoadBalancerConfig& cfg = configManager.getConfig();
        Logger logger(cfg.logging);
        logger.logInfo("Starting load balancer...");

        BackendPool backendPool(cfg.backends);
        Router router(backendPool);
        auto loop = createEventLoop();

        Reactor reactor(std::move(loop), static_cast<ILogger&>(logger));

        Acceptor acceptor(cfg.listen, router, static_cast<ILogger&>(logger),
             [&](int clientFd, const BackendConfig& backend) {
                auto conn = std::make_shared<Connection>(clientFd, backend, static_cast<ILogger&>(logger));
                if (!conn->connectToBackend()) {
                    conn->closeAll();
                    return;
                }
                // Register both client and backend FDs with the Reactor so it can drive I/O
                // EXPECTED: Connection exposes client and backend FDs; if not, add getters.
                reactor.registerConnection(conn, /*clientFd*/ clientFd, /*backendFd*/ conn->backendFd());
            }
        ); 
        acceptor.start();       
        logger.logInfo("Acceptor started; entering Reactor event loop");

        // Run the Reactor loop in the main thread
        // Optional: you could also run Reactor on a thread and block on a condition variable.
        std::thread reactorThread([&](){ reactor.run(); });

        // ---- 8) Wait until a signal arrives, then stop everything cleanly ----
        while (!g_Stop.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        logger.logInfo("Shutdown signal received");

        acceptor.stop();    // stop accepting new connections
        reactor.stop();     // stop event loop (will close the loop and drain)

        if (reactorThread.joinable()) reactorThread.join();

        logger.logInfo("Load balancer stopped gracefully");
        return 0;
    }
    catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << std::endl;
        return 2;
    }
}