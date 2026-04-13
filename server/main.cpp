#include <iostream>

#include "listener/Listener.hpp"
#include "baseStation/BaseStation.hpp"

class MME {

};

int main() {
    auto mme = std::make_shared<MME>();

    auto bs1 = std::make_shared<BaseStation>(1, 0,   1, 50.0f, 10, 1024, mme);
    auto bs2 = std::make_shared<BaseStation>(2, 100, 1, 50.0f, 10, 1024, mme);

    bs1->start();
    bs2->start();

    const std::vector stations = {bs1, bs2};

    Listener listener;
    listener.setStationsOnline(stations);

    if (listener.createServerSocket(8080, 10).isGood()) {
        listener.runServer();
    } else {
        return 1;
    }

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}