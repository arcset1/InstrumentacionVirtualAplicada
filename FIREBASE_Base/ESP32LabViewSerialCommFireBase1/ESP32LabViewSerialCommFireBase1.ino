#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>

// -------------------- WIFI --------------------

#define WIFI_SSID "Obi_WAN_Kenobi"
#define WIFI_PASSWORD "quesadilla"

// -------------------- FIREBASE --------------------

#define Web_API_KEY "AIzaSyDSixv1xRVOaY1WZ4f218YCE4-h2-AHJ-s"
#define DATABASE_URL "https://set-firebase-proyecto-default-rtdb.firebaseio.com"
#define USER_EMAIL "cieloycirros001@gmail.com"
#define USER_PASS "quesadilla"

void processData(AsyncResult &aResult);

UserAuth user_auth(Web_API_KEY, USER_EMAIL, USER_PASS);

FirebaseApp app;
WiFiClientSecure ssl_client;

using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);

RealtimeDatabase Database;

// -------------------- LabVIEW --------------------

char data_labview;
String datoslabview;

String analog1;
String analog2;
String analog3;

int analog_out1;
int analog_out2;
int analog_out3;

// -------------------- Variables Firebase --------------------

int SetPoint = 0;
int PWMActual = 0;

// -------------------- Entradas digitales --------------------

const int pin1 = 18;
const int pin2 = 19;
const int pin3 = 21;

// -------------------- Salidas digitales --------------------

const int pin4 = 22;
const int pin5 = 23;
const int pin6 = 25;

// -------------------- PWM --------------------

const int pin_an1 = 26;
const int pin_an2 = 27;
const int pin_an3 = 14;

const int frecuenciaPWM = 5000;
const int resolucionPWM = 8;

// -------------------- ADC --------------------

const int adc0 = 34;
const int adc1 = 35;
const int adc2 = 32;

// -------------------- Lecturas --------------------

int valorA0;
int valorA1;
int valorA2;

int val_pin1;
int val_pin2;
int val_pin3;

// -------------------- Encoder --------------------

const int pinA = 2;
const int pinB = 4;

volatile long contador = 0;

// -------------------- Timer Firebase --------------------

unsigned long lastFirebase = 0;
const unsigned long firebaseInterval = 3000;

// -------------------- Prototipos --------------------

void IRAM_ATTR leerEncoder();
void enviarDatosFirebase();

void setup()
{
  Serial.begin(9600);

  // WiFi

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(300);
  }

  // SSL

  ssl_client.setInsecure();
  ssl_client.setConnectionTimeout(1000);
  ssl_client.setHandshakeTimeout(5);

  // Firebase

  initializeApp(
      aClient,
      app,
      getAuth(user_auth),
      processData,
      "AuthTask");

  app.getApp<RealtimeDatabase>(Database);
  Database.url(DATABASE_URL);

  // ADC

  pinMode(adc0, INPUT);
  pinMode(adc1, INPUT);
  pinMode(adc2, INPUT);

  // Entradas

  pinMode(pin1, INPUT);
  pinMode(pin2, INPUT);
  pinMode(pin3, INPUT);

  // Salidas

  pinMode(pin4, OUTPUT);
  pinMode(pin5, OUTPUT);
  pinMode(pin6, OUTPUT);

  // Encoder

  pinMode(pinA, INPUT_PULLUP);
  pinMode(pinB, INPUT_PULLUP);

  attachInterrupt(
      digitalPinToInterrupt(pinA),
      leerEncoder,
      RISING);

  // PWM

  ledcAttach(pin_an1, frecuenciaPWM, resolucionPWM);
  ledcAttach(pin_an2, frecuenciaPWM, resolucionPWM);
  ledcAttach(pin_an3, frecuenciaPWM, resolucionPWM);

  ledcWrite(pin_an1, 0);
  ledcWrite(pin_an2, 0);
  ledcWrite(pin_an3, 0);
}

void loop()
{
  app.loop();

  // ---------- Comunicación LabVIEW ----------

  if (Serial.available())
  {
    data_labview = Serial.read();

    datoslabview += data_labview;

    if (data_labview == '\n')
    {
      if (datoslabview.length() >= 12)
      {
        digitalWrite(pin6, (datoslabview[0] == 'a'));
        digitalWrite(pin5, (datoslabview[1] == 'b'));
        digitalWrite(pin4, (datoslabview[2] == 'c'));

        analog1 = "";
        analog2 = "";
        analog3 = "";

        for (int i = 3; i < 6; i++)
        {
          analog1 += datoslabview[i];
          analog2 += datoslabview[i + 3];
          analog3 += datoslabview[i + 6];
        }

        analog_out1 = analog1.toInt();
        analog_out2 = analog2.toInt();
        analog_out3 = analog3.toInt();

        analog_out1 = constrain(analog_out1 - 100, 0, 255);
        analog_out2 = constrain(analog_out2 - 100, 0, 255);
        analog_out3 = constrain(analog_out3 - 100, 0, 255);

        ledcWrite(pin_an1, analog_out1);
        ledcWrite(pin_an2, analog_out2);
        ledcWrite(pin_an3, analog_out3);

        SetPoint = analog_out1;
        PWMActual = analog_out1;
      }

      valorA0 = 1000 + analogRead(adc0);
      valorA1 = 1000 + analogRead(adc1);
      valorA2 = 1000 + analogRead(adc2);

      val_pin1 = digitalRead(pin3);
      val_pin2 = digitalRead(pin2);
      val_pin3 = digitalRead(pin1);

      Serial.print(valorA0);
      Serial.print('\t');

      Serial.print(valorA1);
      Serial.print('\t');

      Serial.print(valorA2);
      Serial.print('\t');

      Serial.print(val_pin1);
      Serial.print('\t');

      Serial.print(val_pin2);
      Serial.print('\t');

      Serial.print(val_pin3);
      Serial.print('\t');

      Serial.println(contador);

      datoslabview = "";
    }
  }

  // ---------- Firebase cada 3 segundos ----------

  if (app.ready())
  {
    if (millis() - lastFirebase >= firebaseInterval)
    {
      lastFirebase = millis();
      enviarDatosFirebase();
    }
  }
}

// -------------------- Firebase --------------------

void enviarDatosFirebase()
{
  Database.set<int>(
      aClient,
      "/Control/SetPoint",
      SetPoint,
      processData,
      "SetPoint");

  Database.set<long>(
      aClient,
      "/Control/PosicionActual",
      contador,
      processData,
      "PosicionActual");

  Database.set<int>(
      aClient,
      "/Control/PWMActual",
      PWMActual,
      processData,
      "PWMActual");
}

// -------------------- Callback Firebase --------------------

void processData(AsyncResult &aResult)
{
  if (!aResult.isResult())
    return;

  if (aResult.isError())
  {
    Firebase.printf(
        "Error: %s\n",
        aResult.error().message().c_str());
  }
}

// -------------------- Encoder --------------------

void IRAM_ATTR leerEncoder()
{
  if (digitalRead(pinB) == LOW)
    contador++;
  else
    contador--;

  if (contador > 10000)
    contador = 0;

  if (contador < -10000)
    contador = 0;
}