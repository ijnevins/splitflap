/*
   Copyright 2021 Scott Bezek and the splitflap contributors
   ... (license) ...
*/
#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>

// --- ADD THIS INCLUDE ---
#include <WiFiClientSecure.h>

#include <json11.hpp>

#include "../core/logger.h"
#include "../core/splitflap_task.h"
#include "../core/task.h"

#include "display_task.h"

class MQTTTask : public Task<MQTTTask> {
    friend class Task<MQTTTask>; // Allow base Task to invoke protected run()

    public:
        MQTTTask(SplitflapTask& splitflapTask, DisplayTask& DisplayTask, Logger& logger, const uint8_t taskCore);

        // This is the public function our web server calls
        void publish(const char* topic, const char* payload);

    protected:
        void run();

    private:
        SplitflapTask& splitflap_task_;
        DisplayTask& display_task_;
        Logger& logger_;

        // --- CHANGE THIS LINE ---
        WiFiClientSecure wifi_client_; // Use the SECURE client
        // --- END CHANGE ---

        PubSubClient mqtt_client_;
        int mqtt_last_connect_time_ = 0;

        void connectWifi();
        void connectMQTT();
        void mqttCallback(char *topic, byte *payload, unsigned int length);
};