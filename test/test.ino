#define HI_HAT_LEVEL_CHCHE_LEN 3
short hiHatLevelCache[HI_HAT_LEVEL_CHCHE_LEN];
short currenthiHatLevelIndex;
short lastHiHatLevel;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  pinMode(13, INPUT);
  pinMode(12, INPUT);
  lastHiHatLevel = 0;
  currenthiHatLevelIndex = 0;
}

void loop() {
    int currentLevel = digitalRead(13);
    int l12 = digitalRead(12);
    hiHatLevelCache[currenthiHatLevelIndex] = currentLevel;
    Serial.println(currentLevel);

    int sum = 0;
    for(short i=0; i<HI_HAT_LEVEL_CHCHE_LEN; ++i)
    {
      sum += hiHatLevelCache[i];
    }
    if (sum == HI_HAT_LEVEL_CHCHE_LEN) {
//      Serial.println(1);
//        if (lastHiHatLevel == 0) Serial.println(1);
        lastHiHatLevel = 1;
    }
    if (sum == 0) {
//       Serial.println(0);
      if (lastHiHatLevel == 1) {
//        Serial.println(0);
        lastHiHatLevel = 0;

        }
    }
        
    currenthiHatLevelIndex += 1;
    if (currenthiHatLevelIndex >= HI_HAT_LEVEL_CHCHE_LEN) {
      currenthiHatLevelIndex = 0;
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
