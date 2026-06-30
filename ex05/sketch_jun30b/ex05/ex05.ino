/*
 * ESP32 三档触摸可调速呼吸灯实验 ex05
 * 实验功能简述：
 * 1. 板载LED(GPIO2)持续平滑呼吸渐变
 * 2. 电容触摸通道T0(GPIO4)单次触碰切换运行档位
 * 3. 共3档呼吸速率，循环切换模式：1→2→3→1
 * 4. 采用软件消抖过滤触摸杂波，上升沿触发切换，避免误动作
 * 5. 非阻塞时间驱动PWM，不占用触摸检测响应
 */

// 硬件引脚分配
const uint8_t touchSignalPin = 4;   // 电容触摸通道T0
const uint8_t lightOutputPin = 2;   // 开发板自带LED引脚

// PWM波形配置参数
const uint16_t pwmFrequency = 5000;
const uint8_t pwmBitDepth = 8;       // 0~255亮度区间

// 触摸判定阈值（适配本机触摸读数，触摸后数值低于1200判定有效触碰）
const int touchJudgeThreshold = 1200;

// 消抖稳定等待时长(ms)
const unsigned int shakeFilterMs = 50;

// 三档呼吸刷新间隔，数值越小明暗变化越快
const int breathDelayTable[3] = {15, 7, 3};

// 触摸消抖、边沿检测缓存变量
bool prevRawTouchState = false;
bool realTimeRawTouch = false;
bool touchStableState = false;
bool lastStableTouchRecord = false;
unsigned long shakeFilterTimer = 0;

// 呼吸灯运行参数
int pwmBrightness = 0;
int brightnessChangeDir = 1;         // 1=变亮 -1=变暗
int runGear = 1;                    // 当前运行档位 1/2/3
unsigned long lastPwmUpdateTick = 0;

void setup() {
  Serial.begin(115200);

  // 绑定LED PWM通道
  ledcAttach(lightOutputPin, pwmFrequency, pwmBitDepth);
  ledcWrite(lightOutputPin, pwmBrightness);

  Serial.println("=== ex05 触摸调速呼吸灯程序启动 ===");
  Serial.print("初始运行档位：");
  Serial.println(runGear);
  Serial.println("触摸引脚：GPIO4(D4)");
}

void loop() {
  // 第一部分：读取触摸信号 + 软件消抖处理
  int touchRawValue = touchRead(touchSignalPin);
  realTimeRawTouch = (touchRawValue < touchJudgeThreshold);

  // 触摸电平发生变化，重置消抖计时
  if (realTimeRawTouch != prevRawTouchState) {
    shakeFilterTimer = millis();
  }

  // 等待信号稳定后更新触摸判定状态
  if ((millis() - shakeFilterTimer) > shakeFilterMs) {
    if (realTimeRawTouch != touchStableState) {
      touchStableState = realTimeRawTouch;
    }
  }

  // 上升沿检测：无触摸 → 刚触摸瞬间切换档位
  if (!lastStableTouchRecord && touchStableState) {
    if (runGear >= 3) {
      runGear = 1;
    } else {
      runGear = runGear + 1;
    }
    Serial.print("检测到有效触摸，切换至档位：");
    Serial.println(runGear);
  }

  // 保存上一轮状态用于下一次边沿判断
  lastStableTouchRecord = touchStableState;
  prevRawTouchState = realTimeRawTouch;

  // 第二部分：非阻塞呼吸灯亮度渐变
  int currentRefreshGap = breathDelayTable[runGear - 1];
  if (millis() - lastPwmUpdateTick >= currentRefreshGap) {
    lastPwmUpdateTick = millis();

    pwmBrightness += brightnessChangeDir;

    // 亮度到达极值反转变化方向
    if (pwmBrightness >= 255) {
      pwmBrightness = 255;
      brightnessChangeDir = -1;
    } else if (pwmBrightness <= 0) {
      pwmBrightness = 0;
      brightnessChangeDir = 1;
    }

    ledcWrite(lightOutputPin, pwmBrightness);
  }

  // 短暂延时，减少CPU占用，不影响触摸实时采集
  delay(1);
}