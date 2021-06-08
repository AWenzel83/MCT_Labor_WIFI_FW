// MCT_Labor Wifi Firmware
#include <ESP8266WiFi.h>    
#include <WiFiUdp.h>
#include <string.h>
#include <stdint.h>

//char * ssid = "SSID"; // your network SSID (name)
//char * pass = "PASS";  // your network password
String ssid = String("SSID");
String pass = String("PASS");
bool connected=false;

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;
unsigned int localPort = 54545;      // local port to listen for UDP packets
IPAddress broadcast_ip;            // Broadcast IP for UDP Packets

void setup() {
  Serial.begin(115200);
}

int8_t serial_decode(char* string)
{
  String address = String(string).substring(1,3);
  String command = String(string).substring(3,7);

  if(!address.equals("00")) return -1;

  if(command.equals("NAME"))
  {
    strcat(string,": MCT_Labor_WIFI");
    return 0;
  }
  if(command.equals("SSID"))
  {
    ssid=String(string).substring(7);
    return 0;
  }  
  if(command.equals("PASS"))
  {
    pass=String(string).substring(7);
    return 0;
  }
  if(command.equals("CONN"))
  {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    return 0;  
  }
 
  return 1;  
}

void loop() {
  static char serial_buffer[256], udp_buffer[256];
  bool string_complete = false;
  while(Serial.available())
  {
    static uint8_t index=0;
    char inChar = (char)Serial.read();
    if(inChar=='$'||inChar=='!'||inChar=='?') index=0;
    else if(inChar == '\n')
    {
      serial_buffer[index]=0;
      string_complete=true;
      break;
    }
    serial_buffer[index++]=inChar;
  }

  if(string_complete)
  {
    if(serial_buffer[0]=='$')   // Befehl an mich?
    {
      int8_t status=serial_decode(serial_buffer);
      if(status>=0)
      {
        serial_buffer[0] = status ? '?' : '!';
        Serial.println(serial_buffer);
      }   
    }
    else                          // Relay to WLAN
    {
      if(connected)
      {
         sendUDPpacket(serial_buffer,broadcast_ip);
      }
    }
  }

  if(!connected)
  {
    if(WiFi.status() == WL_CONNECTED)
    {
      connected=true;
      uint32_t ipBuffer=WiFi.localIP(); 

      Serial.print("$00ADDR");
      Serial.println(ipBuffer>>24,HEX);   
         
      uint32_t maskBuffer=WiFi.subnetMask(); 
      broadcast_ip=ipBuffer|~maskBuffer;
      udp.begin(localPort);
     }
  }
  if(connected)
  {
    if(udp.parsePacket())
    {
      int len=udp.read(udp_buffer,255);
      if(len)
      {
        Serial.write(udp_buffer,len);
      }
    }
  }
   
  delay(10);    // Short wait
}

void sendUDPpacket(char* string, IPAddress& address)
{
  uint8_t packetSize=strlen(string);
  udp.beginPacket(address, 45454); 
  udp.write(string, packetSize);
  udp.endPacket();  
}
