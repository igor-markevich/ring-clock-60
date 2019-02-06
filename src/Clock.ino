//************* Wol Clock by Igor Markevich ******************************
#define FASTLED_ESP8266_RAW_PIN_ORDER

#include <ESP8266WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

#include <NTPClient.h>
#include <Time.h>
#include <TimeLib.h>
//#include <Adafruit_NeoPixel.h>
#include <FastLED.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#define OFFSET 0 //offset "12" from start
#define LED_PIN     2
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS 60
#define STEP 2
CRGB leds[NUM_LEDS];
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
struct RedGreenBlue {
  byte r, g, b;
};
struct TIME {
  byte Hour, Minute;
};
const RedGreenBlue Twelve = { 0, 0 , 50}; // 12
const RedGreenBlue Quarters = { 0, 0, 30 };//3.6.9
const RedGreenBlue Divisions = { 0, 030, 0 };//1,2,4,5,7,8,10 & 11 to give visual reference
const RedGreenBlue Background = {  6, 0, 0 };
const RedGreenBlue Hour = { 255, 255, 0 };
const RedGreenBlue Minute = { 255, 0, 255 };
const RedGreenBlue Second = { 0, 255, 255 };
const char ClockGoBackwards = 0;
const TIME WeekNight = {00, 00}; // Night time to go dim
const TIME WeekMorning = {6, 00}; //Morning time to go bright
const TIME WeekendNight = {23, 00}; // Night time to go dim
const TIME WeekendMorning = {6, 00}; //Morning time to go bright
const int day_brightness = 55;
const int night_brightness = 55;
const int hours_Offset_From_GMT = 2;
const char *ssid      = "ESP";    //  your network SSID (name)
const char *password  = "12345679"; // your network password
byte SetClock; //flag for update time from server
void SetClockFromNTP();
void Draw_Clock(time_t t, byte Phase);
int ClockCorrect(int Pixel);
void SetBrightness(time_t t);

bool IsDst();
uint8_t brightness;// = 255;
uint8_t colorIndex;
CRGBPalette16 currentPalette;
TBlendType    currentBlending;
uint8_t startIndex = 0;
void setup() {
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);//.setCorrection( TypicalLEDStrip );
  Draw_Clock(0, 1); // Just draw a blank clock
  WiFiManager wifiManager;
  // WiFi.begin(ssid, password); // Try to connect to WiFi
  Draw_Clock(0, 2); // Draw the clock background
  wifiManager.autoConnect();
  //while ( WiFi.status() != WL_CONNECTED )
  delay ( 500 ); // keep waiging until we successfully connect to the WiFi
  Draw_Clock(0, 3); // Add the quater hour indicators
  SetClockFromNTP(); // get the time from the NTP server with timezone correction
  FastLED.setBrightness(55);
  currentPalette = Rainbow_gp;
  currentBlending = LINEARBLEND;
  brightness = 20;
}
void SetClockFromNTP ()
{
  timeClient.update(); // get the time from the NTP server
  setTime(timeClient.getEpochTime()); // Set the system yime from the clock
  if (IsDst())
    adjustTime((hours_Offset_From_GMT + 1) * 3600); // offset the system time with the user defined timezone (3600 seconds in an hour)
  else
    adjustTime(hours_Offset_From_GMT * 3600); // offset the system time with the user defined timezone (3600 seconds in an hour)
}
bool IsDst()
{
  if (month() < 3 || month() > 10)  return false;
  if (month() > 3 && month() < 10)  return true;
  int previousSunday = day() - weekday();
  if (month() == 3) return previousSunday >= 24;
  if (month() == 10) return previousSunday < 24;
  return false; // this line never gonna happend
}
void loop() {
  time_t t = now(); // Get the current time
  Draw_Clock(t, 4); // Draw the whole clock face with hours minutes and seconds
  if (second(t) == 0) {// at the start of each minute, update the time from the time server
    if (SetClock == 1){
      SetClockFromNTP(); // get the time from the NTP server with timezone correction
      SetClock = 0;
    }
    else{
      delay(100); // Just wait for 0.1 seconds
      SetClock = 1;
    }
  }
  colorIndex = startIndex;
  FastLED.show();
}
//************* Functions to draw the clock ******************************
void Draw_Clock(time_t t, byte Phase)
{
  if (Phase <= 0)
    for (int i = 0; i < 60; i++)
      leds[i] = CRGB::Black; // for Phase = 0 or less, all pixels are black
  if (Phase >= 1)
    for (int i = 0; i < 60; i++) {
      leds[i] = CRGB(0, 0, 0); //ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);
      //colorIndex += STEP;
    }

  // for Phase = 1 or more, draw minutes with Background colour
  if (Phase >= 2)
    for (int i = 0; i < 60; i = i + 5)
      leds[i] = CRGB(Divisions.r, Divisions.g, Divisions.b); // for Phase = 2 or more, draw 5 minute divisions
  if (Phase >= 3) {
    for (int i = (30 - OFFSET); i < 60; i = i + 15)
      leds[ClockCorrect(i)] = CRGB(Quarters.r, Quarters.g, Quarters.b); // for Phase = 3 or more, draw 15 minute divisions
    leds[ClockCorrect(30 - OFFSET)] = CRGB(Twelve.r, Twelve.g, Twelve.b); // for Phase = 3 and above, draw 12 o'clock indicator
  }
  if (Phase >= 4) {

    int Sec = ClockCorrect(second(t));
    leds[Sec] = CRGB(Second.r, Second.g, Second.b); // draw the second hand first
    int Min = ClockCorrect(minute(t));
    leds[Min] = CRGB(Minute.r, Minute.g, Minute.b); //
    int Hr = (((hour(t) % 12) * 5) + minute(t) / 12);
    leds[Hr] = CRGB(Hour.r, Hour.g, Hour.b); // draw the hour hand last
  }

  
  FastLED.show(); // show all the pixels
}

//************* Function to set the clock brightness ******************************
void SetBrightness(time_t t)
{
  int NowHour = hour(t);
  int NowMinute = minute(t);

  if ((weekday() >= 2) && (weekday() <= 6))
    if ((NowHour > WeekNight.Hour) || ((NowHour == WeekNight.Hour) && (NowMinute >= WeekNight.Minute)) || ((NowHour == WeekMorning.Hour) && (NowMinute <= WeekMorning.Minute)) || (NowHour < WeekMorning.Hour))
      FastLED.setBrightness(night_brightness);
    else
      FastLED.setBrightness(day_brightness);
  else if ((NowHour > WeekendNight.Hour) || ((NowHour == WeekendNight.Hour) && (NowMinute >= WeekendNight.Minute)) || ((NowHour == WeekendMorning.Hour) && (NowMinute <= WeekendMorning.Minute)) || (NowHour < WeekendMorning.Hour))
    FastLED.setBrightness(night_brightness);
  else
    FastLED.setBrightness(day_brightness);
}

//************* This function reverses the pixel order ******************************
int ClockCorrect(int Pixel)
{
  if (ClockGoBackwards == 1)
    return ((60 - Pixel + OFFSET) % 60); // my first attempt at clock driving had it going backwards :)
  else
    return ((Pixel + OFFSET) % 60);
}
