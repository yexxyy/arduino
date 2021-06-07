
void setup() {
  Serial.begin(115200);  // 指定数据传输波特率
}

void loop() {
  int x=analogRead(A0); // 从Arduino ANALOG IN A0位置读取数据
  if (x > 50) {
     Serial.println(x);    // 如果压力值大于10，打印压力值
  }
  int x1=analogRead(A1); // 从Arduino ANALOG IN A0位置读取数据
  if (x1 > 50) {
     Serial.println(x1);    // 如果压力值大于10，打印压力值
  }
  int x2=analogRead(A2); // 从Arduino ANALOG IN A0位置读取数据
  if (x2 > 50) {
     Serial.println(x2);    // 如果压力值大于10，打印压力值
  }
  int x3=analogRead(A3); // 从Arduino ANALOG IN A0位置读取数据
  if (x3 > 50) {
     Serial.println(x3);    // 如果压力值大于10，打印压力值
  }
  delay(10);           // 暂停10毫秒
}
