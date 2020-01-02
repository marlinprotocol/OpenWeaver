#include <cstdio>

#include "./core/Simulator/Simulator.h"
#include "./helpers/Logger/easylogging.h"

using namespace std;

INITIALIZE_EASYLOGGINGPP

void configureLogger() {
	// Load configuration from file
    el::Configurations conf1("./config/Logger-conf1.conf");
    el::Configurations conf2("./config/Logger-conf2.conf");
    // Reconfigure default logger
    el::Loggers::reconfigureLogger("default", conf1);
    // Register another logger apart from default
    el::Logger* businessLogger = el::Loggers::getLogger("business");
    // Reconfigure business logger
    el::Loggers::reconfigureLogger("business", conf2);
}

int main() {
	configureLogger();

	LOG(INFO) << "Starting NetworkSimulator " << "v0.1";
	CLOG(INFO, "business") << "NetworkSimulator started " << "v0.1";

	Simulator simulator;
	return 0;
}