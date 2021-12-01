#include <string>
#include <mqtt/async_client.h>
#include "callback.hpp"
#include <mutex>
#include <map>
#include <functional>
#include <iostream>

#define ERROR_LEVEL 1
#define WARNING_LEVEL 2

class MQTT{
    protected:
        MQTT(std::string network, std::string user, std::string pass);
        MQTT(std::string network);
        ~MQTT() {}

    private:
        mqtt::async_client cli;
        callback *cb;
        static MQTT *client;
        static std::mutex mutex;

    public:
        static MQTT* getInstance(std::string network, std::string user, std::string pass);
        static MQTT* getInstance(std::string network);

        void publish(std::string topic, int qos, std::string msg);
        void publish(std::string topic, int qos, std::string msg, std::function<void(std::string, std::string)> func);
        void error(int error, std::string errormsg);
        void subscribe(std::string topic, int qos, std::function<void(std::string, std::string)> func);
        
        MQTT(MQTT const&) = delete;
        void operator=(MQTT const&) = delete;
};