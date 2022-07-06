#include "mqtt_singleton.h"

#ifdef MQTT_SPDLOG
#include <spdlog/spdlog.h>
#else
#include <iostream>
#endif

MQTT* MQTT::client{nullptr};
std::mutex MQTT::mutex; 

MQTT *MQTT::getInstance(std::string UUID, std::string network, std::string user, std::string pass){
    #ifdef SPDLOG_H
    auto logger = spdlog::get("MQTT");
    #endif
    std::lock_guard<std::mutex> lock(MQTT::mutex);
    if(MQTT::client == nullptr){
        if(network == ""){
            #ifdef SPDLOG_H
            logger->error("trying to initialize without network url!");
            #else
            std::cout << "[MQTT] trying to initialize without network url!" << std::endl;
            #endif
        }
        if(user == "" && pass == "")
            MQTT::client = new MQTT(UUID, network);
        else
            MQTT::client = new MQTT(UUID, network, user, pass);
    }
    return MQTT::client;
}

MQTT::MQTT(std::string UUID, std::string network)
    : cli(network, UUID),
      connOpts(mqtt::connect_options_builder().clean_session().mqtt_version(0).finalize()),
      cb(cli, connOpts) {

    cli.set_callback(cb);

	this->connect();
}

MQTT::MQTT(std::string UUID, std::string network, std::string user, std::string pass) 
    : cli(network, UUID),
      connOpts(mqtt::connect_options_builder()
        .user_name(user).password(pass).clean_session().mqtt_version(0).finalize()),
      cb(cli, connOpts) {

    #ifdef SPDLOG_H
    auto logger = spdlog::get("MQTT");
    #endif

    cli.set_callback(cb);

	this->connect();
}

bool MQTT::connect(){
    #ifdef SPDLOG_H
    auto logger = spdlog::get("MQTT");
    #endif
    if(cb.nretry_ == 0)
        connecting = false;
    if(cb.connected || connecting)
        return cb.connected;
    try {
        #ifdef SPDLOG_H
		logger->info("Connecting to the MQTT server...");
        #else
        std::cout << "[MQTT] Connecting to the MQTT server..." << std::endl;
        #endif
        connecting = true;
        cb.nretry_ = 1; // attempt 1
		cli.connect(connOpts, nullptr, cb)->wait();
        connecting = false;
    }
	catch (const mqtt::exception& exc) {
        #ifdef SPDLOG_H
		logger->warn("Unable to connect to MQTT server, reason: {}", exc.what());
        #else
        std::cout << "[MQTT] Unable to connect to MQTT server, reason: " << exc.what() << std::endl;
        #endif
	}
    return cb.connected;
}

bool MQTT::disconnect(int timeout_ms){
    #ifdef SPDLOG_H
    auto logger = spdlog::get("MQTT");
    #endif
    if(!cb.connected)
        return !cb.connected;
    try {
        #ifdef SPDLOG_H
		logger->info("Disconnecting from the MQTT server...");
        #else
        std::cout << "[MQTT] Disconnecting from the MQTT server..." << std::endl;
        #endif
        cli.disconnect();
        cb.connected = false;
    } catch (const mqtt::exception& exc) {
        #ifdef SPDLOG_H
		logger->info("Unable to disconnect from MQTT server, reason: {}", exc.what());
        #else
        std::cout << "[MQTT] Unable to disconnect from MQTT server, reason: " << exc.what() << std::endl;
        #endif
    }
    return !cb.connected;
}

bool MQTT::isConnected(){return cb.connected;}

void MQTT::subscribe(std::string topic, int qos, std::function<void(std::string, std::string)> func){
    cb.add_callback(topic, qos, func);
}

void MQTT::publish(std::string topic, int qos, std::string msg, bool retain){
    mqtt::message_ptr message = mqtt::make_message(topic, msg);
    message->set_qos(qos);
    message->set_retained(retain);
    cb.publish(message);
}

void MQTT::publish(std::string topic, int qos, std::string msg, std::function<void(std::string, std::string)> func){
    mqtt::message_ptr message = mqtt::make_message(topic, msg);
    message->set_qos(qos);
    cb.publish(message);
    cb.add_callback("topic", func);
}

void MQTT::unsubscribe(std::string topic){
    cb.remove_callback(topic);
}

void MQTT::error(int error, std::string errormsg){
    if(error == 1){
        publish("warning", 2, errormsg);
    }else if(error == 2){
        publish("error", 2, errormsg);
    }
}