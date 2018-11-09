// Arduino Email Client - Send Email via Gmail Relay Server
//
// PHPoC Shield and PHPoC WiFi Shield are Internet Shields for Arduino Uno and
// Mega.
//
// This is an example of using Arduino Uno/Mega and PHPoC [WiFi] Shield to send
// an email via Gmail Relay Server.
//
// Arduino communicates with PHPoC [WiFi] Shield via pins 10, 11, 12 and 13 on
// the Uno, and pins 10, 50, 51 and 52 on the Mega. Therefore, these pins CANNOT
// be used for general I/O.
//
// This example code was written by Sollae Systems. It is released into the
// public domain.
//
// Tutorial for the example is available here:
// https://forum.phpoc.com/articles/tutorials/1238-arduino-gmail-client

#include <Phpoc.h>

PhpocEmail email;

void setup() {
  Serial.begin(9600);
  while(!Serial)
    ;

  // initialize PHPoC [WiFi] Shield:
  Phpoc.begin(PF_LOG_SPI | PF_LOG_NET | PF_LOG_APP);
  //Phpoc.begin();

  Serial.println("Sending email to gmail relay server");

  // [login using your private password]
  // Google may block sign-in attempts from some apps or devices that do not use
  // modern security standards.
  // Change your settings to allow less secure apps to access your account.
  // https://www.google.com/settings/security/lesssecureapps

  // [login using app password]
  // 1. turn on 2-step verification
  // 2. create app password
  // 3. apply app password as your login password

  // setup outgoing relay server - gmail.com:
  email.setOutgoingServer("smtp.gmail.com", 587);
  email.setOutgoingLogin("your_login_id", "your_login_password or app_password");

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
