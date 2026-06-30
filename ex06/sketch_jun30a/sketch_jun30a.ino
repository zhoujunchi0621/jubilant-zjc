/*
 * ex06 双通道互补PWM渐变警示灯
 * 程序功能概述：
 * 1. 两路独立PWM通道驱动两颗LED，亮度输出完全互补
 * 2. 第一颗灯缓慢提亮时，第二颗同步慢慢变暗，循环往复
 * 3. 灯光过渡顺滑无频闪，模拟警车柔和交替渐变灯光效果
 *
 * 硬件接线说明：
 * 主指示灯LED1接GPIO2（开发板自带蓝色LED，无需额外元件）
 * 副指示灯LED2接GPIO4，外接LED并串联220欧姆限流电阻保护灯珠
 */

// 两路灯光硬件引脚定义
const uint8_t lightCh1 = 2;
const uint8_t lightCh2 = 4;

// PWM波形配置参数
const uint16_t pwmFreqSetting = 5000;
const uint8_t pwmBitRange = 8;       // 亮度输出区间 0 ~ 255

void setup() {
  Serial.begin(115200);

  // 分别为两个IO口分配独立PWM通道
  ledcAttach(lightCh1, pwmFreqSetting, pwmBitRange);
  ledcAttach(lightCh2, pwmFreqSetting, pwmBitRange);

  Serial.println("双通道互补渐变灯光程序已运行");
}

void loop() {
  // 第一阶段：通道1由暗到亮，通道2同步由亮转暗
  for (int brightnessVal = 0; brightnessVal <= 255; brightnessVal++) {
    ledcWrite(lightCh1, brightnessVal);
    ledcWrite(lightCh2, 255 - brightnessVal);
    delay(8);
  }

  // 第二阶段：通道1由亮转暗，通道2同步由暗提亮
  for (int brightnessVal = 255; brightnessVal >= 0; brightnessVal--) {
    ledcWrite(lightCh1, brightnessVal);
    ledcWrite(lightCh2, 255 - brightnessVal);
    delay(8);
  }

  Serial.println("完成一轮完整明暗交替循环");
}