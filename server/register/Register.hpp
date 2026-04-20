//
// Created by fisp on 13.04.2026.
//

#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <sqlite3.h>

class Registration {
  sqlite3* db{};
  const std::string storagePath;

  std::mutex mutex;
  std::atomic<bool> running{false};

  bool openDb();
  void closeDb();
  [[nodiscard]] bool createTable() const;
  bool execWithoutArgs(const char* query) const;

  inline sqlite3_stmt* createStatement(const char* query) const;
  static inline void deleteStatement(sqlite3_stmt* statement);

  template <typename func>
  bool operationTemplate(const char* query, func function);

public:
  explicit Registration(std::string storagePath);
  ~Registration();

  bool start();
  void stop();

  bool addUsers(const std::string& imsi, const std::string& imei,
                const std::string& msisdn);

  bool requestAuthInfo(const std::string& imei, const std::string& imsi);

  bool changePathToUe(uint64_t tmsi, int32_t bsId);

  bool updateLocation(uint64_t tmsi, int32_t vlrId, const std::string& imsi);

  bool getMsisdnByTmsi(uint64_t tmsi, std::string& msisdn);

  bool resolveDestination(const std::string& msisdnDst, uint64_t& tmsiDst,
                          int32_t& bsId);
};

template <typename func>
bool Registration::operationTemplate(const char* query, func function) {
    std::lock_guard lock(mutex);
    if (db == nullptr) {
        return false;
    }

    sqlite3_stmt* stmt = createStatement(query);
    if (stmt == nullptr) {
        return false;
    }

    const bool result = function(stmt);
    deleteStatement(stmt);
    return result;
}