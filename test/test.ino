void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(12, INPUT);
}

void loop() {
   int level = digitalRead(12);
   if (level > 0) {
    Serial.println(level); 
   }
}
