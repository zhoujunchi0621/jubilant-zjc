// 硬件引脚定义
const int touchPin = 4;    // T0触摸通道GPIO4
const int ledPin = 2;      // LED输出引脚
const int touchThreshold = 300; // 触摸阈值，数值低于300判定触摸
const unsigned long debounceDelay = 50; // 防抖50ms

// 状态变量
bool ledState = LOW;               // LED当前状态
bool lastTouchState = false;       // 上一轮触摸状态（边缘检测用）
unsigned long lastTouchTime = 0;   // 防抖时间戳

void setup() {
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, ledState); // 初始化灯熄灭
}

void loop() {
  // 1. 读取触摸原始数值
  int touchVal = touchRead(touchPin);
  bool currentTouch = (touchVal < touchThreshold);

  unsigned long now = millis();
  // 2. 防抖判断：距离上次有效触摸超过50ms才处理
  if ((now - lastTouchTime) > debounceDelay) {
    // 3. 边缘检测：上一次未触摸，当前触摸（上升沿瞬间）
    if (currentTouch == true && lastTouchState == false) {
      ledState = !ledState; // 翻转LED状态
      digitalWrite(ledPin, ledState);
      lastTouchTime = now;  // 更新防抖时间戳
      Serial.print("触摸触发，LED状态切换：");
      Serial.println(ledState ? "亮" : "灭");
    }
  }
  // 更新上一轮触摸状态，用于下一次边缘判断
  lastTouchState = currentTouch;
  delay(10);
}