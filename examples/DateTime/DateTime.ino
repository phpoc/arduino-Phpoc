/* arduino RTC date & time test */

#include <SPI.h>
#include <Phpoc.h>

PhpocDateTime datetime;

void setup() {
  Serial.begin(9600);
  while(!Serial)
    ;
    
  Phpoc.begin();
  
  Serial.println("Get year/month/day/dayofWeek/hour/minute/second from RTC in PHPoC Shield");
  
  Serial.print(datetime.year());
  Serial.print('-');
  Serial.print(datetime.month());
  Serial.print('-');
  Serial.print(datetime.day());
  Serial.print(' ');
  Serial.print(datetime.dayofWeek());
  Serial.print(':');
  Serial.print(datetime.hour());
  Serial.print(':');
  Serial.print(datetime.minute());
  Serial.print(':');
  Serial.print(datetime.second());
  Serial.println();

  datetime.date("Y-m-d H:i:s");
}

void loop() {
  Serial.println(datetime.date());
  delay(1000);
}
