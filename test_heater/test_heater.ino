#define HEATER D5

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:
  analogWrite(HEATER, 900);
  delay(1000);
  analogWrite(HEATER, 0);
  delay(1500);
}
