#include <WiFi.h>
#include <WebServer.h>

// AP无线热点参数
const char* hotSpotName = "ESP32_Alarm_Device";
const char* hotSpotPwd = "87654321";
WebServer httpService(80);

// 硬件IO引脚定义
#define SENSE_TOUCH_IO  4
#define WARN_LED_IO     2

// 触摸感应触发判定阈值
int touchTriggerLevel = 500;

// 设备工作模式：0=解除警戒 1=布防监控 2=入侵报警
int deviceWorkMode = 0;

// 首页网页渲染函数
void renderMainWebPage() {
    String webContent = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1.0">
  <title>物联网安防报警控制系统</title>
  <style>
    html,body{font-family:"Microsoft YaHei";text-align:center;margin-top:45px;background:#f8f9fa}
    .state-card{width:85%;max-width:420px;margin:24px auto;padding:22px;border-radius:12px;font-size:26px;font-weight:600;color:#fff}
    .safe-state{background:#38b000}
    .guard-state{background:#fb8500}
    .alarm-state{background:#d00000;animation:flash 0.45s infinite alternate}
    @keyframes flash{0%{opacity:1}100%{opacity:0.4}}
    .opt-btn{padding:16px 32px;font-size:17px;margin:12px 8px;border:0;border-radius:6px;color:#fff;cursor:pointer}
    .lock-btn{background:#fb8500}
    .unlock-btn{background:#38b000}
    .stop-alarm-btn{background:#0077b6}
    .tip-text{color:#555;font-size:15px;margin:10px 0}
    .hardware-desc{color:#777;font-size:13px;margin-top:25px}
  </style>
</head>
<body>
  <h1>ESP32无线安防监控终端</h1>
  <h3>触摸感应入侵报警系统</h3>

  <div id="statePanel" class="state-card safe-state">🟢 当前状态：安全解除警戒</div>

  <div class="button-group">
    <button class="opt-btn lock-btn" onclick="sendOperate('arm')">🔒 开启布防监控</button>
    <button class="opt-btn unlock-btn" onclick="sendOperate('disarm')">🔓 关闭警戒撤防</button>
    <button class="opt-btn stop-alarm-btn" id="shutAlarmBtn" style="display:none">🛑 终止警报提示</button>
  </div>

  <p class="tip-text">操作说明：开启布防后触碰GPIO4触摸引脚，系统将立刻触发入侵警报</p>
  <p class="tip-text">页面自动实时同步设备运行状态</p>
  <p class="hardware-desc">感应采集引脚：GPIO4(T0) | 报警指示灯：GPIO2片载LED</p>

  <script>
    // 向后端发送操作指令
    function sendOperate(optType){
      fetch("/operate?cmd="+optType).then(()=>{
        setTimeout(refreshDeviceState,120);
      })
    }
    // 更新页面显示设备工作状态
    function refreshDeviceState(){
      fetch("/getMode").then(res=>res.text()).then(mode=>{
        let panel = document.getElementById("statePanel");
        let stopBtn = document.getElementById("shutAlarmBtn");
        panel.className = "state-card";
        document.body.style.background = "#f8f9fa";
        if(mode === "0"){
          panel.classList.add("safe-state");
          panel.innerText = "🟢 当前状态：安全解除警戒";
          stopBtn.style.display = "none";
        }else if(mode === "1"){
          panel.classList.add("guard-state");
          panel.innerText = "🟡 当前状态：布防警戒中";
          stopBtn.style.display = "none";
        }else if(mode === "2"){
          panel.classList.add("alarm-state");
          panel.innerText = "🔴 紧急警报！检测到非法触碰入侵";
          stopBtn.style.display = "inline-block";
          document.body.style.background = "#ffe0e0";
        }
      }).catch(err=>{})
    }
    // 定时轮询状态
    setInterval(refreshDeviceState,1000);
    refreshDeviceState();
  </script>
</body>
</html>
)rawliteral";
    httpService.send(200, "text/html; charset=utf-8", webContent);
}

// 处理前端下发控制指令接口
void handleOperateRequest() {
    if(httpService.hasArg("cmd")){
        String operateCmd = httpService.arg("cmd");
        if(operateCmd == "arm"){
            deviceWorkMode = 1;
            Serial.println("系统指令接收：启动布防警戒模式");
        }else if(operateCmd == "disarm"){
            deviceWorkMode = 0;
            digitalWrite(WARN_LED_IO, LOW);
            Serial.println("系统指令接收：解除全部警戒状态");
        }else if(operateCmd == "clear"){
            deviceWorkMode = 0;
            digitalWrite(WARN_LED_IO, LOW);
            Serial.println("系统指令接收：关闭警报恢复安全");
        }
        httpService.send(200, "text/plain", "success");
    }else{
        httpService.send(400, "text/plain", "request param error");
    }
}

// 获取设备当前工作状态接口
void returnDeviceMode() {
    httpService.send(200, "text/plain", String(deviceWorkMode));
}

void setup() {
    Serial.begin(115200);
    // 初始化报警LED输出引脚
    pinMode(WARN_LED_IO, OUTPUT);
    digitalWrite(WARN_LED_IO, LOW);

    Serial.println("\n==== ESP32触摸式安防报警系统启动 ====");
    Serial.print("正在创建无线热点：");
    Serial.println(hotSpotName);
    WiFi.softAP(hotSpotName, hotSpotPwd);

    IPAddress apLocalIp = WiFi.softAPIP();
    Serial.print("热点本地访问IP：");
    Serial.println(apLocalIp);
    Serial.print("浏览器访问地址：http://");
    Serial.println(apLocalIp);

    // 绑定所有服务路由
    httpService.on("/", HTTP_GET, renderMainWebPage);
    httpService.on("/operate", HTTP_GET, handleOperateRequest);
    httpService.on("/getMode", HTTP_GET, returnDeviceMode);

    httpService.begin();
    Serial.println("Web服务已成功启动，等待客户端连接操作");

    // 触摸传感器基准值采集校准
    Serial.println("\n==== 触摸感应模块校准流程 ====");
    Serial.println("3秒内请勿触碰GPIO4感应引脚，采集基准数值");
    delay(3000);
    int baseTouchValue = touchRead(SENSE_TOUCH_IO);
    Serial.print("环境空载触摸基准值：");
    Serial.println(baseTouchValue);
    Serial.print("入侵触发判定阈值：小于 ");
    Serial.println(touchTriggerLevel);
    Serial.println("================================\n");
}

void loop() {
    // 持续处理网页客户端请求
    httpService.handleClient();

    // 布防状态下实时检测触摸信号
    if(deviceWorkMode == 1){
        int currentTouchVal = touchRead(SENSE_TOUCH_IO);
        if(currentTouchVal < touchTriggerLevel){
            deviceWorkMode = 2;
            Serial.print("入侵告警触发！触摸采集数值：");
            Serial.println(currentTouchVal);
        }
    }

    // 报警状态下LED交替闪烁警示
    if(deviceWorkMode == 2){
        digitalWrite(WARN_LED_IO, !digitalRead(WARN_LED_IO));
        delay(100);
    }
}