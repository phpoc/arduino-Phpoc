// Arduino Web Server - Remote Control (Slide Switch)
//
// PHPoC Shield and PHPoC WiFi Shield are Internet Shields for Arduino Uno and
// Mega.
//
// These Shields have the buit-in web server and WebSocket server. These Shields
// contain some buit-in embedded web apps. One of the buit-in embedded web apps
// is "Web Remote Slide". When an user moves a slider on this web app, the web
// app sends the slider name and slider value to Arduino via WebSocket.
//
// This example code shows how Arduino communicate with "Web Remote Slide".
//
// Arduino communicates with PHPoC [WiFi] Shield via pins 10, 11, 12 and 13 on
// the Uno, and pins 10, 50, 51 and 52 on the Mega. Therefore, these pins CANNOT
// be used for general I/O.
//
// This example code was written by Sollae Systems. It is released into the
// public domain.
//
// Tutorial for the example is available here:
// https://forum.phpoc.com/articles/tutorials/1248-arduino-web-server-remote-slide

#include <Phpoc.h>

PhpocServer server(80);

char slideName;
int slideValue;

void setup() {
  Serial.begin(9600);
  while(!Serial)
    ;

  // initialize PHPoC [WiFi] Shield:
  Phpoc.begin(PF_LOG_SPI | PF_LOG_NET);
  //Phpoc.begin();

  // start WebSocket server
  server.beginWebSocket("remote_slide");

  // print IP address of PHPoC [WiFi] Shield to serial monitor:
  Serial.print("WebSocket server address : ");
  Serial.println(Phpoc.localIP());
}

void loop() {
  // wait for a new client:
  PhpocClient client = server.available();

  if (client) {
    // read a string that is terminated by a carriage return and a newline
    // characters:
    String slideStr = client.readLine();

    if(slideStr)
    {
      // when an user moves a slider on web app, web app sends a string to
      // Arduino. The first character of string is slide name, followed by slide
      // value and terminated by a carriage return and a newline characters.
      slideName = slideStr.charAt(0);
      slideValue = slideStr.substring(1).toInt();

      Serial.print(slideName);
      Serial.print('/');
      Serial.println(slideValue);
    }
  }
}
