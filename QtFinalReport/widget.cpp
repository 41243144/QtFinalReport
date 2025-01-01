#include "widget.h"


/*目前已知
 * UI光照持續時間無法更改
 * UI時間器無法設為回傳的時間
 */


Widget::Widget(QWidget *parent)
    : QWidget(parent), mqttClient(new QMqttClient(this))
{

    // 初始化按鈕
    wateringBtn = new QPushButton(QStringLiteral("即時澆水"), this); // 澆水
    wtgSetTimeBtn = new QPushButton(QStringLiteral("時間設定"), this); // 時間設定

    // 初始化標籤
    wtgingBtnLbl = new QLabel(QStringLiteral("澆水："), this);

    // 初始化自動模式選擇框
    autoModeCheckBox1 = new QCheckBox(QStringLiteral("自動模式"), this);
    wtgSetTimeBtn->setEnabled(false);

    // 初始化佈局
    controlGp = new QGroupBox(QStringLiteral("控制區"), this);
    ctrlGpLayout = new QVBoxLayout(controlGp);
    vBtnAndLbl1 = new QHBoxLayout(); // 澆水區域

    // 配置佈局
    vBtnAndLbl1->addWidget(wtgingBtnLbl);
    vBtnAndLbl1->addWidget(autoModeCheckBox1);
    vBtnAndLbl1->addWidget(wateringBtn);
    vBtnAndLbl1->addWidget(wtgSetTimeBtn);

    // 添加到控制區域佈局
    ctrlGpLayout->addLayout(vBtnAndLbl1);

    // 添加到主佈局
    mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(controlGp);
    mainLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    // 設置信號與槽
    connect(autoModeCheckBox1, &QCheckBox::toggled, this, &Widget::CheckMode);
    connect(wtgSetTimeBtn, &QPushButton::clicked, this, &Widget::openTimeDialog);
    connect(wateringBtn, &QPushButton::clicked, this, &Widget::realTimeIrrigation);

    // 主佈局
    setLayout(mainLayout);

    //-------------------------------------------

    auto *layout = new QVBoxLayout(this);

    auto *statusLabel = new QLabel("Disconnected", this);
    layout->addWidget(statusLabel);

    auto *logEdit = new QPlainTextEdit(this);
    logEdit->setReadOnly(true);
    logEdit->setObjectName("logEdit");
    layout->addWidget(logEdit);

    mainLayout->addLayout(layout);

    // MQTT Client 設定
    mqttClient->setHostname("broker.hivemq.com");  // 固定使用的 Broker
    mqttClient->setPort(1883);                     // 固定使用端口

    mqttClient->setUsername("wenwen");
    mqttClient->setPassword("test");

    // 訊號槽連接
    connect(mqttClient, &QMqttClient::connected, this, [=]() {
        statusLabel->setText("Connected to Broker");

        // 訂閱固定主題
        subscribeToTopic();

        // 發送固定訊息
        const QString fixedTopic = "wenwen/test";
        QJsonObject jsonObject;
        jsonObject["id"] = "test-0001";
        jsonObject["command"] = "getCurrentData";

        QJsonDocument jsonDoc(jsonObject);
        QString jsonString = jsonDoc.toJson(QJsonDocument::Compact);
        mqttClient->publish(fixedTopic, jsonString.toUtf8());
    });

    connect(mqttClient, &QMqttClient::disconnected, this, [=]() {
        statusLabel->setText("Disconnected");
    });

    connect(mqttClient, &QMqttClient::messageReceived, this, &Widget::onMessageReceived);

    // 啟動時自動連線
    mqttClient->connectToHost();
}

Widget::~Widget()
{
    delete mqttClient;
}

void Widget::subscribeToTopic()
{
    // 固定訂閱主題
    const QString fixedTopic = "wenwen/test";
    mqttClient->subscribe(fixedTopic);
}

// 接收回傳資料(GUI串接)
void Widget::onMessageReceived(const QByteArray &message, const QMqttTopicName &topic)
{
    auto *logEdit = findChild<QPlainTextEdit *>("logEdit");

    if (logEdit) {
        // 顯示收到的原始訊息
        logEdit->appendPlainText(QString("Received [%1]: %2")
                                     .arg(topic.name(), QString::fromUtf8(message)));

        // 嘗試解析 JSON 資料
        QJsonDocument doc = QJsonDocument::fromJson(message);
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject jsonObj = doc.object();

            // 代表當前傳回的是目前esp32端的資料
            if(jsonObj.contains("wateringTime")){

            }

            // 取得各個項目的值
            _id = jsonObj["id"].toString();
            _wtgDateTime = jsonObj["wateringTime"].toString("yyyy-MM-dd HH:mm:ss");
            _illStartTime = jsonObj["lightStart"].toString("yyyy-MM-dd HH:mm:ss");
            _illContTime = jsonObj["duration"].toString();

            // 顯示解析結果
            logEdit->appendPlainText("Parsed JSON Data:");
            logEdit->appendPlainText("ID: " + _id);
            logEdit->appendPlainText("Watering Time: " + _wtgDateTime);
            logEdit->appendPlainText("Light Start 1: " + _illStartTime);
            logEdit->appendPlainText("duration: " + _illContTime);
        } else {
            logEdit->appendPlainText("Invalid JSON received.");
        }
    }
}

// 模式檢測
void Widget::CheckMode() {
    if (autoModeCheckBox1->isChecked()) {
        wateringBtn->setEnabled(false);
        wtgSetTimeBtn->setEnabled(true);
    } else {
        wateringBtn->setEnabled(true);
        wtgSetTimeBtn->setEnabled(false);
    }
}

