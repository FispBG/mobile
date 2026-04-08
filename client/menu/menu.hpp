//
// Created by fisp on 07.04.2026.
//

#pragma once

class MenuItem {
public:

    virtual ~MenuItem() = default;
    virtual void processRequest();
};

class CloseProgram : public MenuItem {
    public:
    void processRequest() override;
};

class ActiveMode : public MenuItem {
    public:
        void processRequest() override;
};

class MoveDevice : public MenuItem {
    public:
        void processRequest() override;
};

class SendSms : public MenuItem {
    public:
        void processRequest() override;
};

class Menu {
public:

        Menu();
};

