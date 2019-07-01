/*
Controlls a litte servo to press blind control buttons.
The manual homeassistant config is:

cover:
  - platform: mqtt
    command_topic: window-shutter-control/<COVER_ID>/command
    optimistic: true
    device_class: shutter
    retain: true
*/


#include <Servo.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define SERVO_PIN               D1
#define MOVEMENT_DELAY          5
#define ZERO_DEGREE             60
#define PRESS_OFFSET_UP         60 //Has to be >= ZERO_DEGREE
#define PRESS_OFFSET_DOWN       60 //Has to be >= ZERO_DEGREE

#define WLAN_SSID               "...wifi name..."
#define WLAN_PASS               "...secret_password..."
#define MQTT_BROKER_HOST        "hassio"
#define MQTT_BROKER_PORT        1883
#define MQTT_BROKER_USER        "...mqtt_user..."
#define MQTT_BROKER_PASS        ".. mqtt_pass..."

// Uncomment the next line if the broker can be offline for some time.
// So if the connection fails the retry gets delayed by MQTT_OFFLINE_SLEEP_TIME
//#define MQTT_CAN_BE_OFFLINE
#ifdef MQTT_CAN_BE_OFFLINE
    //delay time in milliseconds
    #define MQTT_OFFLINE_SLEEP_TIME 1 * 60 * 60 * 1000
#endif

#define COVER_ID                "1234"
#define CONTROLLER_NAME         "window-shutter-control"

Servo servo;
int currentServoPos = ZERO_DEGREE; // variable to store the servo position
int targetServoPos = ZERO_DEGREE;

WiFiClient wifi;
PubSubClient mqtt(MQTT_BROKER_HOST, MQTT_BROKER_PORT, wifi);
const int mqttRetries = 3;
const String mqttPrefix = CONTROLLER_NAME "/" COVER_ID;
const String commandTopic = mqttPrefix + "/command";
const String autoDiscoveryTopic = "homeassistant/cover/" COVER_ID "_" CONTROLLER_NAME "/config" ;
const String autodiscoveryContent = "{\"name\":\"" CONTROLLER_NAME "\",\"cmd_t\":\"" + commandTopic + "\",\"ret\":\"true\",\"opt\":\"true\",\"dev_cla\":\"shutter\"}";



void setup() {
    Serial.begin(115200);
    delay(100);

    connectWifi();
    mqtt.setCallback(mqttSubCallback);
    mqttConnect();
    //mqttHomeAssistantDiscovery();

    Serial.println("Init servo");
    servo.attach(SERVO_PIN);
    servo.write(ZERO_DEGREE);

    Serial.println("Finished init");
}

void loop() {

    // # MQTT
    mqttConnect();
}

// ####################
// Servo stuff
// ####################

void setZeroPosition() {
    Serial.println("Servo: Zero");
    targetServoPos = ZERO_DEGREE;
    syncServoPosition();
}

void setDownPress() {
    Serial.println("Servo: Down");
    targetServoPos = ZERO_DEGREE + PRESS_OFFSET_DOWN;
    syncServoPosition();
}

void setUpPress() {
    Serial.println("Servo: Up");
    targetServoPos = ZERO_DEGREE - PRESS_OFFSET_UP;
    syncServoPosition();
}

void syncServoPosition() {
    if(targetServoPos > currentServoPos) {
        for (; currentServoPos < targetServoPos; currentServoPos += 1) { 
            servo.write(currentServoPos);
            delay(MOVEMENT_DELAY);
        }   

    } else if (targetServoPos < currentServoPos) {
        for (; currentServoPos > targetServoPos; currentServoPos -= 1) { 
            servo.write(currentServoPos);
            delay(MOVEMENT_DELAY);
        }   
    }
}

// ####################
// Wifi + Mqtt
// ####################

void connectWifi() {
    Serial.printf("Connect to '%s'\n", WLAN_SSID);
    WiFi.begin(WLAN_SSID, WLAN_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    WiFi.enableAP(false);
    WiFi.enableSTA(true);
    Serial.println();
    Serial.println("WiFi connected");
    Serial.printf("IP address: '%s', DNS: '%s'\n", WiFi.localIP().toString().c_str(), WiFi.dnsIP().toString().c_str());
}

void mqttConnect() {
    // Stop if already connected.
    if (mqtt.loop()) {        
        return;
    }

    if (mqtt.state() < -1) {
        Serial.println("Try a clean disconnect");
        mqtt.disconnect();
    }

    Serial.printf("Connecting to MQTT host '%s:%d'... \n", MQTT_BROKER_HOST, MQTT_BROKER_PORT);

    int retries = mqttRetries;
    while (!mqtt.connect(CONTROLLER_NAME, MQTT_BROKER_USER, MQTT_BROKER_PASS)) {
        Serial.printf("Error State (https://pubsubclient.knolleary.net/api.html#state): %d\n" , mqtt.state());
        Serial.println("Retrying MQTT connection in 5 seconds...");
        mqtt.disconnect();
        delay(5000);  // wait 5 seconds
        retries--;
        if (retries == 0) {
        #ifdef MQTT_CAN_BE_OFFLINE
            Serial.printf("Sleeping for %d seconds, waiting for the broker.\n", MQTT_OFFLINE_SLEEP_TIME);
            delay(MQTT_OFFLINE_SLEEP_TIME);
            retries = mqttRetries;
        #else
            // basically die and wait for WDT to reset me
            while (1);
        #endif
        }
    }

    mqtt.subscribe(commandTopic.c_str());

    Serial.printf("Subscibed to '%s'\n", commandTopic.c_str());
    Serial.println("MQTT Connected!");
}

void mqttSubCallback(const char topic[], byte* payload, unsigned int length) {
    Serial.printf("'%s': '%s'\n", topic, (char *) payload);

    if (!strcmp(topic, commandTopic.c_str())){

        char* command = (char *) payload;
        if (!strncmp(command, "OPEN", length)) {
            setUpPress();
        } else if (!strncmp(command, "CLOSE", length)) {
            setDownPress();
        } else if (!strncmp(command, "STOP", length)) {
            setZeroPosition();
        }
    }
}


void mqttHomeAssistantDiscovery(){
    const int contentSize = autodiscoveryContent.length() + 1;
    char charContent[contentSize];
    autodiscoveryContent.toCharArray(charContent, contentSize);
    Serial.printf("Autodiscovery topic: '%s'\n", autoDiscoveryTopic.c_str());
    Serial.printf("Size of discovery message '%s': %d\n", charContent, sizeof(charContent));

    if(!mqtt.publish(autoDiscoveryTopic.c_str(), charContent, true)) {
        Serial.println("Autodiscovery failed!");
    }
}
