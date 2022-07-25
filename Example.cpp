#define SPDLOG_FMT_EXTERNAL // pacman has some problems with upstream fmt, so just tell to use external FMT, see (https://bugs.archlinux.org/task/71029)
#define MQTT_SPDLOG
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include "mqtt_singleton.h"

#include <string>
#include <exception>
#include <iostream>

void on_message(std::string mod, std::string payload);

int main() { 
	auto spdlogger = spdlog::stdout_color_mt("MQTT");

	MQTT* mqtt = MQTT::getInstance("127.0.0.1"); // server IP

	// subscribe to topic, together with setting message handler, print on success
    const std::string mqtt_topic = "Some/Topic";
    mqtt->subscribe(mqtt_topic, MQTT::QOS::EXACTLY_ONCE, &on_message); // topic, qos, message event handler	

	while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

	return 0;
}

void on_message(std::string mod, std::string payload){
	std::cout << "Received MQTT message!" << std::endl;
	std::cout << "payload: " << payload << std::endl;

	MQTT* mqtt = MQTT::getInstance();
	try{
		mqtt->publish("Some/TopicReply", MQTT::QOS::AT_MOST_ONCE, "Hello!");
	}
	catch(const std::exception& e){
		std::cout << "MQTT Exception: " << e.what() << std::endl;
	}
}