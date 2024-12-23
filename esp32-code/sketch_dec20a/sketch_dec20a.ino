#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// WiFi 帳密
const char *ssid = "wenwen 的 S24 Ultra"; // Enter your Wi-Fi name
const char *password = "********";  // Enter Wi-Fi password

// 硬體設備基本變數
char id[20] = "test-0001";
char wateringTime[10] = "00:00";
char lightStart[10] = "00:00";
short int duration = 120;
bool irrigation = false; // 是否正在澆水

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
    returnJson["command"] = updateData["command"];

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
StaticJsonDocument<512> realTimeIrrigation(StaticJsonDocument<512> updateData){
    StaticJsonDocument<512> returnJson;

    returnJson["id"] = updateData["id"];
    returnJson["command"] = updateData["command"];

    if(irrigation == true){
        returnJson["status"] = "error";
        returnJson["error"] = "當前正在澆水";
    }
    return returnJson;
}

// 處理未知命令
StaticJsonDocument<512> unknownCommand(StaticJsonDocument<512> updateData){
    StaticJsonDocument<512> returnJson;

    returnJson["id"] = updateData["id"];
    returnJson["command"] = updateData["command"];
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
    // 如果正在發送訊息，則跳過處理
    if (isPublishing) {
        Serial.println("跳過自己發送的訊息");
        return;
    }

    Serial.print("接收訊息: ");
    Serial.println(topic);

    // 將 payload 轉換為字串格式
    String message;
    for (int i = 0; i < length; i++) {
        message += (char)payload[i];
    }

    // 嘗試解析 JSON
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, message);

    if (error) {
        Serial.print("Message: ");
        Serial.println(message);
        Serial.print("接收到的並不是 JSON 檔案: ");
        Serial.println(error.c_str());
    } else {
        Serial.println("成功接收 JSON 檔案");

        // 確認 ID
        if (!doc.containsKey("id") || doc["id"] != id) {
            Serial.println("沒獲取到 ID 或 ID 不正確");
            return;
        }

        // 命令處理
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

            // 發送回應
            String response;
            serializeJson(result, response);
            publishMessage(response);
        }
    }
}

// 初始化 Wi-Fi 和 MQTT
void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.println("連接 Wi-Fi..");
    }
    Serial.println("成功連接!");

    client.setServer(mqtt_broker, mqtt_port);
    client.setCallback(callback);

    while (!client.connected()) {
        String client_id = "esp32-client-";
        client_id += String(WiFi.macAddress());
        Serial.printf("The client %s connects to the public MQTT broker\n", client_id.c_str());
        if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
            Serial.println("Public EMQX MQTT broker connected");
        } else {
            Serial.print("failed with state ");
            Serial.println(client.state());
            delay(2000);
        }
    }
    client.subscribe(topic);
}

void loop() {
    client.loop();
    delay(10);
}
