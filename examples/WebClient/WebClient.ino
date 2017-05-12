/* arduino web client - GET request for index.html or index.php */

#include <SPI.h>
#include <Phpoc.h>

//char server_name[] = "www.google.com";
//char server_name[] = "216.58.221.36";
char server_name[] = "www.arduino.cc";
PhpocClient client;

void setup() {
  Serial.begin(9600);
  while(!Serial)
    ;

  Serial.println("Sending GET request to web server");
    
  Phpoc.begin(PF_LOG_SPI | PF_LOG_NET);
  //Phpoc.begin();

  if(client.connect(server_name, 80))
  {
    Serial.println("Connected to server");
    client.println("GET / HTTP/1.0");
    client.println();
  }
  else
    Serial.println("connection failed");
}

void loop() {
  if(client.available())
  {
    char c = client.read();
    Serial.print(c);
  }

  if(!client.connected())
  {
    Serial.println("disconnected");
    client.stop();
    while(1)
      ;
  }
}
