#include <WebServer.h>
#include <iostream>
#include <ArduinoJson.h>
#include "LittleFS.h"
#include <WiFi.h>
#include <EEPROM.h>

#define EEPROM_SIZE 512

// ESP-32 built-in LED
const int ledPin = 2;

// Projector 5v -> 3.3v trigger
const int triggerPin = 15; // Projector trigger pin (3.3v in)
bool pinState = false; // State of the trigger pin
bool prevState = false;  // Variable to store the previous state of the trigger pin
bool functionExecuted = false; // Keep track if the screen trigger has been executed already

// Relay Pins
const int relayUp = 13;
const int relayDown = 12;

// Debounce variables
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 4000; // 4 seconds

// WebServer object
WebServer server(80);

volatile bool pendingRestart = false;
unsigned long restartAt = 0;
const unsigned long restartDelayMs = 1200; // let HTTP reply flush

// ----------------------------------------------------------------- RELAY CONTROL --------------------------------------------------------------------------------------------------
// Projector Screen State
bool screenState = false; // false = up, true = down
bool screenLock = false; // false = unlocked, true = locked

void ScreenUp() {
  if (!screenLock) {
    if (screenState) {
      Serial.println("-  Screen Up action triggered.");
      digitalWrite(relayDown, LOW); // turns off down relay
      delay(2000);
      digitalWrite(relayUp, HIGH); // turns on up relay
      delay(10000);
      digitalWrite(relayUp, LOW); // turn off up relay
      screenState = false;
    } else {
      Serial.println("-  Screen Already Up");
    }
  } else {
    Serial.println("-  Screen Locked");
  }
}


void ScreenDown() {
  if (!screenLock) {
    if (!screenState) {
      Serial.println("-  Screen Down action triggered.");
      digitalWrite(relayUp, LOW); // turns off up relay
      delay(2000);
      digitalWrite(relayDown, HIGH); // turns on down relay
      delay(10000);
      digitalWrite(relayDown, LOW); // turns off down relay
      screenState = true;
    } else {
      Serial.println("-  Screen Already Down");
    }
  } else {
    Serial.println("-  Screen Locked");
  }
}

void toggleScreen() {
  if (screenState) {
    ScreenUp();
  } else {
    if (!screenState) {
      ScreenDown();
    }
  }
}

void toggleScreenLock() {
  if (screenLock) {
    screenLock = false;
  } else {
    screenLock = true;
  }
}

void resync() {
  screenState = true;
  ScreenUp();
}

// -------------------------------------------------------------------- SETTING CONTROL --------------------------------------------------------------------------------
void handleSettings() {
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "Body not received");
    return;
  }

  String body = server.arg("plain");
  StaticJsonDocument<256> doc;   // was 200; small bump for safety
  DeserializationError error = deserializeJson(doc, body);
  if (error) {
    server.send(400, "application/json", "{\"message\":\"Invalid JSON\"}");
    return;
  }

  const char* ssid = doc["ssid"] | "";
  const char* password = doc["password"] | "";

  if (strlen(ssid) == 0) {
    server.send(400, "application/json", "{\"message\":\"SSID required\"}");
    return;
  }

  saveCredentials(String(ssid), String(password));

  // Don't run ScreenUp() here (long blocking). We'll resync on next boot.
  server.send(200, "application/json", "{\"message\":\"Settings saved. Rebooting...\"}");

  // schedule a restart after response is sent
  restartAt = millis() + restartDelayMs;
  pendingRestart = true;
}



// --------------------------------------------------------------------- EEPROM -------------------------------------------------------------------------------------------------------
void saveCredentials(const String& ssid, const String& password) {
  EEPROM.begin(EEPROM_SIZE);

  // Save the lengths of SSID and password
  EEPROM.write(0, ssid.length());
  EEPROM.write(1, password.length());

  // Save SSID
  for (int i = 0; i < ssid.length(); ++i) {
    EEPROM.write(2 + i, ssid[i]);
  }

  // Save password
  for (int i = 0; i < password.length(); ++i) {
    EEPROM.write(2 + ssid.length() + i, password[i]);
  }

  EEPROM.commit();
}

