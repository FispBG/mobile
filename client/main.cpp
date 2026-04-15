#include <cstring>
#include <iostream>
#include <string>

#include "./context/context.hpp"
#include "./exchange/exchange.hpp"
#include "./menu/menu.hpp"
#include "../commonFiles/stringFunc/StringFunc.hpp"

int main(const int argc, char** argv) {
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        std::cout << "command to run: ./app <ip> <port> <msisdn> <imsi> <imei> <position>" << std::endl;
        return 0;
    }

    if (argc != 7) {
        std::cerr << "Write: " << argv[0] << " <ip> <port> <msisdn> <imsi> <imei> <position>" << std::endl;
        return 1;
    }

    const std::string ipAddress = argv[1];
    const uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));
    const std::string msisdn = argv[3];
    const std::string imsi = argv[4];
    const std::string imei = argv[5];
    const int32_t coordinate = std::stoi(argv[6]);

    Context context(msisdn, imsi, imei, coordinate);
    Exchange exchange(context, ipAddress, port);

    Menu menu;
    menu.loadAddressBook("../data/address_book.json");

    menu.addItem("active", std::make_unique<CommandActive>(exchange));
    menu.addItem("move", std::make_unique<CommandMove>(exchange));
    menu.addItem("search", std::make_unique<CommandSearch>(exchange));
    menu.addItem("sms", std::make_unique<CommandSms>(exchange));
    menu.addItem("status", std::make_unique<CommandStatus>(exchange));
    menu.addItem("inbox", std::make_unique<CommandInbox>(exchange));
    menu.addItem("outbox", std::make_unique<CommandOutbox>(exchange));
    menu.addItem("exit", std::make_unique<CommandExit>(exchange));
    menu.addItem("quit", std::make_unique<CommandExit>(exchange));

    std::string command;
    while (true) {
        system("clear");
        menu.printAddressBook();
        menu.printMenu();
        std::cout << "Input: ";
        std::getline(std::cin, command);

        command = fixInputString(command);
        if (command.empty()) {
            continue;
        }

        menu.findItem(hashString(command.c_str()));
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        std::cout << "Press any button: ";
        getchar();
    }
}