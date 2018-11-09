// Arduino Web Server - Remote Control (Push Button)
//
// PHPoC Shield and PHPoC WiFi Shield are Internet Shields for Arduino Uno and
// Mega.
//
// These Shields have the buit-in web server and WebSocket server. These Shields
// contain some buit-in embedded web apps. One of the buit-in embedded web apps
// is "Web Remote Push". When an user presses/releases a button on this web
// apps, the web app sends an uppercase/lowercase characters corresponding with
// the name of button to Arduino via WebSocket.
//
// This example code shows how Arduino communicate with the "Web Remote Push".
//
// Arduino communicates with PHPoC [WiFi] Shield via pins 10, 11, 12 and 13 on
// the Uno, and pins 10, 50, 51 and 52 on the Mega. Therefore, these pins CANNOT
// be used for general I/O.
//
// This example code was written by Sollae Systems. It is released into the
// public domain.
//
// Tutorial for the example is available here:
// https://forum.phpoc.com/articles/tutorials/1249-arduino-web-server-remote-push

#include <Phpoc.h>

PhpocServer server(80);

void setup() {
  Serial.begin(9600);
  while(!Serial)
    ;

  // initialize PHPoC [WiFi] Shield:
  Phpoc.begin(PF_LOG_SPI | PF_LOG_NET);
  //Phpoc.begin();

  // start WebSocket server
  server.beginWebSocket("remote_push");

  // print IP address of PHPoC [WiFi] Shield to serial monitor:
  Serial.print("WebSocket server address : ");
  Serial.println(Phpoc.localIP());
}

void loop() {
  // wait for a new client:
  PhpocClient client = server.available();

  if (client) {
    if (client.available() > 0) {
      // read a byte incoming from the client:
      char thisChar = client.read();

      // when an user presses a button on the web apps, the web app sends an
      // uppercase character corresponding with the name of button to Arduino.
      // When an user releases a button on the web apps, the web app sends a
      // lowercase character corresponding with the name of button to Arduino.
      if(thisChar == 'A')
         Serial.println("button A press");
      if(thisChar == 'a')
         Serial.println("button A release");
      if(thisChar == 'B')
         Serial.println("button B press");
      if(thisChar == 'b')
         Serial.println("button B release");
      if(thisChar == 'C')
         Serial.println("button C press");
      if(thisChar == 'c')
         Serial.println("button C release");
    }
  }
}
