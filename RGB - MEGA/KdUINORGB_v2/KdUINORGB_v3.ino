/*
 KdUINORGB_v3
 
 27/06/2016
 Raul Bardaji
 
  Leemos la frecuencia de salida de los sensores TS230R.
  Los datos se guardan en una targeta SD (se usa el Datalogger Shield).
  Tiene la opcion de guardar la calibracion
  Los datos se guardan con el siguiente formato:
    # Measuring time: <MEASURING_TIME>
    # Sample interval: <SAMPLE_INTERVAL>
    # Sensors: <modelo del sensor 1> <modelo del sensor 2> ... <modelo del sensor n>
    # Input pin of sensors: <numero de pin de entrada del sensor 1> <numero de pin de entrada del sensor 2> ... <numero de pin de entrada del sensor n>
    # Notes: <Las notas que quieras poner> // Ejemplo: El sensor TCS230 hace 4 medidas (rojo, verde, azul y sin filtro). Por eso cada medida se compone de 4 lineas (una linea para cada color, en orden de Rojo, Verde, Azul y No Filtro). Cada color es una nueva medida asi que este sensor tarda 4 x MEASURING_TIME en hacer una ronda de medida completa.
    # Start time: <Fecha y Hora UTC de inicio de medidas con formato AAAA-MM-DD hh:mm:ss> 
    <numero de medida> <valor sensor 1> <valor sensor 2> ... <valor sensor n>
    .
    .
    .
  Ejemplo de valores si se usa TCS2300
    1 <valor sensor 1 con filtro Rojo>  <valor sensor 2 con filtro Rojo>  <valor sensor 3 con filtro Rojo>  <valor sensor 4 con filtro Rojo>
    2 <valor sensor 1 con filtro Verde>  <valor sensor 2 con filtro Verde>  <valor sensor 3 con filtro Verde>  <valor sensor 4 con filtro Verde>
    3 <valor sensor 1 con filtro Azul>  <valor sensor 2 con filtro Azul>  <valor sensor 3 con filtro Azul>  <valor sensor 4 con filtro Azul>
    4 <valor sensor 1 sin filtro>  <valor sensor 2 sin filtro>  <valor sensor 3 sin filtro>  <valor sensor 4 sin filtro>
    (en realidad son 4 medidas independientes)
    
  Parametros de configuracion:
    MEASURING_TIME: Cuanto tiempo dura la toma de la medida (en milisegundos)
 */
#include <SD.h>
#include <SPI.h>

// INFORMACION DE METADATOS
// Profundidad donde se va a colocar cada sensor, en orden, de menor a mayor
#define DEPTHS "4 5 6 7"

// Configuracion del baud rate de la comunicacion serie
#define BAUDRATE 38400

// Configuracion de los tiempos de medida en milisegundos
#define MEASURING_TIME 3000

// Configuracion de los pines que controlan los filtros de los sensores TCS230
#define S3_PIN 39
#define S2_PIN 41

// Configuracion de los pines de recepcion de datos de los sensores TCS230
#define SENSOR_21 21
#define SENSOR_20 20
#define SENSOR_19 19
#define SENSOR_18 18

// Constantes de configuracion del sensor TCS230. 
// Estas constantes se usan para la funcion configTCS230()
// NO CAMBIAR
#define RED 0
#define GREEN 1
#define BLUE 2
#define NOFILTER 3

// Constantes del protocolo de comunicacion
// NO CAMBIAR
#define START 'S'
#define STOP 'O'
#define SEND 'E'
#define DELETE 'D'
#define TIME 'T'
#define ONE 'N'
#define OK '+'
#define KO '-'
#define NOSD 'K'
#define CALIBRATION 'C'

// Variable para comprobar que la SD esta conectada correctamente
bool sdOk = false;

// Contadores de pulsos de los sensores
unsigned long pulseCnt21 = 0;
unsigned long pulseCnt20 = 0;
unsigned long pulseCnt19 = 0;
unsigned long pulseCnt18 = 0;

// Variable con el numero de la medida
unsigned int measuringNumber = 0;

// Variable que indica si se tiene que medir o no
boolean measuresNow = false;

// Variable para guardar la fecha y hora actuales
char savedTime[20] = "AAAA:MM:DD hh:mm:ss";
 
