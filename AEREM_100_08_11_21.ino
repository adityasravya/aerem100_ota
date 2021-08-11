/*
   The following program has been developed by the technical team of RRP S4E Innovations Pvt Ltd. Mahape, Navi Mumbai
   The ownership of the code solely rests with RRP S4E Innovations Pvt Ltd.
   Any attempt to recreate copy fully or partially  will be considered a breach.
   Product              : AEREM 100 (Ioniser: Virus Hunter)
   Microcontroller Used : ESP8266
   IOT protocol used    : MQTT (Message Queuing Telemetry Transport)
   Server               : AWS (Amazon Web Services)
   Single click         - AEREM 100 ON / OFF (Indication : RED LED)
   Double click         - Hunter Mode (UV Mode)(Indication : BLUE LED/GREEN LED)
   Multiclick           - More than 5 clicks(To Reset Previously Saved WiFi Credentials) (Indication : Red LED blinks 5 times)
   Long Press           - Between 5 sec to 10 sec (Clear EEPROM)(No customer indication)
   Long Long press      - More than 10 sec (Call WiFi Manager) (with non blocking function)
   Note                 : uv_lamp_op(); function is common for both APP and PUSH BUTTON
   Edited ON            : 11/08/2021
*/

//Include Required Libraries
#include "OneButton.h"                   // It supports detecting events like single, double, multiple clicks and long-time pressing.                    
#include <EEPROM.h>                      // EEPROM Clear/Read/Write/Get/Put. ESP8266 have 512 bytes of internal EEPROM
#include <ESP8266WiFi.h>                 // This library provides ESP8266 specific Wi-Fi routines that we are calling to connect to the network.                 
#include <ESP8266WebServer.h>            // Dead simple web-server. Supports only one simultaneous client, knows how to handle GET and POST.
#include <WiFiManager.h>                 // WiFi Configuration manager with web configuration portal for ESP boards.Library for configuring ESP8266/ESP32 modules WiFi credentials and custom parameters at runtime.                     
#include <ArduinoJson.h>                 // A simple and efficient JSON (JavaScript Object Notation)library for embedded C++.  
#include <DNSServer.h>                   // This library implements a simple DNS server.
#include <FS.h>                          // File System
#include <PubSubClient.h>                // A client library for MQTT messaging.
#include <PubSubClientTools.h>           // Tools for easier usage of PubSubClient.
#include <Thread.h>                      // Simplest kernel for multithreading.                    
#include <ThreadController.h>            // Controls a list of Threads with different timings.ThreadController is an extended class of Thread
#include <Ticker.h>                      // A library for creating Tickers which can call repeating functions. Replaces delay() with non-blocking functions.
#include <WiFiUdp.h>                     // The WiFiUDP class supports sending and receiving multicast packets on STA interface.
#include <ArduinoOTA.h>                  // The OTA programming allows updating/uploading a new program to ESP8266 using Wi-Fi instead of requiring the user to connect the ESP8266 to a computer via USB to perform the update.
#include <ESP8266mDNS.h>                 // This is a simple implementation of multicast DNS query support for an Arduino running on ESP8266 chip.
#include <Espalexa.h>                    // Espalexa allows you to easily control your ESP with the Alexa voice assistant
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <WiFiClientSecure.h>
#include <CertStoreBearSSL.h>
BearSSL::CertStore certStore;
#include <time.h>
char* MQTT_SERVER = "3.13.74.109";       // Sever Static IP address

#define PIN_INPUT 5                      // Connect PUSH BUTTON on GPIO 5(D1) 
#define MAXUVTIME 600000                 // UV Life cycle in minutes(Set the limit) 10000 h = 600000 min.

//Creates a client that can connect to a specified internet IP address and port as defined in client.connect().
WiFiClient espClient;

// Alexa client
Espalexa espalexa;

const String FirmwareVer={"2.0"}; 
#define URL_fw_Version "/adityasravya/ota_automation/main/version.txt"
#define URL_fw_Bin "https://raw.githubusercontent.com/adityasravya/ota_automation/main/AEREM100.bin"
const char* host = "raw.githubusercontent.com";
const int httpsPort = 443;

// DigiCert High Assurance EV Root CA
const char trustRoot[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDxTCCAq2gAwIBAgIQAqxcJmoLQJuPC3nyrkYldzANBgkqhkiG9w0BAQUFADBs
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSswKQYDVQQDEyJEaWdpQ2VydCBIaWdoIEFzc3VyYW5j
ZSBFViBSb290IENBMB4XDTA2MTExMDAwMDAwMFoXDTMxMTExMDAwMDAwMFowbDEL
MAkGA1UEBhMCVVMxFTATBgNVBAoTDERpZ2lDZXJ0IEluYzEZMBcGA1UECxMQd3d3
LmRpZ2ljZXJ0LmNvbTErMCkGA1UEAxMiRGlnaUNlcnQgSGlnaCBBc3N1cmFuY2Ug
RVYgUm9vdCBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMbM5XPm
+9S75S0tMqbf5YE/yc0lSbZxKsPVlDRnogocsF9ppkCxxLeyj9CYpKlBWTrT3JTW
PNt0OKRKzE0lgvdKpVMSOO7zSW1xkX5jtqumX8OkhPhPYlG++MXs2ziS4wblCJEM
xChBVfvLWokVfnHoNb9Ncgk9vjo4UFt3MRuNs8ckRZqnrG0AFFoEt7oT61EKmEFB
Ik5lYYeBQVCmeVyJ3hlKV9Uu5l0cUyx+mM0aBhakaHPQNAQTXKFx01p8VdteZOE3
hzBWBOURtCmAEvF5OYiiAhF8J2a3iLd48soKqDirCmTCv2ZdlYTBoSUeh10aUAsg
EsxBu24LUTi4S8sCAwEAAaNjMGEwDgYDVR0PAQH/BAQDAgGGMA8GA1UdEwEB/wQF
MAMBAf8wHQYDVR0OBBYEFLE+w2kD+L9HAdSYJhoIAu9jZCvDMB8GA1UdIwQYMBaA
FLE+w2kD+L9HAdSYJhoIAu9jZCvDMA0GCSqGSIb3DQEBBQUAA4IBAQAcGgaX3Nec
nzyIZgYIVyHbIUf4KmeqvxgydkAQV8GK83rZEWWONfqe/EW1ntlMMUu4kehDLI6z
eM7b41N5cdblIZQB2lWHmiRk9opmzN6cN82oNLFpmyPInngiK3BD41VHMWEZ71jF
hS9OMPagMRYjyOfiZRYzy78aG6A9+MpeizGLYAiJLQwGXFK3xPkKmNEVX58Svnw2
Yzi9RKR/5CYrCsSXaQ3pjOLAEFe4yHYSkVXySGnYvCoCWw9E1CAx2/S6cCZdkGCe
vEsXCS+0yx5DaMkHJ8HSXPfqIbloEpw8nL+e/IBcm2PN7EeqJSdnoDfzAIJ9VNep
+OkuE6N36B9K
-----END CERTIFICATE-----
)EOF";
X509List cert(trustRoot);


