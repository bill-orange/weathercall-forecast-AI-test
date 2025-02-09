/* 
William E. Webb (c) Test program for using AI with weather forecasting
Arduino_JSON & ChatGPTuino ->  GNU LESSER GENERAL PUBLIC LICENSE Version 2.1, February 1999
weathrcall -> Software License Agreement (MIT License)
              Copyright (c) 2019 Dushyant 
TFT_eSPI -> Software License Agreement (MIT License)
            The original starting point for this library was the Adafruit_ILI9341 library in January 2015.
** user_setup file in OpenAI Library should be renamed and .h file revised **
 
 02/04/2025 Intial Release grahics are not intergrated
 02/05/2025 Graphics integrated and tested
 02/06/2025 Added NTP call for local time, added AI one hour forecast and moved graph setup
            Added 1 hour data point 
 02/09/2025 Fork to use weathercall library
*/

#include <WiFi.h>       // WiFi for ESP32
#include <WiFiMulti.h>  // Setup for multiple ssid
#include <ArduinoJson.h>
#include <weathercall.h>
#include <ChatGPTuino.h>  // For AI Support

#include <TFT_eSPI.h>       //  Graphic driver for multiple drivers and display
TFT_eSPI tft = TFT_eSPI();  //  Instance of display driver
#include <TFT_eWidget.h>    // Widget library

#include "temperature.h"
#include "credentials.h"  // Network name, password, and private API key

#define GFXFF 1                               // No idea what this is.  Taken from library sample
GraphWidget gr = GraphWidget(&tft);           // Graph widget
TraceWidget tr1 = TraceWidget(&gr);           // Traces are drawn on tft using graph instance
#define TIME_Between_Weather_Calls (3600000)  // Every Hour
#define WIFI_SSID ssid
#define WIFI_PASSWORD password
#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

String Key = apiKeyOpenWeather;
String latitude = LAT;       // 90.0000 to -90.0000 negative for Southern hemisphere
String longitude = LON;      // 180.000 to -180.000 negative for West
String units = Units;        // or "imperial"
String language = Language;  // See notes tab
String jsonBuffer;
char *AIShortReply = "                                                  ";

const int TOKENS = 174;  // How lengthy a response you want, every token is about 3/4 a word
const int NUM_MESSAGES = 30;
const char *model = "gpt-4o-mini";  // OpenAI Model being used
int indexData = 0;                  // Used fro plotting points on the graph

weatherData w;
Weathercall weather(Key, "Orange,us");
Weathercall forecast(Key, "Orange,us", 1);

String timconv(long epoc) {
  int secondo = epoc % 60;
  int minus = (epoc / 60);
  int minuto = minus % 60;
  int hrus = minus / 60;
  int horus = hrus % 24;
  //int dayys = hrus /24;
  String uptimus = String(horus) + ":" + String(minuto) + ":" + String(secondo);
  return uptimus;
}

WiFiMulti wifiMulti;                       // WiFi multi-connection instance
ChatGPTuino chat{ TOKENS, NUM_MESSAGES };  // Will store and send your most recent messages (up to NUM_MESSAGES)
temperatureConverter TC;                   // C to F


/*---------------------------- Setup ----------------------------------------*/
void setup() {
  Serial.begin(115200);  // Fast to stop it holding up the stream
  delay(5000);
  Serial.println();  // This block is for debug
  Serial.println("AI OpenWeather Demo");
  Serial.println(F(__FILE__ " " __DATE__ " " __TIME__));
  Serial.println();

  tft.begin();
  tft.setRotation(3);
  fileInfo();

  // Restart traces with new colours
  tr1.startTrace(TFT_WHITE);
  chat.init(key, model);   // Initialize AI chat
  connectToWifiNetwork();  // Connect to internet
}


float timeData[5] = { 0, 1, 2, 3, 4 };
float tempData[5] = { 0, 0, 0, 0, 0 };

