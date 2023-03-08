cmake_minimum_required (VERSION 3.7)

project (soc_mqtt VERSION 1.0 DESCRIPTION "Socrates MQTT lib")

include(GNUInstallDirs)

find_package(PahoMqttCpp REQUIRED)
link_libraries(paho-mqttpp3 paho-mqtt3as)
# target_include_directories(soc_mqtt PUBLIC .)
add_library(soc_mqtt SHARED
    mqtt_singleton.cpp
)

set_target_properties(soc_mqtt PROPERTIES 
VERSION ${PROJECT_VERSION}
SOVERSION 1
PUBLIC_HEADER "mqtt_singleton.h;callback.hpp"
)

configure_file(soc_mqtt.pc.in soc_mqtt.pc @ONLY)


install(TARGETS soc_mqtt
LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${CMAKE_BINARY_DIR}/soc_mqtt.pc
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig
)

target_compile_features(soc_mqtt PRIVATE cxx_std_17)