void loadCredentials(String& ssid, String& password) {
  EEPROM.begin(EEPROM_SIZE);

  // Get lengths of SSID and password
  int ssidLength = EEPROM.read(0);
  int passLength = EEPROM.read(1);

  // Read SSID
  char ssidArray[ssidLength + 1];
  for (int i = 0; i < ssidLength; ++i) {
    ssidArray[i] = EEPROM.read(2 + i);
  }
  ssidArray[ssidLength] = '\0';
  ssid = String(ssidArray);

  // Read password
  char passArray[passLength + 1];
  for (int i = 0; i < passLength; ++i) {
    passArray[i] = EEPROM.read(2 + ssidLength + i);
  }
  passArray[passLength] = '\0';
  password = String(passArray);
}

// --------------------------------------------------------------------- WEB SERVER -------------------------------------------------------------------------------------------------
// handles '/'
void handleRoot() {
  String html = "<html><head><title>Projector Screen Controls</title>";
  html += "<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css\">";
  html += "<style>*{transition: all 0.6s;}html{height: 100%;}body{font-family: 'Lato', sans-serif;color: #888;background-color: #121212;margin: 0;}#main{display: table;width: 100%;height: 100vh;text-align: center;}.fof{display: table-cell;vertical-align: middle;}.fof h1{font-size: 50px;display: inline-block;padding-right: 12px;}button{--hover-shadows: 16px 16px 33px #1212121f, -16px -16px 33px #1212121f;--accent: #FFFFFF;font-weight: bold;letter-spacing: 0.1em;border: none;border-radius: 1.1em;background-color: #212121;cursor: pointer;color: white;padding: 1em 2em;}button:hover{box-shadow: var(--hover-shadows);}button:active{box-shadow: var(--hover-shadows), var(--accent) 0px 0px 30px 5px;background-color: var(--accent);transform: scale(0.95);}#wifiIcon{position: fixed;bottom: 20px;right: 20px;font-size: 30px;cursor: pointer;}#settingsModal{display: none;position: fixed;z-index: 1;left: 0;top: 0;width: 100%;height: 100%;overflow: auto;background-color: rgba(0,0,0,0.4);}#settingsContent{background-color: #fefefe;margin: 15% auto;padding: 20px;border: 1px solid #888;width: 80%;max-width: 300px;}.close{color: #aaa;float: right;font-size: 28px;font-weight: bold;} .close:hover, .close:focus{color: black;text-decoration: none;cursor: pointer;} input[type='text'],input[type='password']{width: 100%;padding: 12px 20px;margin: 8px 0;display: inline-block;border: 1px solid #ccc;box-sizing: border-box;}</style></head><body><div id=\"main\"><div class=\"fof\"><h1>Projector Screen Controls</h1><br><br><button style=\"margin-right:30px;\" id=\"screenUpBtn\">Screen Up</button><button id=\"screenDownBtn\">Screen Down</button></div></div><i id=\"wifiIcon\" class=\"fa fa-wifi\"></i><div id=\"settingsModal\"><div id=\"settingsContent\"><span class=\"close\">&times;</span><h2>Change Wifi Credentials</h2><label for=\"ssid\">SSID:</label><input type=\"text\" id=\"ssid\" name=\"ssid\"><label for=\"password\">Password:</label><input type=\"password\" id=\"password\" name=\"password\"><button id=\"saveSettingsBtn\">Save & Reboot</button></div></div><script>document.getElementById(\"screenUpBtn\").addEventListener(\"click\", function() {fetch(\"/up\").then(response => {if (!response.ok) {throw new Error(\"Network response was not ok\");}return response.text();}).then(data => {console.log(data);}).catch(error => {console.error('There has been a problem with your fetch operation:', error);});}); document.getElementById(\"screenDownBtn\").addEventListener(\"click\", function() {fetch(\"/down\").then(response => {if (!response.ok) {throw new Error(\"Network response was not ok\");}return response.text();}).then(data => {console.log(data);}).catch(error => {console.error('There has been a problem with your fetch operation:', error);});}); document.getElementById(\"wifiIcon\").addEventListener(\"click\", function() {document.getElementById(\"settingsModal\").style.display = \"block\";}); document.querySelector(\".close\").addEventListener(\"click\", function() {document.getElementById(\"settingsModal\").style.display = \"none\";}); document.getElementById(\"saveSettingsBtn\").addEventListener(\"click\", function() {var ssid = document.getElementById(\"ssid\").value;var password = document.getElementById(\"password\").value;var data = {ssid: ssid, password: password};fetch(\"/settings\", {method: \"POST\",headers: {\"Content-Type\": \"application/json\"},body: JSON.stringify(data)}).then(response => {if (!response.ok) {throw new Error(\"Network response was not ok\");}return response.json();}).then(data => {console.log(data);document.getElementById(\"settingsModal\").style.display = \"none\";}).catch(error => {console.error('There has been a problem with your fetch operation:', error);});});</script></body></html>";
  server.send(200, "text/html", html);
}

