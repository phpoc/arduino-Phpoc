#include "SPI.h"
#include "Phpoc.h"

char server[] = "www.arduino.cc";

PhpocClient client;

void setup() {
  Serial.begin(9600);
  while(!Serial)
  ;

  Serial.println("PHPoC SSL Client test");

  Phpoc.begin(PF_LOG_SPI | PF_LOG_NET);
  //Phpoc.begin();

  if(client.connectSSL(server, 443)) {
    Serial.println("Connected to server");
    // Make a HTTP request:
    client.println("GET /asciilogo.txt HTTP/1.1");
    client.println("Host: www.arduino.cc");
    client.println("Connection: close");
    client.println();
    Serial.println("Request sent");
  }
}

void loop() {
  // if there are incoming bytes available
  // from the server, read them and print them:
  while (client.available()) {
    char c = client.read();
    Serial.write(c);
  }

  // if the server's disconnected, stop the client:
  if (!client.connected()) {
    Serial.println();
    Serial.println("disconnecting from server.");
    client.stop();

    // do nothing forevermore:
    while (true);
  }

}
