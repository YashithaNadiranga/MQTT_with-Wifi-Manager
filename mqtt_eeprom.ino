#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <PubSubClient.h>

//Variables
int i = 0;
int statusCode;
const char* ssid = "Nodemcu";
const char* passphrase = "text";
String st;
String content;
String val;
String data;


//Function Decalration
bool testWifi(void);
void launchWeb(void);
void setupAP(void);

WiFiClient espClient;
PubSubClient client(espClient);

//Establishing Local server at port 80 whenever required
ESP8266WebServer server(80);

void setup()
{

  Serial.begin(115200); //Initialising if(DEBUG)Serial Monitor
  Serial.println();
  Serial.println("Disconnecting previously connected WiFi");
  WiFi.disconnect();
  EEPROM.begin(512); //Initialasing EEPROM
  delay(10);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(D3, OUTPUT);
  Serial.println();
  Serial.println();
  Serial.println("Startup");

  //---------------------------------------- Read eeprom for ssid and pass
  Serial.println("Reading EEPROM ssid");

  String esid;
  for (int i = 0; i < 32; ++i)
  {
    esid += char(EEPROM.read(i));
  }
  Serial.println();
  Serial.print("SSID: ");
  Serial.println(esid);
  Serial.println("Reading EEPROM pass");

  String epass = "";
  for (int i = 32; i < 96; ++i)
  {
    epass += char(EEPROM.read(i));
  }
  Serial.print("PASS: ");
  Serial.println(epass);


  WiFi.begin(esid.c_str(), epass.c_str());
  
  client.setServer("test.mosquitto.org", 1883);
  client.setCallback(callback);
  if (testWifi())
  {
    Serial.println("Succesfully Connected!!!");
    Serial.println(WiFi.localIP());
    return;
  }
  else
  {
    Serial.println("Turning the HotSpot On");
    launchWeb();
    setupAP();// Setup HotSpot
  }

  Serial.println();
  Serial.println("Waiting.");
  int c = 0;
  while ((WiFi.status() != WL_CONNECTED))
  {
    Serial.print(".");
    delay(100);
    server.handleClient();
  }


}
void loop() {
  //  if ((WiFi.status() == WL_CONNECTED))
  //  {
  //    for (int i = 0; i < 10; i++)
  //    {
  //      digitalWrite(LED_BUILTIN, HIGH);
  //      delay(1000);
  //      digitalWrite(LED_BUILTIN, LOW);
  //      delay(1000);
  //    }
  //
  //  }
  //  else
  //  {
  //    Serial.println("Wifi Not Connected");
  //  }

  if (!client.connected()) {
    reconnectmqttserver();
    }
    client.loop();

  if (Serial.available()) {
    val = Serial.readString();
          Serial.println(val);
          val.trim();

  }

  if (val == "kill") {
    for (int i = 0; i < 96; ++i) {
          EEPROM.write(i, 0);
          Serial.println(i);
    }
    EEPROM.commit();
    ESP.reset();
       
  } else if(val=="OFF"){
    digitalWrite(LED_BUILTIN, HIGH);
  }
  delay(100);

}

void reconnectmqttserver() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
     if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      client.subscribe("LED26574");
      client.subscribe("RADIO26574");
     } else {
       Serial.print("failed, rc=");
       Serial.print(client.state());
       Serial.println(" try again in 5 seconds");
       delay(5000);
     }
   }
}

char msgmqtt[50];
void callback(char* topic, byte* payload, unsigned int length) {
  String MQTT_DATA = "";
  for (int i=0;i<length;i++) {
   MQTT_DATA += (char)payload[i];
  }
  Serial.print(topic);
  Serial.print(" -  ");
  Serial.println(MQTT_DATA);

  if(MQTT_DATA=="1"){
    digitalWrite(D3, HIGH);
    client.publish("ResponceLED", "LEDON");
  }else if(MQTT_DATA=="0"){
    digitalWrite(D3, LOW);
    client.publish("ResponceLED", "LEDOFF");
  }
  
  if(MQTT_DATA=="ON"){
    digitalWrite(D3, HIGH);
    client.publish("ResponceLED", "LEDON");
    
  }else if(MQTT_DATA=="OFF"){
    digitalWrite(D3, LOW);
    client.publish("ResponceLED", "LEDOFF");
  }
}


//----------------------------------------------- Fuctions used for WiFi credentials saving and connecting to it which you do not need to change
bool testWifi(void)
{
  int c = 0;
  Serial.println("Waiting for Wifi to connect");
  while ( c < 20 ) {
    if (WiFi.status() == WL_CONNECTED)
    {
      return true;
    }
    delay(500);
    Serial.print("*");
    c++;
  }
  Serial.println("");
  Serial.println("Connect timed out, opening AP");
  return false;
}

void launchWeb()
{
  Serial.println("");
  if (WiFi.status() == WL_CONNECTED)
    Serial.println("WiFi connected");
  Serial.print("Local IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("SoftAP IP: ");
  Serial.println(WiFi.softAPIP());
  createWebServer();
  // Start the server
  server.begin();
  Serial.println("Server started");
}

void setupAP(void)
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.softAP("Node AP", "");
  Serial.println("softap");
  launchWeb();
  Serial.println("Web Launched");
}

void createWebServer()
{
  {
        server.on("/", []() {
    
          IPAddress ip = WiFi.softAPIP();
          String ipStr = String(ip[0]) + '.' + String(ip[1]) + '.' + String(ip[2]) + '.' + String(ip[3]);
          content = "<!DOCTYPE HTML>\r\n<html>Hello from ESP8266 at ";
          content += "<form action=\"/scan\" method=\"POST\"><input type=\"submit\" value=\"scan\"></form>";
          content += ipStr;
          content += "<p>";
          content += st;
          content += "</p><form method='get' action='setting'><label>SSID: </label><input name='ssid' length=32><input name='pass' length=64><input type='submit'></form>";
          content += "</html>";
          server.send(200, "text/html", content);
        });

    server.on("/setting", []() {
      String qsid = server.arg("ssid");
      String qpass = server.arg("pass");
      if (qsid.length() > 0 && qpass.length() > 0) {
        Serial.println("clearing eeprom");
        for (int i = 0; i < 96; ++i) {
          EEPROM.write(i, 0);
        }
        Serial.println(qsid);
        Serial.println("");
        Serial.println(qpass);
        Serial.println("");

        Serial.println("writing eeprom ssid:");
        for (int i = 0; i < qsid.length(); ++i)
        {
          EEPROM.write(i, qsid[i]);
          Serial.print("Wrote: ");
          Serial.println(qsid[i]);
        }
        Serial.println("writing eeprom pass:");
        for (int i = 0; i < qpass.length(); ++i)
        {
          EEPROM.write(32 + i, qpass[i]);
          Serial.print("Wrote: ");
          Serial.println(qpass[i]);
        }
        EEPROM.commit();

        content = "{\"Success\":\"saved to eeprom... reset to boot into new wifi\"}";
        statusCode = 200;
      } else {
        content = "{\"Error\":\"404 not found\"}";
        statusCode = 404;
        Serial.println("Sending 404");
      }
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.send(statusCode, "application/json", content);
      ESP.reset();
    });
  }
}
