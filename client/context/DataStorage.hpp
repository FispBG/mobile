//
// Created by fisp on 17.04.2026.
//

#pragma once

#include <string>
#include <vector>

struct AddressBookItem {
  std::string name;
  std::string msisdn;

  AddressBookItem(std::string n, std::string m)
      : name(std::move(n)), msisdn(std::move(m)) {}
};

class DataStorage final {
  std::vector<AddressBookItem> addressBook;
  std::string pathToJson;

 public:
  explicit DataStorage(const std::string &path);
  bool loadAddressBook();

  friend std::ostream &operator<<(std::ostream &os,
                                  const DataStorage &bookItems);
};
