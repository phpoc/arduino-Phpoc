/* arduino web server - remote control (push button) */

#include "SPI.h"
#include "Phpoc.h"

PhpocServer server(80);

void setup() {
  Serial.begin(9600);
  while(!Serial)
    ;

  Phpoc.begin(PF_LOG_SPI | PF_LOG_NET);
  //Phpoc.begin();

  server.beginWebSocket("remote_push");

  Serial.print("WebSocket server address : ");
  Serial.println(Phpoc.localIP());  
}

void loop() {
  // wait for a new client:
  PhpocClient client = server.available();

  if (client) {
    if (client.available() > 0) {
      // read the bytes incoming from the client:
      char thisChar = client.read();

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
