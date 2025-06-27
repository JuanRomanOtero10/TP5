#include <ESP32Time.h>
#include "time.h"

#include <ExampleFunctions.h>
#include <FirebaseClient.h>
#include <U8g2lib.h>
#include "DHT.h"
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
#include <WiFi.h>
#include <WiFiClientSecure.h>

#define WIFI_SSID "REPLACE_WITH_YOUR_SSID"
#define WIFI_PASSWORD "REPLACE_WITH_YOUR_PASSWORD"

#define Web_API_KEY "AIzaSyCsmgOA_Z-4aWB-rgC5ng6ZDS0gqpBBxiU"
#define DATABASE_URL "https://tp5-firebase-cc267-default-rtdb.firebaseio.com/"
#define USER_EMAIL "48417781@est.ort.edu.ar"
#define USER_PASS "Leomessi10"


void printBMP_OLED(void );
void printBMP_OLED2(void) ;

#define P1 0
#define P2 1
#define RST 20
#define ESPERA1 2
#define ESPERA2 3
#define AUMENTAR 4
#define RESTAR 5
int estado = RST;
#define BOTON1 34
#define BOTON2 35
#define LED 25
#define DHTPIN 23
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
float temp;
int valorU;
int millis_valor;
int millis_actual;
int millis_valor_temp;
int millis_actual_temp;

const char* ssid = "MECA-IoT";
const char* password = "IoT$2025";


void processData(AsyncResult &aResult);
UserAuth user_auth(Web_API_KEY, USER_EMAIL, USER_PASS);
WiFiClientSecure client;
FirebaseApp app;
using AsyncClient = AsyncClientClass;
AsyncClient aClient(client);
RealtimeDatabase Database;
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 10000;
String uid;
String databasePath;
String tempPath = "/temperature";
String timePath = "/timestamp";
String parentPath;
unsigned long timestamp;
const char* ntpServer = "pool.ntp.org";

object_t jsonData, obj1, obj2;
JsonWriter writer;
unsigned long Tiempo = 30000;
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return (0);
  }
  time(&now);
  return now;
}

void setup() {
  pinMode(LED, OUTPUT);
  pinMode(BOTON1, INPUT);
  pinMode(BOTON2, INPUT);
  Serial.begin(115200);
  Serial.println(F("DHTxx test!"));
  u8g2.begin();
  dht.begin();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  configTime(0, 0, ntpServer);
  client.setInsecure();
  client.setConnectionTimeout(1000);
  client.setHandshakeTimeout(5);

  // Initialize Firebase
  initializeApp(aClient, app, getAuth(user_auth), processData, "🔐 authTask");
  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
}



void loop() {
  millis_actual_temp = millis();
  if (millis_actual_temp - millis_valor_temp >= 2000) {
    temp = dht.readTemperature();
    if (isnan(temp)) {
      Serial.println(F("Failed to read from DHT sensor!"));
      return;
    }
    switch (estado) {
      case RST:
        {
          millis_valor = millis();
          estado = P1;
        }
        break;
      case P1:
        {
          printBMP_OLED();
          if (digitalRead(BOTON1) == LOW && digitalRead(BOTON2) == LOW) {
            estado = ESPERA1;
          }
        }
        break;
      case ESPERA1:
        {
          if (digitalRead(BOTON1) == HIGH && digitalRead(BOTON2) == HIGH) {
            estado = P2;
          }

        }
        break;
      case P2:
        {
          printBMP_OLED2();
          if (digitalRead(BOTON1) == LOW) {
            estado = SUMAR;
          }
          if (digitalRead(BOTON2) == LOW) {
            estado = RESTAR;
          }
          if (digitalRead(BOTON1) == LOW && digitalRead(BOTON2) == LOW) {
            estado = ESPERA2;
          }
        }
        break;
      case ESPERA2:
        {
          if (digitalRead(BOTON1) == HIGH && digitalRead(BOTON2) == HIGH) {
            estado = P1;
          }
        }
        break;

      case SUMAR:
        {
          if (digitalRead(BOTON2) == LOW) {
            estado = ESPERA2;
          }
          if (digitalRead(BOTON1) == HIGH) {
            Tiempo = Tiempo + 30000;
            estado = P2;
          }
        }
        break;

      case RESTAR:
        {
          if (digitalRead(BOTON1) == LOW) {
            estado = ESPERA2;
          }
          if (digitalRead(BOTON2) == HIGH) {
            if  (Tiempo > 30000) {
              Tiempo = Tiempo - 30000;
            }
            estado = P2;
          }
        }
        break;
    }

  }


  timestamp = getTime();
  parentPath = "/UsersData/" + uid + "/readings/" + String(timestamp);

  uid = app.getUid().c_str();


  writer.create(obj1, "/temperature", temp);
  writer.create(obj2, "/timestamp", timestamp);
  writer.join(jsonData, 2, obj1, obj2);
  Database.set<object_t>(aClient, parentPath, jsonData, processData, "RTDB_Send_Data");
}



void printBMP_OLED(void) {
  char stringU[5];
  char stringtemp[5];
  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_t0_11b_tr); // choose a suitable font
  sprintf (stringtemp, "%.2f" , temp); ///convierto el valor float a string
  sprintf (stringU, "%d" , valorU); ///convierto el valor float a string
  u8g2.drawStr(0, 35, "T. Actual:");
  u8g2.drawStr(60, 35, stringtemp);
  u8g2.drawStr(90, 35, " C");
  u8g2.drawStr(0, 50, "V. Umbral:");
  u8g2.drawStr(60, 50, stringU);
  u8g2.drawStr(75, 50, " C");
  u8g2.sendBuffer();          // transfer internal memory to the display
}

void printBMP_OLED2(void) {
  char stringT[5];
  u8g2.clearBuffer();          // clear the internal memory
  sprintf (stringU, "%d" , Tiempo);
  u8g2.setFont(u8g2_font_t0_11b_tr); // choose a suitable font
  u8g2.drawStr(0, 50, "Tiempo de espera:");
  u8g2.drawStr(60, 50, stringT);
  u8g2.sendBuffer();          // transfer internal memory to the display
}