extern const unsigned char caCert[] PROGMEM;
extern const unsigned int caCertLen;


/* Creates a fully configured client instance.
   PubSubClient (server, port, [callback], client, [stream])
   server:the address of the server, port:the port to connect to. client:the network client to use, for example WiFiClient
   callback and stream are optional.
*/
PubSubClient client(MQTT_SERVER, 1883, espClient);
PubSubClientTools mqtt(client);

ThreadController threadControl = ThreadController();  // Thread Controller keeps track of current Threads and run when necessary.
Thread thread = Thread();

Ticker toggle;                              // Ticker named toggle    : for built-in LED
Ticker uv_error;                            // Ticker named uv_error  : to give customer indication when UV expired
Ticker wifi_timer;                          // Ticker named wifi_timer: every 10 sec check WiFi is connected or not
  
//Declare the global variable for I/O Interface
const byte Fan = 4;                         // GPIO 4 (D2/SDA)   of ESP8266 is assigned to BLDC 12V Fan of AEREM 100 named as "Fan"
const byte Red_LED = 14;                    // GPIO 14(D5/HSCLK) of ESP8266 is assigned to Red LED's of AEREM 100 named as "Red_LED"
const byte Blue_LED = 12 ;                  // GPIO 12(D6/HMISO) of ESP8266 is assigned to Blue LED's of AEREM 100 named as "Blue_LED"
const byte uv_lamp = 13 ;                   // GPIO 13(D7/HMOSI) of ESP8266 is assigned to UV Lamp of AEREM 100 named as "uv_lamp"

//Global Variables for UV Lamp ON time calculation
unsigned long Cumulative_UV_Ontime = 0;     // Variable to store UV lamp ON time so far
unsigned long startTime = 0;                // Start the millis() when UV lamp is ON
unsigned long uv_time_diff_min = 0;         // Variable to store the time difference between UV ON and OFF in minute
unsigned long uv_time_diff = 0;             // Variable to store the time difference between UV ON and OFF in millisecond
bool UV_error = 0;                          // UV lamp expired flag

//Global Variables for OneButton
int singleclick = 0;                        // Variable to check whether Aerem 100 is ON / OFF
int doubleclick = 0;                        // Variable to check whether Aerem 100 is in Hunter Mode or not
unsigned long pressStartTime;               // Variable to store start time when button is press more than 1 sec

//Global Variables for WiFi Function
bool wifi_mqtt_enabled = 0;                 // Flag to check whether mqtt is connected or not
int device_status = 0, change_status = 0;   // Variables to send data to server.
bool after_connect =0;                      // Flag to perform function after establishing connection with WiFi.
bool never_in_loop =0;                      // Flag to make sure after successful WiFi connection "after_connection()" function to be performed once

// Global Variable for EEPROM data storage
bool wifi_status = 0;                       // This flag is to check whether WiFi has been previously connected or not

// Global Variable for strtok() method
String Mode_control;
String Event_id;

// Global variable to give device status to server
bool Click_From_Button;
bool Click_From_Mobile;
bool Click_From_Alexa;
String OFF;
String ON;
String UV;
String UV_Expired;

OneButton button(PIN_INPUT, true);          // The second parameter activeLOW is true, because external wiring sets the button to LOW when pressed.

//Create Publish and Subscribe topic based on MAC ID of ESP8266
String  MAC_UID     =  WiFi.macAddress();
String  pub_token   = "aerem100/" + MAC_UID + "/D2S/ACK";          // Publish topic to send Device Fuctional Status (ON/OFF/UV)
String  pub_status  = "aerem100/" + MAC_UID + "/D2S/STATUS";       // Publish topic to send Alive message every 15sec to server
String  pub_ota     = "aerem100/" + MAC_UID + "/D2S/OTA";
String  pub_uv_time = "aerem100/" + MAC_UID + "/D2S/UVTIME";       // Publish topic to send so far UV ON time to server
String  sub_token   = "aeremS2D/" + MAC_UID + "/#";                // Subscription topic 

//ID's to differentiate between Mobile and Button click
String Button_Event_id= MAC_UID + "/M";
String Alexa_Event_id = MAC_UID + "/A";
String Mobile_Event_id;

//MQTT client name base on ESP8266 MAC ID and Chip ID
String MQTT_CL_ID = MAC_UID +"_" + String(ESP.getChipId());        // ESP.getChipId() :Returns the ESP8266 chip ID as a 32-bit integer.  

//Global variables for Alexa functionality
String ALEXA_ID  = "Aerem100_"+ MAC_UID;
String ALEXA_ION = "ION_"+ MAC_UID;
String ALEXA_UV  = "UV_"+ MAC_UID;

//Callback functions for Alexa
void ion_alexa(uint8_t brightness);
void uv_alexa(uint8_t brightness);

//Fuction Declaration
void wifi_manager_setup(void);
void wifi_mqtt_setup(void);
void clear_eeprom(void);
void uv_lamp_op(void);
void ion_on_off(void);
void customer_indication();
void ota_aerem100(void);
/*........................................................ NORMAL IONISER OPERATION WITH PUSH BUTTON ..................................................................*/
ICACHE_RAM_ATTR void checkTicks() {                                         // With ICACHE_RAM_ATTR : to put the function on the RAM.
    button.tick();                                                          // Check the button ticks
}

