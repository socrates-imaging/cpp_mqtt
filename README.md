# cpp_mqtt
MQTT singleton library to use in other apps.
Based on Paho mqtt library, see [paho.mqtt.cpp](https://github.com/eclipse/paho.mqtt.cpp). This library needs libmosquitto-dev. Install with:
```bash
$ sudo apt-get install libmosquitto-dev
```

Install with:
```
make && sudo make install
```

compile example with
```
$ g++ Example.cpp mqtt_singleton.cpp -o Example -lpaho-mqtt3as -lpaho-mqttpp3 -lspdlog -lfmt
```

**Use with [SPDLOG](https://github.com/gabime)** <br>
Build spdlog:
```bash
$ git clone https://github.com/gabime/spdlog.git
$ cd spdlog && mkdir build && cd build
$ cmake .. && make -j
```
<br>

Then add ```#define MQTT_SPDLOG``` to the top of the main source file, or add ```add_definitions(-DMQTT_SPDLOG)``` To the Cmakelist.txt.
To use the MQTT library create an SPDLOG logger with the name "MQTT":
```cpp
auto MQTT_logger = std::make_shared<spdlog::logger>("MQTT", SINKS);
```

Currently not using secure sockets. Planned in the future, poke Thomas.<br>
<i>ps. Thomas if you read this, update the readme when you've implemented it.</i>


Build Paho libraries with:
```bash
$ sudo apt-get install build-essential gcc make cmake cmake-gui cmake-curses-gui libssl-dev
$ mkdir paho && cd paho
$ git clone https://github.com/eclipse/paho.mqtt.c.git 
$ cd paho.mqtt.c
$ git checkout v1.3.8

$ cmake -Bbuild -H. -DPAHO_ENABLE_TESTING=OFF -DPAHO_BUILD_STATIC=ON \
    -DPAHO_WITH_SSL=ON -DPAHO_HIGH_PERFORMANCE=ON
$ sudo cmake --build build/ --target install
$ sudo ldconfig
$ cd ../

$ git clone https://github.com/eclipse/paho.mqtt.cpp
$ cd paho.mqtt.cpp
$ cmake -Bbuild -H. -DPAHO_BUILD_STATIC=ON \
    -DPAHO_BUILD_DOCUMENTATION=FALSE -DPAHO_BUILD_SAMPLES=FALSE
$ sudo cmake --build build/ --target install
$ sudo ldconfig
```
