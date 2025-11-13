/*
 * This task creates a simple web server to send messages
 * and a new API endpoint to serve MQTT credentials to the browser.
 */
#if HTTP_WEB_SERVER

#pragma once

#include <ESPAsyncWebServer.h>

#include "../core/task.h"
#include "../core/logger.h"
#include "../core/splitflap_task.h"
#include "mqtt_task.h"

class WebServerTask : public Task<WebServerTask> {
    // Grant Task base class permission to run our protected 'run'
    friend class Task<WebServerTask>; 

    public:
        // Constructor
        WebServerTask(SplitflapTask& splitflap_task, MQTTTask& mqtt_task, Logger& logger, const uint8_t task_core);
        
        // Destructor
        virtual ~WebServerTask();

        void setLogger(Logger* logger);

    protected:
        virtual void run();

    private:
        SplitflapTask& splitflap_task_;
        MQTTTask& mqtt_task_;
        Logger& logger_;
        
        // Use a pointer, initialized to null.
        // We will create the object inside run() to avoid boot crashes.
        AsyncWebServer* server_ = nullptr; 

        // Webpage Handlers
        void handleRoot(AsyncWebServerRequest *request);
        void handleSend(AsyncWebServerRequest *request);
        void handleNotFound(AsyncWebServerRequest *request);
        void handleDebugFs(AsyncWebServerRequest *request);
        void listDir(fs::FS &fs, const char *dirname);


        // --- NEW ---
        // API handler to send credentials to the webpage
        void handleMqttCreds(AsyncWebServerRequest *request);
        // --- END NEW ---

        void log(const char* msg);
};

#endif // HTTP_WEB_SERVER