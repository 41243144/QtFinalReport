/*
    本專案使用 ESP32 控制植物澆水和照明，並且透過 MQTT 通訊協議與外部服務器進行通訊。
        ArduinoJson，該函式庫可以幫助我們處理 JSON 資料。
        PubSubClient，該函式庫可以幫助我們與 MQTT 伺服器進行通訊。
    本專案的目標是透過 MQTT 通訊協議，與外部服務器進行通訊，並且根據外部服務器的指令，控制植物的澆水和照明。

    本專案的主要功能如下：
        1. 透過 MQTT 通訊協議，與外部服務器進行通訊。
        2. 根據外部服務器的指令，控制植物的澆水和照明。
        3. 透過 NTP 時間同步，獲取當前的時間。
        4. 透過 WiFi 連接到網際網路。

    本專案的硬體設備如下：
        1. ESP32 開發板
        2. L298N 馬達驅動器
        3. 水泵
        4. LED 燈
        5. 水位感測器
    
    本專案的 MQTT 訊息格式如下：
        {
            "id": "test-0001",
            "command": "getCurrentData"
        }
        {
            "id": "test-0001",
            "command": "updateData",
            "wateringTime": "08:00",
            "lightStart": "08:00",
            "duration": 120
        }
        {
            "id": "test-0001",
            "command": "realTimeIrrigation"
        }
    

    本專案的硬體連接如下：
        1. 將 L298N 馬達驅動器的 IN1 和 IN2 腳分別連接到 ESP32 開發板的 19 和 21 腳。
        2. 將水泵的正極連接到 L298N 馬達驅動器的 OUT1 腳，將水泵的負極連接到 L298N 馬達驅動器的 GND 腳。
        3. 將 LED 燈的正極連接到 ESP32 開發板的 5 腳，將 LED 燈的負極連接到 ESP32 開發板的 GND 腳。
        4. 將水位感測器的 OUT 腳連接到 ESP32 開發板的 13 腳。

*/
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// WiFi 帳密
const char *ssid = "TOTOLINK_LR350"; // Wi-Fi name
const char *password = "940414881027";  // Wi-Fi password


// NTP 時間同步變數
const long gmtOffset_sec = 3600;         // 時間偏移量（秒）
const int daylightOffset_sec = 3600;     // 夏令時偏移量（秒）
const char *ntpServer = "pool.ntp.org";  // NTP 伺服器地址

// 硬體設備基本變數
char id[20] = "test-0001";
char wateringTime[10] = "00:00";
char localtimes[10] = "00:00";
char localtimesoff[10] = "00:00"; // 用於關燈
char lightStart[10] = "00:00";
short int duration = 120;
bool irrigation = false; // 是否正在澆水
#define L298N_1 19
#define L298N_2 21
#define Light 5
#define WaterSensor 13

unsigned long lastWateringTime = 0; // 上次澆水的時間戳
const unsigned long wateringInterval = 60000; // 澆水間隔 1 分鐘（以毫秒為單位）

// MQTT 基本資訊
char *mqtt_broker = "broker.hivemq.com";
char *topic = "wenwen/test";
char *mqtt_username = "wenwen";
char *mqtt_password = "test";
int mqtt_port = 1883;

// 初始化 MQTT
WiFiClient espClient;
PubSubClient client(espClient);

// 標誌變數，避免處理自己發送的訊息
bool isPublishing = false;

// 獲取當前資料
StaticJsonDocument<512> getCurrentData() {
    StaticJsonDocument<512> returnJson;
    returnJson["id"] = id;
    returnJson["wateringTime"] = wateringTime;
    returnJson["lightStart"] = lightStart;
    returnJson["duration"] = duration;

    return returnJson;
}

// 更新資料
StaticJsonDocument<512> updateData(StaticJsonDocument<512> updateData) {
    StaticJsonDocument<512> returnJson;
    returnJson["id"] = updateData["id"];

    if (updateData.containsKey("wateringTime") && updateData.containsKey("lightStart") && updateData.containsKey("duration")) {
        // 更新全域變數
        strncpy(wateringTime, updateData["wateringTime"], sizeof(wateringTime));
        strncpy(lightStart, updateData["lightStart"], sizeof(lightStart));
        duration = updateData["duration"].as<int>();

        returnJson["status"] = "success";
        returnJson["error"] = "";
    } else {
        returnJson["status"] = "error";
        returnJson["error"] = "未包含 wateringTime lightStart duration";
    }
    return returnJson;
}

// 即時澆水
StaticJsonDocument<512> realTimeIrrigation(StaticJsonDocument<512> updateData) {
    StaticJsonDocument<512> returnJson;

    returnJson["id"] = updateData["id"];

    if (irrigation) {
        returnJson["status"] = "error";
        returnJson["error"] = "當前正在澆水";
    } else {
        // 開始澆水操作
        irrigation = true;
        openWater(); // 打開水泵
        Serial.println("即時澆水已啟動");
        returnJson["status"] = "success";
        returnJson["error"] = "";
    }
   
    return returnJson;
}

// 處理未知命令
StaticJsonDocument<512> unknownCommand(StaticJsonDocument<512> updateData){
    StaticJsonDocument<512> returnJson;

    returnJson["id"] = updateData["id"];
    returnJson["status"] = "error";
    returnJson["error"] = "未知的命令";
    return returnJson;
}

