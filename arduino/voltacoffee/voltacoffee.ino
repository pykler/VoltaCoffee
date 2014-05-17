#include <Adafruit_CC3000.h>
#include <SPI.h>
#include "utility/debug.h"
#include "utility/socket.h"

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER); // you can change this clock speed but DI

// #define WLAN_SSID       "Coffee"        // cannot be longer than 32 characters!
// #define WLAN_PASS       "Coffee123"
//#define WLAN_SSID       "Aria2.4GHz"        // cannot be longer than 32 characters!
//#define WLAN_PASS       "asdfasdfasdf"
#define WLAN_SSID "Volta"
#define WLAN_PASS "VoltaSecure"

// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

#define BAUDRATE 115200

#define LISTEN_PORT           23    // What TCP port to listen on for connections.
#define OUTPIN 8
#define INPIN 9
#define BUFSIZ 128

Adafruit_CC3000_Server echoServer(LISTEN_PORT);


void setup(void)
{
  Serial.begin(BAUDRATE);  
  Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);
  
  /* Initialise the module */
  Serial.println(F("\nInitializing..."));
  if (!cc3000.begin())
  {
    Serial.println(F("Couldn't begin()! Check your wiring?"));
    while(1);
  }
  
  Serial.print(F("\nAttempting to connect to ")); Serial.println(WLAN_SSID);
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }
   
  Serial.println(F("Connected!"));
  
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
  }  

  /* Display the IP address DNS, Gateway, etc. */  
  while (! displayConnectionDetails()) {
    delay(1000);
  }
  
  // Start listening for connections
  echoServer.begin();
  
  Serial.print(F("Listening for connections... on PORT "));
  Serial.println(LISTEN_PORT, DEC);
  pinMode(OUTPIN, OUTPUT);
  pinMode(INPIN, INPUT);
}

void loop(void)
{
  char buffer[BUFSIZ];
  uint32_t flags = 0;
  int16_t last_size;
  String input;
  
  Adafruit_CC3000_ClientRef client = echoServer.available();
  if (client) {
     // Check if there is data available to read.
     if (client.available() > 0) {
       // Read a byte and write it to all clients.
       last_size = client.read(buffer, sizeof(buffer) - 1, flags);
       // adding the null char to buffer
       if (last_size > -1) {
         buffer[last_size] = '\0';
       } else {
         buffer[0] = '\0';
       }      
       input = String(buffer);
       Serial.println(input);
       if (input.startsWith("on")) {
         digitalWrite(OUTPIN, HIGH);
         client.write("on sent\n", 9, flags);
       } else if (input.startsWith("off")) {
         digitalWrite(OUTPIN, LOW);
         client.write("off sent\n", 10, flags);
       } else if (input.startsWith("status")) {
         if (digitalRead(INPIN) == HIGH) {
           client.write("on\n", 3, flags);
         } else {
           client.write("off\n", 4, flags);         
         }
       }
       
     }
  }
}

/**************************************************************************/
/*!
    @brief  Tries to read the IP address and other connection details
*/
/**************************************************************************/
bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}
