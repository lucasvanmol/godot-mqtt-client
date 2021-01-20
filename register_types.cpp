#include "register_types.h"

#include "core/class_db.h"
#include "mqtt.h"

void register_mqtt_types() {
    ClassDB::register_class<MQTT>();
}

void unregister_mqtt_types() {
    // Nothing to do here in this example.
}