void singleClick() {                                                        // Fires as soon as single click is detected
    ion_on_off();
}

void ion_on_off(void) {
    if (doubleclick == 1) {
        Click_From_Button = 1;
        Click_From_Mobile = 0;
        Click_From_Alexa = 0;
        unsigned long  UV_back_up = EEPROM.get(0, Cumulative_UV_Ontime);        // Get the so far UV ON time value from EEPROM
        unsigned long endTime = millis();                                       // End the millis() which was started in uv_lamp_op function.
        unsigned long uv_time_diff = (endTime - startTime);                     // Calculate difference between UV OFF time and ON time.
        uv_time_diff_min = (uv_time_diff / 1000) / 60;                          // Convert the difference from millisecond to minute

        Serial.print("Current cycle UV ON time = ");
        Serial.println(uv_time_diff_min);                                       // Print Current cycle UV ON time on serial console.

        Cumulative_UV_Ontime = uv_time_diff_min + UV_back_up;                   // Calculate Total UV ON time
        Serial.print("So far UV ON time = ");
        Serial.println(Cumulative_UV_Ontime);                                   // Print total UV ON time on serial console.
        EEPROM.put(0, Cumulative_UV_Ontime);                                    // Store total UV ON time so far in EEPROM
        EEPROM.commit();                                                        // To save changes to EEPROM
        Cumulative_UV_Ontime = 0;                                               // After saving the value in EEPROM make variable value zero
        if ((device_status == 2)&&(wifi_mqtt_enabled == 1)&&(UV_error == 0)){
            unsigned long  UV_back_up = EEPROM.get(0, Cumulative_UV_Ontime);    // Read UV time from EEPROM
            String UV;                                                          // Declare string "UV"
            UV = String(UV_back_up)+ "/"+ Button_Event_id;                      // Convert unsigned int long to string
            mqtt.publish(pub_uv_time, UV);                                      // Send data to server
        }
        doubleclick = 0;                                                        // Make Hunter Mode flag to zero
    }
    if (singleclick == 0) {
        Serial.println("Ioniser ON : Button");
        Click_From_Button = 1;
        Click_From_Mobile = 0;
        Click_From_Alexa = 0;
        digitalWrite(Red_LED, LOW);                                             // Turn ON the Red LED's
        digitalWrite(Blue_LED, HIGH);                                           // Turn OFF the Blue LED's
        digitalWrite(uv_lamp, LOW);                                             // Turn OFF the UV Lamp
        analogWrite(Fan, 200);                                                  // Fan Speed at LOW (using PWM)
        singleclick = 1;                                                        // Set AEREM 100 Mode-1 flag
        device_status = 1;                                                      // Set Device status flag =1 (ON)(to send status to mqtt)                                
    }
    else {
        Serial.println("Ioniser OFF : Button");
        Click_From_Button = 1;
        Click_From_Mobile = 0;
        Click_From_Alexa = 0;
        uv_error.detach();                                                      // Detach ticker so that in spite of UV expired customer can use AEREM in mode1
        digitalWrite(Red_LED, HIGH);                                            // Turn OFF the Red LED's
        digitalWrite(Blue_LED, HIGH);                                           // Turn OFF the Blue LED's
        analogWrite(Fan, LOW);                                                  // Turn OFF the Fan
        digitalWrite(uv_lamp, LOW);                                             // Turn OFF UV LAMP
        singleclick = 0;                                                        // Make AEREM 100 Mode-1 flag to zero
        device_status = 0;                                                      // Set Device status flag =0 (OFF)(to send status to mqtt)
  }
}

void doubleClick() {                                                            // Fires as soon as double click is detected
    uv_lamp_op();
}

void uv_lamp_op(void) {
    Serial.print("MQTT FLAG :");                                                // Send so far calculated and saved UV time to server if mqtt is connected
    Serial.println(wifi_mqtt_enabled);
    if ((wifi_mqtt_enabled == 1)&&(UV_error == 0)&&(Mode_control == "2")){
        unsigned long  UV_back_up = EEPROM.get(0, Cumulative_UV_Ontime);
        String UV;                                                              // Declare string "UV"
        UV = String(UV_back_up)+ "/"+ Mobile_Event_id;                          // Convert unsigned int long to string
        mqtt.publish(pub_uv_time, UV);                                          // Send data to server
    }
    else if((wifi_mqtt_enabled == 1)&&(UV_error == 0)){
        unsigned long  UV_back_up = EEPROM.get(0, Cumulative_UV_Ontime);
        String UV;                                                              // Declare string "UV"
        UV = String(UV_back_up)+ "/"+ Button_Event_id;                          // Convert unsigned int long to string
        mqtt.publish(pub_uv_time, UV);                                          // Send data to server
    }
    if(Mode_control == "2"){
        Click_From_Mobile = 1;
        Click_From_Alexa = 0;
        Click_From_Button = 0;
        Mode_control = "0";
    }
    else{
        Click_From_Button = 1;
        Click_From_Alexa = 0;
        Click_From_Mobile = 0;
    }

    unsigned long uv_back_up = EEPROM.get(0, Cumulative_UV_Ontime);             // Read the value from EEPROM once entered Hunter Mode
    startTime = millis();                                                       // Start the millis()
    Serial.println("doubleClick() detected.");
    Serial.print("So far UV on time :");
    Serial.println(uv_back_up);
    if (uv_back_up < MAXUVTIME) {                                               // Check whether UV has expired or not
        digitalWrite(Red_LED, HIGH);                                            // Turn OFF the Red LED's
        digitalWrite(Blue_LED, LOW);                                            // Turn ON the Blue LED's
        digitalWrite(uv_lamp, HIGH);                                            // Turn ON the UV LAMP
        analogWrite(Fan, 800);                                                  // FAN at High Speed(using PWM)
        doubleclick = 1;                                                        // Set Hunter Mode flag
        singleclick = 1;
        device_status = 2;                                                      // Make Device Status flag= 2 (Hunter Mode ON)
        UV_error = 0;                                                           // UV expired flag to zero
   }
   else{
        Serial.println("UV Timer Error");
        analogWrite(Fan, 0);                                                    // FAN is OFF in case of UV error
        device_status = 2;                                                      // Make Device Status flag= 2 (Hunter Mode ON)
        UV_error = 1;                                                           // If UV LAMP has expired give Indication to Customer
        uv_error.attach(0.5, customer_indication);                              // Every 5 sec call customer indication function
   }
}

