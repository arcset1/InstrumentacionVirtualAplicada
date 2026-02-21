// -------------------- Recepción LabVIEW --------------------
char data_labview;
String datoslabview;

String analog1;
String analog2;
String analog3;

int analog_out1;
int analog_out2;
int analog_out3;

// -------------------- Entradas digitales --------------------
int pin1 = 6;
int pin2 = 7;
int pin3 = 8;

// -------------------- Salidas digitales --------------------
int pin4 = 4;
int pin5 = 3;
int pin6 = 9;

// -------------------- Salidas PWM --------------------
int pin_an1 = 3;
int pin_an2 = 4;
int pin_an3 = 5;

// -------------------- Lecturas analógicas --------------------
int valorA0;
int valorA1;
int valorA2;

// -------------------- Lecturas digitales --------------------
int val_pin1;
int val_pin2;
int val_pin3;

// -------------------- Encoder --------------------
const int pinA = 2;   // interrupción. Arduino solo tiene para 2 interrupciones externas. en el D2 y D3
const int pinB = 10;  // Aqui va conectado el canal B, si se requieren mas de 2 Interrupciones usar Interrupciones por cambio de Pin

volatile long contador = 0;

void setup()
{
  Serial.begin(9600);

  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);

  pinMode(pin_an1, OUTPUT);
  pinMode(pin_an2, OUTPUT);
  pinMode(pin_an3, OUTPUT);

  pinMode(pin1, INPUT);
  pinMode(pin2, INPUT);
  pinMode(pin3, INPUT);

  pinMode(pin4, OUTPUT);
  pinMode(pin5, OUTPUT);
  pinMode(pin6, OUTPUT);

  pinMode(pinA, INPUT_PULLUP);
  pinMode(pinB, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(pinA), leerEncoder, RISING);
}

void loop()
{
  if (Serial.available())
  {
    data_labview = Serial.read();
    datoslabview += data_labview;

    if (data_labview == '\n')
    {
      // ----------- Salidas digitales -----------
      if (datoslabview[0] == 'a') digitalWrite(pin6, HIGH);
      else digitalWrite(pin6, LOW);

      if (datoslabview[1] == 'b') digitalWrite(pin5, HIGH);
      else digitalWrite(pin5, LOW);

      if (datoslabview[2] == 'c') digitalWrite(pin4, HIGH);
      else digitalWrite(pin4, LOW);

      // ----------- PWM -----------
      for (int i = 3; i < 6; i++)
      {
        analog1 += datoslabview[i];
        analog2 += datoslabview[i + 3];
        analog3 += datoslabview[i + 6];
      }

      analog_out1 = analog1.toInt();
      analog_out2 = analog2.toInt();
      analog_out3 = analog3.toInt();

      analogWrite(A3, analog_out1 - 100);
      analogWrite(A4, analog_out2 - 100);
      analogWrite(A5, analog_out3 - 100);

      analog1 = "";
      analog2 = "";
      analog3 = "";
      datoslabview = "";

      // ----------- Lecturas -----------
      valorA0 = 1000 + analogRead(A0);
      valorA1 = 1000 + analogRead(A1);
      valorA2 = 1000 + analogRead(A2);

      val_pin1 = digitalRead(pin3);
      val_pin2 = digitalRead(pin2);
      val_pin3 = digitalRead(pin1);

      // ----------- Envío a LabVIEW -----------
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
      Serial.println(contador);   // Se agrega encoder

      //delay(50);
    }
  }
}

// -------------------- Interrupción Encoder --------------------
void leerEncoder()
{
  if (digitalRead(pinB) == LOW)
  {
    contador++;   // A adelanta a B
  }
  else
  {
    contador--;   // B adelanta a A
  }

  if(contador > 10000){
      contador = 0;
  }

  if(contador < -10000){
      contador = 0;
  }

//La idea es que al activrse la interrupcion es porque A=1, y si leemos B, y B=0, significa que A adelanta a B
//Por lo tanto, Esta girando en sentido Horario.

}