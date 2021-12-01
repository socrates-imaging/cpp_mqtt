#include "mqtt_singleton.h"

MQTT* MQTT::client{nullptr};
std::mutex MQTT::mutex; 

MQTT *MQTT::getInstance(std::string network, std::string user, std::string pass){
    std::lock_guard<std::mutex> lock(MQTT::mutex);
    if(MQTT::client == nullptr){
        MQTT::client = new MQTT(network, user, pass);
    }
    return MQTT::client;
}

MQTT *MQTT::getInstance(std::string network){
    std::lock_guard<std::mutex> lock(MQTT::mutex);
    if(MQTT::client == nullptr){
        MQTT::client = new MQTT(network);
    }
    return MQTT::client;
}

MQTT::MQTT(std::string network) : cli(network, "MQTT_SINGLETON_XPRESS") {
    mqtt::connect_options connOpts = mqtt::connect_options_builder()
        .mqtt_version(0)
        .clean_session()
        .finalize();

    cb = new callback(cli, connOpts);
    cli.set_callback(*cb);

	try {
		std::cout << "Connecting to the MQTT server..." << std::flush;
		cli.connect(connOpts, nullptr, *cb)->wait();
	}
	catch (const mqtt::exception& exc) {
		std::cerr << "\nERROR: Unable to connect to MQTT server, reason: " << exc << std::endl;
	}
}

MQTT::MQTT(std::string network, std::string user, std::string pass) : cli(network, "MQTT_SINGLETON_EXPRESS") {
    mqtt::connect_options connOpts = mqtt::connect_options_builder()
        .user_name(user)
        .password(pass)
        .clean_session()
        .mqtt_version(0)
        .finalize();

    cb = new callback(cli, connOpts);
    cli.set_callback(*cb);

	try {
		std::cout << "Connecting to the MQTT server..." << std::flush;
		cli.connect(connOpts, nullptr, *cb)->wait();
	}
	catch (const mqtt::exception& exc) {
		std::cerr << "\nERROR: Unable to connect to MQTT server, reason: " << exc.what() << std::endl;
	}
}
  
void MQTT::subscribe(std::string topic, int qos, std::function<void(std::string, std::string)> func){
    cb->add_callback(topic, qos, func);
}

void MQTT::publish(std::string topic, int qos, std::string msg){
    mqtt::message_ptr message = mqtt::make_message(topic, msg);
    message->set_qos(qos);
    cb->publish(message);
}

void MQTT::publish(std::string topic, int qos, std::string msg, std::function<void(std::string, std::string)> func){
    mqtt::message_ptr message = mqtt::make_message(topic, msg);
    message->set_qos(qos);
    cb->publish(message);
    cb->add_callback("topic", func);
}

void MQTT::error(int error, std::string errormsg){
    if(error == 1){
        publish("warning", 2, errormsg);
    }else if(error == 2){
        publish("error", 2, errormsg);
    }
}