void customer_indication() {
    digitalWrite(Red_LED, HIGH);                                                // Turn OFF Red LED
    int state = digitalRead(Blue_LED);                                          // Get the Current State of Blue_LED
    digitalWrite(Blue_LED, !state);                                             // Reverse the state
}

void multiClick() {
    unsigned int buttonclicks;
    Serial.print("multiClick(");
    buttonclicks = button.getNumberClicks();                                    // Get the no of PUSH BUTTON clicks
    Serial.print(button.getNumberClicks());
    Serial.println(") detected.");

    if (buttonclicks > 5) {                                                     // If PUSH BUTTON is click more than 5 times- Reset Saved WiFi Setting
        bool erase_state=0;
        Serial.println("Entered Reset Wifi");
        int EEPROM_addr = sizeof(Cumulative_UV_Ontime) + 2;
        wifi_status = 0;                                                        
        EEPROM.write(EEPROM_addr , wifi_status);                                // After reset make wifi_status flag zero and save that into EEPROM
        EEPROM.commit();                                                        // To save changes to flash   
        //Reset Previously saved WiFi Credentials                   
        WiFiManager wifiManager;                                                // Create WiFi Manager Object
        wifiManager.resetSettings();          
        erase_state = ESP.eraseConfig();                                       
        Serial.print("Reset Status : ");
        Serial.println(erase_state);
        //Indication to give WiFi is Reseted(Red LED Blinking for 5 times
        if(erase_state == 1){
           int i=0;
           while(i<5){                                                          // Blink LED for 5 times
              digitalWrite(Blue_LED,HIGH);                                      // Turn OFF Blue LED
              digitalWrite(Red_LED,LOW);                                        // Turn ON Red LED
              delay(300);                                                       // Provide Delay
              digitalWrite(Red_LED,HIGH);                                       // Turn OFF Red LED
              delay(300);                                                       // Provide Delay
              i++;                                                              // Increment counter
           }
        erase_state=0;
        }
        delay(5000);                                                            // Provide delay before Resetting
        ESP.reset();                                                            // Reset ESP8266
    }
}

void pressStart() {                                                             //Fires as soon as the button is held down for 1 second
    Serial.println("pressStart()");
    pressStartTime = millis() - 1000;
}

void pressStop() {
    unsigned long diff_time=0;
    Serial.print("pressStop(");
    diff_time = millis() - pressStartTime;                                      // Calculate the button press time
    Serial.print(diff_time);
    Serial.println(") detected.");
    if ((diff_time > 5000) && (diff_time < 10000))                              // If button press time is between 5 sec to 10 sec ; clear EEPROM
        clear_eeprom();

    if (diff_time > 10000)                                                      // If button press time is more 10 sec ; call Wifi manager function
        wifi_manager_setup();
}

void clear_eeprom(void) {
    unsigned long uv_back_up = EEPROM.get(0, Cumulative_UV_Ontime );
    if (uv_back_up >= MAXUVTIME) {
        for (int i = 0 ; i < EEPROM.length() ; i++){
            EEPROM.write(i, 0);                                                //Reset EEPROM
        }
        EEPROM.commit();                                                       // Save data in EEPROM
        Serial.println("EEPROM RESET");
        UV_error = 0;                                                          // Once EEPROM is cleared make UV error flag to zero
        uv_error.detach();                                                     // Once EEPROM is cleared stop Customer Indication
    }
    else {
        Serial.print("Time left = ");
        Serial.println(MAXUVTIME - uv_back_up);                                // Print remaining UV life cycle in minutes
    }
}
/*...............................................................WiFI IMPLEMENTATION............................................................................*/
//To toggle built-In LED
void tick() {
    int state = digitalRead(BUILTIN_LED);                                     // Get the Current State of GPIO 1(Built In LED)
    digitalWrite(BUILTIN_LED, !state);                                        // Reverse the state
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
    Serial.println("Entered config mode");
    Serial.println(WiFi.softAPIP());                                          // MAC address of ESP8266’s soft-AP.
    Serial.println(myWiFiManager->getConfigPortalSSID());                     // If you used auto generated SSID, print it
    toggle.attach(0.2, tick);                                                 // To indicate we entered Config Mode; blink LED faster
}

//Global intialization.Object of WiFiManager Library
WiFiManager wifiManager;
void wifi_manager_setup(void) {
    toggle.attach(0.6, tick);                                                 // Every 0.6 sec toggle built in LED
    WiFi.mode(WIFI_STA);                                                      // ESP8266 as a WiFi Station (STA) -- For OTA
    wifiManager.setConfigPortalBlocking(false);                               // Allow Normal operation of AEREM when it is in WiFi configuration Mode
    
    //Sets the function to be called when we enter Access Point for configuration.
    wifiManager.setAPCallback(configModeCallback);
    wifiManager.setConnectTimeout(20);                                        // how long to try to connect for before continuing
    wifiManager.setConfigPortalTimeout(300);                                  // auto close configportal after n seconds

    //Create AP Name on the network based on MAC address
    int str_len = MAC_UID.length() + 1;
    char char_array[str_len];
    MAC_UID.toCharArray(char_array, str_len);

    //To connect ESP8266 to the access point at runtime by the web interface without hard-coded SSID and Password
    if (!wifiManager.autoConnect(char_array, "12345678")) {                   // Tries to connect to the last connected WiFi network.
        Serial.println("Failed to Connect & hit timeout");
    }
    /* WiFi.setAutoReconnect(autoReconnect).
       If parameter autoReconnect is set to true,then module will try to reestablish lost connection to the AP.
       If set to false then module will stay disconnected.
     */
     WiFi.setAutoReconnect(true);
}


