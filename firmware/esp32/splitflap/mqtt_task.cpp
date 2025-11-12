/*
   Copyright 2021 Scott Bezek and the splitflap contributors
   ... (license) ...
*/
#if MQTT
#include <ArduinoOTA.h>

#include "mqtt_task.h"
#include "secrets.h"
#include <esp_task_wdt.h>

// For secure connection to HiveMQ
#include <WiFiClientSecure.h>

using namespace json11;

// Define the availability topic
#define MQTT_AVAILABILITY_TOPIC "home/" DEVICE_INSTANCE_NAME "/availability"

// Constructor
MQTTTask::MQTTTask(SplitflapTask& splitflap_task, DisplayTask& display_task, Logger& logger, const uint8_t task_core) :
        Task<MQTTTask>("MQTT", 8192, 1, task_core), 
        splitflap_task_(splitflap_task),
        display_task_(display_task),
        logger_(logger),
        wifi_client_(), // Creates a WiFiClientSecure object
        mqtt_client_(wifi_client_) { // Passes the secure client to PubSub
    auto callback = [this](char *topic, byte *payload, unsigned int length) { mqttCallback(topic, payload, length); };
    mqtt_client_.setCallback(callback);
}

// --- UPDATED PUBLISH FUNCTION ---
// Now supports the 'retained' flag from the header file
void MQTTTask::publish(const char* topic, const char* payload, bool retained) {
    if (mqtt_client_.connected()) {
        mqtt_client_.publish(topic, payload, retained); // Use the retained flag
    }
}
// --- END UPDATE ---


void MQTTTask::connectWifi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    WiFi.setSleep(WIFI_PS_NONE);

    char buf[256];
    snprintf(buf, sizeof(buf), "Wifi connecting to %s", WIFI_SSID);
    logger_.log(buf); // Use logger instead of display_task

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        logger_.log("Establishing connection to WiFi..");
    }

    snprintf(buf, sizeof(buf), "Connected to network %s", WIFI_SSID);
    logger_.log(buf);

    // --- THIS IS THE FIX FOR THE IP ADDRESS ---
    snprintf(buf, sizeof(buf), "Wifi IP: %s", WiFi.localIP().toString().c_str());
    logger_.log(buf); // Log the IP to serial monitor
    // --- END FIX ---
}

// --- UPDATED CALLBACK FUNCTION ---
// This is the function that is called when a message arrives
void MQTTTask::mqttCallback(char *topic, byte *payload, unsigned int length) {
    char buf[256];
    snprintf(buf, sizeof(buf), "Received mqtt callback for topic %s", topic);
    logger_.log(buf);

    // 1. Prepare the payload string (null-terminate it)
    char payload_str[length + 1];
    memcpy(payload_str, payload, length);
    payload_str[length] = '\0';

    // 2. Show the string on the display
    splitflap_task_.showString(payload_str, length, false, true);

    // 3. Publish this new state as a "retained" message for the dashboard
    char state_topic[32];
    if (strcmp(DEVICE_ID, "A") == 0) {
        strcpy(state_topic, "splitflap/state/A");
    } else {
        strcpy(state_topic, "splitflap/state/B");
    }
    
    logger_.log("Publishing new state to dashboard.");
    publish(state_topic, payload_str, true); // true = retained
}
// --- END UPDATE ---

