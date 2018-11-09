// Arduino Web Server - Remote Control (Touch Pad)
//
// PHPoC Shield and PHPoC WiFi Shield are Internet Shields for Arduino Uno and
// Mega.
// These Shields have the buit-in web server and WebSocket server. These Shields
// contain some buit-in embedded web apps. One of the buit-in embedded web apps
// is "Web Remote Pad". When an user clicks or touches on touchable area of this
// web app, the web app sends x, y coordinates and touch's state to Arduino via
// WebSocket.
//
// This example code shows how Arduino communicate with "Web Remote Pad".
//
// Arduino communicates with PHPoC [WiFi] Shield via pins 10, 11, 12 and 13 on
// the Uno, and pins 10, 50, 51 and 52 on the Mega. Therefore, these pins CANNOT
// be used for general I/O.
//
// This example code was written by Sollae Systems. It is released into the
// public domain.
//
// Tutorial for the example is available here:
// https://forum.phpoc.com/articles/tutorials/1247-arduino-web-server-remote-pad

#include <Phpoc.h>

PhpocServer server(80);

char touch_state;
int touch_x;
int touch_y;

void setup() {
  Serial.begin(9600);
  while(!Serial)
    ;

  // initialize PHPoC [WiFi] Shield:
  Phpoc.begin(PF_LOG_SPI | PF_LOG_NET);
  //Phpoc.begin();

  // start WebSocket server
  server.beginWebSocket("remote_pad");

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
    String touchStr = client.readLine();

    if(touchStr) {
      // when an user clicks or touches on touchable area of web app, web app
      // sends a string to Arduino. The string includes x, y coordinates and
      // touch's state in order (separated by comma). The string is terminated
      // by a carriage return and a newline characters.
      int commaPos1 = touchStr.indexOf(',');
      int commaPos2 = touchStr.lastIndexOf(',');

      touch_x = touchStr.substring(0, commaPos1).toInt();
      touch_y = touchStr.substring(commaPos1 + 1, commaPos2).toInt();
      touch_state = touchStr.charAt(commaPos2 + 1);

      // touch's state is a character. The possible values of touch's state are
      // 'S', 'M' and 'U', standing for "touch start", "touch move" and "touch
      // end", respectively.
      if(touch_state == 'S')
        Serial.print("Touch start at: ");
      else
      if(touch_state == 'M')
        Serial.print("Touch move to: ");
      else
      if(touch_state == 'U')
        Serial.print("Touch end at: ");

      Serial.print(touch_x);
      Serial.print(", ");
      Serial.println(touch_y);
    }
  }
}