/*---------------------------- Loop ----------------------------------------*/
void loop() {

  DrawGraph();

  weather.updateStatus(&w);
  Weatherupdatescreen();
  forecast.updateStatus(&w);
  Forecastupdatescreen();
  AIForecast();
  tr1.startTrace(TFT_WHITE);
  tft.setTextDatum(TC_DATUM);
  for (indexData = 0; indexData <= sizeof(timeData - 1); indexData++) {
    tr1.addPoint(timeData[indexData], tempData[indexData]);
    if (indexData < sizeof(timeData - 1)) tft.drawNumber(tempData[indexData], gr.getPointX(timeData[indexData]) + 5, gr.getPointY(tempData[indexData] + 5));
  }
  tft.drawString("Circled Points are A.I. Generated",175,40);
  tft.drawCircle(gr.getPointX(timeData[indexData - 4]), gr.getPointY(tempData[indexData - 4]), 5, TFT_GREEN);
  tft.drawCircle(gr.getPointX(timeData[indexData - 3]), gr.getPointY(tempData[indexData - 3]), 5, TFT_GREEN);
  tft.drawCircle(gr.getPointX(timeData[indexData - 2]), gr.getPointY(tempData[indexData - 2]), 5, TFT_GREEN);
  yield();
  if (tempData[0] != 0 && tempData[1] != 0) delay(TIME_Between_Weather_Calls);  // Recycle on error
}

/*------------------------ Send weather info to serial port -----------------*/
void AIForecast() {
  // Create the structures that hold the retrieved weather

  String realUserMessage = "I would like you to give me a forecast temperature for 1, 2 and 3 hours in the future. Current conditions are: "
                           + String(w.weather + ", " + String(w.description))
                           + ", temperature "
                           + String(w.current_Temp)
                           + " C, a humidity of " + String(w.humidity)
                           + "% , a wind speed of "
                           + String(w.windspeed * 3.6)
                           + " MPH, a barometric pressure of " + String(w.pressure)
                           + " hPa, and a cloud cover of "
                           + String(w.cloud)
                           + "%. Conditions at "
                           + String(w.dt_txt1)
                           + "3 are estimated to be, description: "
                           + String(w.weather1) + ", " + String(w.description1)
                           + ", temperature "
                           + String(w.current_Temp1)
                           + "F, a humidity of " + String(w.humidity1)
                           + "% , a wind speed of "
                           + String(w.windspeed1 * 3.6)
                           + " MPH, a barometric pressure of " + String(w.pressure1)
                           + " hPa and a clouds cover of "
                           + String(w.cloud1)
                           + "%. The location is "
                           + String(w.Location)
                           + " California. The time of the current weather report is "
                           + String(timconv(w.dt + w.timezone))
                           + ". Regardless of missing information, please provide your best 1, 2 and 3 hour forecast temperature."
                           + "  You may use historical data if you think it will help."
                           + " Please do not provide narrative or discussion, just the 1,2 and 3 hour forecast temperatures in Fahrenheit"
                           + " in the format xxx.x xxx.x xxx.x";  // User message to ChatGPT


  Serial.println(realUserMessage);
  int str_len = realUserMessage.length() + 1;
  char AIPrompt[str_len];
  realUserMessage.toCharArray(AIPrompt, str_len);
  AIShortReply = GetAIReply(AIPrompt);
  Serial.println(AIShortReply);

  // ********************************** Record data for Graph

  sscanf(AIShortReply, "%f %f %f", &tempData[1], &tempData[2], &tempData[3]);
  TC.setCelsius(w.current_Temp);
  tempData[0] = TC.getFahrenheit();
  TC.setCelsius(w.current_Temp1);
  tempData[4] = (TC.getFahrenheit() + (5 * tempData[3])) / 6;
  Serial.print("w.current_Temp1 ");
  Serial.println(TC.getFahrenheit());
  Serial.println(tempData[0]);
  Serial.println(tempData[1]);
  Serial.println(tempData[2]);
  Serial.println(tempData[3]);
  Serial.println(tempData[4]);
}

