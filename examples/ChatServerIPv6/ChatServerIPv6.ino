// Arduino IPv4 & IPv6 Dual Stack Chat Server - 4 Listening Sessions
//
// PHPoC Shield and PHPoC WiFi Shield are Internet Shields for Arduino Uno and
// Mega.
//
// This is an example of using Arduino Uno/Mega and PHPoC [WiFi] Shield to
// create a TCP server that can connect up to 4 TCP clients simultaneously using
// IPv6. It distributes any incoming messages to all connected clients. The
// incoming messages are also printed to the serial monitor.
//
// Arduino communicates with PHPoC [WiFi] Shield via pins 10, 11, 12 and 13 on
// the Uno, and pins 10, 50, 51 and 52 on the Mega. Therefore, these pins CANNOT
// be used for general I/O.
//
// This example code was written by Sollae Systems. It is released into the
// public domain.
//
// Tutorial for the example is available here:
// https://forum.phpoc.com/articles/tutorials/1233-arduino-tcp-ipv6-chat-server

#include <Phpoc.h>

PhpocServer server(23);
boolean alreadyConnected = false; // whether or not the client was connected previously

void setup() {
  Serial.begin(9600);
  while(!Serial)
    ;

  // initialize PHPoC [WiFi] Shield:
  Phpoc.begin(PF_LOG_SPI | PF_LOG_NET);
  //Phpoc.begin();

  // initialize IPv6:
  Phpoc.beginIP6();

  // start listening for TCP clients:
  server.begin();

  // print IP address of PHPoC [WiFi] Shield to serial monitor:
  Serial.print("Chat server address : ");
  Serial.print(Phpoc.localIP());
  Serial.print(' ');
  Serial.print(Phpoc.localIP6());
  Serial.print(' ');
  Serial.print(Phpoc.globalIP6());
  Serial.println();
}

void loop() {
  // wait for a new client:
  PhpocClient client = server.available();

  // when the client sends the first byte, say hello:
  if (client) {
    if (!alreadyConnected) {
      // clear out the transmission buffer:
      client.flush();
      Serial.println("We have a new client");
      client.println("Hello, client!");
      alreadyConnected = true;
    }

    if (client.available() > 0) {
      // read the bytes incoming from the client:
      char thisChar = client.read();
      // echo the bytes back to all connected clients:
      server.write(thisChar);
      // echo the bytes to the server as well:
      Serial.write(thisChar);
    }
  }
}
