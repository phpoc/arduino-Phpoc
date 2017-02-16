/* arduino IPv4 & IPv6 dual stack chat server - 4 listening sessions */

#include <SPI.h>
#include <Phpoc.h>

PhpocServer server(23);
boolean alreadyConnected = false; // whether or not the client was connected previously

void setup() {
  Serial.begin(9600);
  while(!Serial)
    ;

  Phpoc.begin(PF_LOG_SPI | PF_LOG_NET);
  //Phpoc.begin();

  Phpoc.beginIP6();

  server.begin();

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
      // echo the bytes back to the client:
      server.write(thisChar);
      // echo the bytes to the server as well:
      Serial.write(thisChar);
    }
  }
}