void  after_connect_wifi(void){
      // After successful WiFi connection save the status in EEPROM through wifi_status flag
      int EEPROM_addr = sizeof(Cumulative_UV_Ontime) + 2;
      Serial.print("wifi_status flag EEPROM location : ");
      Serial.println(EEPROM_addr);
      wifi_status = 1;                                                       // Make WiFi Status Flag 1
      EEPROM.write(EEPROM_addr , wifi_status);                               // Push it to EEPROM
      EEPROM.commit();                                                       // Save the changes in EEPROM                          
      wifi_status = 0;                                                       // After saving make wifi_status flag zero
      
      //Functions to be called after successful WiFi connection
      ota_aerem100();                                                        // Call OTA function
      mqtt_connection();                                                     // Once Internet is connected to ESP8266 proceed for MQTT
      alexa_setup();                                                         // Alexa set up call
      wifi_timer.attach(10, wificheck);                                      // Ticker to check WiFi connection every 10 sec in background
}

void wificheck() {
    if ((WiFi.status() != WL_CONNECTED)) {                                   // WL_CONNECTED: assigned when connected to a WiFi network;
        Serial.println("Wifi Disconnected!Reconnecting..");
        wifi_manager_setup();                                                // If WiFi is not connected call WiFi manager
    }
    else {
        Serial.println("Wifi signal: ");
        Serial.println(WiFi.RSSI());                                         // Gets the signal strength of the connection to the router
    }
  
    if((wifi_mqtt_enabled == 0)&&((WiFi.status() == WL_CONNECTED))){         // If Internet is there but MQTT connection fails then try for server connectivity                                  
        mqtt_connection();
    }
}
/*............................................................ MQTT IMPLEMENTATION...............................................................................*/
void mqtt_connection(void) {
    Serial.print("Connecting to MQTT ");
    //String to char array
    int str_len =  MQTT_CL_ID.length()+1;
    char char_array[str_len];
    MQTT_CL_ID.toCharArray(char_array,str_len);                              // Unique name for each MQTT client based on ESP8266 Chip ID
    
    // Establishing connection with server
    for (int i = 0; i < 5; i++) {
        if(client.connect(char_array)) {                                     // Connects to a specified IP address and port
            Serial.println("CONNECTED");
            wifi_mqtt_enabled = 1;                                           // On successful connection make MQTT flag 1
            toggle.detach();                                                 // On successful MQTT connection stop built in LED blinking
            digitalWrite(BUILTIN_LED, HIGH);                                 // Turn OFF Built-in LED
            device_subscription();                                           // Function to create subsciption topic for AEREM100
            break;
        }
        else{
            Serial.println("FAILED");
            wifi_mqtt_enabled = 0;                                           // On failure make mqtt flag 0
        }
   }
   
   // Enable Thread
   thread.onRun(publisher);
   thread.setInterval(15000);                                               // Every 15 sec publisher function will be called without disturbing normal operation
   threadControl.add(&thread);
}

void publisher() {
    mqtt.publish(pub_status, "Alive");                                      // Every 15 sec Alive message will be send to server on topic aerem100/" + MAC_UID + "/D2S/STATUS
}

void device_subscription(){
    // String to char array conversion
    int str_len = sub_token.length() + 1;                                   // sub_token = aeremS2D/" + MAC_UID + "/#"
    char char_array[str_len];
    sub_token.toCharArray(char_array, str_len);

    //Subscribes to messages published to the specified topic. boolean subscribe (topic, [qos])
    client.subscribe(char_array);
}

void publish_D2S(String msg) {
    mqtt.publish(pub_token, msg);                                          // To send Device Functional Status to server on topic aerem100/" + MAC_UID + "/D2S/ACK
}

/* This function is called when new messages arrive at the client.
   void callback(const char[] topic, byte* payload, unsigned int length)
   topic const char[] - the topic the message arrived on,
   payload byte[] - the message payload
   length unsigned int - the length of the message payload
*/
void callback(char* topic, byte* payload, unsigned int length) {
    // MQTT Topic / Token
    String topic_S = String(topic);                                       // Convert char array to string
    Serial.print("Topic :");
    Serial.println(topic_S);           
    String received_topic = "aeremS2D/" + MAC_UID + "/S2D";               // Expected format of topic/token for eg : aeremS2D/C4:5B:BE:48:67:46/S2D     
    String received_topic_OTA = "aeremS2D/" + MAC_UID + "/S2D/OTA";       // Expected format of topic/token for eg : aeremS2D/C4:5B:BE:48:67:46/S2D/OTA                          
    //char char_msg[100];
    // MQTT payload 
    char *charPointer = (char *)payload;                                  // Convert byte pointer to char pointer
    char char_msg[100+1]={0};                                          // Declare char array and initialized to zero
    for (int i = 0; i < length; i++) {                                    // Convert char array to string (here payload saved in String called msg)
        char_msg[i] = charPointer[i];
    }
    String msg = String(char_msg);                                        // Received payload in String Format
    Serial.print("Payload is:");                                          // Print payload on server
    Serial.println(msg);

    // strtok() on Payload : Splits payload according to given delimiters.
    int i=0;
    char *piece = strtok(char_msg,"/");                                   // "/" is dilimiter here
    while(piece != NULL){
       String data = String(piece);
       piece = strtok(NULL, "/");
       i++;
       if(i==1){
          Mode_control = data;                                           // Mode Control value can be 0/1/2
          Serial.print("Control is :");
          Serial.println(Mode_control);
       }

       if(i==2){
          Event_id = data;                                               // This is a ID getting from Mobile APP via server                            
          Serial.print("Event id :");
          Serial.println(Event_id);                                   
       }
    }
    Mobile_Event_id = Event_id + "/E";                                   // This is a ID we are returning to Mobile APP via server
                                                           
    // Decision Based on MQTT topic and payload to make device functional
    if (topic_S == received_topic){                                      // First take decision based on Topic/Token
        if (Mode_control == "0") {
            ion_off();                                                   // If message arrived from server is 0 call ion_off(); function
        }
        else if(Mode_control == "1") {
            ion_on();                                                    // If message arrived from server is 1 call ion_on(); function
        }
        else if (Mode_control == "2") {
            uv_lamp_op();                                                // If message arrived from server is 2 call uv_lamp_op(); function
        }
    }   

    if (topic_S == received_topic_OTA){
      // IP address of the esp8266
      // publish the IP address to server.
        Serial.println(WiFi.softAPIP());
    }
}

