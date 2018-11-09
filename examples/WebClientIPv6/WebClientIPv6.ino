// Arduino IPv6 web client - GET request for hello.php or logo.txt
//
// PHPoC Shield and PHPoC WiFi Shield are Internet Shields for Arduino Uno and
// Mega.
//
// This is an example of using Arduino Uno/Mega and PHPoC [WiFi] Shield to make
// an HTTP request to a IPv6 web server and get web content in response. Web
// content is a HTML file and printed to serial monitor.
//
// Arduino communicates with PHPoC [WiFi] Shield via pins 10, 11, 12 and 13 on
// the Uno, and pins 10, 50, 51 and 52 on the Mega. Therefore, these pins CANNOT
// be used for general I/O.
//
// This example code was written by Sollae Systems. It is released into the
// public domain.
//
// Tutorial for the example is available here:
// https://forum.phpoc.com/articles/tutorials/1240-arduino-ipv6-web-client

#include <Phpoc.h>

// hostname of ipv6 web server:
char server_name[] = "example.phpoc.com";
PhpocClient client;

void setup() {
  Serial.begin(9600);
  while(!Serial)
    ;

  Serial.println("Sending GET request to IPv6 web server");

  // initialize PHPoC [WiFi] Shield:
  Phpoc.begin(PF_LOG_SPI | PF_LOG_NET);
  //Phpoc.begin();

  // initialize IPv6:
  Phpoc.beginIP6();

  // connect to IPv6 web server on port 80:
  if(client.connect(server_name, 80))
  {
    // if connected:
    Serial.println("Connected to server");
    // make a HTTP request:
    //client.println("GET / HTTP/1.0");
    client.println("GET /asciilogo.txt HTTP/1.0");
    //client.println("GET /remote_addr/ HTTP/1.0");
    client.println("Host: example.phpoc.com");
    client.println();
  }
  else // if not connected:
    Serial.println("connection failed");
}

void loop() {
  if(client.available())
  {
    // if there is an incoming byte from the server, read them and print them to
    // serial monitor:
    char c = client.read();
    Serial.print(c);
  }

  if(!client.connected())
  {
    // if the server's disconnected, stop the client:
    Serial.println("disconnected");
    client.stop();
    // do nothing forevermore:
    while(true)
      ;
  }
}
