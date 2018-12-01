

#define SERIESRESISTOR 10000 

#define THERMISTORNOMINAL 10000      
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25   
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 5
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3950

int sensorPin = A0;
float sensorValue = 0;
int samples[NUMSAMPLES];

int sensor1Pin = D1;
int sensor2Pin = D2;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  pinMode(sensor1Pin, OUTPUT);
  //pinMode(sensor2Pin, OUTPUT);
  digitalWrite(sensor1Pin,LOW);
  //digitalWrite(sensor2Pin,LOW);
  
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println("welcome");
}

double getTemp() {
  int i;
  float average;
  
  // take N samples in a row, with a slight delay
  for (i=0; i< NUMSAMPLES; i++) {
   samples[i] = analogRead(sensorPin);
   delay(10);
  }
 
  // average all the samples out
  average = 0;
  for (i=0; i< NUMSAMPLES; i++) {
     average += samples[i];
  }
  average /= NUMSAMPLES;

 float RawADC = average;
 
 float res;
 res = SERIESRESISTOR*(1023/RawADC-1);
 Serial.print("Resistance: ");
 Serial.println(res);
 Serial.flush();
 float steinhart;
  steinhart = res / THERMISTORNOMINAL; // (R/Ro)
  steinhart = log(steinhart); // ln(R/Ro)
  steinhart /= BCOEFFICIENT; // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart; // Invert
  steinhart -= 273.15; // convert to C

  float f = steinhart*1.8+32;
  Serial.print("Temp: ");
  Serial.print(f);
  Serial.println(" oF");
  Serial.flush();
  return f;
}



// the loop function runs over and over again forever
void loop() {

  digitalWrite(sensor1Pin, LOW);
  
  delay(10);
  Serial.println(" Sensor #1");
  Serial.flush();
  getTemp();
  
  delay(100);
  
  digitalWrite(sensor1Pin, HIGH);
  Serial.println(" Sensor #2");
  Serial.flush();
  getTemp(); 
  Serial.println("~~~~~~");
  Serial.flush();
  delay(1000);
             
}
