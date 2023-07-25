
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <WiFiManager.h>
#include <Arduino.h>
#ifdef ESP32
#include <AsyncTCP.h>
#include <SPIFFS.h>
#else
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <Hash.h>
#include <FS.h>
#endif
#include <ESPAsyncWebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#ifndef _POT
#define _POT

class MCP_4011_POT
{
  int memorize_last_POT_val, CS, UD;

public:
  MCP_4011_POT(int CS, int UD);
  void _POT_increment_unity_(void);
  void _POT_decrement_unity_(void);
  void _POT_wiper_Set(int, int);
  void _POT_Set(int);
};

#endif
int

    // relay1   D4    4
    // relay2   RX2   16
    // relay3   D15   15
    // relay4   D2    2
    // relay5   TX2   17
    relay2ons = 7,
    relayflip = 0,
    relay1 = 4,
    relay2 = 16,
    relay3 = 15,
    relay4 = 2,
    relay5 = 17;
bool
    flip = true,
    toggle = true;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

const char *host = "esp32";
const char *ssid = "xxx";
const char *password = "xxxx";

// Variables to save date and time
String formattedDate;
String dayStamp;
String timeStamp;

AsyncWebServer servers(80);
WebServer server(81);
const char *relay1On = "relay1on";
const char *relay2On = "relay2on";
const char *relay1Off = "relay1off";
const char *relay3On = "relay3on";
const char *relay3Off = "relay3off";
const char *relay4On = "relay4on";
const char *relay4Off = "relay4off";
const char *relay5On = "relay5on";
const char *relay5Off = "relay5off";
const char *Temp = "temp";
const char *stamp = "timeStamp";
int temps = 0;
// HTML web page to handle 3 input fields (inputString, inputInt, inputFloat)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>CONTROL DATA INTERFACE</title>
  <meta name="viewport" content="width=device-width, initial-scale=1" >
  <meta http-equiv="refresh" content="30">
  <script>
    function submitMessage() {
      alert("Saved value to ESP SPIFFS");
      setTimeout(function(){ document.location.reload(false); }, 500);   
      
    }setInterval(function() { document.location.reload(true);}, 10000 ) ;
  </script></head><body>
  <h1>RELAY TIME SET Version JSY 01</h1>
  <b> TEMPERATURE : </b> %temp% deg celcius 
  <br> </br>
  <b> TIME : </b> %timeStamp% 
  <br> </br>
  <form action="/get" target="hidden-form">
   <b>SUNRISE </b>  (ON HOUR %relay1on%): <input type="number" name="relay1on"  style="width:60px; height:15px;" min ="1" max="24">
   <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>


  <form action="/get" target="hidden-form">
   <b>RELAY 2 </b>  (TOGGLE HOUR %relay2on%): <input type="number" name="relay2on"  style="width:60px; height:15px;" min ="1" max="24">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  
  <form action="/get" target="hidden-form">
    <b> RELAY 3</b>  (ON HOUR %relay3on%): <input type="number" name="relay3on"  style="width:60px; height:15px;" min ="1" max="24">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  
  <form action="/get" target="hidden-form">
    <b> RELAY 4</b> (ON HOUR %relay4on%): <input type="number" name="relay4on"  style="width:60px; height:15px;" min ="1" max="24">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  
  <form action="/get" target="hidden-form">
    <b> RELAY 5</b> (ON HOUR %relay5on%): <input type="number" name="relay5on"  style="width:60px; height:15px;" min ="1" max="24">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br><br>


  <form action="/get" target="hidden-form">
    <b>SUNSET </b>  (OFF HOUR  %relay1off%): <input type="number" name="relay1off"  style="width:60px; height:15px;" min ="1" max="24">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  
  
  <form action="/get" target="hidden-form">
    <b> RELAY 3</b>  (OFF HOUR  %relay3off%): <input type="number" name="relay3off"  style="width:60px; height:15px;" min ="1" max="24">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  
  <form action="/get" target="hidden-form">
    <b> RELAY 4</b> (OFF HOUR  %relay4off%): <input type="number" name="relay4off"  style="width:60px; height:15px;" min ="1" max="24">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form><br>
  
  <form action="/get" target="hidden-form">
    <b> RELAY 5</b> (OFF HOUR  %relay5off%): <input type="number" name="relay5off" style="width:60px; height:15px;" min ="1" max="24">
    <input type="submit" value="Submit" onclick="submitMessage()">
  </form>
  
  <iframe style="display:none" name="hidden-form"></iframe>
