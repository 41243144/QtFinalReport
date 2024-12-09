#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QMqttClient>

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
private slots:
    void onMessageReceived(const QByteArray &message, const QMqttTopicName &topic); // 處理接收到的訊息
    void subscribeToTopic();

private:
    QMqttClient *mqttClient;         // MQTT 客戶端

};
#endif // WIDGET_H