/*------------------------------- WiFi------------------------------------*/
// Connect to the Wifi network
void connectToWifiNetwork() {
  delay(10);
  WiFi.mode(WIFI_STA);
  wifiMulti.addAP(ssid_2, password_2);
  wifiMulti.addAP(ssid_1, password_1);
  // WiFi.scanNetworks will return the number of networks found
  int n = WiFi.scanNetworks();
  Serial.println("scan done");
  if (n == 0) {
    Serial.println("no networks found");
  } else {
    Serial.print(n);
    Serial.println(" networks found");
    for (int i = 0; i < n; ++i) {
      // Print SSID and RSSI for each network found
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.print(")");
      Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
      delay(10);
    }
  }
  // Connect to Wi-Fi using wifiMulti (connects to the SSID with strongest connection)
  Serial.println("Connecting Wifi...");
  if (wifiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
}

/*----------------------------------- Process AI Call ----------------------------------------*/
char *GetAIReply(char *message) {
  chat.putMessage(message, strlen(message));
  chat.getResponse();

  //Serial.println(message);
  return chat.getLastMessageContent();
}

/*---------------------------- File information  ------------------------------------------*/
void fileInfo() {  // uesful to figure our what software is running

  tft.setRotation(3);
  tft.fillScreen(TFT_BLUE);
  tft.setTextColor(TFT_WHITE);  // Print to TFT display, White color
  tft.setRotation(3);
  tft.setTextSize(2);
  tft.drawString("    AI openWeather Test ", 5, 30);
  tft.drawString("    Demos AI Prediction", 5, 50);
  tft.setTextSize(1);
  tft.drawString(__FILENAME__, 0, 90);
  tft.setTextSize(1);
  tft.drawString(__DATE__, 5, 120);
  tft.drawString(__TIME__, 120, 120);
  delay(6000);
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(3);
  tft.setTextSize(1);
}

/*-----------------------------------------------------------------------------------------*/
void DrawGraph() {

  tft.fillScreen(TFT_BLACK);
  gr.createGraph(250, 190, tft.color565(5, 5, 5));  // Graph area is 230 pixels wide, 190 high, dark grey background
  gr.setGraphScale(0, 4, 45, 110);                  // x scale units is from 0 to 100, y scale units is -50 to 50
  gr.setGraphGrid(0.0, 1, 45, 5, TFT_BLUE);         // blue grid
  gr.drawGraph(40, 10);                             // Draw empty graph, top left corner at 40,10 on TFT
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Hours from NOW", 130, 230, GFXFF);  // Print the legend text
  tft.setRotation(2);
  tft.drawString("Temp in degrees F", 70, 0, GFXFF);  // Print the legend text
  tft.setRotation(3);
  // Draw the x axis scale
  tft.setTextDatum(TC_DATUM);  // Top centre text datum & Draw the x axis scale
  for (int index = 0; index <= 4; index++) {
    tft.drawNumber(index, gr.getPointX(index), gr.getPointY(45.0) + 7);
  }
  tft.setTextDatum(MR_DATUM);  // Middle right text datum & Draw the y axis scale
  for (int index = 45; index <= 110; index = index + 10) {
    tft.drawNumber(index, gr.getPointX(0) - 7, gr.getPointY(index));
  }
}
/*-----------------------------------------------------------------------------------------*/

void Weatherupdatescreen() {
  /////////////////////variable to screen label
  Serial.print("Weather and Description: ");
  Serial.println(w.weather + ": " + w.description);
  Serial.print("Temperature in °C: ");
  Serial.println(w.current_Temp);
  Serial.print("Temperature Min °C: ");
  Serial.println(w.min_temp);
  Serial.print("Temperature Max °C: ");
  Serial.println(w.max_temp);
  Serial.print("Humidity %: ");
  Serial.println(w.humidity);
  Serial.print("Pressure hPa: ");
  Serial.println(w.pressure);
  Serial.print("Wind Direction / Speed km/h: ");
  Serial.println(Wind_NWES_direction(w.windeg) + " " + (w.windspeed * 3.6));
  Serial.print("Clouds %: ");
  Serial.println(w.cloud);
  Serial.print("Rains in mm (or snow): ");
  Serial.println(w.rain);
  Serial.print("Location/Country ");
  Serial.println(w.Location + "," + w.Country);
  Serial.print("Sunrise");
  Serial.println(timconv(w.sunrise + w.timezone));
  Serial.print("Sunset");
  Serial.println(timconv(w.sunset + w.timezone));
  Serial.print("last updated: ");
  Serial.println(timconv(w.dt + w.timezone));
  Serial.print("Full Response1: ");
  Serial.println(weather.getResponse().c_str());
}
/*-----------------------------------------------------------------------------------------*/

void Forecastupdatescreen() {

  Serial.print("Time Date: ");
  Serial.println(w.dt_txt1);
  Serial.print("Weather and Description 1: ");
  Serial.println(w.weather1 + ": " + w.description1);
  Serial.print("Temperature in °C 1: ");
  Serial.println(w.current_Temp1);
  Serial.print("Temperature Min °C 1: ");
  Serial.println(w.min_temp1);
  Serial.print("Temperature Max °C 1: ");
  Serial.println(w.max_temp1);
  Serial.print("Humidity % 1: ");
  Serial.println(w.humidity1);
  Serial.print("Pressure hPa 1: ");
  Serial.println(w.pressure1);
  Serial.print("Wind Direction / Speed km/h 1: ");
  Serial.println(Wind_NWES_direction(w.windeg1) + " " + (w.windspeed1 * 3.6));
  Serial.print("Clouds % 1: ");
  Serial.println(w.cloud1);
  Serial.print("Rains in mm (or snow) 1: ");
  Serial.println(w.rain1);

  Serial.println();
  Serial.print("Time Date: ");
  Serial.println(w.dt_txt2);
  Serial.print("Weather and Description 2: ");
  Serial.println(w.weather2 + ": " + w.description2);
  Serial.print("Temperature in °C 2: ");
  Serial.println(w.current_Temp2);
  Serial.print("Temperature Min °C 2: ");
  Serial.println(w.min_temp2);
  Serial.print("Temperature Max °C 2: ");
  Serial.println(w.max_temp2);
  Serial.print("Humidity % 2: ");
  Serial.println(w.humidity2);
  Serial.print("Pressure hPa 2: ");
  Serial.println(w.pressure2);
  Serial.print("Wind Direction / Speed km/h 2: ");
  Serial.println(Wind_NWES_direction(w.windeg2) + " " + (w.windspeed2 * 3.6));
  Serial.print("Clouds % 2: ");
  Serial.println(w.cloud2);
  Serial.print("Rains in mm (or snow) 2: ");
  Serial.println(w.rain2);

  Serial.print("Full Response1: ");
  Serial.println(forecast.getResponse().c_str());
}
/*-----------------------------------------------------------------------------------------*/

String Wind_NWES_direction(int windegree) {
  /////////////////////Wind degree to NSEW value
  String direction;
  switch (windegree) {

    case 337 ... 359:
      direction = "N";
      break;

    case 0 ... 23:
      direction = "N";
      break;

    case 24 ... 68:
      direction = "NE";
      break;

    case 69 ... 113:
      direction = "E";
      break;

    case 114 ... 158:
      direction = "SE";
      break;

    case 159 ... 203:
      direction = "S";
      break;

    case 204 ... 248:
      direction = "SW";
      break;

    case 249 ... 293:
      direction = "W";
      break;

    case 294 ... 336:
      direction = "NW";
      break;

    default:
      // if nothing else matches, do the default
      // default is optional
      direction = "?";
      break;
  }
  return direction;
}

