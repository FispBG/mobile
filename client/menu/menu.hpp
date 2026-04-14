//
// Created by fisp on 07.04.2026.
//

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "../exchange/exchange.hpp"

class MenuItem {
public:
    virtual void action() = 0;
    virtual ~MenuItem() = default;
};

class CommandActive : public MenuItem {
    Exchange& exchange;
public:
    explicit CommandActive(Exchange& exchange) : exchange(exchange) {}
    void action() override;
};

class CommandMove : public MenuItem {
    Exchange& exchange;
public:
    explicit CommandMove(Exchange& exchange) : exchange(exchange) {}
    void action() override;
};

class CommandSearch : public MenuItem {
    Exchange& exchange;
public:
    explicit CommandSearch(Exchange& exchange) : exchange(exchange) {}
    void action() override;
};

class CommandSms : public MenuItem {
    Exchange& exchange;
public:
    explicit CommandSms(Exchange& exchange) : exchange(exchange) {}
    void action() override;
};

class CommandStatus : public MenuItem {
    Exchange& exchange;
public:
    explicit CommandStatus(Exchange& exchange) : exchange(exchange) {}
    void action() override;
};

class CommandInbox : public MenuItem {
    Exchange& exchange;
public:
    explicit CommandInbox(Exchange& exchange) : exchange(exchange) {}
    void action() override;
};

class CommandOutbox : public MenuItem {
    Exchange& exchange;
public:
    explicit CommandOutbox(Exchange& exchange) : exchange(exchange) {}
    void action() override;
};

class CommandExit : public MenuItem {
    Exchange& exchange;
public:
    explicit CommandExit(Exchange& exchange) : exchange(exchange) {}
    void action() override;
};

struct AddressBookItem {
    std::string name;
    std::string MSISDN;
};

class Menu {
    std::unordered_map<uint64_t, std::unique_ptr<MenuItem>> items;
    std::vector<AddressBookItem> addressBook;

public:
    void addItem(const std::string& name, std::unique_ptr<MenuItem> item);
    void findItem(const uint64_t& hashCommand);

    bool loadAddressBook(const std::string& path);
    void printAddressBook() const;

    static void printMenu();
};

