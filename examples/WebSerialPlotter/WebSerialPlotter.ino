// Arduino Web Serial Plotter
//
// PHPoC Shield and PHPoC WiFi Shield are Internet Shields for Arduino Uno and
// Mega.
//
// These Shields have the buit-in web server and WebSocket server. These Shields
// contain some buit-in embedded web apps. One of the buit-in embedded web apps
// is "Web Serial Plotter". Web Serial Plotter is similar to Serial Plotter on
// Arduino IDE, except for the following points:
//   - Web Serial Plotter is accessed on web browser through Internet (Serial
// Plotter is accessed on Arduino IDE through USB cable).
//   - Web Serial Plotter can be accessed from any OS (Android, iOS, Windows,
// macOS, Linux...) without any installation.
//   - Web Serial Plotter is easily customizable via a setting page.
//
// This example code shows how to use "Web Serial Plotter".
//
// Arduino communicates with PHPoC [WiFi] Shield via pins 10, 11, 12 and 13 on
// the Uno, and pins 10, 50, 51 and 52 on the Mega. Therefore, these pins CANNOT
// be used for general I/O.
//
// This example code was written by Sollae Systems. It is released into the
// public domain.
//
// Tutorial for the example is available here:
// https://forum.phpoc.com/articles/tutorials/1246-arduino-web-serial-plotter

#include <Phpoc.h>

float y1;
float y2;
float y3;

void setup() {
  Serial.begin(9600);
  while(!Serial)
    ;
  // initialize PHPoC [WiFi] Shield:
  Phpoc.begin();
}

void loop() {
  for(int i = 0; i < 360; i += 10) {
    y1 = 2 * sin(i * M_PI / 180);
    y2 = 3 * sin((i + 90)* M_PI / 180);
    y3 = 5 * sin((i + 180)* M_PI / 180);

    Serial.print(y1);
    Serial.print(" "); // a space ' ' or  tab '\t' character is printed between the two values.
    Serial.print(y2);
    Serial.print(" "); // a space ' ' or  tab '\t' character is printed between the two values.
    Serial.println(y3); // the last value is followed by a carriage return and a newline characters.
    delay(100);
  }
}
