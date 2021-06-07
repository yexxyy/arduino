//#include <Wire.h>
//#include <MPU6050.h>
//MPU6050 mpu;

#define SERIAL_RATE 115200

/*
鼓的声音配置
Logic的音轨设置为鼓音源后，打开钢琴卷帘，从下往上数，以0开始，相应的音符对应的序号就是
比如低音鼓在音符C1，C1的序号是36
*/
#define NOTE_KICK 36  // 低音鼓
#define NOTE_SNARE 38 // 军鼓
#define NOTE_LOW_TOM 41  // 低音桶鼓
//#define NOTE_MID_TOM 45  // 中音桶鼓
//#define NOTE_HIGH_TOM 48 // 高音桶鼓

#define NOTE_HI_HAT_CLOSED 42 // 踩镲关闭打击
#define NOTE_HI_HAT_OPEN 46  // 踩镲打开打击
#define NOTE_HI_HAT_FOOT_CLOSE 44  //踩镲脚踩下去

#define NOTE_CRASH_LEFT 49 // 左边那个镲

// #define NOTE_RIDE_EDGE 52 // 右边那个镲（边缘）



#define NOTE_ON_CMD 0x90  // 音符开始标记
#define NOTE_OFF_CMD 0x80  //音符结束标记
#define MAX_MIDI_VELOCITY 127  //音符最大力度

#define KICK_THRESHOLD 1   
#define SNARE_THRESHOLD 200
#define LOW_TOM_THRESHOLD 150
#define HI_HAT_THRESHOLD 120
#define CRASH_LEFT_THRESHOLD 150


#define NUM_PIEZOS 4  // 总共4个压电传感器
#define START_SLOT A0
#define KICK_SLOT 13

#define SIGNAL_BUFFER_SIZE 60
#define PEAK_BUFFER_SIZE 20
#define MAX_TIME_BETWEEN_PEAKS 10
#define MIN_TIME_BETWEEN_NOTES 35

int lastKickLevel;

unsigned short slotMap[NUM_PIEZOS];

unsigned short noteMap[NUM_PIEZOS];

unsigned short thresholdMap[NUM_PIEZOS];

short currentSignalIndex[NUM_PIEZOS];
short currentPeakIndex[NUM_PIEZOS];
unsigned short signalBuffer[NUM_PIEZOS][SIGNAL_BUFFER_SIZE];
unsigned short peakBuffer[NUM_PIEZOS][PEAK_BUFFER_SIZE];

boolean noteReady[NUM_PIEZOS];
unsigned short noteReadyVelocity[NUM_PIEZOS];
boolean isLastPeakZeroed[NUM_PIEZOS];

unsigned long lastPeakTime[NUM_PIEZOS];
unsigned long lastNoteTime[NUM_PIEZOS];



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
  
  thresholdMap[0] = LOW_TOM_THRESHOLD;
  thresholdMap[1] = SNARE_THRESHOLD;
  thresholdMap[2] = HI_HAT_THRESHOLD;
  thresholdMap[3] = CRASH_LEFT_THRESHOLD;
  
  noteMap[0] = NOTE_LOW_TOM;
  noteMap[1] = NOTE_SNARE;
  noteMap[2] = NOTE_HI_HAT_CLOSED;
  noteMap[3] = NOTE_CRASH_LEFT; 
  
}

void loop()
{

  pedalHandler();
  unsigned long currentTime = millis();
  for(short i=0; i<NUM_PIEZOS; ++i)
  {
    unsigned short newSignal = analogRead(slotMap[i]);
    signalBuffer[i][currentSignalIndex[i]] = newSignal;
   
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


// 利用midi键盘的延音踏板来控制底鼓，设置引脚的电位，如果是高电位，则发送midi数据；如果一直保持高电位只发送一次
void pedalHandler() {
  int currentLevel = digitalRead(KICK_SLOT);
  if (currentLevel == 1 &&  lastKickLevel == 0) {
     noteFire(NOTE_KICK, 127);
   }
  lastKickLevel = currentLevel;
}

void recordNewPeak(short slot, short newPeak)
{
  isLastPeakZeroed[slot] = (newPeak == 0);
  
  unsigned long currentTime = millis();
  lastPeakTime[slot] = currentTime;
  
  //new peak recorded (newPeak)
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

void noteFire(int note, int velocity)
{
  Serial.println(velocity);
  if(velocity > MAX_MIDI_VELOCITY)
    velocity = MAX_MIDI_VELOCITY;
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
