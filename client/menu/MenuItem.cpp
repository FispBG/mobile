//
// Created by fisp on 17.04.2026.
//

#include "MenuItem.h"

#include <iostream>

#include "../commonFiles/stringFunc/StringFunc.hpp"
#include "../exchange/Exchange.hpp"

void CommandActive::action() {
  std::string input;
  std::cout << "Input on/off: ";
  std::getline(std::cin, input);
  input = stringStrip(input);

  input = fixInputString(input);

  if (input == "on") {
    getExchange().activate();
    return;
  }

  if (input == "off") {
    getExchange().deactivate();
    return;
  }

  std::cout << "Input: ACTIVE ON|OFF" << std::endl;
}

void CommandMove::action() {
  std::string input;
  std::cout << "position: ";
  std::getline(std::cin, input);
  input = stringStrip(input);

  if (!isIntNumber(input)) {
    std::cout << "Write: MOVE <position>" << std::endl;
    return;
  }

  getExchange().move(std::stoi(input));
}

void CommandSearch::action() { getExchange().printStations(); }

void CommandSms::action() {
  std::string input;
  std::cout << "Input MSISDN: ";
  std::getline(std::cin, input);

  const std::vector<std::string> params = split(input, ' ');
  if (params.empty()) {
    std::cout << "Write: SMS <MSISDN> <TEXT>" << std::endl;
    return;
  }

  const std::string msisdn = params[0];
  std::string text;

  if (params.size() > 1) {
    text = input.substr(msisdn.size() + 1);
  } else {
    std::cout << "Text: ";
    std::getline(std::cin, text);
  }

  text = stringStrip(text);

  if (text.empty()) {
    std::cout << "SMS text is empty." << std::endl;
    return;
  }

  getExchange().sendSms(msisdn, text);
}

void CommandStatus::action() { getExchange().printStatus(); }

void CommandInbox::action() { getExchange().printInbox(); }

void CommandOutbox::action() { getExchange().printOutbox(); }

void CommandExit::action() {
  getExchange().deactivate();
  std::exit(0);
}