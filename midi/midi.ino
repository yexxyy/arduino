

void setup()
{
  Serial.begin(115200);
  while(!Serial);
  pinMode(13, OUTPUT);
}

void loop()
{
  digitalWrite(13, HIGH);

  sendNote(36, 100);  // 动
  delay(300);
  sendNote(42, 127);  // 次
  delay(300);
  sendNote(38, 90);  // 打
  delay(300);
  sendNote(42, 127);  // 次
  delay(300); 
}

void sendNote(byte note, byte volume) {
  Serial.println(note);
  Serial.write(0x90);  // MIDI音符开始标记
  Serial.write(note);
  Serial.write(volume);
  delay(10);
  Serial.write(0x80);  //MIDI音符结束标记
  Serial.write(note);
  Serial.write(volume);
  delay(10);
}
