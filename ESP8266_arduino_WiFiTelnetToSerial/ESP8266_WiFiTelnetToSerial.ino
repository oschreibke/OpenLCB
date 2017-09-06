/*
  WiFiTelnetToSerial - Example Transparent UART to Telnet Server for esp8266

  Copyright (c) 2015 Hristo Gochkov. All rights reserved.
  This file is part of the ESP8266WiFi library for Arduino environment.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

/*
  This is a derivative of the example code described above
  The main changes relate to the debugging messages included and the use of Serial instead of Serial1,
  the functionality is essentially unchanged.
  
  
  Otto Schreibke 2017
*/

#include <ESP8266WiFi.h>
//#define SHOWMESSAGES

//how many clients should be able to telnet to this ESP8266
#define MAX_SRV_CLIENTS 1

// fill your ssid and password in the next two lines and uncomment them
//const char* ssid = "<fill me>";
//const char* password = "<fill me>";
const IPAddress SERVERIP(192, 168, 0, 112);
const IPAddress GATEWAY(192, 168, 0, 1);
const IPAddress SUBNET(255, 255, 255, 0);
const IPAddress DNS(192, 168, 0, 32);
//const IPAddress DNS2(192, 168, 0, 32);

WiFiServer server(23);  //listen to port 23
WiFiClient serverClients[MAX_SRV_CLIENTS];

void setup() {
  Serial.begin(115200);

#ifdef SHOWMESSAGES
  Serial.println("\nESP8266_WiFiTelnetToSerial");
  Serial.print("Connecting to "); Serial.println(ssid);
#endif

  // set the fixed ip adresses
  WiFi.config(SERVERIP, GATEWAY, SUBNET, DNS); 
  
  WiFi.begin(ssid, password);

  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20) delay(500);
  if (i == 21) {
#ifdef SHOWMESSAGES
    Serial.print("Could not connect to"); Serial.println(ssid);
#endif
    while (1) delay(500);
  }
  
  //start UART and the server
  //Serial.begin(115200);
  server.begin();
  server.setNoDelay(true);

#ifdef SHOWMESSAGES
  Serial.print("Ready! Use 'telnet ");
  Serial.print(WiFi.localIP());
  Serial.println(" 23' to connect");
  Serial.print("subnetMask"); Serial.println(WiFi.subnetMask());
  Serial.print("gatewayIP"); Serial.println(WiFi.gatewayIP());
  Serial.print("dnsIP"); Serial.println(WiFi.dnsIP());

#endif
}

void loop() {
  uint8_t i;
  //check if there are any new clients
  if (server.hasClient()) {
    for (i = 0; i < MAX_SRV_CLIENTS; i++) {
      //find free/disconnected spot
      if (!serverClients[i] || !serverClients[i].connected()) {
        if (serverClients[i]) serverClients[i].stop();
        serverClients[i] = server.available();
#ifdef SHOWMESSAGES
        Serial.print("New client = "); Serial.println(i);
#endif
        continue;
      }
    }
    //no free/disconnected spot so reject
    WiFiClient serverClient = server.available();
    serverClient.stop();
  }
  //check clients for data
  for (i = 0; i < MAX_SRV_CLIENTS; i++) {
    if (serverClients[i] && serverClients[i].connected()) {
      if (serverClients[i].available()) {
        //get data from the telnet client and push it to the UART
        //while(serverClients[i].available()) Serial.write(serverClients[i].read());
        if (serverClients[i].available()) {
          int byteCnt = serverClients[i].available();
#ifdef SHOWMESSAGES
          Serial.print("Bytes Available = ");
          Serial.println(byteCnt);
#endif
          for (int x = 0; x < byteCnt; x++) {
            Serial.write(serverClients[i].read());
            Serial.print("");
          }
#ifdef SHOWMESSAGES
          Serial.println();
#endif
        }
      }
    }
  }
  //check UART for data
  if (Serial.available()) {
    size_t len = Serial.available();
    uint8_t sbuf[len];
    Serial.readBytes(sbuf, len);
    //push UART data to all connected telnet clients
    for (i = 0; i < MAX_SRV_CLIENTS; i++) {
      if (serverClients[i] && serverClients[i].connected()) {
        serverClients[i].write(sbuf, len);
        delay(1);
      }
    }
  }
}
