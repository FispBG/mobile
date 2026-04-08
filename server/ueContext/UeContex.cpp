//
// Created by fisp on 08.04.2026.
//

#include "UeContex.hpp"

#include <bits/this_thread_sleep.h>

void UeContext::start() {
    running = true;
    // std::thread(&UeContext::readSocket, this).detach();
}

void UeContext::stop() {
    running = false;
}

void UeContext::readSocket() {

}