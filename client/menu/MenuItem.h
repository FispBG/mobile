//
// Created by fisp on 17.04.2026.
//

#pragma once

class Exchange;

class MenuItem {
  Exchange& exchange;

 public:
  explicit MenuItem(Exchange& exchange) : exchange(exchange) {}
  virtual void action() = 0;
  virtual ~MenuItem() = default;

  Exchange& getExchange() const { return exchange; }
};

class CommandActive : public MenuItem {
 public:
  explicit CommandActive(Exchange& exchange) : MenuItem(exchange) {}
  void action() override;
};

class CommandMove : public MenuItem {
 public:
  explicit CommandMove(Exchange& exchange) : MenuItem(exchange) {}
  void action() override;
};

class CommandSearch : public MenuItem {
 public:
  explicit CommandSearch(Exchange& exchange) : MenuItem(exchange) {}
  void action() override;
};

class CommandSms : public MenuItem {
 public:
  explicit CommandSms(Exchange& exchange) : MenuItem(exchange) {}
  void action() override;
};

class CommandStatus : public MenuItem {
 public:
  explicit CommandStatus(Exchange& exchange) : MenuItem(exchange) {}
  void action() override;
};

class CommandInbox : public MenuItem {
 public:
  explicit CommandInbox(Exchange& exchange) : MenuItem(exchange) {}
  void action() override;
};

class CommandOutbox : public MenuItem {
 public:
  explicit CommandOutbox(Exchange& exchange) : MenuItem(exchange) {}
  void action() override;
};

class CommandExit : public MenuItem {
 public:
  explicit CommandExit(Exchange& exchange) : MenuItem(exchange) {}
  void action() override;
};
