int lastHiHatLevel;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(13, INPUT);
  lastHiHatLevel = 1;
}

void loop() {
    int currentLevel = digitalRead(13);
    if (currentLevel == 1) {
      Serial.println(currentLevel);
    }  

  
//  if ((currentLevel == 0) && (lastHiHatLevel == 1 )) {
//    Serial.println("test");
//  }
//   if (currentLevel == 0 && lastHiHatLevel == 1) {
//    Serial.print(lastHiHatLevel);
//    Serial.println(currentLevel);
//   }
//   Serial.print(lastHiHatLevel);
//   Serial.println(currentLevel);
   lastHiHatLevel = currentLevel;
}
