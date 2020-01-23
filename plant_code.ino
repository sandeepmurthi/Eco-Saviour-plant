#include <ESP8266WiFi.h>
#include <DHT.h>
#include <SPI.h>               // include Arduino SPI library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         // https://github.com/tzapu/WiFiManager
#include <Adafruit_GFX.h>      // include adafruit graphics library
#include <Adafruit_PCD8544.h>  // include adafruit PCD8544 (Nokia 5110) library
#include "ThingSpeak.h"
#include "secrets.h"


// Set web server port number to 80
WiFiServer server(80);


#define DHTPIN  D5           // DHT11 data pin is connected to NodeMCU pin D5
#define DHTTYPE DHT11        // DHT11 sensor is used
DHT dht11(DHTPIN, DHTTYPE);  // configure DHT library
// Nokia 5110 LCD module connections (CLK, DIN, D/C, CS, RST)
Adafruit_PCD8544 display = Adafruit_PCD8544(D4, D3, D2, D1, D0);

WiFiClient  client;

unsigned long myChannelNumber = SECRET_CH_ID;
const char * myWriteAPIKey = SECRET_WRITE_APIKEY;

static  int pm_at_25;
static  int pm_at_100;

static int Humidity;
static int Temperature;
static int Air_qualify;

static int time_delay = 25;
static int timer_count = 0;


String myStatus = "";

void setup() {

  // initialize the display
  display.begin();
  display.setContrast(60);
  display.clearDisplay();   // clear the screen and buffer
  display.setTextSize(1);
  display.setTextColor(BLACK, WHITE);
  display.setCursor(0, 0);
  display.print("MATRUSRI");
  display.setCursor(0, 20);
  display.print("Engineering");
  display.setCursor(0, 40);
  display.print("College");

  display.display();
  delay(3000);

  WiFiManager wifiManager;
  //wifiManager.resetSettings();
  wifiManager.autoConnect("ECO SAVIOUR Plant");



  Serial.begin(9600);  // Initialize serial
  //Initialize Ticker every 0.5s
  // blinker.attach(20, post_thingspeak); //Use <strong>attach_ms</strong> if you need
  WiFi.mode(WIFI_STA);
  dht11.begin();
  ThingSpeak.begin(client);  // Initialize ThingSpeak
}

void loop() {

  // Connect or reconnect to WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(SECRET_SSID);
    while (WiFi.status() != WL_CONNECTED)
    {
      Serial.print(".");
      ESP.restart();
      //setup();
      //delay(5000);
    }
    Serial.println("\nConnected.");
  }
  while (Serial.available())
  {
    getG5(Serial.read());
  }

  Air_qualify = analogRead(A0);// read airquality sensor
  Humidity = dht11.readHumidity();// read humidity  from DTH11
  Temperature = dht11.readTemperature();// read temperature from DHT11
  timer_count = timer_count + 1;
  Serial.println(Humidity);
  Serial.println(Temperature);
  Serial.println(pm_at_25);
  Serial.println(pm_at_100);
  Serial.println(Air_qualify);
  Serial.println(timer_count);


  display.clearDisplay();   // clear the screen and buffer
  display.setTextSize(1);
  display.setTextColor(BLACK, WHITE);
  display.setCursor(6, 0);
  display.print("TEMPERATURE:");
  // print temperature (in °C)
  display.setCursor(18, 12);
  if (Temperature < 0)   // if temperature < 0
    display.printf("-%02u.%1u C", (abs(Temperature) / 10) % 100, abs(Temperature) % 10);
  else            // temperature >= 0
    display.printf(" %02u.%1u C", (Temperature) % 100, Temperature % 10);
  // print degree symbol ( ° )
  display.drawRect(50, 12, 3, 3, BLACK);

  display.setCursor(15, 28);
  display.print("HUMIDITY:");
  // print humidity (in %)
  display.setCursor(24, 40);
  display.printf("%02u.%1u %%", (Humidity) % 100, Humidity % 10);

  // now update the display
  display.display();
  delay(1000);
  timer_count = timer_count + 1;

  display.clearDisplay();   // clear the screen and buffer
  display.setTextSize(1);
  display.setTextColor(BLACK, WHITE);
  display.setCursor(6, 0);
  display.print("PM-2.5 AQI:");
  display.setCursor(30, 12);
  display.printf("%03u", (pm_at_25) % 100, pm_at_25 % 10);

  display.setTextSize(1);
  display.setTextColor(BLACK, WHITE);
  display.setCursor(15, 28);
  display.print("PM-10 AQI:");
  display.setCursor(30, 40);
  display.printf("%03u", (pm_at_100) % 100, pm_at_100 % 10);


  display.display();
  delay(1000);  // wait a second
  timer_count = timer_count + 1;

  display.clearDisplay();   // clear the screen and buffer
  display.setTextSize(1);
  display.setTextColor(BLACK, WHITE);
  display.setCursor(6, 0);
  display.print("Air Quality :");
  display.setCursor(30, 12);
  display.printf("%03u", (Air_qualify) % 100, Air_qualify % 10);

  display.display();
  delay(1000);  // wait a second
  timer_count = timer_count + 1;


  if (timer_count >= time_delay)
  {

    // set the fields with the values
    ThingSpeak.setField(1, Temperature);
    ThingSpeak.setField(2, Humidity);
    ThingSpeak.setField(3, pm_at_25);
    ThingSpeak.setField(4, pm_at_100);
    ThingSpeak.setField(5, Air_qualify);
    // set the status
    ThingSpeak.setStatus(myStatus);
    // write to the ThingSpeak channel
    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    if (x == 200) {
      Serial.println("Channel update successful.");
    }
    else {
      Serial.println("Problem updating channel. HTTP error code " + String(x));
    }
    timer_count = 0;
    //delay(1000);
  }

  // delay(1000); // Wait 20 seconds to update the channel again
}

void getG5(unsigned char ucData)
{
  static unsigned int ucRxBuffer[250];
  static unsigned int ucRxCnt = 0;
  ucRxBuffer[ucRxCnt++] = ucData;
  if (ucRxBuffer[0] != 0x42 && ucRxBuffer[1] != 0x4D)
  {
    ucRxCnt = 0;
    return;
  }
  if (ucRxCnt > 38)
  {
    pm_at_25 = (int)ucRxBuffer[12] * 256 + (int)ucRxBuffer[13];
    pm_at_100 = (int)ucRxBuffer[14] * 256 + (int)ucRxBuffer[15];
    ucRxCnt = 0;
    return;
  }
}

