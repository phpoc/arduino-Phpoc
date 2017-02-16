/* arduino IPv6 web client - GET request for hello.php or logo.txt */

#include <SPI.h>
#include <Phpoc.h>

char server_name[] = "ipv6test.phpoc.com";
PhpocClient client;

void setup() {
  Serial.begin(9600);
  while(!Serial)
    ;

  Serial.println("Sending GET reqeust to IPv6 web server");
    
  Phpoc.begin(PF_LOG_SPI | PF_LOG_NET);
  //Phpoc.begin();

  Phpoc.beginIP6();

  if(client.connect(server_name, 80))
  {
    Serial.println("Connected to server");
    client.println("GET /hello.php HTTP/1.0");
    //client.println("GET /logo.txt HTTP/1.0");
    client.println("Host: ipv6test.phpoc.com");
    client.println("Connection: close");
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
