/*
 * ex09 电容触摸实时数据监测仪表盘系统
 * 实验功能说明：
 * 1. ESP32搭建网页服务，同时实现页面下发与传感器数据上传双向通信
 * 2. 前端页面通过定时AJAX轮询接口，不间断获取触摸引脚原始采集数值
 * 3. 页面中央大号数字实时刷新读数，搭配进度条直观展示信号强弱
 * 4. 人体靠近GPIO4触摸引脚时检测值降低，松开后数值自动回升
 * 硬件配置说明：
 * 电容触摸采集通道T0，硬件引脚GPIO4，无需额外外围电路
 */
#include <WiFi.h>
#include <WebServer.h>

// 无线网络接入凭证
const char* wifiAccount = "ESP32-LAB124";
const char* wifiPasswd = "ydl20060621";

// 电容触摸采集引脚定义
const uint8_t touchCollectIO = 4;

// 网页服务绑定80标准端口
WebServer webService(80);

// 构造传感器监控面板前端页面
String buildMonitorHtml() {
  String pageCode = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ESP32触摸信号监测仪表</title>
<style>
*{margin:0;padding:0;box-sizing:border-box;font-family:"Microsoft YaHei"}
body{background:#080c16;width:100%;min-height:100vh;display:flex;align-items:center;justify-content:center}
.monitor-box{width:92%;max-width:460px;background:#121a2b;padding:45px 30px;border-radius:22px;border:1px solid #25334e}
.title-text{font-size:28px;color:#e8edf7;text-align:center;margin-bottom:36px}
.desc-tip{font-size:16px;color:#a0b4d8;text-align:center;margin-bottom:22px}
.data-num{font-size:6rem;font-weight:bold;text-align:center;padding:24px 0;background:#0b101b;border-radius:16px;margin-bottom:28px;color:#ffffff}
.progress-track{width:100%;height:12px;background:#1f2c44;border-radius:6px;overflow:hidden}
.progress-fill{height:100%;width:0%;background:#3cd086;transition:width 0.25s ease}
.bottom-notice{margin-top:32px;text-align:center;color:#788fb3;font-size:15px}
</style>
</head>
<body>
<div class="monitor-box">
  <h2 class="title-text">电容触摸实时监测仪</h2>
  <p class="desc-tip">传感器原始采集数值</p>
  <div id="dataShow" class="data-num">----</div>
  <div class="progress-track">
    <div id="progressBar" class="progressFill"></div>
  </div>
  <p class="bottom-notice">手指贴近触摸引脚 → 检测数值变小；移开后数值恢复升高</p>
</div>

<script>
// 获取页面显示元素
let numPanel = document.getElementById("dataShow");
let barElement = document.getElementById("progressBar");
// 每180毫秒请求一次传感器数据
function refreshSensorData(){
  fetch("/getSensorData")
  .then(res=>{
    if(!res.ok) throw "数据请求异常";
    return res.text();
  })
  .then(rawData=>{
    let sensorVal = parseInt(rawData.trim());
    if(isNaN(sensorVal)) return;
    numPanel.innerText = sensorVal;
    // 数值映射进度条区间0~1200
    let fullRange = 1200;
    let scalePercent = Math.max(0,Math.min(100,(sensorVal/fullRange)*100));
    barElement.style.width = scalePercent + "%";
    // 触摸低数值切换红色警示
    if(sensorVal < 400){
      numPanel.style.color = "#ff6363";
    }else{
      numPanel.style.color = "#ffffff";
    }
  })
  .catch(error=>{
    numPanel.innerText = "读取失败";
    console.log("轮询报错：",error);
  })
}
// 定时循环拉取数据
setInterval(refreshSensorData,180);
</script>
</body>
</html>
)rawliteral";
  return pageCode;
}

// 主页路由：返回监测面板页面
void pageIndexHandler() {
  webService.send(200, "text/html; charset=utf-8", buildMonitorHtml());
}

// 数据接口：返回当前触摸原始采样值
void sensorDataHandler() {
  int touchRawValue = touchRead(touchCollectIO);
  webService.send(200, "text/plain", String(touchRawValue));
}

void setup() {
  Serial.begin(115200);
  delay(800);

  // WiFi连接流程
  Serial.print("正在连接无线网络：");
  Serial.println(wifiAccount);
  WiFi.begin(wifiAccount, wifiPasswd);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print("·");
  }
  Serial.println("\nWiFi接入成功");
  Serial.print("监测面板访问地址：http://");
  Serial.println(WiFi.localIP());

  // 注册网页服务接口路由
  webService.on("/", pageIndexHandler);
  webService.on("/getSensorData", sensorDataHandler);
  webService.begin();
  Serial.println("实时传感器仪表盘服务已启动");
}

void loop() {
  // 持续处理前端AJAX请求
  webService.handleClient();
}