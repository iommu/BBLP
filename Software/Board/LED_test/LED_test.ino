void setup() {
  // put your setup code here, to run once:
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  pinMode(14, OUTPUT);
  digitalWrite(14, HIGH);
  pinMode(12, OUTPUT);
  digitalWrite(12, HIGH);
}

void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(12, LOW);
  delay(2000);
  digitalWrite(12, HIGH);
  digitalWrite(14, LOW);
  delay(2000);
  digitalWrite(14, HIGH);
  digitalWrite(13, LOW);
  delay(2000);
  digitalWrite(13, HIGH);
}
