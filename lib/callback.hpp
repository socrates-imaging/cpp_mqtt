#include <mqtt/async_client.h>

const int	N_RETRY_ATTEMPTS = 5;

class action_listener : public virtual mqtt::iaction_listener{
    std::string name_;

	void on_failure(const mqtt::token& tok) override {
		std::cout << name_ << " failure";
		if (tok.get_message_id() != 0)
			std::cout << " for token: [" << tok.get_message_id() << "]" << std::endl;
		std::cout << std::endl;
	}

	void on_success(const mqtt::token& tok) override {
		std::cout << name_ << " success";
		if (tok.get_message_id() != 0)
			std::cout << " for token: [" << tok.get_message_id() << "]" << std::endl;
		auto top = tok.get_topics();
		if (top && !top->empty())
			std::cout << "\ttoken topic: '" << (*top)[0] << "', ..." << std::endl;
		std::cout << std::endl;
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
		std::this_thread::sleep_for(std::chrono::milliseconds(2500));
		try {
			cli_.connect(connOpts_, nullptr, *this)->wait();
		}
		catch (const mqtt::exception& exc) {
			std::cerr << "Error: " << exc.what() << std::endl;
			reconnect();//TODO remove exits for propper fault exceptions
		}
	}
	// Re-connection failure
	void on_failure(const mqtt::token& tok) override {
		std::cout << "Connection attempt failed: " << tok.get_reason_code() << std::endl;
		std::cout << "Reason: " << tok.get_return_code() << std::endl;
		if (++nretry_ > N_RETRY_ATTEMPTS)
			abort();
		reconnect();
    }


	// (Re)connection success
	// Either this or connected() can be used for callbacks.
	void on_success(const mqtt::token& tok) override {
		std::cout << "Connection attempt success to server: " << tok.get_connect_response().get_server_uri() << std::endl;
		std::cout << tok.get_message_id() << std::endl;
	}

	// (Re)connection success

    // Callback for when the connection is lost.
	// This will initiate the attempt to manually reconnect.
	void connection_lost(const std::string& cause) override {
		std::cout << "\nConnection lost" << std::endl;
		if (!cause.empty())
			std::cout << "\tcause: " << cause << std::endl;

		std::cout << "Reconnecting..." << std::endl;
		nretry_ = 0;
		reconnect();
	}


	// Callback for when a message arrives.
	void message_arrived(mqtt::const_message_ptr msg) override {
		std::string topic = msg->get_topic();
		dispatch[topic](msg->to_string(), "temp");
		std::cout << "Message arrived" << std::endl;
		std::cout << "\ttopic: '" << msg->get_topic() << "'" << std::endl;
		std::cout << "\tpay load: '" << msg->to_string() << "'\n" << std::endl;
	}

	void delivery_complete(mqtt::delivery_token_ptr token) override {
		std::cout << token.get()->get_message().get()->get_topic() << std::endl;
	}


    public:
	    callback(mqtt::async_client& cli, mqtt::connect_options& connOpts)
				: nretry_(0), cli_(cli), connOpts_(connOpts), subListener_("Subscription") {}
	
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