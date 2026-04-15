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

- Запуск клиента после сборки: ./client [ip] [port] [msisdn] [imsi] [imei] [position]
- Все параметры обязательны
- Получение справки по возможным флагам: ./client --help

#### Команды для запуска сервера:

- Запуск клиента после сборки: ./server 9000
- или просто ./server, стандартный порт 9000
- Получение справки по возможным флагам: ./client --help

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
- cmake

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
- для запуска адресного санитайзера в CMakeList.txt по адресам ./server/
раскомментируйте: 
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

---

### Структура файлов 

. <br>
├── client <br>
│   ├── context <br>
│   │   ├── context.cpp <br>
│   │   └── context.hpp <br>
│   ├── data <br>
│   │   └── address_book.json <br>
│   ├── exchange <br>
│   │   ├── exchange.cpp <br>
│   │   └── exchange.hpp <br>
│   ├── menu <br>
│   │   ├── menu.cpp <br>
│   │   └── menu.hpp <br>
│   ├── main.cpp <br>
│   └── CMakeLists.txt <br>
├── commonFiles <br>
│   ├── byteFunc <br>
│   │   ├── BytesTransform.cpp <br>
│   │   └── BytesTransform.hpp <br>
│   ├── resultFunc <br>
│   │   ├── ResultFunction.cpp <br>
│   │   └── ResultFunction.hpp <br>
│   └── stringFunc <br>
│       └── StringFunc.hpp <br>
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