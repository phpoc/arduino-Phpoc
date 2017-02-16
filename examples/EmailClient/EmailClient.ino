/* arduino email client - send email to server directly */

#include "SPI.h"
#include "Phpoc.h"

PhpocEmail email;

void setup() {
  Serial.begin(9600);
  while(!Serial)
    ;
    
  Phpoc.begin(PF_LOG_SPI | PF_LOG_NET | PF_LOG_APP);
  //Phpoc.begin();
  
  Serial.println("Sending email to server directly");

  // setup From/To/Subject
  email.setFrom("from_email_address", "from_user_name");
  email.setTo("to_email_address", "to_user_name");
  email.setSubject("Mail from PHPoC Shield for Arduino");

  // write email message
  email.beginMessage();
  email.println("Hello, world!");
  email.println("I am PHPoC Shield for Arduino");
  email.println("Good bye");
  email.endMessage();
  
  // send email
  if(email.send() > 0)
    Serial.println("Email send ok");
  else
    Serial.println("Email send failed");
}

void loop() {
}
