/*
  TSL230R v0.2
  Llegeix la frequencia del sensor de llum TS230R.
  Els sensors no es configuren, ja estan configurats per HW (soldats). 
 */
 

#define COLOR_PIN 18 // Entrada del sensor 4 pel pin 19
#define S3_PIN 8 // BLUE WIRE
#define S2_PIN 9 // GREEN WIRE

#define DEBUG 13         // Pin amb un led-> Debug
#define ESPERA 5000  // 60000 ms = 5 min


unsigned long pulseCnt = 0;      // Contador de pulsos de del sensor 

unsigned int vegada = 0;

int blau = 0;
int verd = 0;
int vermell = 0;
int nofilter = 0;

void setup() 
{
	 
  attachInterrupt(4, addPulse, RISING); // attach interrupt al pin19,
                                         // que s'activu amb flang de
                                         // pujada

  // Configuracio dels pins
  
  pinMode(COLOR_PIN, INPUT);

  pinMode(DEBUG, OUTPUT);          

  pinMode(S3_PIN, OUTPUT); 
           
  pinMode(S2_PIN, OUTPUT);          

  digitalWrite(DEBUG, HIGH);
  
  vegada = 0;
  
  //Configuraci serial
  Serial.begin(9600);
}

void loop()
{
  digitalWrite(S3_PIN, LOW);
  digitalWrite(S2_PIN, LOW);
  // Reset del contador de pulsos
  pulseCnt = 0;      // Contador de pulsos de del sensor
  delay(ESPERA);
  vermell = pulseCnt;
  
  Serial.print(vermell);
  Serial.print(" ");

  digitalWrite(S3_PIN, HIGH);
  digitalWrite(S2_PIN, LOW);
  // Reset del contador de pulsos
  pulseCnt = 0;      // Contador de pulsos de del sensor
  delay(ESPERA);
  blau = pulseCnt;

  Serial.print(verd);
  Serial.print(" ");

  digitalWrite(S3_PIN, HIGH);
  digitalWrite(S2_PIN, HIGH);
  // Reset del contador de pulsos
  pulseCnt = 0;      // Contador de pulsos de del sensor
  delay(ESPERA);
  verd = pulseCnt;

  Serial.print(blau);
  Serial.print(" ");

  digitalWrite(S3_PIN, LOW);
  digitalWrite(S2_PIN, HIGH);
  // Reset del contador de pulsos
  pulseCnt = 0;      // Contador de pulsos de del sensor
  delay(ESPERA);
  nofilter = pulseCnt;

  Serial.println(nofilter);

  delay(ESPERA);
}

void addPulse()
{
  // Incrementa el contador de pulsos
  pulseCnt++;
  return;
}


