// Arduino Email Client - Send Email to Server Directly
//
// PHPoC Shield and PHPoC WiFi Shield are Internet Shields for Arduino Uno and
// Mega.
//
// This is an example of using Arduino Uno/Mega and PHPoC [WiFi] Shield to send
// an email
//
// Arduino communicates with PHPoC [WiFi] Shield via pins 10, 11, 12 and 13 on
// the Uno, and pins 10, 50, 51 and 52 on the Mega. Therefore, these pins CANNOT
// be used for general I/O.
//
// This example code was written by Sollae Systems. It is released into the
// public domain.
//
// Tutorial for the example is available here:
// https://forum.phpoc.com/articles/tutorials/1252-arduino-email-client

#include <Phpoc.h>

PhpocEmail email;

void setup() {
  Serial.begin(9600);
  while(!Serial)
    ;

  // initialize PHPoC [WiFi] Shield:
  Phpoc.begin(PF_LOG_SPI | PF_LOG_NET | PF_LOG_APP);
  //Phpoc.begin();

  Serial.println("Sending email to server directly");

  // setup From/To/Subject:
  email.setFrom("from_email_address", "from_user_name");
  email.setTo("to_email_address", "to_user_name");
  email.setSubject("Mail from PHPoC Shield for Arduino");

  // write email message:
  email.beginMessage();
  email.println("Hello, world!");
  email.println("I am PHPoC Shield for Arduino");
  email.println("Good bye");
  email.endMessage();

  // send email:
  if(email.send() > 0)
    Serial.println("Email send ok");
  else
    Serial.println("Email send failed");
}

void loop() {
}
