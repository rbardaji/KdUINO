/*
  KdUINOProfile2017
 
  00/00/2017
  Carlos Rodero, Raul Bardaji

  Mejoras en el software y hardware de KdUINO para las medidas del perfil.
  Diseño con un boton pull-up tipo LED. Nos servira para indicar cuando tomar las medidas y si estas son correctas. Tambien tiene función de inicializarse (Reset).

  Leemos la frecuencia de salida de los sensores PAR.
  Los datos se guardan en una targeta SD (se usa el Datalogger Shield de Adafruit).
  Usamos el Real Clock Time del Datalogger Shield de Adafruit (DS1307 RTC) mediante la libreria necesaria para el Arduino Mega
  
  Los datos se guardan con el siguiente formato:
    # Start Time <Fecha y Hora UTC de inicio de medidas con formato AAAA-MM-DD-hh:mm:ss>, 
    # Measuring Time <MEASURING_TIME>, 
    # Mode <Buoy == 1, Profiler == 0>,
    # Count Measurements <countMeasurements>,
    # Sensors <PAR, RGB>   
    # Input pin of sensors: <numero de pin de entrada del sensor 1> <numero de pin de entrada del sensor 2> 
    .
    .
    .
    
  Parametros de configuracion:
    MEASURING_TIME: Cuanto tiempo dura la toma de la medida (en milisegundos)
*/

#include <Wire.h>
#include "RTClib.h"
#include <SD.h>
#include <SPI.h>

// INFORMACION DE METADATOS
// Profundidad de cada medida
#define DEPTH "0.5"

// Configuracion del baud rate de la comunicacion serie
#define BAUDRATE 9600

// Configuracion de los tiempos de medida en milisegundos
#define MEASURING_TIME 1000

// Configuracion de los sensores tipo PAR o RGB
#define SENSORS "PAR"

// Configuracion del modo Buoy (1) o Profiler (0)
#define MODE "PROFILER"

// Configuracion de los pines de recepcion de datos de los sensores TCS230
#define SENSOR_2 2
#define SENSOR_3 3

// Configuración de los pines Input y Output del LedButton
#define LED_PIN 8  
#define BUTTON_PIN 9

// Variable para comprobar que la SD esta conectada correctamente
bool sdOk = false;

// Variable para comprobar que el Real Clock Time funciona correctamente
bool rtcOk = false;

// Contadores de pulsos de los sensores
unsigned long pulseCnt2 = 0;
unsigned long pulseCnt3 = 0;
unsigned long value1;
unsigned long value2;

// Variable utilizada como contador de medidas
int countMeasurements;

// Variable para indicar el estado del boton
int buttonState = 0;

// Variable para guardar la fecha y hora actuales
char savedTime[20];

// pin for ARDUINO UNO SD
const int chipSelect = 10;

File dataFile;
RTC_DS1307 rtc;

void setup() 
{
  // Configuracion de pines
  pinMode(SENSOR_2, INPUT);
  pinMode(SENSOR_3, INPUT);
  pinMode(LED_PIN, OUTPUT); 
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Configuracion de interrupciones
  attachInterrupt(0, addPulse2, RISING);
  attachInterrupt(1, addPulse3, RISING);

  // Configuracion comunicacion serie
  Serial.begin(BAUDRATE);

  // Configuracion RTC 
  rtcOk = true;
  if (!rtc.begin()) 
  {
    rtcOk = false;  
    Serial.println("Couldn't find RTC");
    digitalWrite(LED_PIN, LOW);
    while(1);
  }
  // RTC se configura con fecha y hora en el momento que se compila el programa
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // Configuracion de targeta SD
  if (!SD.begin(chipSelect)) 
  {
    Serial.println("NO SD");
    digitalWrite(LED_PIN, LOW);
    while(1);
    
  }
  else
  {
    dataFile = SD.open("datalog.txt", FILE_WRITE);
    if (!dataFile) 
    {
      Serial.println("error opening datalog.txt");
      digitalWrite(LED_PIN, LOW);
    }
    else
    {
      // Escribimos la cabecera del archivo csv
      dataFile.println("date,period,mode,num,type,sensor1,sensor2");
      dataFile.flush();
      sdOk = true;
      digitalWrite(LED_PIN, HIGH);
    }
  }

  // Inicializacion de variables
  resetCounters();
  countMeasurements = 1; 
}

