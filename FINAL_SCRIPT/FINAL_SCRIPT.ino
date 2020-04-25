#include <ArduinoHttpClient.h>
#include <MKRGSM.h>
#include <ArduinoJson.h>
#include <Arduino_MKRGPS.h>

// PIN Number
const char PINNUMBER[]     = ""; //blank if no pin
// APN data: check settings on mobile for sim provider or contact carrier for network settings
const char GPRS_APN[]      = "telenetwap.be"; //This is for Telenet
const char GPRS_LOGIN[]    = "";
const char GPRS_PASSWORD[] = "";


GSMClient client;
GPRS gprs;
GSM gsmAccess;
GSMLocation location;

char server[] = "indy-bap-backend.herokuapp.com";
char path[] = "/api/locations";
int port = 80;

StaticJsonDocument<200> jsonBuffer;
HttpClient httpClient = HttpClient(client, server, port);
JsonObject root = jsonBuffer.to<JsonObject>();
String response;
int statusCode = 0;
String dataStr;

void setup() {
  // initialize serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println("Starting GSM connection to server!");
  // connection state
  boolean connected = false;

  // After starting the modem with GSM.begin()
  // attach the shield to the GPRS network with the APN, login and password
  while (!connected) {
    if ((gsmAccess.begin(PINNUMBER) == GSM_READY) &&
        (gprs.attachGPRS(GPRS_APN, GPRS_LOGIN, GPRS_PASSWORD) == GPRS_READY)) {
      connected = true;
      location.begin();
    } else {
      Serial.println("Not connected");
      delay(1000);
    }
  }
  Serial.println("connecting...");
  // if you get a connection, report back via serial:
  if (client.connect(server, port)) {
    Serial.println("connected to server");
  } else {
    // if you didn't get a connection to the server:
    Serial.println("connection to server failed");
  }
  if(!GPS.begin()){
    Serial.println("Failed to initialize GPS!");
    while(1);      
  }
}

void loop() {
  unsigned long wait = millis();
  bool gps = false;
  while (millis() - wait < 2000 && !gps) {
    if(GPS.available()){
      Serial.println("GPS method");
      root["longitude"] = GPS.longitude();
      root["latitude"] = GPS.latitude();
      root["altitude"] = GPS.altitude();
      root["speed"] = GPS.speed();
      root["satellites"] = GPS.satellites();
      root["method"] = "GPS";
      root["device"] = "poc";
      gps=true;
    }
  }
  if(!gps){
    Serial.println("GPRS method");
    unsigned long timeout = millis();
    while (millis() - timeout < 2000) {
      if (location.available() && location.accuracy() < 300 && location.accuracy() != 0) {
        root["longitude"] = String(location.longitude(),20);
        root["latitude"] = String(location.latitude(),20);
        root["altitude"] = String(location.altitude(),20);
        root["method"] =  "GPRS";
        root["device"] = "poc";
      }
    }
  }
  //if you get a connection, report back via serial:
  if (client.connect(server, port)) {
    postToServer(root);
  } else {
    // if you didn't get a connection to the server:
    Serial.println("connection failed");
  }
  delay(3000); // Wait for 3 seconds to post again
   // read the status code and body of the response
  statusCode = httpClient.responseStatusCode();
  response = httpClient.responseBody();

  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(response);
 
}

void postToServer(JsonObject data) {
  dataStr = "";
  serializeJson(data, dataStr);
  Serial.println(dataStr);
  httpClient.beginRequest();
  httpClient.post(path);
  httpClient.sendHeader("Content-Type", "application/json");
  httpClient.sendHeader("Content-Length", dataStr.length());
  httpClient.beginBody();
  httpClient.print(dataStr);
  httpClient.endRequest();
}
