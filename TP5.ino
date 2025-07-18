
// Juan Román Otero - Octaviano Sznajdleder - Adam Bairros    Grupo1
#define ENABLE_USER_AUTH
#define ENABLE_DATABASE
#include <ESP32Time.h>
#include "time.h"
#include <FirebaseClient.h>
#include <ExampleFunctions.h>
#include <U8g2lib.h>
#include "DHT.h"
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Wire.h>


#define WIFI_SSID "MECA-IoT"
#define WIFI_PASSWORD "IoT$2025"
#define Web_API_KEY "AIzaSyCsmgOA_Z-4aWB-rgC5ng6ZDS0gqpBBxiU"
#define DATABASE_URL "https://tp5-firebase-cc267-default-rtdb.firebaseio.com/"
#define USER_EMAIL "48417781@est.ort.edu.ar"
#define USER_PASS "Leomessi10"


void printBMP_OLED(void );
void printBMP_OLED2(void);

#define P1 0
#define P2 1
#define RST 20
#define ESPERA1 2
#define ESPERA2 3
#define SUMAR 4
#define RESTAR 5
int estado = RST;
#define BOTON1 34
#define BOTON2 35
#define LED 25
#define DHTPIN 23
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
float temp;
int valorU =  23 ;
int millis_valor;
int millis_actual;
int millis_valor_temp;
int millis_actual_temp;

const char* ssid = "MECA-IoT";
const char* password = "IoT$2025";


void processData(AsyncResult &aResult);             // Declaración anticipada de la función `processData`, para manejar la respuesta de Firebase después de una operación
UserAuth user_auth(Web_API_KEY, USER_EMAIL, USER_PASS);     // Crea un objeto de autenticación de usuario para Firebase, usando la API key, email y contraseña definidos
WiFiClientSecure client;                                    // Cliente seguro para conexiones HTTPS (requerido por Firebase)
FirebaseApp app;                                            // Objeto principal de la aplicación Firebase
using AsyncClient = AsyncClientClass;
AsyncClient aClient(client);
RealtimeDatabase Database;                                // Instancia del objeto que representa la base de datos en tiempo real de Firebase
unsigned long lastSendTime = 0;
String uid;                                               // ID del usuario autenticado
String databasePath;                                      // Ruta completa del nodo donde se escriben los datos
String tempPath = "/temperature";
String timePath = "/timestamp";
String parentPath;
unsigned long timestamp;                                  // Marca de tiempo actual
const char* ntpServer = "pool.ntp.org";                   // Dirección del servidor NTP (para obtener la hora actual desde internet)

object_t jsonData, obj1, obj2;                            // Se declaran tres objetos JSON de tipo `object_t`, que se usarán para construir la estructura de datos:
JsonWriter writer;                                       // Se crea una instancia de `JsonWriter`, que es la clase encargada de construir y unir objetos JSON
unsigned long Tiempo = 30000;
unsigned long getTime() {             // Función que obtiene el tiempo actual en formato timestamp (segundos desde 1970)
  time_t now;                         // Variable para almacenar el tiempo en formato epoch (timestamp)
  struct tm timeinfo;                 // Estructura para almacenar información detallada de la hora (día, mes, año, etc.)
  if (!getLocalTime(&timeinfo)) {             // Si falla la obtención de la hora, muestra un mensaje de error en el monitor serie
    Serial.println("Failed to obtain time");
    return (0);                       // Devuelve 0 si no se pudo obtener la hora
  }
  time(&now);                          // Si se obtuvo la hora correctamente, se guarda el tiempo actual en 'now'
  return now;                         // Devuelve el timestamp en segundos
}

void processData(AsyncResult &aResult) {      // Función callback que se ejecuta después de una operación Firebase (lectura/escritura)
  if (!aResult.isError()) {                   //Si no hubo error
    Serial.println("Firebase: operación exitosa");    //printea que fue exitoso
  } else {                                            // si hay error
    Serial.print("Firebase error: ");                 //printea
    Serial.println(aResult.error().message());        //te muestra el error
  }
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
  initializeApp(aClient, app, getAuth(user_auth), processData, "🔐 authTask");      //inicializa la app con el cliente, la instancia de la app, los datos del usuario, processdata, una etiqueta para autenticacion
  app.getApp<RealtimeDatabase>(Database);                                         // Extrae desde la app la instancia correspondiente a la base de datos en tiempo real (Realtime Database)y la guarda en el objeto `Database`
  Database.url(DATABASE_URL);                                                     // Asigna la URL de la base de datos, para que sepa dónde enviar y leer datos.

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
}



