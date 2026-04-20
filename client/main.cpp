#include "../commonFiles/inputArgsFunc/InputArgsFunc.hpp"
#include "../commonFiles/stringFunc/StringFunc.hpp"
#include "../commonFiles/validateFunc/ValidateFunc.hpp"
#include "./context/Context.hpp"
#include "./exchange/Exchange.hpp"
#include "./menu/Menu.hpp"
#include "menu/MenuItem.h"

#include <cstring>
#include <iostream>
#include <string>

void helpPrint() {
  std::cout
      << "command to run: ./app -ip <ip> -port <port> "
         "-msisdn <msisdn> -imsi <imsi> - imei <imei> -position <position>"
      << std::endl;
}

int main(const int argc, const char** argv) {
  std::vector<std::string> needFlags{"-ip",   "-port", "-msisdn",
                                     "-imsi", "-imei", "-position"};

  std::unordered_map<std::string, std::string> inputFlags;
  if (!loopForInputArgs(argc, argv, helpPrint, inputFlags)) {
    return 1;
  };

  for (const auto& flag : needFlags) {
    if (!inputFlags.contains(flag)) {
      std::cout << "Missing flag: " << flag << std::endl;
      return 1;
    }
  }

  const std::string ipAddress = inputFlags["-ip"];
  uint16_t port{0};
  if (!isPort(inputFlags["-port"], port)) {
    std::cout << "Invalid port: " << port << std::endl;
    return 1;
  }

  const std::string msisdn = inputFlags["-msisdn"];
  const std::string imsi = inputFlags["-imsi"];
  const std::string imei = inputFlags["-imei"];

  if (!isNumber(inputFlags["-position"])) {
    std::cout << "Invalid position: " << inputFlags["-position"] << std::endl;
    return 1;
  }
  const int32_t position = std::stoi(inputFlags["-position"]);

  Context context(msisdn, imsi, imei, position);
  Exchange exchange(context, ipAddress, port);

  Menu menu("../data/address_book.json");
  menu.loadAddressBook();

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