void setup() 
{
  // Configuracion de pines
  pinMode(SENSOR_21, INPUT);
  pinMode(SENSOR_20, INPUT); 
  pinMode(SENSOR_19, INPUT); 
  pinMode(SENSOR_18, INPUT);          
  pinMode(S3_PIN, OUTPUT); 
  pinMode(S2_PIN, OUTPUT);

  // Configuracion de interrupciones
  attachInterrupt(2, addPulse21, RISING);
  attachInterrupt(3, addPulse20, RISING);
  attachInterrupt(4, addPulse19, RISING);
  attachInterrupt(5, addPulse18, RISING);

  // Configuracion comunicacion serie
  Serial.begin(BAUDRATE);

  // Configuracion de targeta SD
  pinMode(SS, OUTPUT);
  sdOk = true;
  // Si se usa la libreria de Adafruit, hay que usar la siguiente linea de codigo
  //if (!SD.begin(10,11,12,13)) sdOk = false;
  // Si se usa una sd shield de Arduino, hay que sar la siguiente linea de codigo. 
  // Donde 4 es el chipselect de las Arduino shield (hay otras).
  if (!SD.begin(4)) sdOk = false;

  // Inicializacion de variables
  measuringNumber = 0;
  resetCounters();
  measuresNow = false;
  // si se quiere empezar midiendo hay que descomentar las siguientes dos lineas
  // measuresNow = true;
  // metadata(savedTime);
}

void loop()
{
  char orden;

  // Miramos si nos han mandado alguna orden por la comunicacion serie
  if (Serial.available() > 0)
  {
    orden = Serial.read();
    switch (orden)
    {
      case START:
        Serial.println(START);
        if (sdOk)
        { 
          // Guardamos que estamos en el 'Mode: Bouy'
          saveBuoyMode();
          // Escribe los metadatos
          metadata(savedTime);
          // Reinicia el contador de medidas
          measuringNumber = 0;
          // Activa las medidas
          measuresNow = true;
          Serial.println(OK);
        }
        else
        {
          // Si ha entrado aqui hay algun error. Descartaremos todo el buffer de la comunicacion serie
          while (Serial.available() > 0) orden = Serial.read();
          Serial.println(NOSD);
        }
        break;
      case STOP:
        Serial.println(STOP);
        measuresNow = false;
        Serial.println(OK);
        break;
      case SEND:
        Serial.println(SEND);
        if (sdOk)
        {
          sendData();
          Serial.println(OK);
        }
        else
        {
          // Si ha entrado aqui hay algun error. Descartaremos todo el buffer de la comunicacion serie
          while (Serial.available() > 0) orden = Serial.read();
          Serial.println(NOSD);
        }
        break;
      case DELETE:
        Serial.println(DELETE);
        if (sdOk)
        {
          deleteData();
          Serial.println(OK);
        }
        else
        {
          // Si ha entrado aqui hay algun error. Descartaremos todo el buffer de la comunicacion serie
          while (Serial.available() > 0) orden = Serial.read();
          Serial.println(NOSD);
        }
        break;
     case TIME:
        Serial.println(TIME);
        if (sdOk)
        {
          readTime();
          Serial.println(OK);
        }
        else 
        {
          // Si ha entrado aqui hay algun error. Descartaremos todo el buffer de la comunicacion serie
          while (Serial.available() > 0) orden = Serial.read();
          Serial.println(NOSD);
        }
        break;
     case OK:
        Serial.println(OK);
        break;
     case ONE:
        Serial.println(ONE);
        doOneMeasurement();
        Serial.println(OK);
        break;
      case CALIBRATION:
        Serial.println(CALIBRATION);
        calibrate();
        Serial.println(OK);
        break;
      default:
        // Si ha entrado aqui hay algun error. Descartaremos todo el buffer de la comunicacion serie
        while (Serial.available() > 0) orden = Serial.read();
        Serial.println(KO);
        break;
    }
  }
  if (measuresNow)
  {
    doTheMeasurement();
  }
}

// Funcion para medir con todos los sensores y guardar los datos en la SD
void doTheMeasurement()
{  
  // Medida del sensor TCS230 con el filtro del rojo
  configTCS230(RED);
  measurement();

  // Medida del sensor TCS230 con el filtro del verde
  configTCS230(GREEN);
  measurement();

  // Medida del sensor TCS230 con el filtro del azul
  configTCS230(BLUE);
  measurement();

  // Medida del sensor TCS230 sin filtro
  configTCS230(NOFILTER);
  measurement();
}

// Funcion para medir con todos los sensores y enviar los datos por el pierto Serie
void doOneMeasurement()
{  
  // Medida del sensor TCS230 con el filtro del rojo
  configTCS230(RED);
  oneMeasurement();

  // Medida del sensor TCS230 con el filtro del verde
  configTCS230(GREEN);
  oneMeasurement();

  // Medida del sensor TCS230 con el filtro del azul
  configTCS230(BLUE);
  oneMeasurement();

  // Medida del sensor TCS230 sin filtro
  configTCS230(NOFILTER);
  oneMeasurement();
  // Enviamos un \r\n para respetar el protocolo
  Serial.println("");
  
}

