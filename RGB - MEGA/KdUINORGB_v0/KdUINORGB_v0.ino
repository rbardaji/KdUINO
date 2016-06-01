/*
 KdUINORGB_v0
 
 12/02/2016
 Raul Bardaji
 
  Leemos la frecuencia de salida de los sensores TS230R.
  Los datos se guardan en una targeta SD (se usa el Datalogger Shield).
  Los datos se guardan con el siguiente formato:
    # Measuring time: <MEASURING_TIME>
    # Sample interval: <SAMPLE_INTERVAL>
    # Sensors: <modelo del sensor 1> <modelo del sensor 2> ... <modelo del sensor n>
    # Input pin of sensors: <numero de pin de entrada del sensor 1> <numero de pin de entrada del sensor 2> ... <numero de pin de entrada del sensor n>
    # Notes: <Las notas que quieras poner> // Ejemplo: El sensor TCS230 hace 4 medidas (rojo, verde, azul y sin filtro). Por eso cada medida se compone de 4 lineas (una linea para cada color, en orden de Rojo, Verde, Azul y No Filtro). Cada color es una nueva medida asi que este sensor tarda 4 x MEASURING_TIME en hacer una ronda de medida completa.
    # Start time: <Fecha y Hora UTC de inicio de medidas con formato AAAA-MM-DD hh:mm:ss> 
    0 0 ... <numero de medida> //Poner tantos 0 como numero de sensores -1
    <valor sensor 1> <valor sensor 2> ... <valor sensor n>
    .
    .
    .
    <pin del Arduino conectado al sensor n> <valor del rojo> <valor del verde> <valor del azul> <valor sensor sin filtro>
  Parametros de configuracion:
    SAMPLE_INTERVAL: Cada cuanto se hace una medida (en milisegundos)
    MEASURING_TIME: Cuanto tiempo dura la toma de la medida (en milisegundos)
 */

// Configuracion de los tiempos de medida en milisegundos
#define MEASURING_TIME 1000
#define SAMPLE_INTERVAL 10000

// Configuracion de los pines que controlan los filtros de los sensores TCS230
#define S3_PIN 8
#define S2_PIN 9

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

// Contadores de pulsos de los sensores
unsigned long pulseCnt21 = 0;
unsigned long pulseCnt20 = 0;
unsigned long pulseCnt19 = 0;
unsigned long pulseCnt18 = 0;

// Variable con el numero de la medida
unsigned int measuringNumber = 0;

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
  Serial.begin(9600);

  // Inicializacion de variables
  measuringNumber = 0;
  resetCounters();

  // Escribe los metadatos
  metadata("AAAA:MM:DD hh:mm:ss");
}

void loop()
{
  // Guardamos el numero de medida
  saveMeasuringNumber();
  
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
  
  // Esperamos hasta la siguiente medida
  delay(SAMPLE_INTERVAL);
}

// Realizacion de la medida
void measurement()
{
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

// Guardar metadatos en la SD
void metadata(char date[20])
{
  Serial.print("# Measuring time: ");Serial.println(MEASURING_TIME);
  Serial.print("# Sample interval: ");Serial.println(SAMPLE_INTERVAL);
  // MODIFICAR A PARTIR DE AQUI PARA CADA CONFIGURACION DE SENSORES QUE TENGAMOS
  Serial.println("# Sensors: TCS230 TCS230 TCS230 TCS230");
  Serial.println("# Input pin of sensors: 21 20 19 18");
  Serial.println("# Notes: El sensor TCS230 hace 4 medidas (rojo, verde, azul y sin filtro). Por eso cada medida se compone de 4 lineas (una linea para cada color, en orden de Rojo, Verde, Azul y No Filtro). Cada color es una nueva medida asi que este sensor tarda 4 x MEASURING_TIME en hacer una ronda de medida completa.");
  Serial.print("# Start time: "); Serial.println(date); 
}
void saveMeasuringNumber()
{
  measuringNumber++;
  Serial.print("0 0 0 ");Serial.println(measuringNumber);
}

// Guardar las medidas en la SD
void saveData()
{
  Serial.print(pulseCnt21);
  Serial.print(" ");
  Serial.print(pulseCnt20);
  Serial.print(" ");
  Serial.print(pulseCnt19);
  Serial.print(" ");
  Serial.println(pulseCnt18);
}