// handles /up
void handleScreenUp() {
  ScreenUp();
  server.send(200, "text/plain", "Screen is moving up");
}

// handles /down
void handleScreenDown() {
  ScreenDown();
  server.send(200, "text/plain", "Screen is moving down");
}

// handles '/toggle'
void handleToggleScreen() {
  server.send(200, "text/plain", "Screen toggled");
  toggleScreen();
}

// handles /lock
void handleScreenLock() {
  toggleScreenLock();
  server.send(200, "text/plain", "Screen lock toggled");
}

// Change wifi credentials screen via. ap
void handleWifiSetUp() {
  String html = "<html><head><title>Projector Screen Setup</title>";
  html += "<link rel=\"stylesheet\" href=\"https://cdnjs.cloudflare.com/ajax/libs/font-awesome/4.7.0/css/font-awesome.min.css\">";
  html += "<style>*{transition: all 0.6s;}html{height: 100%;}body{font-family: 'Lato', sans-serif;color: #888;background-color: #121212;margin: 0;}#main{display: table;width: 100%;height: 100vh;text-align: center;}.fof{display: table-cell;vertical-align: middle;}.fof h1{font-size: 50px;display: inline-block;padding-right: 12px;}button{--hover-shadows: 16px 16px 33px #1212121f, -16px -16px 33px #1212121f;--accent: #FFFFFF;font-weight: bold;letter-spacing: 0.1em;border: none;border-radius: 1.1em;background-color: #212121;cursor: pointer;color: white;padding: 1em 2em;}button:hover{box-shadow: var(--hover-shadows);}button:active{box-shadow: var(--hover-shadows), var(--accent) 0px 0px 30px 5px;background-color: var(--accent);transform: scale(0.95);}#settingsModal{display: none;position: fixed;z-index: 1;left: 0;top: 0;width: 100%;height: 100%;overflow: auto;background-color: rgba(0,0,0,0.4);}#settingsContent{background-color: #fefefe;margin: 15% auto;padding: 20px;border: 1px solid #888;width: 80%;max-width: 300px;}.close{color: #aaa;float: right;font-size: 28px;font-weight: bold;} .close:hover, .close:focus{color: black;text-decoration: none;cursor: pointer;} input[type='text'],input[type='password']{width: 100%;padding: 12px 20px;margin: 8px 0;display: inline-block;border: 1px solid #ccc;box-sizing: border-box;}</style></head><body><div id=\"main\"><div class=\"fof\"><h1>Projector Screen Setup</h1><br><br><button id=\"openSettings\">Setup Wi-Fi</button></div></div><div id=\"settingsModal\"><div id=\"settingsContent\"><span class=\"close\">&times;</span><h2>Change Wifi Credentials</h2><label for=\"ssid\">SSID:</label><input type=\"text\" id=\"ssid\" name=\"ssid\"><label for=\"password\">Password:</label><input type=\"password\" id=\"password\" name=\"password\"><button id=\"saveSettingsBtn\">Save & Reboot</button></div></div><script>document.getElementById(\"openSettings\").addEventListener(\"click\", function() {document.getElementById(\"settingsModal\").style.display = \"block\";}); document.querySelector(\".close\").addEventListener(\"click\", function() {document.getElementById(\"settingsModal\").style.display = \"none\";}); document.getElementById(\"saveSettingsBtn\").addEventListener(\"click\", function() {var ssid = document.getElementById(\"ssid\").value;var password = document.getElementById(\"password\").value;var data = {ssid: ssid, password: password};fetch(\"/settings\", {method: \"POST\",headers: {\"Content-Type\": \"application/json\"},body: JSON.stringify(data)}).then(response => {if (!response.ok) {throw new Error(\"Network response was not ok\");}return response.json();}).then(data => {console.log(data);document.getElementById(\"settingsModal\").style.display = \"none\";}).catch(error => {console.error('There has been a problem with your fetch operation:', error);});});</script></body></html>";
  server.send(200, "text/html", html);
}

