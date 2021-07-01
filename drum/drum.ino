#define SERIAL_RATE 115200
/*
 * 鼓的声音配置
 * Logic的音轨设置为鼓音源后，打开钢琴卷帘，从下往上数，以0开始，相应的音符对应的序号就是
 * 比如低音鼓在音符C1，C1的序号是36
*/
#define NOTE_KICK 36  // 低音鼓
#define NOTE_SNARE 38 // 军鼓
#define NOTE_LOW_TOM 41  // 低音桶鼓
#define NOTE_MID_TOM 45  // 中音桶鼓
#define NOTE_HIGH_TOM 48 // 高音桶鼓

#define NOTE_HI_HAT_CLOSED 42 // 踩镲关闭打击
#define NOTE_HI_HAT_OPEN 46  // 踩镲打开打击
#define NOTE_HI_HAT_FOOT_CLOSE 44  // 踩镲踩下
#define NOTE_CRASH_LEFT 49 // 左边那个镲

#define NOTE_ON_CMD 0x90  // 音符开始标记
#define NOTE_OFF_CMD 0x80  //音符结束标记
#define MAX_MIDI_VELOCITY 127  //音符最大力度
  
// 设置阈值，低于阈值的压力值按照0处理
#define SNARE_THRESHOLD 15
#define LOW_TOM_THRESHOLD 15
#define HI_HAT_THRESHOLD 10
#define CRASH_LEFT_THRESHOLD 20
#define HIGH_TOM_THRESHOLD 60
#define KICK_THRESHOLD 50

#define SNARE_SCALE 0.6
#define LOW_TOM_SCALE 7
#define HI_HAT_SCALE 4.5
#define CRASH_LEFT_SCALE 6
#define HIGH_TOM_SCALE 5.5
#define KICK_SCALE 4

#define SNARE_ZERO 15
#define LOW_TOM_ZERO 80
#define HI_HAT_ZERO 150
#define CRASH_LEFT_ZERO 100
#define HIGH_TOM_ZERO 20
#define KICK_ZERO 500

#define NUM_PIEZOS 6  // 总共5个压电传感器
#define START_SLOT A0
#define KICK_SLOT 13

#define SIGNAL_BUFFER_SIZE 100
#define PEAK_BUFFER_SIZE 30
#define MAX_TIME_BETWEEN_PEAKS 30
#define MIN_TIME_BETWEEN_NOTES 60

// 缓存踏板控制的数字电位
int lastKickLevel;

unsigned short slotMap[NUM_PIEZOS];
unsigned short noteMap[NUM_PIEZOS];
unsigned short thresholdMap[NUM_PIEZOS];
float scaleMap[NUM_PIEZOS];
unsigned short zeroMap[NUM_PIEZOS];

short currentSignalIndex[NUM_PIEZOS];
short currentPeakIndex[NUM_PIEZOS];

/*
 * 当我们击打压电片的时候，数值的变化是先变大后变小
 * 为了准确的找出应该何时发出midi信号，我们需要将接收到的数值缓存起来
 * 在每一轮读取压电片的时候，才能在一个时间范围内的数据，找出最大的压力值
*/
unsigned short signalBuffer[NUM_PIEZOS][SIGNAL_BUFFER_SIZE];
unsigned short peakBuffer[NUM_PIEZOS][PEAK_BUFFER_SIZE];

boolean noteReady[NUM_PIEZOS];
unsigned short noteReadyVelocity[NUM_PIEZOS];
boolean isLastPeakZeroed[NUM_PIEZOS];

unsigned long lastPeakTime[NUM_PIEZOS];
unsigned long lastNoteTime[NUM_PIEZOS];


// 程序参数初始化
void setup()
{
  Serial.begin(SERIAL_RATE);
  while(!Serial);
  pinMode(KICK_SLOT, INPUT);

  for(short i=0; i<NUM_PIEZOS; ++i)
  {
    currentSignalIndex[i] = 0;
    currentPeakIndex[i] = 0;
    memset(signalBuffer[i],0,sizeof(signalBuffer[i]));
    memset(peakBuffer[i],0,sizeof(peakBuffer[i]));
    noteReady[i] = false;
    noteReadyVelocity[i] = 0;
    isLastPeakZeroed[i] = true;
    lastPeakTime[i] = 0;
    lastNoteTime[i] = 0;    
    slotMap[i] = START_SLOT + i;
  }
  
  noteMap[0] = NOTE_CRASH_LEFT;
  noteMap[1] = NOTE_HI_HAT_OPEN;
  noteMap[2] = NOTE_HIGH_TOM;
  noteMap[3] = NOTE_LOW_TOM; 
  noteMap[4] = NOTE_SNARE; 
  noteMap[5] = NOTE_KICK;
  
  thresholdMap[0] = CRASH_LEFT_THRESHOLD;
  thresholdMap[1] = HI_HAT_THRESHOLD;
  thresholdMap[2] = HIGH_TOM_THRESHOLD;
  thresholdMap[3] = LOW_TOM_THRESHOLD;
  thresholdMap[4] = SNARE_THRESHOLD;
  thresholdMap[5] = KICK_THRESHOLD;

  scaleMap[0] = CRASH_LEFT_SCALE;
  scaleMap[1] = HI_HAT_SCALE;
  scaleMap[2] = HIGH_TOM_SCALE;
  scaleMap[3] = LOW_TOM_SCALE; 
  scaleMap[4] = SNARE_SCALE; 
  scaleMap[5] = KICK_SCALE; 

  zeroMap[0] = CRASH_LEFT_ZERO;
  zeroMap[1] = HI_HAT_ZERO;
  zeroMap[2] = HIGH_TOM_ZERO;
  zeroMap[3] = LOW_TOM_ZERO; 
  zeroMap[4] = SNARE_ZERO; 
  zeroMap[5] = KICK_ZERO; 
}

