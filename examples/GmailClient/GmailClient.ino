/* arduino email client - send email via gmail relay server */

#include "SPI.h"
#include "Phpoc.h"

PhpocEmail email;

void setup() {
  Serial.begin(9600);
  while(!Serial)
    ;
    
  Phpoc.begin(PF_LOG_SPI | PF_LOG_NET | PF_LOG_APP);
  //Phpoc.begin();
  
  Serial.println("Sending email to gmail relay server");

  // [login using your private password]
  // Google may block sign-in attempts from some apps or devices that do not use modern security standards.
  // Change your settings to allow less secure apps to access your account.
  // https://www.google.com/settings/security/lesssecureapps
  
  // [login using app password]
  // 1. turn on 2-step verification
  // 2. create app password
  // 3. apply app password as your login password

  // setup outgoing relay server - gmail.com
  email.setOutgoingServer("smtp.gmail.com", 587);
  email.setOutgoingLogin("your_login_id", "your_login_password or app_password");

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
