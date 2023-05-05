int pinD27 = 27;

void setup() {
  pinMode(pinD27, INPUT);
  Serial.begin(9600);
}

void loop() {
  Serial.println(uint32_t(100 - (0.02442*analogRead(pinD27))));
  delay(3000);
}
