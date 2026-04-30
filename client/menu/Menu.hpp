//
// Created by fisp on 07.04.2026.
//

#pragma once

#include "../context/DataStorage.hpp"

#include <memory>
#include <string>
#include <unordered_map>

class MenuItem;

class Menu {
  DataStorage addressBook;
  std::unordered_map<uint64_t, std::unique_ptr<MenuItem>> items;

 public:
  explicit Menu(const std::string& pathAddressBook): addressBook(DataStorage(pathAddressBook)) {}

  void addItem(const std::string& name, std::unique_ptr<MenuItem> item);
  void findItem(const uint64_t& hashCommand);

  bool loadAddressBook();
  void printAddressBook() const;

  static void printMenu();
};
