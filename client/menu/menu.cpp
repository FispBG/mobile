//
// Created by fisp on 07.04.2026.
//

#include "menu.hpp"

#include <cctype>
#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>
#include "../../commonFiles/stringFunc/StringFunc.hpp"

void CommandActive::action() {
    std::string input;
    std::cout << "Input on/off: ";
    std::getline(std::cin, input);
    input = stringStrip(input);

    input = fixInputString(input);

    if (input == "on") {
        exchange.activate();
        return;
    }

    if (input == "off") {
        exchange.deactivate();
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

    exchange.move(std::stoi(input));
}

void CommandSearch::action() {
    exchange.printStations();
}

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

    exchange.sendSms(msisdn, text);
}

void CommandStatus::action() {
    exchange.printStatus();
}

void CommandInbox::action() {
    exchange.printInbox();
}

void CommandOutbox::action() {
    exchange.printOutbox();
}

void CommandExit::action() {
    exchange.deactivate();
    std::exit(0);
}

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

bool Menu::loadAddressBook(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    nlohmann::json jsonData;
    file >> jsonData;

    addressBook.clear();

    for (const auto& item : jsonData) {
        if (addressBook.size() >= 5) {
            break;
        }

        AddressBookItem record;
        record.name = item.value("name", "");
        record.MSISDN = item.value("MSISDN", "");

        if (!record.MSISDN.empty()) {
            addressBook.push_back(std::move(record));
        }
    }

    return true;
}

void Menu::printAddressBook() const {
    std::cout << "Address book:" << std::endl;

    for (const auto& item : addressBook) {
        std::cout << item.name << " : " << item.MSISDN << std::endl;
    }

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