void loop() {
  app.loop();
  if (app.ready()) {
    millis_actual_temp = millis();                                      
    if (millis_actual_temp - millis_valor_temp >= 2000) {   
      temp = dht.readTemperature();                                   
      if (isnan(temp)) {                                        
        Serial.println(F("Failed to read from DHT sensor!"));     
        return;
      }
    }
    unsigned long currentMillis = millis();                         // Guarda el tiempo actual
    if (currentMillis - lastSendTime >= Tiempo) {                   // millis_valor es el intervalo (ej: 30000)
      lastSendTime = currentMillis;                                 //guarda el ultimo tiempo
      timestamp = getTime();                                        // Obtiene el tiempo actual en formato timestamp (segundos desde 1970)
      uid = app.getUid().c_str();                                    // Obtiene el UID del usuario autenticado en Firebase
      parentPath = "/UsersData/" + uid + "/readings/" + String(timestamp);    // Construye la ruta donde se guardarán los datos en Firebase:

      writer.create(obj1, "/temperature", temp);                              // Crea un objeto JSON con el dato de temperatura
      writer.create(obj2, "/timestamp", timestamp);                          // Crea otro objeto JSON con el timestamp actual
      writer.join(jsonData, 2, obj1, obj2);                                 // Junta ambos objetos en un único objeto JSON (`jsonData`)
      Database.set<object_t>(aClient, parentPath, jsonData, processData, "RTDB_Send_Data");     // Envía el objeto JSON a Firebase, en la ruta definida como `parentPath`Usa `processData` para saber si la operación fue exitosa
    }



    switch (estado) {
      case RST:
        millis_valor = millis();
        estado = P1;
        break;

      case P1:
        printBMP_OLED();
        if (digitalRead(BOTON1) == LOW && digitalRead(BOTON2) == LOW) {
          estado = ESPERA1;
        }
        break;

      case ESPERA1:
        if (digitalRead(BOTON1) == HIGH && digitalRead(BOTON2) == HIGH) {
          estado = P2;
        }

        break;

      case P2:
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
        break;

      case ESPERA2:
        if (digitalRead(BOTON1) == HIGH && digitalRead(BOTON2) == HIGH) {
          estado = P1;
        }
        break;

      case SUMAR:
        if (digitalRead(BOTON2) == LOW) {
          estado = ESPERA2;
        }
        if (digitalRead(BOTON1) == HIGH) {
          Tiempo = Tiempo + 30000;
          estado = P2;
          Serial.print("El tiempo de espera es: ");
          Serial.println(Tiempo);
        }
        break;

      case RESTAR:
        if (digitalRead(BOTON1) == LOW) {
          estado = ESPERA2;
        }
        if (digitalRead(BOTON2) == HIGH) {
          if  (Tiempo > 30000) {
            Tiempo = Tiempo - 30000;
          }
          Serial.print("El tiempo de espera es: ");
          Serial.println(Tiempo);
          estado = P2;
        }
        break;
    }
  }
}


void printBMP_OLED(void) {
  char stringU[5];
  char stringtemp[6];
  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_t0_11b_tr); // choose a suitable font
  sprintf (stringtemp, "%.2f" , temp); ///convierto el valor float a string
  sprintf (stringU, "%d" , valorU); ///convierto el valor float a string
  u8g2.drawStr(0, 35, "T. Actual:");
  u8g2.drawStr(60, 35, stringtemp);
  u8g2.drawStr(90, 35, " C");
  u8g2.drawStr(0, 50, "V. Umbral:");
  u8g2.drawStr(60, 50, stringU);
  u8g2.drawStr(70, 50, " C");
  u8g2.sendBuffer();          // transfer internal memory to the display
}

void printBMP_OLED2(void) {
  char stringT[10];
  u8g2.clearBuffer();          // clear the internal memory
  sprintf (stringT, "%lu" , Tiempo / 1000);
  u8g2.setFont(u8g2_font_t0_11b_tr); // choose a suitable font
  u8g2.drawStr(0, 50, "Tiempo espera: ");
  u8g2.drawStr(85, 50, stringT);
  u8g2.sendBuffer();          // transfer internal memory to the display
}