// Metodo para hacer un reset via Software
void (* resetFunc) (void) = 0;

void loop() {
  Serial.println("In the loop");
  // Guardamos el estado del boton
  buttonState = digitalRead(BUTTON_PIN);
  // Si el boton no esta presionado
  if (buttonState == LOW)
  {
    Serial.println("Button high!");
    // esperamos 1 segundo
    delay(1000);
    buttonState = digitalRead(BUTTON_PIN);
    if (buttonState == HIGH)
    {
      digitalWrite(LED_PIN, LOW);
      saveInfoData();
      measurement();
      qc();
      led_count_measurements();
      countMeasurements++;
      digitalWrite(LED_PIN, HIGH);
    }
    else
    {
      // esperamos 2 segundos
      delay(2000);
      buttonState = digitalRead(BUTTON_PIN);
      if (buttonState == LOW)
      {
        // simulamos comportamiento de Reset mediante el LED y hacemos Reset
        digitalWrite(LED_PIN, LOW);
        delay(1000);
        digitalWrite(LED_PIN, HIGH);
        resetFunc();
      }
    }
  }
  // Evitamos el "debounce" del boton con un pequeño delay
  delay(100); 
}

void qc()
{
  if (value1 > 14000 || value1 < 10)
  {
    for(int i = 0; i < 30; i++)
    {
      delay(100);
      digitalWrite(LED_PIN, HIGH);
      delay(100);
      digitalWrite(LED_PIN, LOW);
      delay(100);
      digitalWrite(LED_PIN, HIGH);
    }
  }
}

// Metodo para hacer intermitencia en el LED tantas veces se haya pulsado el boton
void led_count_measurements()
{
  int i = 0;
  digitalWrite(LED_PIN, LOW);
  for (i = 0; i < countMeasurements; i++)
  {
    delay(250);
    digitalWrite(LED_PIN, HIGH);
    delay(250);
    digitalWrite(LED_PIN, LOW);
  }
}

// Realizacion de la medida
void measurement()
{
  // Ponemos los contadores a 0
  resetCounters();
  // Esperamos el tiempo de medida
  delay(MEASURING_TIME);
  value1 = pulseCnt2;
  value2 = pulseCnt3;
  // Guardamos las medidas en la SD
  saveData();
}

// Poner todos los contadores de pulsos a 0 
void resetCounters()
{
  pulseCnt2 = 0;
  pulseCnt3 = 0;
}

// Funciones que incrementan el numero de pulsos de cada sensor TCS230, se activan con las interrupciones
void addPulse2(){ pulseCnt2++; }
void addPulse3(){ pulseCnt3++; }

// Definimos la fecha actual para las medidas
void date(){  
  DateTime now = rtc.now();
  sprintf(savedTime, "%04d/%02d/%02d-%02d:%02d:%02d", now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second());
  dataFile.print(savedTime);
  dataFile.flush();
}

// Guardamos los datos de la fecha, numero de medida y tipo de medida
void saveInfoData()
{
  date();
  dataFile.print(",");
  dataFile.print(MEASURING_TIME);
  dataFile.print(",");
  dataFile.print(MODE);
  dataFile.print(",");
  dataFile.print(countMeasurements);
  // measuringNumber necesario para medidas tipo RGB
  dataFile.print(",");
  dataFile.print(SENSORS);
  dataFile.print(",");
  dataFile.flush();
}

// Guardamos las medidas en la SD
void saveData()
{
    Serial.println(countMeasurements);
    Serial.println(value1);
    dataFile.print(value1);
    dataFile.print(",");
    dataFile.print(value2);
    Serial.println(value2);
    dataFile.print("");
    dataFile.println("");
    dataFile.flush();
}
