// Arduino RTC Date & Time Test
//
// PHPoC Shield and PHPoC WiFi Shield are Internet Shields for Arduino Uno and
// Mega.
//
// PHPoC [WiFi] Shield has a built-in rechargeable battery and RTC that allow to
// keep date and time information even when power is down.
//
// This is an example of using Arduino Uno/Mega to get date and time information
// from RTC on PHPoC [WiFi] Shield and print it to serial monitor.
//
// Arduino communicates with PHPoC [WiFi] Shield via pins 10, 11, 12 and 13 on
// the Uno, and pins 10, 50, 51 and 52 on the Mega. Therefore, these pins CANNOT
// be used for general I/O.
//
// This example code was written by Sollae Systems. It is released into the
// public domain.
//
// Tutorial for the example is available here:
// https://forum.phpoc.com/articles/tutorials/1244-arduino-date-and-time

#include <Phpoc.h>

PhpocDateTime datetime;

void setup() {
  Serial.begin(9600);
  while(!Serial)
    ;

  // initialize PHPoC [WiFi] Shield:
  Phpoc.begin(PF_LOG_SPI | PF_LOG_NET);

  Serial.println("Get year/month/day/dayofWeek/hour/minute/second from RTC in PHPoC Shield");

  Serial.print(datetime.year());
  Serial.print('-');
  Serial.print(datetime.month());
  Serial.print('-');
  Serial.print(datetime.day());
  Serial.print(' ');
  Serial.print(datetime.dayofWeek());
  Serial.print(' ');
  Serial.print(datetime.hour());
  Serial.print(':');
  Serial.print(datetime.minute());
  Serial.print(':');
  Serial.print(datetime.second());
  Serial.println();

  // set date and time format:
  datetime.date(F("Y-m-d H:i:s"));
}

void loop() {
  Serial.println(datetime.date());
  delay(10);
}