</body></html>)rawliteral";

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

String readFile(fs::FS &fs, const char *path)
{
  // Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if (!file || file.isDirectory())
  {
    Serial.println("- empty file or failed to open file");
    return String();
  }
  // Serial.println("- read from file:");
  String fileContent;
  while (file.available())
  {
    fileContent += String((char)file.read());
  }
  file.close();
  // Serial.println(fileContent);
  return fileContent;
}

void writeFile(fs::FS &fs, const char *path, const char *message)
{
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if (!file)
  {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message))
  {
    Serial.println("- file written");
  }
  else
  {
    Serial.println("- write failed");
  }
  file.close();
}
String processor2(const String &var)
{
  // Serial.println(var);
  return readFile(SPIFFS, "/timeStamp.txt");
}
// Replaces placeholder with stored values
String processor(const String &var)
{
  // Serial.println(var);
  readFile(SPIFFS, "/temp.txt");
  readFile(SPIFFS, "/timeStamp.txt");
  if (var == "relay1on")
  {
    return readFile(SPIFFS, "/relay1on.txt");
  }
  else if (var == "relay2on")
  {
    return readFile(SPIFFS, "/relay2on.txt");
  }
  else if (var == "relay3on")
  {
    return readFile(SPIFFS, "/relay3on.txt");
  }
  else if (var == "relay4on")
  {
    return readFile(SPIFFS, "/relay4on.txt");
  }
  else if (var == "relay5on")
  {
    return readFile(SPIFFS, "/relay5on.txt");
  }
  else if (var == "relay1off")
  {
    return readFile(SPIFFS, "/relay1off.txt");
  }
  else if (var == "relay3off")
  {
    return readFile(SPIFFS, "/relay3off.txt");
  }
  else if (var == "relay4off")
  {
    return readFile(SPIFFS, "/relay4off.txt");
  }
  else if (var == "relay5off")
  {
    return readFile(SPIFFS, "/relay5off.txt");
  }
  else if (var == "temp")
  {
    return readFile(SPIFFS, "/temp.txt");
  }
  else if (var == "timeStamp")
  {
    return readFile(SPIFFS, "/timeStamp.txt");
  }
  return String();
}
String inputMessage = "";
String timeMessage = "";
/*
   Login page
*/

const char *loginIndex =
    "<form name='loginForm'>"
    "<table width='20%' bgcolor='A09F9F' align='center'>"
    "<tr>"
    "<td colspan=2>"
    "<center><font size=4><b>ESP32 Login Page</b></font></center>"
    "<br>"
    "</td>"
    "<br>"
    "<br>"
    "</tr>"
    "<tr>"
    "<td>Username:</td>"
    "<td><input type='text' size=25 name='userid'><br></td>"
    "</tr>"
    "<br>"
    "<br>"
    "<tr>"
    "<td>Password:</td>"
    "<td><input type='Password' size=25 name='pwd'><br></td>"
    "<br>"
    "<br>"
    "</tr>"
    "<tr>"
    "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
    "</tr>"
    "</table>"
    "</form>"
    "<script>"
    "function check(form)"
    "{"
    "if(form.userid.value=='admin' && form.pwd.value=='admin')"
    "{"
    "window.open('/serverIndex')"
    "}"
    "else"
    "{"
    " alert('Error Password or Username')/*displays error message*/"
    "}"
    "}"
    "</script>";

/*
   Server Index Page
*/

