const int ledPin = 2;
// 时序定义
const unsigned long dot = 200;
const unsigned long dash = 600;
const unsigned long gapEle = 200;
const unsigned long gapChar = 500;
const unsigned long gapWord = 2000;

// 时间戳与状态机变量
unsigned long prevT = 0;
int state = 0; // 0空闲 1S短 2O长 3S短
int cnt = 0;   // 当前闪烁次数
int ledStat = LOW;

void setup() {
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, LOW);
}

void loop() {
  unsigned long now = millis();
  if(now - prevT < 50) return;
  prevT = now;

  switch(state){
    case 0: // 空闲，开始S
      cnt = 0;
      state = 1;
      break;
    case 1: // S：3次短闪
      if(cnt < 3){
        flash(dot, gapEle);
        cnt++;
      }else{
        delayMs(gapChar);
        cnt = 0;
        state = 2;
      }
      break;
    case 2: // O：3次长闪
      if(cnt < 3){
        flash(dash, gapEle);
        cnt++;
      }else{
        delayMs(gapChar);
        cnt = 0;
        state = 3;
      }
      break;
    case 3: // S：3次短闪
      if(cnt < 3){
        flash(dot, gapEle);
        cnt++;
      }else{
        delayMs(gapWord);
        state = 0; // 一轮结束，重新循环
      }
      break;
  }
}

// 点亮指定时长，熄灭间隔
void flash(unsigned long tOn, unsigned long tOff){
  digitalWrite(ledPin, HIGH);
  delayMs(tOn);
  digitalWrite(ledPin, LOW);
  delayMs(tOff);
}

// 基于millis的非阻塞延时函数
void delayMs(unsigned long t){
  unsigned long s = millis();
  while(millis() - s < t){}
}