//Function to give device status update to Mobile Application
void update_status(void) {
    // Give device status if controlling from APP
    if ((change_status != device_status)&&(Click_From_Mobile == 1)){
        change_status = device_status;
        if (device_status == 0){
             OFF = "OFF/" + Mobile_Event_id;
             publish_D2S(OFF);
        }

        if (device_status == 1){
             ON = "ON/" + Mobile_Event_id;
             publish_D2S(ON);
        }

        if((device_status == 2)&& (UV_error == 0)){
             UV = "UV/" + Mobile_Event_id;
             publish_D2S(UV);
        }

         if((device_status == 2)&& (UV_error == 1)){
             UV_Expired = "UV_Expired/" + Mobile_Event_id;
             publish_D2S(UV_Expired);
        }
     }

     // Give device status if controlling from Button
     if ((change_status != device_status)&&(Click_From_Button == 1)){
        change_status = device_status;
        if (device_status == 0){
             OFF = "OFF/" + Button_Event_id;
             publish_D2S(OFF);
        }

        if (device_status == 1){
             ON = "ON/" + Button_Event_id;
             publish_D2S(ON);
        }

        if((device_status == 2)&& (UV_error == 0)){
             UV = "UV/" + Button_Event_id;
             publish_D2S(UV);
        }

         if((device_status == 2)&& (UV_error == 1)){
             UV_Expired = "UV_Expired/" + Button_Event_id;
             publish_D2S(UV_Expired);
        }
     }

     // Give device status if controlling from Button
     if ((change_status != device_status)&&(Click_From_Alexa == 1)){
        change_status = device_status;
        if (device_status == 0){
             OFF = "OFF/" + Alexa_Event_id;
             publish_D2S(OFF);
        }

        if (device_status == 1){
             ON = "ON/" + Alexa_Event_id;
             publish_D2S(ON);
        }

        if((device_status == 2)&& (UV_error == 0)){
             UV = "UV/" + Alexa_Event_id;
             publish_D2S(UV);
        }

         if((device_status == 2)&& (UV_error == 1)){
             UV_Expired = "UV_Expired/" + Alexa_Event_id;
             publish_D2S(UV_Expired);
        }
     }
}

void ion_on() {
  if (singleclick == 0) {
    Serial.println("Ioniser Turned ON : Mobile");
    Click_From_Mobile = 1;
    Click_From_Button = 0;
    Click_From_Alexa = 0;
    digitalWrite(Red_LED, LOW);                                             // Turn ON Red LED's
    digitalWrite(Blue_LED, HIGH);                                           // Turn OFF Blue LED's
    digitalWrite(uv_lamp, LOW);                                             // Turn OFF UV lamp
    analogWrite(Fan, 200);                                                  // Fan speed at low                                   
    singleclick = 1;
    device_status = 1;                                                      // Device ON
  }
}

void ion_off() {
    Serial.println("Ioniser Turned OFF : Mobile");
    if (doubleclick == 1) {                                                 // if doubleclick is equal to one then store the uv lamp time in eeprom
        Click_From_Mobile = 1;
        Click_From_Button = 0;
        Click_From_Alexa = 1;
        unsigned long int UV_back_up = EEPROM.get(0, Cumulative_UV_Ontime); // Get the so far UV ON time value from EEPROM
        unsigned long endTime = millis();                                   // Start the millis()
        unsigned long uv_time_diff = (endTime - startTime);                 // Calculate difference between OFF time and ON time of UV
        uv_time_diff_min = (uv_time_diff / 1000) / 60;                      // Convert the difference from millisecond to minute
        Serial.print("Current cycle UV on time = ");
        Serial.println(uv_time_diff_min);                                   // Print Current cycle UV ON time on serial console.
        Cumulative_UV_Ontime = uv_time_diff_min + UV_back_up;               // Calculate total UV ON time
        Serial.print("So far UV ON time = ");
        Serial.println(Cumulative_UV_Ontime);                               // Print total UV ON time on serial console.
        EEPROM.put(0, Cumulative_UV_Ontime);                                // Store total UV ON time so far in EEPROM
        Cumulative_UV_Ontime = 0;                                           // After saving the value in EEPROM make variable value zero
        if (EEPROM.commit()) {                                              // Check wheather the data is save in EEPROM or not
            Serial.println("EEPROM Successfully Committed");
        }
       else {
            Serial.println("ERROR! EEPROM commit failed");
       }
       if ((device_status == 2)&&(wifi_mqtt_enabled == 1)&& (UV_error == 0)){
          unsigned long  UV_back_up = EEPROM.get(0, Cumulative_UV_Ontime);
          String UV;                                                        // Declare string "UV"
          UV = String(UV_back_up)+ "/"+ Mobile_Event_id;                    // Convert unsigned int long to string
          mqtt.publish(pub_uv_time, UV);                                    // Send data to server
      }
    doubleclick = 0;
  }
    Click_From_Mobile = 1;
    Click_From_Button = 0;
    Click_From_Alexa = 0;
    digitalWrite(Red_LED, HIGH);                                            // Turn OFF Blue LED's
    digitalWrite(Blue_LED, HIGH);                                           // Turn OFF Red LED's
    analogWrite(Fan, LOW);                                                  // Turn OFF Fan
    digitalWrite(uv_lamp, LOW);                                             // Turn OFF UV Lamp
    singleclick = 0;                                                        // Mode-1 flag = 0
    device_status = 0;                                                      // Device Status : OFF
}

