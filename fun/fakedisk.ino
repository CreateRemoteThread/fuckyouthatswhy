/*
 DEPRECATED - FOR REFERENCE ONLY
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <FS.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

IPAddress local_IP(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

WiFiServer wifiServer(23);
#define PIN_RX_MODE 2

void setup() {
  
  WiFi.softAPConfig(local_IP, gateway, subnet);
  WiFi.softAP("A","B");
  WiFi.softAPIP();
  Serial.begin(9600);
  SPIFFS.begin();
  // pinMode(PIN_TX_MUX,OUTPUT);
  pinMode(PIN_RX_MODE,INPUT);
  wifiServer.begin();
}

char maskCommand[3];
char maskIndex = 0;
char returnValue = 0;

int parseCommand(char c1,char c2)
{
  if(c1 == '\r' && c2 == '\n')
  {
    if(maskIndex == 3)
    {
      maskIndex = 0;
      return 1;
    }
    return 0;
  }
  else
  {
    char m[2];
    m[0] = c1;
    m[1] = c2;
    char cmd = strtoul( m, NULL, 16) & 0xFF;
    maskCommand[maskIndex++] = cmd;
    return 0;
  }
}

int lockMode = 0;
char thisChar[2];

void loop() {
  WiFiClient client = wifiServer.available();

  if (client) {

    while (client.connected()) {
      while (client.available()>0) {
        char c = client.read();
        Serial.write(c);
        delay(10);
      }
      delay(10);
      while (Serial.available())
      {
        char inChar = (char)Serial.read();
        if(lockMode != 0)
        {
          thisChar[2-lockMode] = inChar;
          lockMode -= 1;
          if(lockMode == 0)
          {
            if(thisChar[0] == '!' && thisChar[1] == '!')
            {
              client.write("!");
            }
            else
            {
              if(parseCommand(thisChar[0],thisChar[1]) == 1)
              {
                // client.write("+");
                Serial.write("\xE5");
              }
              else
              {
                // client.write(".");
              }
            }
          }
        }
        else if(inChar == '!')
        {
          lockMode = 2;
        }
        else
        {
          client.write(inChar);
        }
      }
    }
    client.stop();
  }
  delay(10);
  /*
  while(Serial.available())
  {
            char inChar = (char)Serial.read();
        client.write(inChar);
  }
  */
}