void MQTTTask::connectMQTT() {
    char buf[400];

    // This tells the client to trust the server.
    wifi_client_.setInsecure();

    mqtt_client_.setServer(MQTT_SERVER, MQTT_PORT);
    logger_.log("Attempting MQTT connection...");
    snprintf(buf, sizeof(buf), "MQTT connecting to %s:%d", MQTT_SERVER, MQTT_PORT);
    logger_.log((buf)); // Use logger

    // --- Logic is unchanged ---
    if (mqtt_client_.connect(DEVICE_INSTANCE_NAME, MQTT_USER, MQTT_PASSWORD, MQTT_AVAILABILITY_TOPIC, 1, true, "offline")) {
        logger_.log("MQTT connected");

        // Determine this device's unique topic
        char subscribe_topic[32];
        if (strcmp(DEVICE_ID, "A") == 0) {
            strcpy(subscribe_topic, "splitflap/device/A");
        } else {
            strcpy(subscribe_topic, "splitflap/device/B");
        }
        
        // Subscribe ONLY to this device's topic
        snprintf(buf, sizeof(buf), "Subscribing to: %s", subscribe_topic);
        logger_.log(buf);
        mqtt_client_.subscribe(subscribe_topic, 1);

        // Publish availability
        mqtt_client_.publish(MQTT_AVAILABILITY_TOPIC, "online", true);

        snprintf(buf, sizeof(buf), "MQTT connected! (%s:%d)", MQTT_SERVER, MQTT_PORT);
        logger_.log((buf)); // Use logger
    } else {
        snprintf(buf, sizeof(buf), "MQTT failed rc=%d will try again in 5 seconds", mqtt_client_.state());
        logger_.log(buf);

        snprintf(buf, sizeof(buf), "MQTT failed rc=%d", mqtt_client_.state());
        logger_.log((buf)); // Use logger
    }
}

// run() function
void MQTTTask::run() {
    // This is the watchdog fix
    esp_task_wdt_delete(NULL); 
    
    char buf[256];
    display_task_.setMessage(0, ""); // This is fine, it will be skipped by the hollowed-out function
    display_task_.setMessage(1, "");
    
    connectWifi();
    connectMQTT();

    ArduinoOTA
        .onStart([this]() {
            if (ArduinoOTA.getCommand() == U_FLASH) {
                logger_.log("Start OTA (flash)");
            } else { // U_SPIFFS
                logger_.log("Start OTA (filesystem)");
            }
        })
        .onEnd([this]() {
            logger_.log("OTA End");
        })
        .onProgress([this](unsigned int progress, unsigned int total) {
            char buf2[256];
            static uint32_t last_progress;
            if (millis() - last_progress > 1000) {
                snprintf(buf2, sizeof(buf2), "OTA Progress: %d%%", (int)(progress * 100 / total));
                logger_.log(buf2);
                last_progress = millis();
            }
        })
        .onError([this](ota_error_t error) {
            char buf2[256];
            snprintf(buf2, sizeof(buf2), "OTA Error: %u", error);
            logger_.log(buf2);
        })
        .setHostname(DEVICE_INSTANCE_NAME)
        .setPassword(OTA_PASSWORD)
        .begin();

    wl_status_t wifi_last_status = WL_DISCONNECTED;
    uint32_t last_availability_publish = 0;
    while(1) {
        long now = millis();
        wl_status_t wifi_new_status = WiFi.status();
        if (wifi_new_status != wifi_last_status) {
            if (wifi_new_status == WL_CONNECTED) {
                snprintf(buf, sizeof(buf), "Wifi IP: %s", WiFi.localIP().toString().c_str());
                logger_.log((buf)); // Use logger
            } else {
                snprintf(buf, sizeof(buf), "Wifi connecting to %s", WIFI_SSID);
                logger_.log((buf)); // Use logger
            }
            wifi_last_status = wifi_new_status;
        }
        if (!mqtt_client_.connected() && (now - mqtt_last_connect_time_) > 5000) {
            logger_.log("Reconnecting MQTT");
            mqtt_last_connect_time_ = now;
            connectMQTT();
        }
        if (mqtt_client_.connected()) {
            if (now > last_availability_publish + 1800000) {
                mqtt_client_.publish(MQTT_AVAILABILITY_TOPIC, "online", true);
                last_availability_publish = now;
            }
        }
        mqtt_client_.loop();
        ArduinoOTA.handle();
        delay(1);
    }
}

#endif