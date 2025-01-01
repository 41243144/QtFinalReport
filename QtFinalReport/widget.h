#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QMqttClient>
#include <QApplication>

#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QCheckBox>
#include <QTimeEdit>
#include <QDialog>
#include <QComboBox>
#include <QJsonObject>
#include <QJsonDocument>
#include <QLineEdit>
#include <QMessageBox>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QPlainTextEdit>

class Widget : public QWidget
{
    Q_OBJECT

public:

    QLabel          *wtgingBtnLbl; // 澆水標籤
    QLabel          *illumBtnLbl; // 光照標籤

    QCheckBox       *autoModeCheckBox1; //澆水模式

    QPushButton     *wateringBtn; // 澆水
    QPushButton     *wtgSetTimeBtn; // 時間設定

    QHBoxLayout     *vBtnAndLbl1; // 澆水
    QVBoxLayout     *mainLayout; //

    QGroupBox       *controlGp; //
    QVBoxLayout     *ctrlGpLayout; //

    //全域變數 存json
    QString         _wtgDateTime; // 澆水時間
    QString         _illStartTime; // 光照起始時間
    QString         _illContTime; // 光照持續時間
    QString         _id;

    Widget(QWidget *parent = nullptr);
    ~Widget();
private slots:
    void onMessageReceived(const QByteArray &message, const QMqttTopicName &topic); // 處理接收到的訊息
    void subscribeToTopic();

    void CheckMode();
    void realTimeIrrigation(); // 即時澆水
    void openTimeDialog();
    //QJsonObject updateData(); // 更新

private:
    QMqttClient *mqttClient;         // MQTT 客戶端

};
#endif // WIDGET_H
