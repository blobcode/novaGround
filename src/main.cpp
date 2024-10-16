#include "mqtt/async_client.h"
#include <boost/json.hpp>
#include <boost/lockfree/spsc_queue.hpp>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <format>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <thread>

using namespace std;
using namespace std::chrono;

const std::string CLIENT_ID("novaground");

struct sensor_datapoint {
    int id;
    double value;
};

boost::lockfree::spsc_queue<sensor_datapoint> sensor_buffer{8192};

double get_sensor() {
    std::default_random_engine rnd{std::random_device{}()};
    std::uniform_real_distribution<double> dist(0, 100);

    return dist(rnd);
}

// recv
void consumer_func(mqtt::async_client_ptr cli) {
    while (true) {
        auto msg = cli->consume_message();

        if (!msg) {
            continue;
        }

        cout << msg->get_topic() << ": " << msg->to_string() << endl;
    }
}

void publisher_func(mqtt::async_client_ptr cli) {
    while (true) {
        boost::json::array data;
        sensor_datapoint sd;
        while (sensor_buffer.pop(sd)) {
            data.push_back({to_string(sd.id), sd.value});
        }
        boost::json::value payload = {{"data", data}};
        string s_payload = boost::json::serialize(payload);
        cli->publish("novaground/telemetry", s_payload)->wait();
        this_thread::sleep_for(milliseconds(1000));
    }
}

void sample_func() {
    while (true) {
        sensor_datapoint sd;
        sd.id = 0;
        sd.value = get_sensor();
        sensor_buffer.push(sd);

        // sampling frequency
        this_thread::sleep_for(milliseconds(10));
    }
}

int main(int argc, char* argv[]) {
    string address = "mqtt://localhost:1883";

    // Create an MQTT client using a smart pointer to be shared among threads.
    auto cli = std::make_shared<mqtt::async_client>(address, CLIENT_ID);

    // Connect options for a persistent session and automatic reconnects.
    auto connOpts = mqtt::connect_options_builder()
                        .clean_session(false)
                        .automatic_reconnect(seconds(2), seconds(30))
                        .finalize();

    auto TOPICS = mqtt::string_collection::create({"novaground/command"});
    const vector<int> QOS{1};

    cli->start_consuming();

    auto rsp = cli->connect(connOpts)->get_connect_response();
    cout << "connected to mqtt broker\n" << endl;

    // Subscribe if this is a new session with the server
    if (!rsp.is_session_present()) {
        cli->subscribe(TOPICS, QOS);
    }

    std::thread sample(sample_func);
    std::thread consumer(consumer_func, cli);
    std::thread publisher(publisher_func, cli);

    sample.join();
    publisher.join();
    consumer.join();

    cli->disconnect();
    return 0;
}