// ---------------------------------------------------------------------- SETUP & LOOP -----------------------------------------------------------------------------------------------
void setup() {
  // Initialize serial communication at 115200 bits per second:
  Serial.begin(115200);

  // Initialize the relay pins as outputs
  pinMode(relayUp, OUTPUT);
  pinMode(relayDown, OUTPUT);
  digitalWrite(relayUp, LOW);
  digitalWrite(relayDown, LOW);

  // Initialize the Projector trigger pin
  pinMode(triggerPin, INPUT);

  // Initialize built-in LED pin as output
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW); // Turn off the LED initially

 // // // WIFI NETWORK // // //
  String ssid, password;
  loadCredentials(ssid, password); // load credentials from EEPROM
  WiFi.begin(ssid.c_str(), password.c_str()); // Attempt to connect with loaded credentials
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 50) { // Check if wifi is connected 25 times
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) { // Start web server endpoints for wifi connection
    Serial.println("WiFi connected");
    server.on("/", handleRoot);
    server.on("/up", handleScreenUp);
    server.on("/down", handleScreenDown);
    server.on("/toggle", handleToggleScreen);
    server.on("/settings", HTTP_POST, handleSettings);
    server.on("/lock", handleScreenLock);
    server.begin();
    Serial.println("HTTP server started on Wifi network");
  } else { // If wifi could not connect with credentials, start an ap with web server to change the credentials
    Serial.println("Failed to connect to WiFi using credentials, new wifi network setup mode started.");
    IPAddress local_IP(192, 168, 32, 32);
    IPAddress gateway(192, 168, 32, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.mode(WIFI_AP);
    WiFi.softAP("Screen Controller Wifi Setup");
    WiFi.softAPConfig (local_IP, gateway, subnet);
    server.on("/", handleWifiSetUp);
    server.on("/settings", HTTP_POST, handleSettings);
    server.begin();
    Serial.println("HTTP server started on AP network");
  }
}

// WiFi reconnection variables
unsigned long lastReconnectAttempt = 0;
const unsigned long reconnectInterval = 10000; // 10 seconds

void loop() {
  // Check the trigger pin and debounce logic
  int reading = digitalRead(triggerPin); // Read triggerPin
  unsigned long currentTime = millis();

  if (reading != prevState) {
    lastDebounceTime = currentTime; // Reset debounce timer
    prevState = reading; // Update the previous state of the pin
  }

  if ((currentTime - lastDebounceTime) > debounceDelay) {
    // State change persisted for debounce delay duration
    if (reading != pinState) {
      pinState = reading;

      if (pinState == HIGH && !functionExecuted) {
        Serial.println("-  3.3v received and debounced");
        ScreenDown();
        functionExecuted = true; // Set flag to true
      } else if (pinState == LOW && functionExecuted) {
        Serial.println("-  3.3v removed and debounced");
        ScreenUp();
        functionExecuted = false; // Reset flag
      }
    }
  }

  // Handle web server requests
  server.handleClient();

  if (pendingRestart && (millis() >= restartAt)) {
    pendingRestart = false;
    ESP.restart();
  }

  // Non-blocking WiFi reconnection
  if (WiFi.status() != WL_CONNECTED) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt >= reconnectInterval) {
      lastReconnectAttempt = now;
      Serial.println("WiFi disconnected. Attempting to reconnect...");
      WiFi.disconnect(); // Reset connection
      WiFi.reconnect();
    }
  } else {
    // Debugging for successful WiFi connection
    static bool wasDisconnected = true; // Tracks if we were previously disconnected
    if (wasDisconnected) {
      Serial.println("WiFi reconnected successfully!");
      wasDisconnected = false;
    }
  }
}
