/*
 * ex08 ESP32物联网安防报警器
 * 实验核心功能：
 * 1. 搭建ESP32网页服务，提供布防、撤防控制按钮
 * 2. 采用全局变量存储系统工作状态与报警锁定状态
 * 3. 系统布防状态下，触摸GPIO4引脚触发报警锁定
 * 4. 报警触发后LED持续高频闪烁，松开触摸不解除报警
 * 5. 仅网页点击撤防按钮可重置系统、关闭报警灯光
 * 6. 未布防状态下，触摸引脚无任何响应
 * 
 * 硬件适配：
 * 触摸检测引脚：GPIO4(T0)
 * 状态指示灯：GPIO2（板载自带LED）
 */
#include <WiFi.h>
#include <WebServer.h>

// 无线网络配置信息
const char* wifiAccount = "ESP32-LAB124";
const char* wifiPwd = "ydl20060621";

// 硬件引脚定义
const uint8_t touchSensorPin = 4;
const uint8_t alarmLedPin = 2;

// 触摸检测参数（适配高数值触摸硬件）
const int touchSensitivity = 1200;
const unsigned int shakeEliminateTime = 50;

// 报警灯光闪烁速率
const unsigned int ledFlashInterval = 150;

// 初始化网页服务端口
WebServer webServer(80);

// 系统全局状态变量
bool systemArmState = false;     // 系统布防状态
bool alarmLockState = false;     // 报警锁定状态
bool ledWorkState = false;       // LED当前电平状态

// 触摸防抖检测变量
bool preRawTouchState = false;
bool stableTouchState = false;
bool preStableTouchState = false;
unsigned int shakeTimer = 0;

// LED非阻塞闪烁计时变量
unsigned long lastFlashTime = 0;

// 构建安防系统网页界面
String buildWebPage() {
  String stateInfo;
  String armBtn, disarmBtn;

  // 根据系统状态动态渲染页面按钮与提示文字
  if (alarmLockState) {
    stateInfo = "⚠️ 设备报警锁定！请立即撤防";
    armBtn = "";
    disarmBtn = "<a href=\"/disarm\"><button style=\"padding:12px 26px;font-size:17px;background:#e53935;color:#fff;border:none;border-radius:8px;cursor:pointer\">撤防解除警报</button></a>";
  } else if (systemArmState) {
    stateInfo = "✅ 系统已布防，处于警戒监测状态";
    armBtn = "";
    disarmBtn = "<a href=\"/disarm\"><button style=\"padding:12px 26px;font-size:17px;background:#757575;color:#fff;border:none;border-radius:8px;cursor:pointer\">一键撤防</button></a>";
  } else {
    stateInfo = "🔓 系统未布防，无监测功能";
    armBtn = "<a href=\"/arm\"><button style=\"padding:12px 26px;font-size:17px;background:#2e7d32;color:#fff;border:none;border-radius:8px;cursor:pointer\">开启布防</button></a>";
    disarmBtn = "";
  }

  // 规整自适应网页代码，修复页面错乱BUG
  String htmlContent = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>智能安防报警系统</title>
  <style>
    body {
      font-family: "Microsoft YaHei", sans-serif;
      background-color: #121721;
      color: #ffffff;
      text-align: center;
      padding-top: 70px;
      margin: 0;
    }
    .main-box {
      width: 85%;
      max-width: 420px;
      margin: 0 auto;
      background-color: #1e2532;
      padding: 35px 20px;
      border-radius: 16px;
      box-shadow: 0 4px 20px rgba(0,0,0,0.25);
    }
    h2 {
      margin: 0 0 25px 0;
      font-size: 24px;
    }
    .state-show {
      font-size: 20px;
      margin-bottom: 35px;
      letter-spacing: 1px;
    }
    button {
      margin: 0 8px;
      transition: 0.2s;
    }
    button:hover {
      opacity: 0.9;
    }
  </style>
</head>
<body>
  <div class="main-box">
    <h2>ESP32智能安防主机</h2>
    <p class="state-show">当前系统状态：<strong>)rawliteral" + stateInfo + R"rawliteral(</strong></p>
    )rawliteral" + armBtn + disarmBtn + R"rawliteral(
  </div>
</body>
</html>
)rawliteral";
  return htmlContent;
}

// 主页路由：加载安防控制面板
void rootPageHandle() {
  webServer.send(200, "text/html; charset=utf-8", buildWebPage());
}

// 系统布防接口
void armSystemHandle() {
  systemArmState = true;
  alarmLockState = false;
  digitalWrite(alarmLedPin, LOW);
  Serial.println("系统成功布防，进入警戒模式");
  webServer.sendHeader("Location", "/");
  webServer.send(303);
}

// 系统撤防接口
void disarmSystemHandle() {
  systemArmState = false;
  alarmLockState = false;
  digitalWrite(alarmLedPin, LOW);
  Serial.println("系统撤防成功，报警状态已重置");
  webServer.sendHeader("Location", "/");
  webServer.send(303);
}

void setup() {
  Serial.begin(115200);
  pinMode(alarmLedPin, OUTPUT);
  digitalWrite(alarmLedPin, LOW);

  // WiFi带容错连接
  Serial.print("正在连接无线网络：");
  Serial.println(wifiAccount);
  WiFi.begin(wifiAccount, wifiPwd);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi连接成功！");
  Serial.print("系统访问地址：http://");
  Serial.println(WiFi.localIP());

  // 注册标准路由接口
  webServer.on("/", rootPageHandle);
  webServer.on("/arm", armSystemHandle);
  webServer.on("/disarm", disarmSystemHandle);
  webServer.begin();
  Serial.println("安防网页服务启动完成");
}

void loop() {
  webServer.handleClient();

  // 触摸信号采集与软件防抖
  int touchValue = touchRead(touchSensorPin);
  bool currentTouch = (touchValue < touchSensitivity);

  if (currentTouch != preRawTouchState) {
    shakeTimer = millis();
  }

  if ((millis() - shakeTimer) > shakeEliminateTime) {
    if (currentTouch != stableTouchState) {
      stableTouchState = currentTouch;
    }
  }

  // 触摸上升沿触发报警（核心功能修复）
  if (!preStableTouchState && stableTouchState) {
    if (systemArmState && !alarmLockState) {
      alarmLockState = true;
      Serial.println("检测到触碰入侵，报警已锁定！");
    }
  }

  // 更新触摸状态缓存
  preStableTouchState = stableTouchState;
  preRawTouchState = currentTouch;

  // 报警状态持续闪烁灯光
  if (alarmLockState) {
    if (millis() - lastFlashTime >= ledFlashInterval) {
      lastFlashTime = millis();
      ledWorkState = !ledWorkState;
      digitalWrite(alarmLedPin, ledWorkState);
    }
  } else {
    digitalWrite(alarmLedPin, LOW);
  }
}
