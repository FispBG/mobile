//
// Created by fisp on 17.04.2026.
//

#include "DataStorage.hpp"

#include <fstream>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

DataStorage::DataStorage(const std::string &path) : pathToJson(path) {
  loadAddressBook();
}

bool DataStorage::loadAddressBook() {
  if (pathToJson.empty()) {
    return false;
  }

  std::ifstream file(pathToJson);
  if (!file.is_open()) {
    return false;
  }

  nlohmann::json jsonData;
  file >> jsonData;

  addressBook.clear();
  addressBook.reserve(5);

  for (const auto &item : jsonData) {
    if (addressBook.size() >= 5) {
      break;
    }

    std::string name = item.value("name", "");
    std::string msisdn = item.value("MSISDN", "");

    if (!msisdn.empty()) {
      addressBook.emplace_back(std::move(name), std::move(msisdn));
    }
  }

  return true;
}

std::ostream &operator<<(std::ostream &os, const DataStorage &bookItems) {
  for (const auto &item : bookItems.addressBook) {
    os << item.name << " : " << item.msisdn << std::endl;
  }

  return os;
}