//即時澆水
void Widget::realTimeIrrigation() {
    // 獲取當前時間
    QDateTime currentDateTime = QDateTime::currentDateTime();

    // 發送訊息
    const QString fixedTopic = "wenwen/test";
    QJsonObject jsonObject;
    jsonObject["id"] = "test-0001";
    jsonObject["command"] = "realTimeIrrigation";

    QJsonDocument jsonDoc(jsonObject);
    QString jsonString = jsonDoc.toJson(QJsonDocument::Compact);
    mqttClient->publish(fixedTopic, jsonString.toUtf8());
}

// 選擇時間
void Widget::openTimeDialog() {
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("選擇時間"));

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // 澆水欄 -----------------------------------
    QHBoxLayout *dwtgLayout = new QHBoxLayout();
    QLabel *dwtgLabel = new QLabel(QStringLiteral("澆水："));

    // 澆水時間選擇器
    QTimeEdit *dwtgTimeEdit = new QTimeEdit(QTime::fromString(_wtgDateTime, "HH:mm"));

    dwtgTimeEdit->setDisplayFormat("HH:mm");

    // 布局樣式
    dwtgLabel->setAlignment(Qt::AlignLeft); // 文字居中
    dwtgLabel->setStyleSheet(
        "QLabel {"
        "   letter-spacing: 8px;" // 文字間隔
        "}"
        );
    dwtgLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    dwtgTimeEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    dwtgLayout->addWidget(dwtgLabel);
    dwtgLayout->addWidget(dwtgTimeEdit);


    //光照欄 -----------------------------------
    QHBoxLayout *dillLayout = new QHBoxLayout();
    QLabel *dillLabel = new QLabel(QStringLiteral("光照(開始:持續)："));
    QLineEdit *dillContLE = new QLineEdit();
    QTimeEdit *dillTimeEdit = new QTimeEdit(QTime::fromString(_illStartTime, "HH:mm"));// 光照時間選擇器(起始時間)

    dillContLE->setPlaceholderText(tr("光照持續時間"));
    dillTimeEdit->setDisplayFormat("HH:mm");

    // 布局樣式
    dillLabel->setAlignment(Qt::AlignLeft); // 文字居中
    dillLabel->setStyleSheet(
        "QLabel {"
        "   letter-spacing: 5px;" // 文字間隔
        "}"
        );
    dillLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    dillTimeEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    dillContLE->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    dillLayout->addWidget(dillLabel);
    dillLayout->addWidget(dillTimeEdit);
    dillLayout->addWidget(dillContLE);


    //狀態欄(光照持續時間) -----------------------------------
    QHBoxLayout *dStateLayout = new QHBoxLayout();
    QLabel *dStateLabel = new QLabel(QStringLiteral("光照持續時間："));
    QLabel *dillState = new QLabel(QStringLiteral("光照持續時間"));
    dillState->setText(_illContTime);

    // 布局樣式
    dStateLabel->setAlignment(Qt::AlignLeft); // 文字居中
    dStateLabel->setStyleSheet(
        "QLabel {"
        "   letter-spacing: 5px;" // 文字間隔
        "}"
        );
    dStateLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    dillState->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);


    dStateLayout->addWidget(dStateLabel);
    dStateLayout->addWidget(dillState);

    // "送出"按鈕 -----------------------------------
    QHBoxLayout *dBtnLayout = new QHBoxLayout();
    QPushButton *pushData = new QPushButton(QStringLiteral("送出"), &dialog);

    dBtnLayout->addWidget(pushData);

    layout->addLayout(dwtgLayout);
    layout->addLayout(dillLayout);
    layout->addLayout(dStateLayout);
    layout->addLayout(dBtnLayout);

    // 信號
    connect(pushData, &QPushButton::clicked, this, [=, &dialog]() {
        if(!dillContLE->text().isEmpty() && dillContLE->text().toInt() <= 720 && dillContLE->text().toInt() >= 0){
            const QString fixedTopic = "wenwen/test";
            QJsonObject jsonObject;
            QTime wtgTime = dwtgTimeEdit->time();
            QTime illTime = dillTimeEdit->time();
            _illContTime = dillContLE->text();

            // 取得當前日期
            QDate currentDate = QDate::currentDate();

            // 日期時間合併
            _wtgDateTime = QDateTime(currentDate, wtgTime).toString("HH:mm");
            _illStartTime = QDateTime(currentDate, illTime).toString("HH:mm");

            // 插入到 JSON
            jsonObject["id"] = "test-0001";
            jsonObject["command"] = "updateData";
            jsonObject["wateringTime"] = _wtgDateTime;
            jsonObject["lightStart"] = _illStartTime;
            jsonObject["duration"] = _illContTime;

            QJsonDocument jsonDoc(jsonObject);
            QString jsonString = jsonDoc.toJson(QJsonDocument::Compact);
            mqttClient->publish(fixedTopic, jsonString.toUtf8());

            // 關閉對話框
            dialog.accept();
        }
    });


    if (dialog.exec() == QDialog::Accepted) {
        QDateTime wtgDateTime = QDateTime::fromString(_wtgDateTime, "yyyy-MM-dd HH:mm:ss");

        if (wtgDateTime.isValid()) {
            QTime wateringTime = wtgDateTime.time();
            dwtgTimeEdit->setTime(wateringTime);

        } else {
            qDebug() << "Invalid _wtgDateTime format:" << _wtgDateTime;
        }
    }
}
