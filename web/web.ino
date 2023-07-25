
#include <WiFiManager.h>

#define WIFI_MANAGER_STATION_NAME "Phijo smart"

WiFiManager wifiManager;
enum relays
{
    one,
    two,
    three

};
relays relay;

void setup()
{
    wifiManager.setConfigPortalBlocking(false);
    // If wifi is connected again, the device can connect with that network again.
    // and so on and so on.
    wifiManager.autoConnect(WIFI_MANAGER_STATION_NAME);
}

void loop()
{
    //Serial.println((relays)one);
   // relay = one;
    // The portal cannot be blocking because the code in de loop needs to be run,
    // even when it is in ap mode or connected by the wifi network.
    // Run webserver processing, if setConfigPortalBlocking(false)
    wifiManager.process();
    // How do I know in this loop when the device is connected with the wifi or not?
    if (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
        Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        // If wifi is not connected, the ap (access point mode) needs to start up.
        ESP.restart();
    }
}