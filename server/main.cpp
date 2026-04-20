#include "../commonFiles/validateFunc/ValidateFunc.hpp"
#include "baseStation/BaseStation.hpp"
#include "listener/Listener.hpp"
#include "mme/MME.hpp"
#include "register/Register.hpp"
#include "smsc/SMSC.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <vector>

namespace MainConfig {
  inline const std::string pathToStationConfig = "../config/basst.json";
  inline const std::string pathToDbConfig = "./registration.db";
}

std::vector<BasestationConfig> loadStationsConfig(const std::string& path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    throw std::runtime_error("Cant open stations config.");
  }

  nlohmann::json data;
  file >> data;

  std::vector<BasestationConfig> stations;

  if (data.is_array()) {
    for (const auto& item : data) {
      stations.push_back(
          {item.value("bs_id", 0), item.value("position", 0),
           item.value("MME_id", 1), item.value("radius", 120),
           item.value("maxConnections", 4), item.value("bufferSizeMax", 4),
           static_cast<uint8_t>(item.value("networkVersion", 4))});
    }
  }

  return stations;
}

int main(const int argc, const char** argv) {
  if (argc == 2 && strcmp(argv[1], "--help") == 0) {
    std::cout << "command to run: ./app port        or simple ./app        "
                 "default port = 9000"
              << std::endl;
  }

  uint16_t serverPort{0};

  if (!(argc > 1 && isPort(argv[1], serverPort))) {
    serverPort = 9000;
  }

  auto registration =
      std::make_shared<Registration>(MainConfig::pathToDbConfig);
  auto mme = std::make_shared<MME>(1, registration);
  const auto smsc = std::make_shared<SMSC>(std::chrono::milliseconds(15000));

  if (!registration->start()) {
    std::cout << "Error open db." << std::endl;
    return 1;
  }

  mme->setSmsc(smsc);
  smsc->setMME(mme);
  mme->start();
  smsc->start();

  // Для первого запуска, чтобы создалась бд
  registration->addUsers("12345601112233", "158863118273320", "89991112233");
  registration->addUsers("12345603334455", "325314891006270", "89993334455");
  registration->addUsers("12345605556677", "891352784123650", "89995556677");
  registration->addUsers("12345607778899", "891355118756160", "89997778899");

  std::vector<BasestationConfig> stations =
      loadStationsConfig(MainConfig::pathToStationConfig);
  if (stations.size() < 2) {
    std::cerr << "Json station broken" << std::endl;
    return 1;
  }

  auto bs1 = std::make_shared<BaseStation>(
      stations[0].bsId, stations[0].position, stations[0].mmeId,
      stations[0].radius, stations[0].maxConnections, stations[0].bufferSizeMax,
      mme);

  auto bs2 = std::make_shared<BaseStation>(
      stations[1].bsId, stations[1].position, stations[1].mmeId,
      stations[1].radius, stations[1].maxConnections, stations[1].bufferSizeMax,
      mme);

  mme->registerStation(bs1->getId(), bs1);
  mme->registerStation(bs2->getId(), bs2);

  bs1->start();
  bs2->start();

  Listener listener;
  listener.setStationsOnline({bs1, bs2});

  const auto socketResult = listener.createServerSocket(serverPort, 16);
  if (!socketResult.isGood()) {
    std::cerr << "Fail start server socket." << std::endl;
    bs1->stop();
    bs2->stop();
    smsc->stop();
    mme->stop();
    return 1;
  }

  listener.runServer();

  while (true) {
    const int key = getchar();
    if (key == 'q') {
      std::cout << "Stopping server..." << std::endl;
      listener.stopServer();
      bs1->stop();
      bs2->stop();
      smsc->stop();
      mme->stop();
      std::cout << "Server stopped." << std::endl;
      return 0;
    }
  }
}