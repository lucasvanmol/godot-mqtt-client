#ifndef GODOT_MQTT_H
#define GODOT_MQTT_H

#include "core/reference.h"
#include <MQTTClient.h>


class MQTT : public Reference {
    GDCLASS(MQTT, Reference);


protected:
    MQTTClient client;
    volatile MQTTClient_deliveryToken deliveredtoken;

    static void _bind_methods();

    
public:
    void deliveryComplete(MQTTClient_deliveryToken dt);
    int messageArrived(char *topicName, int topicLen, MQTTClient_message *message);
    void connectionLost(char *cause);

    enum Protocol { MQTTv311, MQTTv31, MQTTv5 };
    enum Transport { TCP, WEBSOCKET };
    enum Error
    { 
        SUCCESS, 
        UNNACCEPTABLE_PROTOCOL_VERSION, 
        ID_REJECTED,
        MQTTREASONCODE_SERVER_UNAVAILABLE,
        BAD_LOGIN,
        NOT_AUTHORIZED 
    };

    int connect(String server_uri, String client_id, int keepalive, int protocol, bool cleansession);
    int disconnect();
    int publish(String topic, String payload, int qos, bool retain);
    int subscribe(String topic, int qos);
    int unsubscribe(String topic);

    MQTT();
    ~MQTT();
};

#endif // GODOT_MQTT_H