const char *serverIndex =
    "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
    "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
    "<input type='file' name='update'>"
    "<input type='submit' value='Update'>"
    "</form>"
    "<div id='prg'>progress: 0%</div>"
    "<script>"
    "$('form').submit(function(e){"
    "e.preventDefault();"
    "var form = $('#upload_form')[0];"
    "var data = new FormData(form);"
    " $.ajax({"
    "url: '/update',"
    "type: 'POST',"
    "data: data,"
    "contentType: false,"
    "processData:false,"
    "xhr: function() {"
    "var xhr = new window.XMLHttpRequest();"
    "xhr.upload.addEventListener('progress', function(evt) {"
    "if (evt.lengthComputable) {"
    "var per = evt.loaded / evt.total;"
    "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
    "}"
    "}, false);"
    "return xhr;"
    "},"
    "success:function(d, s) {"
    "console.log('success!')"
    "},"
    "error: function (a, b, c) {"
    "}"
    "});"
    "});"
    "</script>";

#define OTA_HOSTNAME ""                        // Leave empty for esp8266-[ChipID]
#define WIFI_MANAGER_STATION_NAME "phil smart" // Leave e mpty for auto generated name ESP + ChipID

void setup_OTA()
{
  /*use mdns for host name resolution*/
  if (!MDNS.begin(host))
  { // http://esp32.local
    Serial.println("Error setting up MDNS responder!");
    while (1)
    {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  /*return index page which is stored in serverIndex */
  server.on("/login", HTTP_GET, []()
            {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", loginIndex); });
  server.on("/serverIndex", HTTP_GET, []()
            {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex); });
  /*handling uploading firmware file */
  server.on(
      "/update", HTTP_POST, []()
      {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart(); },
      []()
      {
        HTTPUpload &upload = server.upload();
        if (upload.status == UPLOAD_FILE_START)
        {
          Serial.printf("Update: %s\n", upload.filename.c_str());
          if (!Update.begin(UPDATE_SIZE_UNKNOWN))
          { // start with max available size
            Update.printError(Serial);
          }
        }
        else if (upload.status == UPLOAD_FILE_WRITE)
        {
          /* flashing firmware to ESP*/
          if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
          {
            Update.printError(Serial);
          }
        }
        else if (upload.status == UPLOAD_FILE_END)
        {
          if (Update.end(true))
          { // true to set the size to the current progress
            Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
          }
          else
          {
            Update.printError(Serial);
          }
        }
      });
  // Send web page with input fields to client
  servers.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
             {
    request->send_P(200, "text/html", index_html, processor);
    request->send_P(200, "text/html", index_html, processor2); });

  // Send a GET request to <ESP_IP>/get?inputString=<inputMessage>
  servers.on("/get", HTTP_GET, [](AsyncWebServerRequest *request)
             {
    String inputMessage;
    // GET inputString value on <ESP_IP>/get?inputString=<inputMessage>
    if (request->hasParam(relay1On)) {
      inputMessage = request->getParam(relay1On)->value();
      writeFile(SPIFFS, "/relay1on.txt", inputMessage.c_str());
    }

    // GET inputInt value on <ESP_IP>/get?inputInt=<inputMessage>
    else if (request->hasParam(relay2On)) {
      inputMessage = request->getParam(relay2On)->value();
      writeFile(SPIFFS, "/relay2on.txt", inputMessage.c_str());
    }
    // GET inputInt value on <ESP_IP>/get?inputInt=<inputMessage>
    else if (request->hasParam(relay3On)) {
      inputMessage = request->getParam(relay3On)->value();
      writeFile(SPIFFS, "/relay3on.txt", inputMessage.c_str());
    }
    // GET inputFloat value on <ESP_IP>/get?inputFloat=<inputMessage>
    else if (request->hasParam(relay4On)) {
      inputMessage = request->getParam(relay4On)->value();
      writeFile(SPIFFS, "/relay4on.txt", inputMessage.c_str());
    }
    else if (request->hasParam(relay5On)) {
      inputMessage = request->getParam(relay5On)->value();
      writeFile(SPIFFS, "/relay5on.txt", inputMessage.c_str());
    }
    // GET inputInt value on <ESP_IP>/get?inputInt=<inputMessage>
    else if (request->hasParam(relay1Off)) {
      inputMessage = request->getParam(relay1Off)->value();
      writeFile(SPIFFS, "/relay1off.txt", inputMessage.c_str());
    }
    // GET inputFloat value on <ESP_IP>/get?inputFloat=<inputMessage>
    else if (request->hasParam(relay3Off)) {
      inputMessage = request->getParam(relay3Off)->value();
      writeFile(SPIFFS, "/relay3off.txt", inputMessage.c_str());
    }
    else if (request->hasParam(relay4Off)) {
      inputMessage = request->getParam(relay4Off)->value();
      writeFile(SPIFFS, "/relay4off.txt", inputMessage.c_str());
    }
    // GET inputInt value on <ESP_IP>/get?inputInt=<inputMessage>
    else if (request->hasParam(relay5Off)) {
      inputMessage = request->getParam(relay5Off)->value();
      writeFile(SPIFFS, "/relay5off.txt", inputMessage.c_str());
    } else if (request->hasParam(Temp)) {
      inputMessage = request->getParam(Temp)->value();
      writeFile(SPIFFS, "/temp.txt", inputMessage.c_str());
    } else if (request->hasParam(stamp)) {
      inputMessage = request->getParam(stamp)->value();
      writeFile(SPIFFS, "/timeStamp.txt", inputMessage.c_str());
    }
    else {
      inputMessage = "No message sent";
    }
    Serial.println(inputMessage);
    request->send(200, "text/text", inputMessage); });
  servers.onNotFound(notFound);

  server.begin();
}

void setup_wifi()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
}

void setup_wifi_manager()
{
  // WiFiManager
  // Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  // reset saved settings
  // wifiManager.resetSettings();

  // set custom ip for portal
  // wifiManager.setAPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  // fetches ssid and pass from eeprom and tries to connect
  // if it does not connect it starts an access point with the specified name
  // and goes into a blocking loop awaiting configuration
  if (WIFI_MANAGER_STATION_NAME == "")
  {
    // use this for auto generated name ESP + ChipID
    wifiManager.autoConnect();
  }
  else
  {
    wifiManager.autoConnect(WIFI_MANAGER_STATION_NAME);
  }
}
#define CS_POT_1 5
#define UD_POT_1 18

// #define CS_POT_2 6
// #define UD_POT_2 7

MCP_4011_POT POT_1(CS_POT_1, UD_POT_1);
// MCP_4011_POT POT_2(CS_POT_2, UD_POT_2);

void _POT_self_test(MCP_4011_POT *obj, int _POT_NUMBER);
void POT_set(MCP_4011_POT *obj, int _POT_NUMBER, int pot_val);
void _POT_self_test(MCP_4011_POT *obj, int _POT_NUMBER)
{
  Serial.print("POT_ Self TEST: MIN TO MAX with unit steps: WITH 3S DELAY to meassure the POTTY value with Multimeter \n POT NUMBER: ");
  Serial.print(_POT_NUMBER);
  Serial.println("");
  for (int i = 0; i < 64; i++)
  {
    Serial.println("\n_POT_Set(");
    Serial.print(i);
    Serial.print(")");
    obj->_POT_Set(i);
    delay(3000);
  }
}
void POT_set(MCP_4011_POT *obj, int _POT_NUMBER, int pot_val)
{
  Serial.print("POT_ Self set \n POT NUMBER: ");
  Serial.print(_POT_NUMBER);
  Serial.println("");
  Serial.println("\n_POT_Set(");
  Serial.print(pot_val);
  Serial.print(")");
  obj->_POT_Set(pot_val);
  // delay(3000);
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Booting");
  // Initialize SPIFFS
  POT_set(&POT_1, 1, 50); // set pot to 50 percent
#ifdef ESP32
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
#else
  if (!SPIFFS.begin())
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
#endif

  setup_wifi_manager();
  setup_OTA();
  servers.begin();
  timeClient.begin();
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(3600);
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);
  pinMode(relay4, OUTPUT);
  pinMode(relay5, OUTPUT);
  relay2ons = readFile(SPIFFS, "/relay2on.txt").toInt();
}

