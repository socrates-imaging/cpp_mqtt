#pragma once
#include <map>
#include <mutex>
#include <string>
#include <iostream>
#include <functional>
#include <mqtt/async_client.h>
#include "callback.hpp"

#define ERROR_LEVEL 1
#define WARNING_LEVEL 2

class MQTT{
    protected:
        MQTT(std::string UUID, std::string network, std::string user, std::string pass);
        MQTT(std::string UUID, std::string network);
        ~MQTT() {}

    private:
        mqtt::async_client cli;
        mqtt::connect_options connOpts;
        callback cb;
        static MQTT *client;
        static std::mutex mutex;
        bool connecting = false;
    public:
        static MQTT* getInstance(std::string UUID = "", std::string network = "", std::string user = "", std::string pass = "");
        
        enum QOS{
            AT_MOST_ONCE = 0,
            AT_LEAST_ONE = 1,
            EXACTLY_ONCE = 2
        };


        bool isConnected();

        void publish(std::string topic, int qos, std::string msg, bool retain = false);
        void publish(std::string topic, int qos, std::string msg, std::function<void(std::string, std::string)> func);
        void error(int error, std::string errormsg);
        void subscribe(std::string topic, int qos, std::function<void(std::string, std::string)> func);
        void unsubscribe(std::string topic);
        bool connect();
        bool disconnect(int timeout_ms = 200);

        MQTT(MQTT const&) = delete;
        void operator=(MQTT const&) = delete;
};