#include <Adafruit_CC3000.h>
#include <SPI.h>
#include "utility/debug.h"
#include "config.h"

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER); // you can change this clock speed but DI

#define BAUDRATE 115200

#define OUTPIN 8
#define INPIN 9

Adafruit_CC3000_Client client;

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
  
  pinMode(OUTPIN, OUTPUT);
  pinMode(INPIN, INPUT);
  
  // startup();
  
}

void loop(void) {
  Serial.println(F("LOOP"));
  char buf[300], command[30], mid[60], c;
  int bufsiz=sizeof(buf), stat;
  String res;
  Serial.println(F("Checking Commands"));
  if (pconnect()) { 
    getCommand();
    while(((c = timedRead()) > 0) && (c != '*'));
    if (c == '*') {
      clientRead(buf, bufsiz);
      Serial.println(String(buf));
      if (buf[0] == 's'){
        sscanf(buf, "%s %s", command, mid);
        Serial.println(String(command));
        stat = control(String(command));
        res = String(command) + " " + String(stat, DEC);
        res.toCharArray(command, 30);
        Serial.println(F("Responding now"));
        client.close(); delay(1000);
        while (!pconnect());
        respondCommand(command, String(mid));
        timedRead();
        client.close();
        Serial.println(F("done"));
        
      }
    }
  }
  Serial.println(F("Waiting ..."));
  delay(5000);
}

boolean pconnect(void) {
    uint32_t ip = 0L, t;
  const unsigned long
    connectTimeout  = 15L * 1000L; // Max time to wait for server connection

  
  // Look up server's IP address
  Serial.print(F("\r\nGetting server IP address..."));
  t = millis();
  while((0L == ip) && ((millis() - t) < connectTimeout)) {
    if(cc3000.getHostByName(TWITTER_PROXY, &ip)) break;
    delay(1000);
  }
  if(0L == ip) {
    Serial.println(F("failed"));
    return 0;
  }
  cc3000.printIPdotsRev(ip);
  Serial.println();
  
  // Request JSON-formatted data from server (port 80)
  Serial.print(F("Connecting to twitter proxy server..."));
  client = cc3000.connectTCP(ip, 80);
  if(client.connected()) {
    Serial.print(F("connected.\r\nRequesting data..."));
    return 1;
  } else {
    Serial.println(F("failed"));
    return 0;
  }
}

boolean startup(void) {
  if(pconnect()) {
    char msg[64];
    sprintf(msg, "Volta Coffee Machine, awaiting orders :) <-- %d", millis());
    sendTweet(msg, "");
  }
  
  Serial.print(F("OK\r\nAwaiting response..."));
  char c = 0;
  // Dirty trick: instead of parsing results, just look for opening
  // curly brace indicating the start of a successful JSON response.
  while(((c = timedRead()) > 0) && (c != '{'));
  if(c == '{')   Serial.println(F("success!"));
  else if(c < 0) Serial.println(F("timeout"));
  else           Serial.println(F("error (invalid Twitter credentials?)"));
  client.close();
  return (c == '{');
}

boolean control(String command) {
  boolean status = false;
  if (digitalRead(INPIN) == HIGH) status = true;

  if (!status && command.startsWith("start")) {
     digitalWrite(OUTPIN, HIGH);
     return true;
  }
  if (status && command.startsWith("stop")) {
     digitalWrite(OUTPIN, LOW);
     return true;
  }
  if (command.startsWith("status")) return status;

  return false;
}

void sendTweet(char* msg, String reply_id) {
  client.print(F("GET /twitter/"));
  client.print(TWITTER_KEYID);
  client.print(F("/statuses/update?status="));
  urlEncode(client, msg, false, false);
  if (reply_id != "") {
    client.print(F("in_reply_to_status_id="));
    client.print(reply_id);
  }
  client.print(F(" HTTP/1.0\r\n"));
  client.print(F("HOST: "));
  client.print(TWITTER_PROXY);
  client.print(F("\r\n"));
  client.print(F("Connection: close\r\n\r\n"));
}

void getCommand() {
  client.print(F("GET /arduino/"));
  client.print(TWITTER_KEYID);
  client.print(F("/command"));
  client.print(F(" HTTP/1.0\r\n"));
  client.print(F("HOST: "));
  client.print(TWITTER_PROXY);
  client.print(F("\r\n"));
  client.print(F("Connection: close\r\n\r\n"));
}

void respondCommand(char * msg, String mid) {
  client.print(F("GET /arduino/"));
  client.print(TWITTER_KEYID);
  client.print(F("/respond?command="));
  urlEncode(client, msg, false, false);
  client.print(F("&mid="));
  client.print(mid);
  client.print(F(" HTTP/1.0\r\n"));
  client.print(F("HOST: "));
  client.print(TWITTER_PROXY);
  client.print(F("\r\n"));
  client.print(F("Connection: close\r\n\r\n"));
}

int timedRead(void) {
  unsigned long start = millis();
  while((!client.available()) && ((millis() - start) < 500L));
  return client.read();  // -1 on timeout
}

int clientRead(char* buf, int len) {
  int last_size = 0, startTime = millis();
  while((!client.available()) &&
        ((millis() - startTime) < 500L));
  if(client.available()) {
    last_size = client.read(buf, len-1);
    buf[last_size] = 0;
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

// URL-encoding output function for Print class.
// Input from RAM or PROGMEM (flash).  Double-encoding is a weird special
// case for Oauth (encoded strings get encoded a second time).
static const char PROGMEM hexChar[] = "0123456789ABCDEF";
void urlEncode(
  Print      &p,       // EthernetClient, Sha1, etc.
  const char *src,     // String to be encoded
  boolean     progmem, // If true, string is in PROGMEM (else RAM)
  boolean     x2)      // If true, "double encode" parenthesis
{
  uint8_t c;

  while((c = (progmem ? pgm_read_byte(src) : *src))) {
    if(((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) ||
       ((c >= '0') && (c <= '9')) || strchr_P(PSTR("-_.~"), c)) {
      p.write(c);
    } else {
      if(x2) p.print("%25");
      else   p.write('%');
      p.write(pgm_read_byte(&hexChar[c >> 4]));
      p.write(pgm_read_byte(&hexChar[c & 15]));
    }
    src++;
  }
}