// Realizacion solo una medida y la enviamos por el puerto serie
void oneMeasurement()
{
  // Ponemos los contadores a 0
  resetCounters();
  // Esperamos el tiempo de medida
  delay(MEASURING_TIME);
  // Enviamos la medida el puerto serie
  Serial.print(" ");
  Serial.print(pulseCnt21);
  Serial.print(" ");
  Serial.print(pulseCnt20);
  Serial.print(" ");
  Serial.print(pulseCnt19);
  Serial.print(" ");
  Serial.print(pulseCnt18);
  
  
}
// Realizacion de la medida
void measurement()
{
  // Incrementamos el numero de medida
  measuringNumber++;
  
  // Ponemos los contadores a 0
  resetCounters();
  // Esperamos el tiempo de medida
  delay(MEASURING_TIME);
  // Guardamos las medidas en la SD
  saveData();
}

// Poner todos los contadores de pulsos a 0 
void resetCounters()
{
  pulseCnt21 = 0;
  pulseCnt20 = 0;
  pulseCnt19 = 0;
  pulseCnt18 = 0;
}

// Funciones que incrementan el numero de pulsos de cada sensor TCS230, se activan con las interrupciones
void addPulse21(){ pulseCnt21++; }
void addPulse20(){ pulseCnt20++; }
void addPulse19(){ pulseCnt19++; }
void addPulse18(){ pulseCnt18++; }

// Funcion de configuracion del sensor TCS230
void configTCS230(int color)
{
  switch (color)
  {
    case RED:
      digitalWrite(S3_PIN, LOW);
      digitalWrite(S2_PIN, LOW);
      break;
    case GREEN:
      digitalWrite(S3_PIN, HIGH);
      digitalWrite(S2_PIN, HIGH);
      break;
    case BLUE:
      digitalWrite(S3_PIN, HIGH);
      digitalWrite(S2_PIN, LOW);
      break;
    case NOFILTER:
      digitalWrite(S3_PIN, LOW);
      digitalWrite(S2_PIN, HIGH);
      break;
    default:
      break;
  }
}

// Guardamos la sentencia '# Mode: Buoy' en la sd
void saveBuoyMode()
{
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  if (dataFile) 
  {
    dataFile.println("# Mode: Buoy");
    dataFile.close();
  }
}

// Guardamos la sentencia '# Mode: Calibration' en la sd
void saveCalibrationMode()
{
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  if (dataFile) 
  {
    dataFile.println("# Mode: Calibration");
    dataFile.close();
  }
}

// Guardar metadatos en la SD
void metadata(char date[20])
{ 
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  if (dataFile) 
  {
    dataFile.print("# Measuring time: ");dataFile.println(MEASURING_TIME);
    dataFile.print("# Depths: ");dataFile.println(DEPTHS);
    // MODIFICAR A PARTIR DE AQUI PARA CADA CONFIGURACION DE SENSORES QUE TENGAMOS
    dataFile.println("# Sensors: TCS3200 TCS3200 TCS3200 TCS3200");
    dataFile.println("# Input pin of sensors: 21 20 19 18");
    dataFile.println("# Notes: El sensor TCS230 hace 4 medidas (rojo, verde, azul y sin filtro). Por eso cada medida se compone de 4 lineas (una linea para cada color, en orden de Rojo, Verde, Azul y No Filtro). Cada color es una nueva medida asi que este sensor tarda 4 x MEASURING_TIME en hacer una ronda de medida completa.");
    dataFile.print("# Start time: "); dataFile.println(date);
    dataFile.close();
  } 
}

// Guardar las medidas en la SD
void saveData()
{
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  if (dataFile) 
  {
    dataFile.print(measuringNumber);
    dataFile.print(" ");
    dataFile.print(pulseCnt21);
    dataFile.print(" ");
    dataFile.print(pulseCnt20);
    dataFile.print(" ");
    dataFile.print(pulseCnt19);
    dataFile.print(" ");
    dataFile.println(pulseCnt18);
    dataFile.close();
  }
}

// Envia los datos guardados a trabes de la comunicacion serie
void sendData()
{
  File dataFile = SD.open("datalog.txt");
  if (dataFile) 
  {
    // Lee del fichero hasta que ya no queden caracteres para leer
    while (dataFile.available()) Serial.write(dataFile.read());
  }
  dataFile.close();
}

// Borrar el archivo datalog.txt
void deleteData() { SD.remove("datalog.txt"); }

// Leer la fecha y hora que nos pasan por la comunicacion serie
void readTime()
{
  int i = 0;
  int watchdog = 0;
  
  while ((i < 19)||(watchdog < 5))
  {
    if (Serial.available() > 0)
    {
      savedTime[i] = Serial.read();
      i++;
    }
    else 
    {
      watchdog++;
      delay(500);
    }
  }
  savedTime[i] = '\0';
  Serial.println(savedTime);
}

void calibrate()
{
  // Guardamos que estamos en el 'Mode: Bouy'
  saveCalibrationMode();
  // Escribe los metadatos
  metadata(savedTime);
  // Reinicia el contador de medidas
  measuringNumber = 0;
  // Hacemos 4 medidas
  doTheMeasurement();
  doTheMeasurement();
  doTheMeasurement();
  doTheMeasurement();
}

