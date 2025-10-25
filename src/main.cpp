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
#include <IConnection.h>
#include "ILogger.h"
#include "connection_pool.h"

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

        ConnectionPool connectionPool(cfg.connectionPool);
        Reactor reactor(std::move(loop), static_cast<ILogger&>(logger), connectionPool);
        reactor.setIdleTimeout(std::chrono::seconds(30));
        Acceptor acceptor(cfg.listen, router, static_cast<ILogger&>(logger), connectionPool,
             [&](std::shared_ptr<IConnection> conn, int clientFd, const BackendConfig& backend) {
                if (!conn->connectToBackend()) {
                    conn->closeAll();
                    return;
                }
                reactor.registerConnection(conn, clientFd, conn->getBackendFd());
            }
        ); 
        acceptor.start();       
        logger.logInfo("Acceptor started; entering Reactor event loop");


        std::thread reactorThread([&](){ reactor.run(); });

        while (!g_Stop.load(std::memory_order_relaxed)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        logger.logInfo("Shutdown signal received");

        acceptor.stop();    
        reactor.stop();    

        if (reactorThread.joinable()) reactorThread.join();

        logger.logInfo("Load balancer stopped gracefully");
        return 0;
    }
    catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << std::endl;
        return 2;
    }
}