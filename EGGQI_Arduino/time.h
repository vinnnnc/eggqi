#include <Ds1302.h>

// Define the DS1302 RTC module pins
const int rtcSCLK = 5; // SCLK (Clock) pin
const int rtcIO = 4;   // IO (Data) pin
const int rtcCE = 2;   // CE (Chip Enable) pin

Ds1302 rtc(rtcCE, rtcSCLK, rtcIO);

const static char *WeekDays[] =
    {
        "Monday",
        "Tuesday",
        "Wednesday",
        "Thursday",
        "Friday",
        "Saturday",
        "Sunday"};

#define countof(a) (sizeof(a) / sizeof(a[0]))

// Initialize RTC module
void initTime()
{
  rtc.init();
}

// Get date and time from RTC module
String getTime()
{
  Ds1302::DateTime now;
  rtc.getDateTime(&now);
  char datestring[20];

  snprintf_P(datestring,
             countof(datestring),
             PSTR("%02u-%02u-20%02u_%02u:%02u:%02u"),
             now.month,
             now.day,
             now.year,
             now.hour,
             now.minute,
             now.second);
  return datestring;
}
