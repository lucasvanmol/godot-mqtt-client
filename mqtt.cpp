#include "mqtt.h"


// BUGS:
// Low message limit - prob due to broker.emqx.io
// Freezes at MQTTClient_connect when bad address is used

// TODO:
// use MQTTAsync.h
// use transport (websocket/wss/tcp) - hostname - port instead of uri
// TLS

void MQTT::deliveryComplete(MQTTClient_deliveryToken dt) {
    emit_signal("message_delivered", dt);

    print_verbose(vformat("MQTT: Message with token value %d delivery confirmed", dt));
    deliveredtoken = dt;
}

void deliveryCompleteShim(void *context, MQTTClient_deliveryToken dt) {
    MQTT* ctx = reinterpret_cast<MQTT *>(context);
    ctx->deliveryComplete(dt);
}

int MQTT::messageArrived(char *topicName, int topicLen, MQTTClient_message *message) {
    String topic(topicName);
    String payload(reinterpret_cast<char *>(message->payload));

    emit_signal("message_recieved", topic, payload);

    print_verbose("MQTT: Message arrived");
    print_verbose(vformat("  topic  : %s", topic));
    print_verbose(vformat("  message: %s", payload));

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

int messageArrivedShim(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    MQTT* ctx = reinterpret_cast<MQTT *>(context);
    return ctx->messageArrived(topicName, topicLen, message);
}

void MQTT::connectionLost(char *cause) {
    String causestr(cause);
    emit_signal("connection_lost", causestr);

    print_verbose("MQTT: Connection lost!");
    print_verbose(vformat("    cause:%s", causestr));
}

void connectionLostShim(void *context, char *cause) {
    MQTT* ctx = reinterpret_cast<MQTT *>(context);
    ctx->connectionLost(cause);
}

int MQTT::connect(String server_uri, String client_id, int keepalive, int protocol, bool cleansession) {
    int rc;
    
    // Create client
    print_verbose("MQTT: Creating client...");
    const char* addr = server_uri.utf8().get_data();
    const char* c_id = client_id.utf8().get_data();

    rc = MQTTClient_create(&client, addr, c_id, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    if (rc != MQTTCLIENT_SUCCESS) {
        ERR_PRINT(vformat("Client could not be created, error code %d", rc));
        return rc;
    }
    
    rc = MQTTClient_setCallbacks(client, this, &connectionLostShim, &messageArrivedShim, &deliveryCompleteShim);
    if (rc != MQTTCLIENT_SUCCESS) {
        ERR_PRINT(vformat("Couldn't set callbacks, error code %d", rc));
        return rc;
    }
    
    // Configure options
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;

    conn_opts.keepAliveInterval = keepalive;
    conn_opts.cleansession = (int) cleansession;

    switch (protocol)
    {
    case MQTTv311:
        conn_opts.MQTTVersion = MQTTVERSION_3_1_1;
        break;
    
    case MQTTv31:
        conn_opts.MQTTVersion = MQTTVERSION_3_1;
        break;
    
    case MQTTv5:
        conn_opts.MQTTVersion = MQTTVERSION_3_1;
        break;

    default:
        conn_opts.MQTTVersion = protocol;
        break;
    }

    print_verbose("MQTT: Connecting...");

    rc = MQTTClient_connect(client, &conn_opts);
    if (rc != MQTTCLIENT_SUCCESS) {
        ERR_PRINT(vformat("Couldn't connect, error code %d", rc));
        return rc;
    }

    return rc;
}


int MQTT::publish(String topic, String payload, int qos, bool retain) {
    const char* topic_string = topic.utf8().get_data();
    const char* msg_string = payload.utf8().get_data();
    void *message = reinterpret_cast<void *>(const_cast<char *>(msg_string));

    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;

    pubmsg.payload = message;
    pubmsg.payloadlen = strlen(msg_string);
    pubmsg.qos = qos;
    pubmsg.retained = retain;
    deliveredtoken = 0;

    MQTTClient_publishMessage(client, topic_string, &pubmsg, &token);

    return token;
}


void MQTT::subscribe(String topic, int qos) {
    MQTTClient_subscribe(client, topic.utf8().get_data(), qos);
}

void MQTT::_bind_methods() {
    /*
    VARIANT_ENUM_CAST(MQTT::Protocol);
    VARIANT_ENUM_CAST(MQTT::Transport);
    VARIANT_ENUM_CAST(MQTT::Error);
    */

    // Methods
    ClassDB::bind_method(D_METHOD("connect_to_server", "server_uri", "client_id", "keepalive", "protocol", "cleansession"), &MQTT::connect, DEFVAL(20), DEFVAL(MQTTv311), DEFVAL(true));
    ClassDB::bind_method(D_METHOD("publish", "topic", "payload", "qos", "retain"), &MQTT::publish, DEFVAL(0), DEFVAL(false));
    ClassDB::bind_method(D_METHOD("subscribe", "topic", "qos"), &MQTT::subscribe, DEFVAL(0));

    // Signals
    ADD_SIGNAL(MethodInfo("message_recieved", PropertyInfo(Variant::STRING, "topic"),  PropertyInfo(Variant::STRING, "payload")));
    ADD_SIGNAL(MethodInfo("message_delivered", PropertyInfo(Variant::INT, "delivery_token")));
    ADD_SIGNAL(MethodInfo("connection_lost", PropertyInfo(Variant::STRING, "cause")));
}

MQTT::MQTT() {
}

MQTT::~MQTT() {
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
}