/*....................................................................... Over The Air (OTA)..........................................................................*/
void ota_aerem100(void){
    int str_len = MAC_UID.length() + 1; 
    char char_array[str_len];
    MAC_UID.toCharArray(char_array, str_len);

    ArduinoOTA.setHostname(char_array);
    ArduinoOTA.setPassword("RRP@s4e&123");
    ArduinoOTA.onStart([](){                              
    });

    ArduinoOTA.onEnd([](){                               
    });
  
    ArduinoOTA.onError([](ota_error_t error) {
      (void)error;
      ESP.restart();
   });

   /* setup the OTA server */
   ArduinoOTA.begin();
}
//........................................................................ ALEXA IMPLEMENTATION.....................................................................................
void alexa_setup(){
    // Define your devices here. 
    espalexa.addDevice(ALEXA_ION, ion_alexa);                        //simplest definition, default state off
    espalexa.addDevice(ALEXA_UV, uv_alexa);                          //third parameter is beginning state (here fully on)
    espalexa.begin();
    Serial.println("Alex setup initiated");
}

//our callback functions
void ion_alexa(uint8_t brightness) {
    if (brightness) {
      Serial.println("AEREM100 ON:ALEXA");
      ion_on_alexa();
    }
    else  {
      Serial.println("AEREM100 OFF :ALEXA");
      ion_off_alexa();
    }
}

void uv_alexa(uint8_t brightness) {
  if (brightness) {
      Serial.println("UV ON: ALEXA");
      uv_lamp_op_alexa();
  }
  else  {
      Serial.println("AEREM100 OFF: ALEXA");
      ion_off_alexa();
  }
}


void ion_on_alexa() {
  if (singleclick == 0) {
    Serial.println("Ioniser Turned ON : ALEXA");
    Click_From_Alexa = 1;
    Click_From_Button = 0;
    Click_From_Mobile = 0; 
    digitalWrite(Red_LED, LOW);                                             // Turn ON Red LED's
    digitalWrite(Blue_LED, HIGH);                                           // Turn OFF Blue LED's
    digitalWrite(uv_lamp, LOW);                                             // Turn OFF UV lamp
    analogWrite(Fan, 200);                                                  // Fan at LOW speed
    singleclick = 1;
    device_status = 1;                                                      // Device ON
  }
}

void ion_off_alexa() {
    Serial.println("Ioniser Turned OFF : ALEXA");
    Click_From_Alexa = 1;
    Click_From_Button = 0;
    Click_From_Mobile = 0;
    digitalWrite(Red_LED, HIGH);                                            // Turn OFF Blue LED's
    digitalWrite(Blue_LED, HIGH);                                           // Turn OFF Red LED's
    analogWrite(Fan, LOW);                                                  // Turn OFF Fan
    digitalWrite(uv_lamp, LOW);                                             // Turn OFF UV Lamp
    singleclick = 0;                                                        // Mode-1 flag = 0
    device_status = 0;                                                      // Device Status : OFF
    
    if (doubleclick == 1) {                                                 // if doubleclick is equal to one then store the uv lamp time in eeprom
        Click_From_Alexa = 1;
        Click_From_Button = 0;
        Click_From_Button = 0;
        unsigned long int UV_back_up = EEPROM.get(0, Cumulative_UV_Ontime);     // Get the so far UV ON time value from EEPROM
        unsigned long endTime = millis();                                       // Start the millis()
        unsigned long uv_time_diff = (endTime - startTime);                     // Calculate difference between OFF time and ON time of UV
        uv_time_diff_min = (uv_time_diff / 1000) / 60;                          // Convert the difference from millisecond to minute

        Serial.print("Current cycle UV on time = ");
        Serial.println(uv_time_diff_min);                                       // Print Current cycle UV ON time on serial console.

        Cumulative_UV_Ontime = uv_time_diff_min + UV_back_up;                   // Calculate total UV ON time
        Serial.print("So far UV ON time = ");
        Serial.println(Cumulative_UV_Ontime);                                   // Print total UV ON time on serial console.
        EEPROM.put(0, Cumulative_UV_Ontime);                                    // Store total UV ON time so far in EEPROM
        Cumulative_UV_Ontime = 0;                                               // After saving the value in EEPROM make variable value zero
        if (EEPROM.commit()) {                                                  // Check wheather the data is save in EEPROM or not
            Serial.println("EEPROM Successfully Committed");
        }
        else {
            Serial.println("ERROR! EEPROM commit failed");
       }

       if ((wifi_mqtt_enabled == 1)&& (UV_error == 0)&& (Click_From_Alexa == 1)){
            unsigned long  UV_back_up = EEPROM.get(0, Cumulative_UV_Ontime);
            String UV;                                                          // Declare string "UV"
            UV = String(UV_back_up)+ "/"+ Alexa_Event_id ;                      // Convert unsigned int long to string
            mqtt.publish(pub_uv_time, UV);                                      // Send data to server
        }
    doubleclick = 0;
  }
}

