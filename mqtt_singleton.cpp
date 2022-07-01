#include "mqtt_singleton.h"

#ifdef MQTT_SPDLOG
#include <spdlog/spdlog.h>
#else
#include <iostream>
#endif

MQTT* MQTT::client{nullptr};
std::mutex MQTT::mutex; 

MQTT *MQTT::getInstance(std::string network, std::string user, std::string pass){
    #ifdef SPDLOG_H
    auto logger = spdlog::get("MQTT");
    #endif
    std::lock_guard<std::mutex> lock(MQTT::mutex);
    if(MQTT::client == nullptr){
        if(network == ""){
            #ifdef SPDLOG_H
            logger->error("[MQTT] trying to initialize without network url!");
            #else
            std::cout << "[MQTT] trying to initialize without network url!" << std::endl;
            #endif
        }
        if(user == "" && pass == "")
            MQTT::client = new MQTT(network);
        else
            MQTT::client = new MQTT(network, user, pass);
    }
    return MQTT::client;
}

MQTT::MQTT(std::string network)
    : cli(network, "MQTT_SINGLETON_EXPRESS"),
      connOpts(mqtt::connect_options_builder().clean_session().mqtt_version(0).finalize()),
      cb(cli, connOpts) {

    // connOpts SetAutomaticReconnect
    
    std::cout << "Creating MQTT with only network " << std::endl;

    #ifdef SPDLOG_H
    auto logger = spdlog::get("MQTT");
    #endif

    cli.set_callback(cb);

	this->connect();
}

MQTT::MQTT(std::string network, std::string user, std::string pass) 
    : cli(network, "MQTT_SINGLETON_EXPRESS"),
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
    if(connected)
        return connected;
    try {
        #ifdef SPDLOG_H
		logger->info("[MQTT] Connecting to the MQTT server...");
        #else
        std::cout << "[MQTT] Connecting to the MQTT server..." << std::endl;
        #endif
		cli.connect(connOpts, nullptr, cb)->wait();
        connected = true;
    }
	catch (const mqtt::exception& exc) {
        #ifdef SPDLOG_H
		logger->info("[MQTT] Unable to connect to MQTT server, reason: {}", exc.what());
        #else
        std::cout << "[MQTT] Unable to connect to MQTT server, reason: " << exc.what() << std::endl;
        #endif
	}
    return connected;
}

bool MQTT::disconnect(int timeout_ms){
    #ifdef SPDLOG_H
    auto logger = spdlog::get("MQTT");
    #endif
    if(!connected)
        return !connected;
    try {
        #ifdef SPDLOG_H
		logger->info("[MQTT] Disconnecting from the MQTT server...");
        #else
        std::cout << "[MQTT] Disconnecting from the MQTT server..." << std::endl;
        #endif
        cli.disconnect();
        connected = false;
    } catch (const mqtt::exception& exc) {
        #ifdef SPDLOG_H
		logger->info("[MQTT] Unable to disconnect from MQTT server, reason: {}", exc.what());
        #else
        std::cout << "[MQTT] Unable to disconnect from MQTT server, reason: " << exc.what() << std::endl;
        #endif
    }
    return !connected;
}
  
void MQTT::subscribe(std::string topic, int qos, std::function<void(std::string, std::string)> func){
    cb.add_callback(topic, qos, func);
}

void MQTT::publish(std::string topic, int qos, std::string msg){
    mqtt::message_ptr message = mqtt::make_message(topic, msg);
    message->set_qos(qos);
    cb.publish(message);
}

void MQTT::unsubscribe(std::string topic){
    cb.remove_callback(topic);
}

void MQTT::publish(std::string topic, int qos, std::string msg, std::function<void(std::string, std::string)> func){
    mqtt::message_ptr message = mqtt::make_message(topic, msg);
    message->set_qos(qos);
    cb.publish(message);
    cb.add_callback("topic", func);
}

void MQTT::error(int error, std::string errormsg){
    if(error == 1){
        publish("warning", 2, errormsg);
    }else if(error == 2){
        publish("error", 2, errormsg);
    }
}