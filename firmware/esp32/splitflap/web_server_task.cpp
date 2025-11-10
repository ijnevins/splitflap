/*
 * This task creates a simple web server to send messages
 * to other split-flap devices via MQTT.
 */
#if HTTP_WEB_SERVER

#include "web_server_task.h"
#include "secrets.h" // For DEVICE_ID
#include <ESPAsyncWebServer.h> 


// --- HTML with two buttons ---
const char* html = R"rawliteral(
<!DOCTYPE HTML><html><head>
<title>Split-Flap Control</title>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<style>
  body { font-family: Arial, sans-serif; background: #f0f0f0; text-align: center; margin-top: 50px; }
  form { background: #fff; padding: 20px; border-radius: 8px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); display: inline-block; }
  input[type="text"] { width: 300px; padding: 12px; font-size: 16px; border-radius: 4px; border: 1px solid #ccc; display: block; margin-bottom: 15px; }
  .buttons { display: flex; justify-content: space-between; }
  input[type="submit"] { padding: 10px 20px; font-size: 16px; border: none; border-radius: 4px; cursor: pointer; }
  .self { background: #007bff; color: white; }
  .self:hover { background: #0056b3; }
  .other { background: #28a745; color: white; }
  .other:hover { background: #1e7e34; }
</style>
</head><body>
<h1>Split-Flap Control</h1>
<form action="/send" method="POST">
  <input type="text" name="message" placeholder="Enter message" autofocus>
  <div class="buttons">
    <input type="submit" name="action" value="self" class="self" title="Send to My Display">
    <input type="submit" name="action" value="other" class="other" title="Send to Other Display">
  </div>
</form>
</body></html>
)rawliteral";


// --- Constructor ---
WebServerTask::WebServerTask(SplitflapTask& splitflap_task, MQTTTask& mqtt_task, Logger& logger, const uint8_t task_core) :
        Task<WebServerTask>("WebServer", 4096, 1, task_core),
        splitflap_task_(splitflap_task),
        mqtt_task_(mqtt_task),
        logger_(logger),
        server_(80) { // <-- Initialize server_ object on port 80
}

// --- Destructor ---
WebServerTask::~WebServerTask() {
    server_.end(); // <-- Use the dot operator
}

void WebServerTask::run() {
    log("Starting Web Server Task...");

    // Define web server routes
    server_.on("/", HTTP_GET, std::bind(&WebServerTask::handleRoot, this, std::placeholders::_1));
    server_.on("/send", HTTP_POST, std::bind(&WebServerTask::handleSend, this, std::placeholders::_1));
    server_.onNotFound(std::bind(&WebServerTask::handleNotFound, this, std::placeholders::_1));

    // Start the server
    server_.begin();
    log("Web server started.");
}

void WebServerTask::handleRoot(AsyncWebServerRequest *request) {
    request->send(200, "text/html", html);
}

// --- handleSend Logic for Direct Messages ---
void WebServerTask::handleSend(AsyncWebServerRequest *request) {
    String text, action, publish_topic;
    char log_buf[256];

    // Get message text
    if (request->hasParam("message", true)) {
        text = request->getParam("message", true)->value();
    } else {
        request->send(400, "text/plain", "Missing 'message' parameter");
        return;
    }

    // Get which button was pressed
    if (request->hasParam("action", true)) {
        action = request->getParam("action", true)->value();
    } else {
        request->send(400, "text/plain", "Missing 'action' parameter");
        return;
    }

    snprintf(log_buf, sizeof(log_buf), "Received POST: '%s', Action: '%s'", text.c_str(), action.c_str());
    log(log_buf);

    // Determine the *target* topic based on the button clicked
    if (strcmp(DEVICE_ID, "A") == 0) {
        // This is Device A
        if (action == "self") {
            publish_topic = "splitflap/device/A"; // Publish to self
        } else { // action == "other"
            publish_topic = "splitflap/device/B"; // Publish to B
        }
    } else {
        // This is Device B
        if (action == "self") {
            publish_topic = "splitflap/device/B"; // Publish to self
        } else { // action == "other"
            publish_topic = "splitflap/device/A"; // Publish to A
        }
    }

    // Publish the message to the determined topic
    snprintf(log_buf, sizeof(log_buf), "Publishing to MQTT topic: %s", publish_topic.c_str());
    log(log_buf);
    
    mqtt_task_.publish(publish_topic.c_str(), text.c_str());

    // Redirect back to the root page
    request->redirect("/");
}
// --- END NEW LOGIC ---

void WebServerTask::handleNotFound(AsyncWebServerRequest *request) {
    request->send(44, "text/plain", "404: Not Found");
}

void WebServerTask::log(const char* msg) {
    logger_.log(msg);
}

#endif // HTTP_WEB_SERVER