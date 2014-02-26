void setup() {
  int tempPin = 2;
  Serial.begin(115200);
}


void loop() {
  int analogVal = analogRead(A4);
  float tempV;
  int tempC;
  tempV = (analogVal / 1024.0) * 4.88;
  Serial.println(tempV);
  tempC = (tempV * 100) - 273;
  Serial.println(tempC);
  Serial.println("*********");
  delay(500);
}
