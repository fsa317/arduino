

void setup() {
  // put your setup code here, to run once:
  pinMode(D6,OUTPUT);
  pinMode(D7,OUTPUT);
  pinMode(D8,OUTPUT);
  
  analogWrite(D6,0);
  analogWrite(D7,0);
  analogWrite(D8,0);
}

void loop() {
  // put your main code here, to run repeatedly:
  delay(5000);
  analogWrite(D6,1024); //blue
 // analogWrite(D7,1024); //green
 // analogWrite(D8,1024); //red
  
 // delay(3000);
 // analogWrite(D6,255);
  
  


}
