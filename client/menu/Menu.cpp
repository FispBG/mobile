//
// Created by fisp on 07.04.2026.
//

#include "Menu.hpp"

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

#include "../../commonFiles/stringFunc/StringFunc.hpp"
#include "MenuItem.h"

void Menu::addItem(const std::string& name, std::unique_ptr<MenuItem> item) {
  const uint64_t hashName = hashString(name.c_str());
  items[hashName] = std::move(item);
}

void Menu::findItem(const uint64_t& hashCommand) {
  const auto result = items.find(hashCommand);

  if (result != items.end()) {
    result->second->action();
  } else {
    std::cout << "Command not found." << std::endl;
  }
}

bool Menu::loadAddressBook() {
  if (!addressBook.loadAddressBook()) {
    return false;
  }

  return true;
}

void Menu::printAddressBook() const {
  std::cout << "Address book:" << std::endl;
  std::cout << addressBook << std::endl;
  std::cout << "-------------------" << std::endl;
}

void Menu::printMenu() {
  std::cout << "ACTIVE - off/on client" << std::endl;
  std::cout << "MOVE - change position" << std::endl;
  std::cout << "SEARCH - search stations" << std::endl;
  std::cout << "SMS - send sms to user" << std::endl;
  std::cout << "STATUS - print connection status" << std::endl;
  std::cout << "INBOX - print vector received message" << std::endl;
  std::cout << "OUTBOX - print vector send message" << std::endl;
  std::cout << "EXIT - close program" << std::endl;
  std::cout << "-------------------" << std::endl;
}