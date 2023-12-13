/*
*******************************************************************************
* Copyright (c) 2022 by M5Stack
*                  Equipped with M5Core2 sample source code
*                          配套  M5Core2 示例源代码
* Visit for more information: https://docs.m5stack.com/en/unit/catm_gnss
* 获取更多资料请访问: https://docs.m5stack.com/zh_CN/unit/catm_gnss
*
* Describe: catm_gnss.
* Date: 2022/06/012
*******************************************************************************
This case will use UNIT CATM+GNSS combined with M5Core to implement MQTT
Client. After successfully connecting to MQTT, press button B to realize data
publishing.
Before use, connect the UNIT CATM+GNSS to PORT C(G16/17)
Libraries:
- [TinyGSM](https://github.com/vshymanskyy/TinyGSM)
- [PubSubClient](https://github.com/knolleary/pubsubclient.git)
*/

#define RX2 16
#define TX2 17

#define TINY_GSM_MODEM_SIM7080

// Set serial for debug console (to the Serial Monitor, default speed 115200)
#define SerialMon Serial
#define SerialAT  Serial2

#define TINY_GSM_RX_BUFFER 650

#define TINY_GSM_DEBUG SerialMon

#define MODULE_BAUD 115200

// Your GPRS credentials, if any
const char apn[]      = "iot.1nce.net";
const char gprsUser[] = "";
const char gprsPass[] = "";

#include <TinyGsmClient.h>
#include <PubSubClient.h>

TinyGsm modem(SerialAT);
TinyGsmClient client(modem);


uint32_t lastReconnectAttempt = 0; 


const char* broker = "mqtt.test.com";

const char* topic_up   = "cat1/up";
const char* topic_down = "cat1/down";

PubSubClient mqtt(broker, 1883, client);

unsigned long start;

inline String time() {
    return "..." + String((millis() - start) / 1000) + 's';
}

void log(String info) {
    SerialMon.println(info);
}

void mqttCallback(char* topic, byte* payload, unsigned int len) {
    log("Message arrived :");
    log(topic);
    log("payload: ");
    char _payload[len];
    memcpy(_payload, payload, len);
    _payload[len] = '\0';
    log(_payload);
}

boolean mqttConnect() {
    log("Connecting to ");
    log(broker);

    // Connect to MQTT Broker
    // boolean status = mqtt.connect("2");

    // Or, if you want to authenticate MQTT:
    boolean status = mqtt.connect("esp32+m5stack", gprsUser, gprsPass);

    if (status == false) {
        SerialMon.println(" fail");
        return false;
    }
    SerialMon.println(" success");
    mqtt.publish(topic_up, "GsmClientTest started");
    mqtt.subscribe(topic_down);
    log("Subscribe Topic: " + String(topic_down));
    return mqtt.connected();
}

void setup() {
    start = millis();
    Serial.begin(115200);
    log("Initializing modem..." + time());

    // Set GSM module baud rate
    if (TinyGsmAutoBaud(SerialAT, MODULE_BAUD, MODULE_BAUD) == 0) {
        log("UART connect error" + time());
    }
    //   modem.restart();
    modem.init();
    String modemInfo = modem.getModemInfo();
    log("Modem Info: ");
    log(modemInfo + time());
    while (!modem.getSimStatus()) {
        log("not sim card" + time());
    }
}

void loop() {
    log("Waiting for network...." + time());
    if (!modem.waitForNetwork()) {
        log("fail" + time());
        delay(10000);
        return;
    }
    if (modem.isNetworkConnected()) {
        log("Network connected" + time());
    }
    log("GPRS connect..." + time());
    if (!modem.gprsConnect(apn, gprsUser, gprsPass)) {
        log("fail");
        delay(10000);
        return;
    }
    if (modem.isGprsConnected()) {
        log("GPRS connected");
    }

    String ccid = modem.getSimCCID();
    log("CCID: ");
    log(ccid);

    String imei = modem.getIMEI();
    log("IMEI: " + imei);

    String imsi = modem.getIMSI();
    log("IMSI: " + imsi);

    String cop = modem.getOperator();
    log("Operator: " + cop);

    IPAddress local = modem.localIP();
    log("REMOTE IP: " + local.toString());

    int csq = modem.getSignalQuality();
    log("RSSI:" + String(csq) + time());
    log("IP:" + local.toString() + time());

    // MQTT Broker setup
    mqtt.setCallback(mqttCallback);

    while (true) {
        if (!mqtt.connected()) {
            log("=== MQTT NOT CONNECTED ===");
            // Reconnect every 10 seconds
            uint32_t t = millis();
            if (t - lastReconnectAttempt > 3000L) {
                lastReconnectAttempt = t;
                if (mqttConnect()) {
                    lastReconnectAttempt = 0;
                    log("mqtt.test.com" + time());
                    log("MQTT Connected!" + time());
                    log("Topic: " + String(topic_up));
                }
            }
            delay(100);
        } else {
            mqtt.loop();
            mqtt.publish(topic_up, "Hello From UNIT CATM+GNSS");
            delay(10000);
        }
    }
}