void loop()
{
  server.handleClient();
  // delay(1);
  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.println("Connection Failed! Rebooting swcthing to manual clock...");
    // delay(5000);
    // ESP.restart();

    while (!timeClient.update())
    {
      timeClient.forceUpdate();
    }
    // The formattedDate comes with the following format:
    // 2018-05-28T16:00:13Z
    // We need to extract date and time
    formattedDate = timeClient.getFormattedDate();
    // Extract date
    int splitT = formattedDate.indexOf("T");
    dayStamp = formattedDate.substring(0, splitT);
    // Extract time
    timeStamp = formattedDate.substring(splitT + 1, formattedDate.length() - 1);
    int hours = timeStamp.substring(0, 2).toInt();
  }

  // To access your stored values on inputString, inputInt, inputFloat
  int relay1ons = readFile(SPIFFS, "/relay1on.txt").toInt();
  relay2ons = readFile(SPIFFS, "/relay2on.txt").toInt();
  int relay3ons = readFile(SPIFFS, "/relay3on.txt").toInt();
  int relay4ons = readFile(SPIFFS, "/relay4on.txt").toInt();
  int relay5ons = readFile(SPIFFS, "/relay5on.txt").toInt();

  int relay1offs = readFile(SPIFFS, "/relay1off.txt").toInt();
  int relay3offs = readFile(SPIFFS, "/relay3off.txt").toInt();
  int relay4offs = readFile(SPIFFS, "/relay4off.txt").toInt();
  int relay5offs = readFile(SPIFFS, "/relay5off.txt").toInt();
  inputMessage = timeStamp;

  writeFile(SPIFFS, "/timeStamp.txt", inputMessage.c_str());
  inputMessage = temps++;
  writeFile(SPIFFS, "/temp.txt", inputMessage.c_str());
  String timing = readFile(SPIFFS, "/timeStamp.txt");
  Serial.print("HOUR saved: ");
  Serial.println(timing);

  Serial.println("v0.1_jsy_01");
  Serial.print("HOUR: ");
  Serial.println(timeStamp);
  Serial.print("hour ");
  Serial.println(hours);
  Serial.println(formattedDate);
  Serial.print("DATE: ");
  Serial.println(dayStamp);
  Serial.print("*** relay1on : ");
  Serial.println(relay1ons);
  Serial.print("*** relay2on : ");
  Serial.println(relay2ons);
  Serial.print("*** relay3on : ");
  Serial.println(relay3ons);
  Serial.print("*** relay4on : ");
  Serial.println(relay4ons);
  Serial.print("*** relay5on : ");
  Serial.println(relay5ons);

  Serial.print("*** relay1off : ");
  Serial.println(relay1offs);
  Serial.print("*** relay3off : ");
  Serial.println(relay3offs);
  Serial.print("*** relay4off : ");
  Serial.println(relay4offs);
  Serial.print("*** relay5off : ");
  Serial.println(relay5offs);
  Serial.print("temperature : ");
  Serial.println(inputMessage);
  if (hours == relayflip)
  {
    relayflip = relay2ons + hours;
    if (flip)
    {
      toggle = !toggle;
      digitalWrite(relay2, toggle);
      flip = false;
    }
  }
  else
  {
    flip = true;
  }
  if (hours >= 18 && hours < 18 + relay1ons)
  { // 18 is hardcoded sunset
    digitalWrite(relay1, HIGH);
  }
  else if (hours >= 5 && hours < 5 + relay1offs)
  { // 5 is hardcoded sunrise
    digitalWrite(relay1, HIGH);
  }
  else
  {
    digitalWrite(relay1, LOW);
  }

  // Todo relay 2 temperature based

  if (hours >= relay3ons && hours < relay3offs)
  {
    digitalWrite(relay3, HIGH);
  }
  else
  {
    digitalWrite(relay3, LOW);
  }

  if (hours >= relay4ons && hours < relay4offs)
  {
    digitalWrite(relay4, HIGH);
  }
  else
  {
    digitalWrite(relay4, LOW);
  }

  if (hours >= relay5ons && hours < relay5offs)
  {
    digitalWrite(relay5, HIGH);
  }
  else
  {
    digitalWrite(relay5, LOW);
  }

  delay(5000);
}
