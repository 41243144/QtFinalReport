﻿#include "widget.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QVBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QPlainTextEdit>

Widget::Widget(QWidget *parent)
    : QWidget(parent), mqttClient(new QMqttClient(this))
{
    auto *layout = new QVBoxLayout(this);

    auto *statusLabel = new QLabel("Disconnected", this);
    layout->addWidget(statusLabel);

    auto *logEdit = new QPlainTextEdit(this);
    logEdit->setReadOnly(true);
    logEdit->setObjectName("logEdit");
    layout->addWidget(logEdit);

    // MQTT Client 設定
    mqttClient->setHostname("broker.hivemq.com");  // 固定使用的 Broker
    mqttClient->setPort(1883);                     // 固定使用端口

    // 訊號槽連接
    connect(mqttClient, &QMqttClient::connected, this, [=]() {
        statusLabel->setText("Connected to Broker");

        // 訂閱固定主題
        subscribeToTopic();

        // 發送固定訊息
        const QString fixedTopic = "wenwen/test";
        mqttClient->publish(fixedTopic, "getCurrentData");
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

            // 取得各個項目的值
            QString id = jsonObj["id"].toString();
            QString command = jsonObj["command"].toString();
            if(command == "getCurrentData"){
            }
            QString wateringTime = jsonObj["wateringTime"].toString();
            QString lightStart1 = jsonObj["lightStart1"].toString();
            QString lightStart2 = jsonObj["lightStart2"].toString();

            // 顯示解析結果
            logEdit->appendPlainText("Parsed JSON Data:");
            logEdit->appendPlainText("ID: " + id);
            logEdit->appendPlainText("Command: " + command);
            logEdit->appendPlainText("Watering Time: " + wateringTime);
            logEdit->appendPlainText("Light Start 1: " + lightStart1);
            logEdit->appendPlainText("Light Start 2: " + lightStart2);
        } else {
            logEdit->appendPlainText("Invalid JSON received.");
        }
    }
}
