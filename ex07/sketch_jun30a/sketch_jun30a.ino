/*
 * ex07 ESP32网页亮度无级调节系统
 * 实现功能说明：
 * 1. ESP32接入局域网WiFi，内置网页服务页面
 * 2. 客户端手机/电脑同WiFi访问设备IP，打开调光控制面板
 * 3. 页面内置0~255区间滑动调节条，拖动实时下发亮度参数
 * 4. JS采用fetch异步GET传输数值，ESP32解析参数控制PWM输出LED亮度
 * 硬件配置：
 * 板载LED输出引脚GPIO2，使用硬件PWM驱动实现平滑亮度变化
 */
#include <WiFi.h>
#include <WebServer.h>

// 无线网络账号密码
const char* wifiName = "ESP32-LAB124";
const char* wifiPwd = "ydl20060621";

// PWM硬件参数配置
const uint8_t ledOutputIO = 2;
const uint16_t pwmWaveFreq = 5000;
const uint8_t pwmBitWidth = 8; // 亮度输出区间0~255

// 搭建80端口网页服务
WebServer webService(80);
// 全局存储当前LED亮度值
int lightLv = 0;

// 构造前端控制面板页面
String buildWebPage() {
  String pageCode = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32灯光调节器</title>
  <style>
    html,body{margin:0;padding:0;font-family:Microsoft YaHei;background:#222730;color:#fff}
    .box{width:90%;max-width:420px;margin:60px auto;padding:35px;border-radius:16px;background:#303845}
    h2{text-align:center;margin-top:0}
    .slide-bar{width:100%;height:14px;margin:30px 0}
    .num-show{font-size:28px;text-align:center;color:#42b983;font-weight:bold}
    .tip{text-align:center;margin-top:20px;font-size:14px;color:#aaa}
  </style>
</head>
<body>
  <div class="box">
    <h2>LED无级亮度控制面板</h2>
    <input class="slide-bar" type="range" id="lightSlider" min="0" max="255" value="0" step="1">
    <div>当前亮度数值：<span id="numDisplay" class="num-show">0</span></div>
    <div class="tip">拖动滑块实时调节开发板LED明暗</div>
  </div>

  <script>
    // 获取页面DOM元素
    let slider = document.getElementById("lightSlider");
    let textNum = document.getElementById("numDisplay");

    // 初始化加载时读取设备当前亮度
    fetch("/readLight")
      .then(res => res.text())
      .then(data => {
        slider.value = data;
        textNum.innerText = data;
      })
      .catch(e => console.log("初始化亮度读取失败",e));

    // 滑块拖动监听事件
    slider.oninput = function(){
      let nowVal = this.value;
      textNum.innerText = nowVal;
      // GET请求发送亮度数值到服务器
      fetch("/setLight?data="+nowVal)
        .catch(err => console.log("亮度下发请求异常",err));
    }
  </script>
</body>
</html>
)rawliteral";
  return pageCode;
}

// 首页路由：返回完整控制面板页面
void pageRootHandler() {
  webService.send(200, "text/html; charset=utf-8", buildWebPage());
}

// 接收滑块上传的亮度参数并更新PWM
void setLightHandler() {
  if(webService.hasArg("data")){
    int inputVal = webService.arg("data").toInt();
    // 数值边界约束，防止超出0-255范围
    if(inputVal < 0) inputVal = 0;
    if(inputVal > 255) inputVal = 255;
    lightLv = inputVal;
    ledcWrite(ledOutputIO, lightLv);
    webService.send(200, "text/plain", "success");
    Serial.print("已更新灯光亮度：");
    Serial.println(lightLv);
  }else{
    webService.send(400, "text/plain", "缺少亮度参数data");
  }
}

// 查询当前灯光亮度接口
void getLightHandler() {
  webService.send(200, "text/plain", String(lightLv));
}

void setup() {
  Serial.begin(115200);
  // 绑定LED PWM通道
  ledcAttach(ledOutputIO, pwmWaveFreq, pwmBitWidth);
  ledcWrite(ledOutputIO, lightLv);

  // WiFi连接流程
  Serial.print("正在尝试连接无线网络");
  WiFi.begin(wifiName, wifiPwd);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print("·");
  }
  Serial.println("\nWiFi连接成功！");
  Serial.print("访问调光页面地址：http://");
  Serial.println(WiFi.localIP());

  // 注册全部网络接口路由
  webService.on("/", pageRootHandler);
  webService.on("/setLight", HTTP_GET, setLightHandler);
  webService.on("/readLight", HTTP_GET, getLightHandler);
  webService.begin();
  Serial.println("ESP32网页调光服务已启动");
}

void loop() {
  // 持续处理浏览器发来的网络请求
  webService.handleClient();
}