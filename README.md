# Qt 期末報告(自動化澆水系統)

## 組員&分工

| 學號  | 姓名 | **工作內容** |
| :--: | :--: | :--: |
| 41243116  | 余家豪 | **ESP32程式撰寫 & 硬體架設** |
| 41243144  | 温博鈞 | **MQTT傳輸 & 硬體傳輸串接** |
| 41243146  | 黃境安 | **GUI介面** |



## 計畫書

##### 使用者利用GUI控制澆水時間以及植物燈開關，利用MQTT將控制訊號傳給ESP32，GUI反饋澆水時間以及環境資訊。

## 傳輸介面(Qt傳給Esp32)

##### `getCurrentData` : 獲取當前澆水時間及植物燈設定時間，回傳Json格式。

| 回傳  | **內容** |
| :--: | :--: |
| id | **設備id** |
| wateringTime | **澆水時間** |
| lightStart | **植物燈開啟時間** |
| duration | **植物燈開燈持續時間(分鐘)** |

發送格式:
```json
{
    "id" : "test-0001",
    "command" : "getCurrentData",
}
回傳範例:
```json
{
    "id" : "test-0001",
    "wateringTime" : "12:00",
    "lightStart" : "20:00",
    "duration" : "120"
}
```

---

##### `updateData` : 更新澆水及植物燈開關時間
| 發送  | **內容** |
| :--: | :--: |
| id | **設備id** |
| command | **指令** |
| wateringTime | **澆水時間** |
| lightStart | **植物燈開啟時間** |
| duration | **植物燈開燈持續時間(分鐘)** |

| 回傳  | **內容** |
| :--: | :--: |
| id | **設備id** |
| status | **狀態** |
| error | **錯誤訊息** |

發送範例:
```json
{
    "id" : "test-0001",
    "command" : "updateData",
    "wateringTime" : "12:00",
    "lightStart" : "20:00",
    "duration" : "120"
}
```

回傳範例:
```json
{
    "id" : "test-0001",
    "status" : "success",
    "error" : ""
}
```

---
##### `realTimeIrrigation` : 發送及時澆水訊息
| 回傳  | **內容** |
| :--: | :--: |
| id | **設備id** |
| command | **指令** |
| status | **狀態** |
| error | **錯誤訊息** |

回傳範例:
```json
{
    "id" : "test-0001",
    "status" : "error",
    "error" : "目前正在澆水中"
}
```


---
## 完成進度

- [x] GUI介面
- [x] MQTT傳輸
- [x] Esp32控制
- [x] 硬體設備
