# mobile

### Описание: 
Клиенто-серверное приложение, которое имеет приложение с раздельным
запуском для сервера и клиента.

Сервер состоит из блоков: UE, eNode-b, MME, SMSC, VLR, HLR.

Клиенты могу присоединяться к серверу и обмениваться сообщениями.

#### Запустив клиента есть различные команды: 

- ACTION [ON/OFF[ - включает и выключает устройство (при включении 
connect к серверу, при отключении disconnect)
- MOVE [x] - изменение позиции в пространстве по 1 оси
- STATUS - вывод текущего состояния подключения к серверу
- SMS [MSISDN] [text] - отправка смс по номеру телефона
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

- для клиента <br><br>
  cd client/tests <br>
  cmake -B build/ CMakeList.txt <br>
  cmake --build build/ <br>
  cd build/<br>
  ./test<br>

- для сервера <br><br>
  cd server/tests <br>
  cmake -B build/ CMakeList.txt <br>
  cmake --build build/ <br>
  cd build/<br>
  ./test<br>

---

### Санитазеры и статический анализатор
- для запуска адресного санитайзера в CMakeList.txt по адресам ./server/
раскомментируйте: 
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")

---

### Структура файлов 

├── client <br>
│   ├── build <br>
│   ├── cmake <br>
│   │   └── cppcheck.cmake <br>
│   ├── CMakeLists.txt <br>
│   ├── includes <br>
│   │   ├── AppSettings.h <br>
│   │   ├── DataPool.h <br>
│   │   ├── Menu.h <br>
│   │   ├── NetWork.h <br>
│   │   ├── SocketClient.h <br>
│   │   └── Tests.h <br>
│   ├── sources <br>
│   │   ├── AppSettings.cpp <br>
│   │   ├── DataPool.cpp <br>
│   │   ├── main.cpp <br>
│   │   ├── Menu.cpp <br>
│   │   ├── NetWork.cpp <br>
│   │   ├── SocketClient.cpp <br>
│   │   └── Tests.cpp <br>
│   └── tests <br>
│      ├── AppSettingsTest.cpp <br>
│      ├── CheckAppTest.cpp <br>
│      ├── CMakeLists.txt <br>
│      ├── DataPoolTets.cpp <br>
│      └── FunctionTest.cpp <br>
├── commonFunc <br>
│   ├── includes <br>
│   │   ├── PacketFunction.h <br>
│   │   ├── ResultStatus.h <br>
│   │   ├── StandardPackets.h <br>
│   │   ├── StringFunction.h <br>
│   │   ├── ValidationFunction.h <br>
│   │   └── VectorProcess.h <br>
│   └── sources <br>
│      ├── PacketFunction.cpp <br>
│      ├── ResultStatus.cpp <br>
│      ├── StringFunction.cpp <br>
│      ├── ValidationFunction.cpp <br>
│      └── VectorProcess.cpp <br>
├── README.md <br>
└── server <br>
├── cmake <br>
│   └── cppcheck.cmake <br>
├── CMakeLists.txt <br>
├── includes <br>
│   ├── ConfigApp.h <br>
│   ├── EpollServer.h <br>
│   ├── ManageServer.h <br>
│   ├── MathFunc.h <br>
│   ├── SocketServer.h <br>
│   └── ThreadPool.h <br>
├── sources <br>
│   ├── ConfigApp.cpp <br>
│   ├── EpollServer.cpp <br>
│   ├── main.cpp <br>
│   ├── ManageServer.cpp <br>
│   ├── MathFunc.cpp <br>
│   ├── SocketServer.cpp <br>
│   └── ThreadPool.cpp <br>
└── tests <br>
   ├── CMakeLists.txt <br>
   ├── configAppTest.cpp <br>
   ├── epollTest.cpp <br>
   ├── mathFuncTest.cpp <br>
   └── socketTest.cpp <br>

---

### Лицензия

Copyright (c) 2026 fisp

---

### Контакты

- GitHub: [github.com/fisp](https://github.com/fisp)