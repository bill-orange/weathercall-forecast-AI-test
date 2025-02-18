/* 
William E. Webb (c) MIT LICENSE Test program for using AI with weather forecasting
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
 02/10/2024 Fork to rely on AI to decode JSON Strings
 02/18/2025 Added MIT License
*/

#include <WiFi.h>         // WiFi for ESP32
#include <WiFiMulti.h>    // Setup for multiple ssid
#include <ArduinoJson.h>  // Needed for weathercall
#include <weathercall.h>  // Gets curent weather and forecast from OpenWeather
#include <ChatGPTuino.h>  // For AI Support

#include <TFT_eSPI.h>       // Graphic driver for multiple drivers and display
TFT_eSPI tft = TFT_eSPI();  // Instance of display driver
#include <TFT_eWidget.h>    // Widget library

#include "credentials.h"  // Network name, password, and private API key

SET_LOOP_TASK_STACK_SIZE(12 * 1024);  // needed to handle really long strings

#define GFXFF 1                               // No idea what this is.  Taken from library sample
GraphWidget gr = GraphWidget(&tft);           // Graph widget
TraceWidget tr1 = TraceWidget(&gr);           // Traces are drawn on tft using graph instance
#define TIME_Between_Weather_Calls (3600000)  // Every Hour
#define WIFI_SSID ssid                        //ssid
#define WIFI_PASSWORD password                // password
#define __FILENAME__ (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)

String Key = apiKeyOpenWeather;                                             // openweathermap free key
String latitude = LAT;                                                      // 90.0000 to -90.0000 negative for Southern hemisphere
String longitude = LON;                                                     // 180.000 to -180.000 negative for West
String units = Units;                                                       // or "imperial"
String language = Language;                                                 // See notes tab
char *AIShortReply = "                                                  ";  // get forecast

const int TOKENS = 174;             // How lengthy a response you want, every token is about 3/4 a word
const int NUM_MESSAGES = 30;        // Set some restrictions on the AI
const char *model = "gpt-4o-mini";  // OpenAI Model being used
int indexData = 0;                  // Used fro plotting points on the graph

weatherData w;                              // Instance of weathercall
Weathercall weather(Key, "Orange,us");      // Probably should put Orange, US in credentials
Weathercall forecast(Key, "Orange,us", 1);  // Maybe should use Villa Park since there is an Orange, Florida

WiFiMulti wifiMulti;                       // WiFi multi-connection instance
ChatGPTuino chat{ TOKENS, NUM_MESSAGES };  // Will store and send your most recent messages (up to NUM_MESSAGES)

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

  DrawGraph();  // Literally draws the graph

  weather.updateStatus(&w);   // Fetches weather
  forecast.updateStatus(&w);  // Fetches forecast
  AIForecast();               // AI fills in blanks in forecast

  tr1.startTrace(TFT_WHITE);                                             // Init line draw
  tft.setTextDatum(TC_DATUM);                                            //  No idea what this does
  for (indexData = 0; indexData <= sizeof(timeData - 1); indexData++) {  //Plot all the points
    tr1.addPoint(timeData[indexData], tempData[indexData]);
    if (indexData < sizeof(timeData )) tft.drawNumber(tempData[indexData], gr.getPointX(timeData[indexData]) + 5, gr.getPointY(tempData[indexData] + 5));
  }
  tft.drawString("Circled Points are A.I. Generated", 175, 40);                                                // added legend
  tft.drawCircle(gr.getPointX(timeData[indexData - 4]), gr.getPointY(tempData[indexData - 4]), 5, TFT_GREEN);  // this and below make the little green circles
  tft.drawCircle(gr.getPointX(timeData[indexData - 3]), gr.getPointY(tempData[indexData - 3]), 5, TFT_GREEN);
  tft.drawCircle(gr.getPointX(timeData[indexData - 2]), gr.getPointY(tempData[indexData - 2]), 5, TFT_GREEN);
  tft.drawCircle(gr.getPointX(timeData[indexData - 1]), gr.getPointY(tempData[indexData - 1]), 5, TFT_GREEN);
  yield();
  if (tempData[0] != 0 && tempData[1] != 0) delay(TIME_Between_Weather_Calls);  // Recycle on error
}

/*------------------------ Send weather info to serial port -----------------*/
void AIForecast() {
  // Create the structures that hold the retrieved weather

  String realUserMessage = "I would like you to give me a forecast temperature for 1, 2, 3 and 4 hours in the future. Do NOT provide narrative or discussion, just the 1, 2, 3 and 4 hour. You response MUST use the format xxx.x xxx.x xxx.x xxx.x and it must not contain anything else Current conditions are provided in this JSON String: "
                           + String(forecast.getResponse().c_str())
                           + " Future forecast weather is provided in this JSON String"
                           + String(weather.getResponse().c_str());  // User message to ChatGPT


  Serial.println(realUserMessage);
  int str_len = realUserMessage.length() + 1;
  char AIPrompt[str_len];
  realUserMessage.toCharArray(AIPrompt, str_len);
  AIShortReply = GetAIReply(AIPrompt);
  Serial.println(AIShortReply);

  // ********************************** Record data for Graph AI refuses to use Fahrenheit

  sscanf(AIShortReply, "%f %f %f %f", &tempData[1], &tempData[2], &tempData[3], &tempData[4]);  // AI told me to use sscanf
                                                                                                // Conversion
  tempData[0] = celsiusToFahrenheit(w.current_Temp);
  tempData[1] = celsiusToFahrenheit(tempData[1]);
  tempData[2] = celsiusToFahrenheit(tempData[2]);
  tempData[3] = celsiusToFahrenheit(tempData[3]);
  tempData[4] = celsiusToFahrenheit(tempData[4]);

  Serial.println(tempData[0]);  // For debug
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
  tft.drawString(__FILENAME__, 10, 90);
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
/*----------------------------------- Conversion --------------------------------------*/
float celsiusToFahrenheit(float celsius) {
  return (celsius * 9.0 / 5.0) + 32.0;
}

/*------------------------------------------------------------------------------------*/