void uv_lamp_op_alexa(void) {
    Click_From_Alexa = 1;
    Click_From_Button = 0;
    Click_From_Mobile = 0;
    
    unsigned long uv_back_up = EEPROM.get(0, Cumulative_UV_Ontime);         // Read the value from EEPROM once entered Hunter Mode
    startTime = millis();                                                   // Start the millis()
    Serial.println("doubleClick() detected. : ALEXA");
    Serial.print("So far UV on time :");
    Serial.println(uv_back_up);

    if (uv_back_up < MAXUVTIME) {                                           // Check whether UV has expired or not
        digitalWrite(Blue_LED, LOW);                                        // Turn ON the Blue LED's
        digitalWrite(Red_LED, HIGH);                                        // Turn OFF the Red LED's
        digitalWrite(uv_lamp, HIGH);                                        // Turn ON the UV LAMP
        analogWrite(Fan, 800);                                              // FAN at High Speed(using PWM)
        doubleclick = 1;                                                    // Set Hunter Mode flag
        singleclick = 1;
        device_status = 2;                                                  // Make Device Status flag= 2 (Hunter Mode ON)
        UV_error = 0;                                                       // UV expired flag to zero
   }
   else{
        Serial.println("UV Timer Error");
        device_status = 2;                                                  // Make Device Status flag= 2 (Hunter Mode ON)
        UV_error = 1;                                                       // If UV LAMP has expired give Indication to Customer
        uv_error.attach(0.5, customer_indication);                          // Every 5 sec call customer indication function
   }

    //Send so far calculated and saved UV time to server if mqtt is connected
    Serial.print("If 1 Publish UV ON time to server: ");
    Serial.println(wifi_mqtt_enabled);
    if ((wifi_mqtt_enabled == 1)&& (UV_error == 0)) {
        String UV;                                                          // Declare string "UV"
        UV = String(uv_back_up)+ "/" +Alexa_Event_id;                            // Convert unsigned int long to string
        mqtt.publish(pub_uv_time, UV);                                      // Send data to server
    }
}
/*.........................................................SET UP & LOOP ..........................................................................................*/
void setup() {
    //Sets the data rate in bits per second (baud) for serial data transmission.
    Serial.begin(115200);
    Serial.print("Chip ID is:");
    Serial.println(MQTT_CL_ID);
    setClock();
    //Configures the specified pin to behave either as an input or an output
    pinMode(BUILTIN_LED, OUTPUT);
    pinMode(Red_LED, OUTPUT);                                                  // Blue LED as Output
    pinMode(Blue_LED, OUTPUT);                                                 // Red LED as Output
    pinMode(Fan, OUTPUT);                                                      // Fan as Output
    pinMode(uv_lamp, OUTPUT);                                                  // UV Lamp as Output

    //Write a HIGH or a LOW value to a digital pin.
    digitalWrite(Red_LED, HIGH);                                               // Turn OFF the Blue LED
    digitalWrite(Blue_LED, HIGH);                                              // Turn OFF the Red LED
    digitalWrite(BUILTIN_LED, HIGH);                                           // Turn OFF the Built-in LED
    digitalWrite(uv_lamp, LOW);                                                // Turn OFF the UV lamp
    //Writes an analog value (PWM wave) to a pin
    analogWrite(Fan, LOW);                                                     // Turn OFF the Fan

    //Set the PWM range - 10 bit timer
    analogWriteRange(1023);

    //Set PWM frequency
    analogWriteFreq(17500);
    
    //Initialize the EEPROM
    EEPROM.begin(512);

    //attachInterrupt(digitalPinToInterrupt(pin), ISR, mode)
    attachInterrupt(digitalPinToInterrupt(PIN_INPUT), checkTicks, CHANGE);      // Attach the interrupt on Push Button
    button.attachClick(singleClick);                                            // Fires as soon as single click is detected
    button.attachDoubleClick(doubleClick);                                      // Fires as soon as double click is detected
    button.attachMultiClick(multiClick);                                        // Fires as soon as multiple clicks have been detected
    button.setPressTicks(1000);
    button.attachLongPressStart(pressStart);                                    // Fires as soon as the button is held down for 1 second
    button.attachLongPressStop(pressStop);                                      // Fires when the button is released after a long hold

    Serial.println(pub_token);                                                  // Print publish topic (MAC_D2S) on console
    Serial.println(sub_token);                                                  // Print subscribe topic (MAC_S2D) on console

    //On every power on check wifi status flag if yes then only call wifi manager
    int EEPROM_addr = sizeof(Cumulative_UV_Ontime) + 2;
    wifi_status = EEPROM.read(EEPROM_addr);
    Serial.print("Wifi-status is :");
    Serial.println(wifi_status);
    if (wifi_status == 1) {
        wifi_manager_setup();
    }
}

void loop() {
    button.tick();
    wifiManager.process();                                       // Function for Non Blocking WiFi configuration mode
    
    if((WiFi.status() == WL_CONNECTED)&& never_in_loop == 0){
        never_in_loop =1;
        after_connect_wifi();
        after_connect = 1;
    }

    // On successful WiFi Connection functions to be called in loop
    if(after_connect ==1){
        ArduinoOTA.handle();
        espalexa.loop();
        client.setCallback(callback);                            // This function is called when new messages arrive at the client.
        client.loop();                                           // This should be called regularly  to allow the client to process incoming messages and maintain its connection to the server.
        threadControl.run();
        update_status();
        repeatedCall(); 
   }
   delay(10);
}


/***************************************OTA***************************************************************************************************************/
void setClock() {
   // Set time via NTP, as required for x.509 validation
  configTime(3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
  }
}

void FirmwareUpdate()
{  
  WiFiClientSecure client;
  client.setTrustAnchors(&cert);
  if (!client.connect(host, httpsPort)) {
    Serial.println("Connection failed");
    return;
  }
  client.print(String("GET ") + URL_fw_Version + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP8266\r\n" +
               "Connection: close\r\n\r\n");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      //Serial.println("Headers received");
      break;
    }
  }
  String payload = client.readStringUntil('\n');

  payload.trim();
  if(payload.equals(FirmwareVer) )
  {   
     Serial.println("Device already on latest firmware version"); 
  }
  else
  {
    Serial.println("New firmware detected");
    ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW); 
    t_httpUpdate_return ret = ESPhttpUpdate.update(client, URL_fw_Bin);
        
    switch (ret) {
      case HTTP_UPDATE_FAILED:
        Serial.printf("HTTP_UPDATE_FAILD Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
        break;

      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("HTTP_UPDATE_NO_UPDATES");
        break;

      case HTTP_UPDATE_OK:
        Serial.println("HTTP_UPDATE_OK");
        break;
    } 
  }
 }  

 unsigned long previousMillis_2 = 0;
unsigned long previousMillis = 0;        // will store last time LED was updated
const long interval = 60000;
const long mini_interval = 1000;
 void repeatedCall(){
    unsigned long currentMillis = millis();
    if ((currentMillis - previousMillis) >= interval) 
    {
      // save the last time you blinked the LED
      previousMillis = currentMillis;
      setClock();
      FirmwareUpdate();
    }

    if ((currentMillis - previousMillis_2) >= mini_interval) {
     if(WiFi.status() == !WL_CONNECTED) 
          wificheck();
   }
 }



  
