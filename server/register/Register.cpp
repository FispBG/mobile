//
// Created by Fisp on 13.04.2026.
//

#include "Register.hpp"
#include "../commonFiles/resultFunc/ResultFunction.hpp"

#include <fstream>
#include <iostream>
#include <utility>

Registration::Registration(std::string storagePath)
    : storagePath(std::move(storagePath)){};

Registration::~Registration() { stop(); }

bool Registration::openDb() {
  if (db != nullptr) {
    return true;
  }

  if (sqlite3_open(storagePath.c_str(), &db) != SQLITE_OK) {
    return false;
  }

  return true;
}

void Registration::closeDb() {
  if (db != nullptr) {
    sqlite3_close(db);
    db = nullptr;
  }
}

bool Registration::execWithoutArgs(const char* query) const {
  char* error;
  const auto execCode = sqlite3_exec(db, query, nullptr, nullptr, &error);

  if (error != nullptr) {
    logger(RES_ERROR(error));
    sqlite3_free(error);
  }

  return execCode == SQLITE_OK;
}

bool Registration::createTable() const {
  constexpr auto query = R"sql(
    CREATE TABLE IF NOT EXISTS users (
      imsi TEXT PRIMARY KEY,
      imei TEXT NOT NULL UNIQUE,
      msisdn TEXT NOT NULL UNIQUE,
      tmsi INTEGER UNIQUE,
      stationId INTEGER NOT NULL DEFAULT -1,
      vlrId INTEGER NOT NULL DEFAULT -1
    );
  )sql";

  return execWithoutArgs(query);
}

bool Registration::start() {
  if (running.load()) {
    return false;
  }

  {
    std::lock_guard lock(mutex);

    if (!openDb()) {
      std::cout << "Error opening database" << std::endl;
      return false;
    }

    if (!createTable()) {
      std::cout << "Error creating table" << std::endl;
      closeDb();
      return false;
    }
  }

  running.store(true);
  return true;
}

void Registration::stop() {
  if (!running.load()) {
    return;
  }

  closeDb();
  running.store(false);
}

inline sqlite3_stmt* Registration::createStatement(const char* query) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, query, -1, &stmt, nullptr) != SQLITE_OK) {
    return nullptr;
  }
  return stmt;
};

inline void Registration::deleteStatement(sqlite3_stmt* statement) {
  if (statement != nullptr) {
    sqlite3_finalize(statement);
  }
}

bool Registration::addUsers(const std::string& imsi, const std::string& imei,
                            const std::string& msisdn) {
  constexpr auto query = R"sql(
    INSERT INTO users (imsi, imei, msisdn)
    VALUES (?, ?, ?)
    ON CONFLICT(imsi) DO UPDATE SET
      imei = excluded.imei,
      msisdn = excluded.msisdn;
  )sql";

  return operationTemplate(query, [&](sqlite3_stmt* statement) {
    sqlite3_bind_text(statement, 1, imsi.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 2, imei.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 3, msisdn.c_str(), -1, SQLITE_TRANSIENT);
    return sqlite3_step(statement) == SQLITE_DONE;
  });
}

bool Registration::requestAuthInfo(const std::string& imei,
                                   const std::string& imsi) {
  constexpr auto query = R"sql(
    SELECT 1
    FROM users
    WHERE imsi = ?
    AND imei = ?
  )sql";

  return operationTemplate(query, [&](sqlite3_stmt* statement) {
    sqlite3_bind_text(statement, 1, imsi.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 2, imei.c_str(), -1, SQLITE_TRANSIENT);
    return sqlite3_step(statement) == SQLITE_ROW;
  });
}

bool Registration::changePathToUe(const uint64_t tmsi, const int32_t bsId) {
  constexpr auto query = R"sql(
    UPDATE users
    SET stationId = ?
    WHERE tmsi = ?
  )sql";

  return operationTemplate(query, [&](sqlite3_stmt* statement) {
    sqlite3_bind_int(statement, 1, bsId);
    sqlite3_bind_int64(statement, 2, static_cast<sqlite3_int64>(tmsi));

    if (sqlite3_step(statement) != SQLITE_DONE) {
      return false;
    }
    return sqlite3_changes(db) > 0;
  });
}

bool Registration::updateLocation(const uint64_t tmsi, const int32_t vlrId,
                                  const std::string& imsi) {
  constexpr auto query = R"sql(
    UPDATE users
    SET tmsi = ?,
      stationId = ?,
      vlrId = ?
    WHERE imsi = ?
  )sql";

  return operationTemplate(query, [&](sqlite3_stmt* statement) {
    sqlite3_bind_int64(statement, 1, static_cast<sqlite3_int64>(tmsi));
    sqlite3_bind_int(statement, 2, vlrId);
    sqlite3_bind_int(statement, 3, vlrId);
    sqlite3_bind_text(statement, 4, imsi.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(statement) != SQLITE_DONE) {
      return false;
    }

    return sqlite3_changes(db) > 0;
  });
}

bool Registration::getMsisdnByTmsi(const uint64_t tmsi, std::string& msisdn) {
  constexpr auto query = R"sql(
    SELECT msisdn
    FROM users
    WHERE tmsi = ?
  )sql";

  return operationTemplate(query, [&](sqlite3_stmt* statement) {
    sqlite3_bind_int64(statement, 1, static_cast<sqlite3_int64>(tmsi));

    if (sqlite3_step(statement) != SQLITE_ROW) {
      return false;
    }

    msisdn = reinterpret_cast<const char*>(sqlite3_column_text(statement, 0));
    return true;
  });
}

bool Registration::resolveDestination(const std::string& msisdnDst,
                                      uint64_t& tmsiDst, int32_t& bsId) {
  constexpr auto query = R"sql(
    SELECT tmsi, stationId
    FROM users
    WHERE msisdn = ?
    AND tmsi IS NOT NULL
    AND stationId >= 0
  )sql";

  return operationTemplate(query, [&](sqlite3_stmt* statement) {
    sqlite3_bind_text(statement, 1, msisdnDst.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(statement) != SQLITE_ROW) {
      return false;
    }

    tmsiDst = sqlite3_column_int64(statement, 0);
    bsId = sqlite3_column_int(statement, 1);
    return true;
  });
}