void loop()
{
  // 踏板控制底鼓方法
  pedalHandler();

  unsigned long currentTime = millis();
  for(short i=0; i<NUM_PIEZOS; ++i)
  {
    short newSignal = analogRead(slotMap[i]);
//    if (newSignal > 0) Serial.println(newSignal);

    // 对原始信号进行归零，抵消掉本来的压力值；"归一化"处理，使得所有的数值分布在0-MAX_MIDI_VELOCITY以内
    newSignal = newSignal - zeroMap[i];
    if (newSignal < 0)  newSignal = 0;
    newSignal = newSignal / scaleMap[i];
    
    if (newSignal > MAX_MIDI_VELOCITY) newSignal = MAX_MIDI_VELOCITY;

    signalBuffer[i][currentSignalIndex[i]] = newSignal;
   /*
    * 当前压力值小于阈值的时候，再回顾之前缓存起来的压力值
    * 下面这段代码总体思路就是：
    * 压电片被击打的时候，压力值是先变大，再变小；
    * 当压力值小于阈值，并且上一个峰值不是0的时候，那么这个信号的前方肯定是一个类似波峰的一系列数据
    * 这时我们一直向前找，也就是位于阈值上方这个波峰的最大值找出来
    * 找出来的便是我们需要发送midi信号数据
   */
    if(newSignal < thresholdMap[i])
    {
      if(!isLastPeakZeroed[i] && (currentTime - lastPeakTime[i]) > MAX_TIME_BETWEEN_PEAKS)
      {
        recordNewPeak(i,0);
      }
      else
      {
        short prevSignalIndex = currentSignalIndex[i]-1;
        if(prevSignalIndex < 0) prevSignalIndex = SIGNAL_BUFFER_SIZE-1;        
        unsigned short prevSignal = signalBuffer[i][prevSignalIndex];
        
        unsigned short newPeak = 0;

        while(prevSignal >= thresholdMap[i])
        {
          if(signalBuffer[i][prevSignalIndex] > newPeak)
          {
            newPeak = signalBuffer[i][prevSignalIndex];        
          }
          
          prevSignalIndex--;
          if(prevSignalIndex < 0) prevSignalIndex = SIGNAL_BUFFER_SIZE-1;
          prevSignal = signalBuffer[i][prevSignalIndex];
        }
        
        if(newPeak > 0)
        {
          recordNewPeak(i, newPeak);
        }
      }
    }
        
    currentSignalIndex[i]++;
    if(currentSignalIndex[i] == SIGNAL_BUFFER_SIZE) currentSignalIndex[i] = 0;
  }
}


void recordNewPeak(short slot, short newPeak)
{
  isLastPeakZeroed[slot] = (newPeak == 0);
  
  unsigned long currentTime = millis();
  lastPeakTime[slot] = currentTime;
  
  peakBuffer[slot][currentPeakIndex[slot]] = newPeak;
 
  short prevPeakIndex = currentPeakIndex[slot]-1;
  if(prevPeakIndex < 0) prevPeakIndex = PEAK_BUFFER_SIZE-1;        
  unsigned short prevPeak = peakBuffer[slot][prevPeakIndex];


  if(newPeak > prevPeak && (currentTime - lastNoteTime[slot])>MIN_TIME_BETWEEN_NOTES)
  {
    noteReady[slot] = true;
    if(newPeak > noteReadyVelocity[slot])
      noteReadyVelocity[slot] = newPeak;
  }
  else if(newPeak < prevPeak && noteReady[slot])
  {
    noteFire(noteMap[slot], noteReadyVelocity[slot]);
    noteReady[slot] = false;
    noteReadyVelocity[slot] = 0;
    lastNoteTime[slot] = currentTime;
  }
  
  currentPeakIndex[slot]++;
  if(currentPeakIndex[slot] == PEAK_BUFFER_SIZE) currentPeakIndex[slot] = 0;  
}


/*
 * 利用midi键盘的延音踏板来设置引脚的电位，以此控制底鼓
 * 如果是高电位，则发送midi数据；如果一直保持高电位只发送一次
 * 利用电位只能标记是否应该发出声音，无法控制力度的大小
*/
void pedalHandler() {
  int currentLevel = digitalRead(KICK_SLOT);
  if (currentLevel == 1 && lastKickLevel == 0) {
      noteMap[1] = NOTE_HI_HAT_CLOSED; 
      noteFire(NOTE_HI_HAT_FOOT_CLOSE, 127);

   }
   if (currentLevel == 0 && lastKickLevel == 1) {
     noteMap[1] = NOTE_HI_HAT_OPEN; 
   }
  lastKickLevel = currentLevel;
}


// 发送midi数据
void noteFire(int note, int velocity)
{
  if(velocity > MAX_MIDI_VELOCITY) velocity = MAX_MIDI_VELOCITY;
  midiNoteOn(note, velocity);
  midiNoteOff(note, velocity);
}

void midiNoteOn(byte note, byte midiVelocity)
{
  Serial.write(NOTE_ON_CMD);
  Serial.write(note);
  Serial.write(midiVelocity);
}

void midiNoteOff(byte note, byte midiVelocity)
{
  Serial.write(NOTE_OFF_CMD);
  Serial.write(note);
  Serial.write(midiVelocity);
}
