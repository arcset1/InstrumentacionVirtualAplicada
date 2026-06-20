#define ENABLE_USER_AUTH
#define ENABLE_DATABASE

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <FirebaseClient.h>


// ================= WIFI =================

#define WIFI_SSID     "Obi_WAN_Kenobi"
#define WIFI_PASSWORD "quesadilla"


// ================= FIREBASE =================

#define Web_API_KEY  "AIzaSyDSixv1xRVOaY1WZ4f218YCE4-h2-AHJ-s"
#define DATABASE_URL "https://set-firebase-proyecto-default-rtdb.firebaseio.com"
#define USER_EMAIL   "cieloycirros001@gmail.com"
#define USER_PASS    "quesadilla"


void processData(AsyncResult &aResult);

UserAuth user_auth(Web_API_KEY, USER_EMAIL, USER_PASS);

FirebaseApp app;
WiFiClientSecure ssl_client;

using AsyncClient = AsyncClientClass;
AsyncClient aClient(ssl_client);

RealtimeDatabase Database;


// ================= VARIABLES PID =================

float SetPoint = 0;
float Kp       = 0;
float Ki       = 0;
float Kd       = 0;

float error            = 0;
float errorAnterior    = 0;
float integral         = 0;
float derivadaFiltrada = 0;

float Ts = 0.05;

int OutputPWM = 0;

const float ALPHA_D     = 0.1;
const float ZONA_MUERTA = 2.0;


// ================= ENCODER =================

const int pinA = 2;
const int pinB = 4;

volatile long contador = 0;

float PosicionActual = 0;


// ================= PWM =================

const int pinPWM_A = 26;   // giro horario   (error positivo)
const int pinPWM_B = 27;   // giro antihorario (error negativo)

const int frecuenciaPWM = 5000;
const int resolucionPWM = 8;


// ================= TIMERS =================

unsigned long lastFirebase = 0;
const unsigned long firebaseInterval = 500;

unsigned long lastPID = 0;


// ================= PROTOTIPOS =================

void IRAM_ATTR leerEncoder();
void leerFirebase();
void enviarFirebase();
void PID();


// =================================================
// SETUP
// =================================================

void setup()
{
    Serial.begin(115200);

    // --- WiFi ---
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(300);
        Serial.print(".");
    }

    Serial.println();
    Serial.println("WiFi conectado");

    // --- SSL ---
    ssl_client.setInsecure();
    ssl_client.setConnectionTimeout(1000);
    ssl_client.setHandshakeTimeout(5);

    // --- Firebase ---
    initializeApp(aClient, app, getAuth(user_auth), processData, "AuthTask");
    app.getApp<RealtimeDatabase>(Database);
    Database.url(DATABASE_URL);

    // --- Encoder ---
    pinMode(pinA, INPUT_PULLUP);
    pinMode(pinB, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(pinA), leerEncoder, RISING);

    // --- Motor (dos canales PWM) ---
    ledcAttach(pinPWM_A, frecuenciaPWM, resolucionPWM);
    ledcAttach(pinPWM_B, frecuenciaPWM, resolucionPWM);
    ledcWrite(pinPWM_A, 0);
    ledcWrite(pinPWM_B, 0);
}


// =================================================
// LOOP
// =================================================

void loop()
{
    app.loop();

    if (app.ready())
    {
        leerFirebase();

        if (millis() - lastPID >= 50)
        {
            lastPID = millis();
            PID();
        }

        if (millis() - lastFirebase >= firebaseInterval)
        {
            lastFirebase = millis();
            enviarFirebase();
        }
    }
}


// =================================================
// LEER FIREBASE
// =================================================

void leerFirebase()
{
    SetPoint = Database.get<float>(aClient, "/Control/LabView/SetPoint");
    Kp       = Database.get<float>(aClient, "/Control/LabView/Kp");
    Ki       = Database.get<float>(aClient, "/Control/LabView/Ki");
    Kd       = Database.get<float>(aClient, "/Control/LabView/Kd");

    Serial.println("------ FIREBASE ------");
    Serial.print("SetPoint: "); Serial.println(SetPoint);
    Serial.print("Kp: ");       Serial.println(Kp);
    Serial.print("Ki: ");       Serial.println(Ki);
    Serial.print("Kd: ");       Serial.println(Kd);
}


// =================================================
// PID
// =================================================

void PID()
{
    // --- Posición actual en grados ---
    PosicionActual = (360.0 * contador) / 378.0;

    // --- Error ---
    error = SetPoint - PosicionActual;

    // --- Zona muerta ---
    if (abs(error) < ZONA_MUERTA)
        error = 0;

    // --- Derivada filtrada ---
    float derivadaCruda = (error - errorAnterior) / Ts;
    derivadaFiltrada    = ALPHA_D * derivadaCruda + (1.0 - ALPHA_D) * derivadaFiltrada;

    // --- Salida PID ---
    float salidaPID = (Kp * error)
                    + (Ki * integral)
                    + (Kd * derivadaFiltrada);

    // --- Limitar salida ---
    float salidaLimitada = constrain(salidaPID, -255.0, 255.0);

    // --- Anti-windup ---
    bool saturado = (abs(salidaPID) >= 255.0);
    if (!saturado)
        integral += error * Ts;

    errorAnterior = error;

    // --- PWM por dirección ---
    OutputPWM = (int)abs(salidaLimitada);

    if (salidaLimitada >= 0)
    {
        ledcWrite(pinPWM_A, OutputPWM);  // D26 activo — giro horario
        ledcWrite(pinPWM_B, 0);
    }
    else
    {
        ledcWrite(pinPWM_A, 0);
        ledcWrite(pinPWM_B, OutputPWM);  // D27 activo — giro antihorario
    }

    // --- Monitor serial ---
    Serial.println("------ PID ------");
    Serial.print("Posicion:   "); Serial.println(PosicionActual);
    Serial.print("SetPoint:   "); Serial.println(SetPoint);
    Serial.print("Error:      "); Serial.println(error);
    Serial.print("Integral:   "); Serial.println(integral);
    Serial.print("Derivada:   "); Serial.println(derivadaFiltrada);
    Serial.print("Salida PID: "); Serial.println(salidaLimitada);
    Serial.print("Saturado:   "); Serial.println(saturado ? "SI" : "NO");
    Serial.print("PWM:        "); Serial.println(OutputPWM);
    Serial.println(salidaLimitada >= 0 ? "D26 activo — HORARIO" : "D27 activo — ANTIHORARIO");
    Serial.println("-----------------");
}


// =================================================
// ENVIAR A FIREBASE
// =================================================

void enviarFirebase()
{
    Database.set<int>  (aClient, "/Control/OutputPWM",            OutputPWM);
    Database.set<float>(aClient, "/Control/PosicionActual",       PosicionActual);
    Database.set<long> (aClient, "/Control/ContadorInterrupcion", contador);
}


// =================================================
// CALLBACK FIREBASE
// =================================================

void processData(AsyncResult &aResult)
{
    if (!aResult.isResult()) return;

    if (aResult.isError())
    {
        Firebase.printf(
            "Error Firebase: %s\n",
            aResult.error().message().c_str()
        );
    }
}


// =================================================
// INTERRUPCIÓN ENCODER
// =================================================

void IRAM_ATTR leerEncoder()
{
    if (digitalRead(pinB) == LOW)
        contador++;
    else
        contador--;
}

