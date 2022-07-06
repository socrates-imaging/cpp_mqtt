// uses the C/C++ mqtt-paho library. Needs libmosquitto-dev to compile, follow https://github.com/eclipse/paho.mqtt.cpp to install.
#pragma once

#include <sstream>
#include <mqtt/async_client.h>

#ifdef MQTT_SPDLOG
#include <spdlog/spdlog.h>
#else
#include <iostream>
#endif

const int	N_RETRY_ATTEMPTS = 5;

class action_listener : public virtual mqtt::iaction_listener{
    std::string name_;

	void on_failure(const mqtt::token& tok) override {
		#ifdef SPDLOG_H
		auto logger = spdlog::get("MQTT");
		#endif
		std::stringstream sstream;
		sstream << "[MQTT] " << name_ << " failure";
		if (tok.get_message_id() != 0)
			sstream << " for token: [" << tok.get_message_id() << "]" << std::endl;
		sstream << std::endl;
		#ifdef SPDLOG_H
		logger->warn(sstream.str());
		#else
		std::cout << sstream.str() << std::endl;
		#endif
	}

	void on_success(const mqtt::token& tok) override {
		#ifdef SPDLOG_H
		auto logger = spdlog::get("MQTT");
		#endif
		std::stringstream sstream;
		sstream << name_ << " success";
		if (tok.get_message_id() != 0)
			sstream << " for token: [" << tok.get_message_id() << "]" << std::endl;
		auto top = tok.get_topics();
		if (top && !top->empty())
			sstream << "\ttoken topic: '" << (*top)[0] << "', ..." << std::endl;
		sstream << std::endl;
		#ifdef SPDLOG_H
		logger->info(sstream.str());
		#else
		std::cout << sstream.str() << std::endl;
		#endif
	}
    public:
	    action_listener(const std::string& name) : name_(name) {}
};

class callback : public virtual mqtt::callback,
					public virtual mqtt::iaction_listener{
    // Counter for the number of connection retries
	int nretry_;
	// Reference to the client for reconnect
	mqtt::async_client& cli_;
	// Options to use if we need to reconnect
	mqtt::connect_options& connOpts_;
	// An action listener to display the result of actions.
	action_listener subListener_;
	//Map to the callable functions
    std::map<std::string, std::function<void(std::string, std::string)>> dispatch;


	void reconnect() {
		#ifdef SPDLOG_H
		auto logger = spdlog::get("MQTT");
		#endif
		std::this_thread::sleep_for(std::chrono::milliseconds(2500));
		try {
			cli_.connect(connOpts_, nullptr, *this);
			nretry_ = 0;
		}
		catch (const mqtt::exception& exc) {
			#ifdef SPDLOG_H
			logger->error("exception: {}", exc.what());
			#else
			std::cout << "[MQTT] exception " << exc.what() << std::endl;
			#endif
			// abort();//TODO remove exits for propper fault exceptions
			reconnect();
		}
	}
	// Re-connection failure
	void on_failure(const mqtt::token& tok) override {
		connected = false;
		if (nretry_ > N_RETRY_ATTEMPTS){
			nretry_ = 0;
			cli_.disconnect();
			#ifdef SPDLOG_H
			auto logger = spdlog::get("MQTT");
			logger->error("Connection attempt failed after {} attemps: {} Reason: {}", N_RETRY_ATTEMPTS, tok.get_reason_code(), tok.get_return_code());
			#else
			std::cout << "[MQTT] Connection attempt failed after " << N_RETRY_ATTEMPTS << " attempts: " << tok.get_reason_code() << " Reason: " << tok.get_return_code() << std::endl;
			#endif
			return;
		}
		#ifdef SPDLOG_H
		auto logger = spdlog::get("MQTT");
		logger->debug("Connection attempt failed: {0}\n\t\\ Reason: {1}, reconnecting...", tok.get_reason_code(), tok.get_return_code());
		#else
		std::cout << "[MQTT] Connection attempt failed: " << tok.get_reason_code() << " Reason: " << tok.get_return_code() << "reconnecting..." << std::endl;
		#endif
		nretry_++;
		reconnect();
    }


	// (Re)connection success
	// Either this or connected() can be used for callbacks.
	void on_success(const mqtt::token& tok) override {
		connected = true;
		nretry_ = 0;
		#ifdef SPDLOG_H
		auto logger = spdlog::get("MQTT");
		logger->info("(Re)Connection attempt success to server: {}", tok.get_connect_response().get_server_uri());
		#else
		std::cout << "[MQTT] (Re)Connection attempt success to server: " << tok.get_connect_response().get_server_uri() << std::endl;
		#endif
	}

	// (Re)connection success

    // Callback for when the connection is lost.
	// This will initiate the attempt to manually reconnect.
	void connection_lost(const std::string& cause) override {
		connected = false;
		#ifdef SPDLOG_H
		auto logger = spdlog::get("MQTT");
		logger->warn("Connection lost, cause: {}", cause.empty() ? "empty" : cause);
		logger->info("Reconnecting...");
		#else
		std::cout << "[MQTT] Connection lost, cause: " << (cause.empty() ? "empty" : cause) << std::endl;
		#endif
		nretry_ = 0;
		reconnect();
	}


	// Callback for when a message arrives.
	void message_arrived(mqtt::const_message_ptr msg) override {
		std::string topic = msg->get_topic();
		dispatch[topic]("mod-temp", msg->to_string());
		#ifdef SPDLOG_H
		auto logger = spdlog::get("MQTT");
		logger->info("Message arrived\n|\ttopic: '{0}'\n\\\tpayload: '{1}'", msg->get_topic(), msg->to_string());
		#else
		std::cout << "[MQTT] Message arrived\n|\ttopic: '" << msg->get_topic() << "'\n\\\tpayload: '" << msg->to_string() << "'" << std::endl;
		#endif 
	}

	void delivery_complete(mqtt::delivery_token_ptr token) override {
		#ifdef SPDLOG_H
		auto logger = spdlog::get("MQTT");
		logger->info("delivery: {}", token.get()->get_message().get()->get_topic());
		#else
		std::cout << "[MQTT] delivery: " << token.get()->get_message().get()->get_topic() << std::endl;
		#endif
	}


    public:
	    callback(mqtt::async_client& cli, mqtt::connect_options& connOpts)
				: nretry_(0), cli_(cli), connOpts_(connOpts), subListener_("Subscription") {}
	bool connected;

	void remove_callback(std::string topic){
		cli_.unsubscribe(topic);
		dispatch.erase(topic);
	}
	
	void add_callback(std::string topic, int qos, std::function<void(std::string, std::string)> func){
       dispatch.emplace(topic, func);
	   cli_.subscribe(topic, qos);
    }

	void add_callback(std::string topic, std::function<void(std::string, std::string)> func){
       dispatch.emplace(topic, func);
    }

	void publish(mqtt::message_ptr message){
		cli_.publish(message);
	}
};