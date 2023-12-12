#define TINY_GSM_MODEM_SIM800
#define TINY_GSM_RX_BUFFER 256
 
#include <LiquidCrystal_I2C.h>
#include <TinyGsmClient.h>
#include <ArduinoHttpClient.h>
#include <DHT.h>
#include <SoftwareSerial.h>
 
#define rxPin 0
#define txPin 2
 
SoftwareSerial sim800(txPin, rxPin);

#define DHTPIN 14 
DHT dht(DHTPIN, DHT11);

LiquidCrystal_I2C lcd (0x27, 16, 2);

const int sensor_pin = A0;

const char FIREBASE_HOST[]  = "sensor-3acd7-default-rtdb.firebaseio.com";
const String FIREBASE_AUTH  = "zsBLj76AdlWNb5nmjQOQvSQwG86ZAxOhNycH4uvG";
const String FIREBASE_PATH  = "sensor_suhu";
const int SSL_PORT          = 443;
 
char apn[]  = "internet";
char user[] = "wsp";
char pass[] = "wsp123";
 
 
TinyGsm modem(sim800);
 
TinyGsmClientSecure gsm_client_secure_modem(modem, 0);
HttpClient http_client = HttpClient(gsm_client_secure_modem, FIREBASE_HOST, SSL_PORT);
 
unsigned long previousMillis = 0;



void setup()
{

  lcd.backlight();
  
  
  Serial.begin(115200);
  dht.begin();

  Serial.print("device serial initialize");
 
  sim800.begin(9600);
  Serial.println("SIM800L serial initialize");
 
  Serial.println("Initializing modem...");
  modem.restart();
  String modemInfo = modem.getModemInfo();
  Serial.print("Modem: ");
  Serial.println(modemInfo);
 
  http_client.setHttpResponseTimeout(10 * 1000);
}
 
void loop()
{
 
  Serial.print(F("Connecting to "));
  Serial.print(apn);
  if (!modem.gprsConnect(apn, user, pass))
  {
    Serial.println(" fail");
    delay(500);
    return;
  }
  Serial.println(" OK");
 
  http_client.connect(FIREBASE_HOST, SSL_PORT);
 
  while (true) {
    if (!http_client.connected())
    {
      Serial.println();
      http_client.stop();
      Serial.println("HTTP  not connect");
      break;
    }
    else
    {
      dht_loop();
    }
  }
}
void PostToFirebase(const char* method, const String & path , const String & data, HttpClient* http)
{
  String response;
  int statusCode = 0;
  http->connectionKeepAlive();
 
  String url;
  if (path[0] != '/')
  {
    url = "/";
  }
  url += path + ".json";
  url += "?auth=" + FIREBASE_AUTH;
  Serial.print("POST:");
  Serial.println(url);
  Serial.print("Data:");
  Serial.println(data);
 
  String contentType = "application/json";
  http->put(url, contentType, data);
 
  statusCode = http->responseStatusCode();
  Serial.print("Status code: ");
  Serial.println(statusCode);
  response = http->responseBody();
  Serial.print("Response: ");
  Serial.println(response);
 
  if (!http->connected())
  {
    Serial.println();
    http->stop();// Shutdown
    Serial.println("HTTP POST disconnected");
  }
 
}
 
void dht_loop()
{ String moisture_percentage;
  moisture_percentage = ( 100.00 - ( (analogRead(sensor_pin)/1023.00) * 100.00 ) );
  String h = String(dht.readHumidity(), 2);
  String t = String(dht.readTemperature(), 2);
  delay(100);
  // lcd.createChar(1, kelembaban);
  // lcd.createChar(2, suhu);

  lcd.begin();
  lcd.backlight();

  lcd.setCursor(0, 0);
  // lcd.write(2);
  lcd.print("S= ");
  lcd.print(t);
  lcd.print(" °C");

  lcd.setCursor(9, 0);
  // lcd.write(2);
  lcd.print("H= ");
  lcd.print(h);
  lcd.print(" %");
  
  lcd.setCursor(0, 3);
  // lcd.write(2);
  lcd.print("Tanah= ");
  lcd.print(moisture_percentage);
  lcd.print(" %");

  Serial.print("Temperature = ");
  Serial.print(t);
  Serial.println(" °C");
  Serial.print("Humidity = ");
  Serial.print(h);
  Serial.println(" %");
  Serial.print("kelembapan tanah = ");
  Serial.print(moisture_percentage);
  Serial.println(" %");

  String Data = "{";
  Data += "\"temp\":" + t + ",";
 
  Data += "\"humid\":" + h + ",";

  Data += "\"kelembapan_tanah\":" + moisture_percentage + "";
  Data += "}";
 
  PostToFirebase("PATCH", FIREBASE_PATH, Data, &http_client);
}