// 發送 MQTT 訊息
void publishMessage(String message) {
    isPublishing = true; // 標記為正在發送
    client.publish(topic, message.c_str());
    isPublishing = false; // 發送完畢後重置標誌
}

// 處理 MQTT 訊息回呼
void callback(char *topic, byte *payload, unsigned int length) {

    String message;
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, message);

    if (error) {
        Serial.println("接收到的並不是 JSON 檔案: ");
        Serial.println(error.c_str());
    } else {
        if (!doc.containsKey("id") || doc["id"] != id) {
            Serial.println("ID 不匹配");
            return;
        }

        if (doc.containsKey("command")) {
            String command = doc["command"].as<String>();
            StaticJsonDocument<512> result;

            if (command == "getCurrentData") {
                result = getCurrentData();
            } else if (command == "updateData") {
                result = updateData(doc);
            } else if (command == "realTimeIrrigation") {
                result = realTimeIrrigation(doc);
            } else {
                result = unknownCommand(doc);
            }

            String response;
            serializeJson(result, response);
            publishMessage(response); // 發送回應
        }
    }
}


// 獲取當前時間（放入字串）
// 獲取當前時間（放入字串）
void LocalTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return;
    }

    // 增加 7 小時
    timeinfo.tm_hour += 7;

    // 處理進位，確保小時不超過 24
    if (timeinfo.tm_hour >= 24) {
        timeinfo.tm_hour -= 24;
    }

    char myHour[3], myMinute[3];
    strftime(myHour, sizeof(myHour), "%H", &timeinfo);
    strftime(myMinute, sizeof(myMinute), "%M", &timeinfo);
    snprintf(localtimes, sizeof(localtimes), "%s:%s", myHour, myMinute);
}


// 計算關燈時間
void LocalTimeoff() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return;
    }

    // 獲取當前時間的總分鐘數
    int hour = timeinfo.tm_hour;
    int minute = timeinfo.tm_min;

    // 計算加上持續時間後的總分鐘數
    int totalMinutes = hour * 60 + minute + duration;

    // 計算新的小時和分鐘
    hour = ((totalMinutes / 60) % 24)+7;
    minute = totalMinutes % 60;

    // 格式化為 HH:MM
    snprintf(localtimesoff, sizeof(localtimesoff), "%02d:%02d", hour, minute);
    Serial.print("關燈時間已設置為: ");
    Serial.println(localtimesoff);
}

// 澆水檢查
bool waterCheck() {
  
    return digitalRead(WaterSensor)  < 1;
}

// 開燈
void openLight() {
    digitalWrite(Light, HIGH);
}

// 關燈
void offLight() {
    digitalWrite(Light, LOW);
}

// 開啟水泵
void openWater() {
          
    digitalWrite(L298N_1, HIGH);Serial.println("...");

}

// 關閉水泵
void offWater() {
    digitalWrite(L298N_1, LOW);
}

// 初始化 Wi-Fi 和 MQTT
void setup() {
    pinMode(L298N_1,OUTPUT);
    pinMode(L298N_2,OUTPUT);
    pinMode(Light,OUTPUT);
    pinMode(WaterSensor,INPUT);
    digitalWrite(L298N_1, LOW);
    digitalWrite(L298N_2, LOW);

    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.println("連接 Wi-Fi...");
    }
    Serial.println("成功連接!");

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    client.setServer(mqtt_broker, mqtt_port);
    client.setCallback(callback);
    
    while (!client.connected()) {
        String client_id = "esp32-client-" + String(WiFi.macAddress());
        if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
            Serial.println("MQTT 伺服器已連接!");
        } else {
            Serial.print("MQTT 連接失敗, 狀態: ");
            Serial.println(client.state());
            delay(2000);
        }
    }
    client.subscribe(topic);
}

// 主循環邏輯
void loop() {
    client.loop(); // 處理 MQTT 客戶端
    LocalTime();   // 更新當前時間
    Serial.print("當前時間: ");
    Serial.println(localtimes);
    Serial.print("預計澆水時間: ");
    Serial.println(wateringTime);
    Serial.print("預計開燈時間: ");
    Serial.println(lightStart);
    Serial.print("預計關燈時間: ");
    Serial.println(localtimesoff);

    // 開燈邏輯
    if (String(localtimes) == String(lightStart)) {
        openLight();
        LocalTimeoff(); // 設定關燈時間
    }

    // 關燈邏輯
    if (String(localtimes) == String(localtimesoff)) {
        offLight();
        Serial.println("燈已關閉");
    }

   // 澆水邏輯
    if (irrigation) {
        if (!waterCheck()) {
            offWater();
            irrigation = false;
            Serial.println("澆水完成，已關閉水泵");
        }
    } else {
        // 獲取當前的時間（以毫秒為單位）
        unsigned long currentMillis = millis();
    
        // 如果當前時間和設定的澆水時間一致，且與上次澆水間隔超過 1 分鐘
        if (String(localtimes) == String(wateringTime) && 
            currentMillis - lastWateringTime > wateringInterval) {
            
            openWater();
            irrigation = true;
            lastWateringTime = currentMillis; // 更新上次澆水時間
            Serial.println("已開始澆水");
        }
    }
    delay(1000);
}