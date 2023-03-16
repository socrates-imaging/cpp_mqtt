#include "mqtt_singleton.h"

#include <memory>
#ifdef MQTT_SPDLOG
#include <spdlog/spdlog.h>
#else
#include <iostream>
#endif

// Static member initialization
std::mutex MQTT::mutex; 
std::unique_ptr<MQTT> MQTT::client = nullptr; 

void MQTT::initalize(std::string UUID, std::string network, std::string user, std::string pass, bool clean_sessions){
    #ifdef SPDLOG_H
    auto logger = spdlog::get("MQTT");
    #endif

    std::lock_guard<std::mutex> lock(MQTT::mutex);
    
    if(client != nullptr){
        #ifdef SPDLOG_H
        logger->warn("MQTT singleton already initalized!");
        #else
        std::cout << "[MQTT] MQTT Singleton already initalized!" << std::endl;
        #endif
        return;
    }

    if(network == ""){
        #ifdef SPDLOG_H
        logger->error("trying to initialize without network url!");
        #else
        std::cout << "[MQTT] trying to initialize without network url!" << std::endl;
        #endif
    }

    auto options = mqtt::connect_options_builder().mqtt_version(0).automatic_reconnect(std::chrono::seconds(3), std::chrono::seconds(10));

    if(user != "" || pass != ""){
        options.user_name(user).password(pass);
    }
    if (clean_sessions){
        options.clean_session(true);
    } else {
        options.clean_session(false);
    }

    client = std::make_unique<MQTT>(UUID, network, options.finalize());
}

MQTT* MQTT::getInstance(){
    std::lock_guard<std::mutex> lock(MQTT::mutex);
    if(client == nullptr){
        #ifdef SPDLOG_H
        auto logger = spdlog::get("MQTT");
        logger->error("trying to get instance without initalizing!");
        #else
        std::cout << "[MQTT] trying to get instance without initalizing!" << std::endl;
        #endif
    }
    return client.get();
}


MQTT::MQTT(std::string UUID, std::string network, mqtt::connect_options _connOpts)
:  cli(network, UUID), connOpts(_connOpts),  cb(cli, connOpts) 
{
    cli.set_callback(cb); 
    this->connect();
}

bool MQTT::connect(){
    #ifdef SPDLOG_H
    auto logger = spdlog::get("MQTT");
    #endif
    try {
        cli.connect(connOpts, nullptr, cb)->wait();
        cb.connected = true;
    }
	catch (const mqtt::exception& exc) {
        std::cout << "Failed to connect" << std::endl;
        cb.connected = false;
        return false;
	}
    return true;
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

bool MQTT::isConnected(){
    std::cout << "MQTT is connected: " << cb.connected << std::endl;
    //return cli.is_connected();
    return cb.connected;
}

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