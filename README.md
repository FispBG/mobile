# mobile

### Описание:
Клиенто-серверное приложение, которое имеет приложение с раздельным
запуском для сервера и клиента.

Сервер состоит из блоков: UE, eNode-b, MME, SMSC, VLR, HLR.

Клиенты могу присоединяться к серверу и обмениваться сообщениями.

#### Запустив клиента есть различные команды:

- ACTION - включает и выключает устройство (при включении
  connect к серверу, при отключении disconnect)
- MOVE - изменение позиции в пространстве по 1 оси
- STATUS - вывод текущего состояния подключения к серверу
- SMS - отправка смс по номеру телефона
- SEARCH - обход доступности вышек связи
- INBOX - хранилище входящих смс
- OUTBOX - хранилище исходящих смс
- EXIT - закрытие клиента

#### Список зарегистрированных пользователей

(IMSI, IMEI, MSISDN)

- ("12345601112233", "158863118273320", "89991112233")
- ("12345603334455", "325314891006270", "89993334455")
- ("12345605556677", "891352784123650", "89995556677")
- ("12345607778899", "891355118756160", "89997778899")

#### Команды для запуска клиента:

- Запуск клиента после сборки: ```./client -ip <ip> -port <port> -msisdn <msisdn> -imsi <imsi> - imei <imei> -position <position>```
- Все параметры обязательны
- Получение справки по возможным флагам: ```./client --help```
- Готовый пресет для запуска клиента 1:
  ```./client -ip 127.0.0.1 -port 9000 -msisdn 89991112233 -imsi 12345601112233 -imei 158863118273320 -position 100```
- Готовый пресет для запуска клиента 2:
  ```./client -ip 127.0.0.1 -port 9000 -msisdn 89993334455 -imsi 12345603334455 -imei 325314891006270 -position -100```


#### Команды для запуска сервера:

- Запуск клиента после сборки: ```./server 9000```
- или просто ./server, стандартный порт 9000
- Получение справки по возможным флагам: ```./client --help```

### Зависимости

Компилятор
- GCC >= 13.3.0
- Стандарт: C++20

#### Библиотеки
- GTest >= v1.14.x

#### Окружение
- ОС: Linux
- CMake >= 3.28

#### Сборка
Необходим cmake >= 3.28

- Сборка для сервера из корня(../mobile)
cd server <br>
cmake -B build/ CMakeList.txt <br>
cmake --build build/ <br>

Или через запуск скрипта <br>
cd server <br>
bash ./run.sh <br>

- Сборка для сервера из корня(../mobile)
cd client <br>
cmake -B build/ CMakeList.txt <br>
cmake --build build/ <br>

Или через запуск скрипта <br>
cd client <br>
bash ./run.sh <br>
---

### Запуск тестов

- для сервера

  cd server/Testing <br>
  cmake -B build/ CMakeList.txt <br>
  cmake --build build/ <br>
  cd build/<br>
  ./tests

---

### Санитазеры и статический анализатор
- Сборка для сервера из корня c санитайзерами и cppcheck(../mobile)
  cd server <br>
  cmake -B build/ CMakeList.txt -DCMAKE_BUILD_TYPE=Debug <br>
  cmake --build build/ <br>

Или через запуск скрипта <br>
cd server <br>
bash ./run.sh Debug <br>

- Сборка для сервера из корня c санитайзерами и cppcheck(../mobile)
  cd client <br>
  cmake -B build/ CMakeList.txt -DCMAKE_BUILD_TYPE=Debug <br>
  cmake --build build/ <br>

Или через запуск скрипта <br>
cd client <br>
bash ./run.sh Debug <br>

---

### Структура файлов

. <br>
├── client <br>
│   ├── context <br>
│   │   ├── Context.cpp <br>
│   │   ├── Context.cpp <br>
│   │   ├── DataStorage.cpp <br>
│   │   └── DataStorage.hpp <br>
│   ├── data <br>
│   │   └── address_book.json <br>
│   ├── exchange <br>
│   │   ├── ReadPacket.cpp <br>
│   │   ├── ReadPacket.cpp <br>
│   │   ├── SerializedPacket.cpp <br>
│   │   ├── Exchange.cpp <br>
│   │   └── Exchange.hpp <br>
│   ├── menu <br>
│   │   ├── MenuItem.cpp <br>
│   │   ├── MenuItem.cpp <br>
│   │   ├── Menu.cpp <br>
│   │   └── Menu.hpp <br>
│   ├── main.cpp <br>
│   └── CMakeLists.txt <br>
├── commonFiles <br>
│   ├── byteFunc <br>
│   │   ├── BytesTransform.cpp <br>
│   │   └── BytesTransform.hpp <br>
│   ├── resultFunc <br>
│   │   ├── ResultFunction.cpp <br>
│   │   └── ResultFunction.hpp <br>
│   ├── inputArgsFunc <br>
│   │   └── InputArgsFunc.hpp <br>
│   ├── utilityFunc <br>
│   │   └── UtilityFunc.hpp <br>
│   ├── validateFunc <br>
│   │   └── ValidateFunc.hpp <br>
│   └── stringFunc <br>
│     └── StringFunc.hpp <br>
└── server <br>
│  ├── baseStation <br>
│  │   ├── BaseStation.cpp <br>
│  │   └── BaseStation.hpp <br>
│  ├── config <br>
│  │   └── basst.json <br>
│  ├── listener <br>
│  │   ├── Listener.cpp <br>
│  │   └── Listener.hpp <br>
│  ├── mme <br>
│  │   ├── MME.cpp <br>
│  │   └── MME.hpp <br>
│  ├── register <br>
│  │   ├── Register.cpp <br>
│  │   └── Register.hpp <br>
│  ├── smsc <br>
│  │   ├── SMSC.cpp <br>
│  │   └── SMSC.hpp <br>
│  ├── Testing <br>
│  │   ├── BaseStationTest.cpp <br>
│  │   ├── CMakeLists.txt <br>
│  │   ├── ListenerTest.cpp <br>
│  │   ├── MMETest.cpp <br>
│  │   ├── RegisterTest.cpp <br>
│  │   ├── SMSCTest.cpp <br>
│  │   ├── TestConfug.hpp <br>
│  │   └── UeContextTest.cpp <br>
│  └── ueContext <br>
│  │  ├── HandleMessage.cpp <br>
│  │  ├── HandleMessage.hpp <br>
│  │  ├── UeContext.cpp <br>
│  │  └── UeContext.hpp <br>
│  ├── CMakeLists.txt <br>
│  ├── main.cpp <br>
│  └── README.md <br>

---

### Лицензия

Copyright (c) 2026 fisp

---

### Контакты

- GitHub: [github.com/fisp](https://github.com/fisp)