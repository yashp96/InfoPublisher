#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QDebug>
#include <QtMqtt/QMqttClient>
#include <QtMqtt/QMqttSubscription>
#include <QString>
#include <QTimer>
#include <QMessageBox>
#include <QCloseEvent>

#define EPAPER_SOF  0xF1

#define IMG_BIN_SIZE_MAX        268800
#define IMG_BIN_TX_BYTES        900 //300
#define IMG_TRANSFER_NUMBERS    149 //448 /*(IMG_BIN_SIZE_MAX/IMG_BIN_TX_BYTES)*/

#define IMG_WIDTH_MAX   600
#define IMG_HEIGHT_MAX  448

// eInk Color Definitions
#define BLACK_8BIT      0x00
#define WHITE_8BIT      0x11
#define GREEN_8BIT      0x22
#define BLUE_8BIT       0x33
#define RED_8BIT        0x44
#define YELLOW_8BIT     0x55
#define ORANGE_8BIT     0x66
#define CLEAN_8BIT      0x77

#define BLACK_4BIT      0x00
#define WHITE_4BIT      0x01
#define GREEN_4BIT      0x02
#define BLUE_4BIT       0x03
#define RED_4BIT        0x04
#define YELLOW_4BIT     0x05
#define ORANGE_4BIT     0x06
#define CLEAN_4BIT      0x07

/*
 * FRAME FORMAT
 * | SOF (1) | COMMAND (1) | DATA SEQUENCE (2) | DATA (n Bytes <= 300) |
*/

typedef enum EPAPER_BUFFER_INDICES
{
    EPAPER_SOF_INDX = 0,
    EPAPER_CMD_INDX,
    EPAPER_DATA_SEQ_L_INDX,
    EPAPER_DATA_SEQ_H_INDX,
    EPAPER_DATA_INDX
}epaper_indices_t;

typedef enum EPAPER_COMMANDS
{
    EPAPER_INIT = 0x01,
    EPAPER_REFRESH = 0x02,
    EPAPER_SLEEP = 0x03,
    EPAPER_INIT_IMG = 0x04,
    EPAPER_TX_IMG = 0x05,
    EPAPER_TEST = 0x06,
    EPAPER_CLEAR = 0x07
}epaper_commands_t;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QMqttTopicName EpaperTopic;
    QMqttClient* ePaperClient;
    QByteArray ImageBlob;
    QTimer* TransmitTimer;
    QTimer* UpdateImgTimer;

    QByteArray ImagePixelData;

    epaper_commands_t NextStage = EPAPER_INIT;
    epaper_commands_t LastStage = EPAPER_INIT;
    bool UpdateEpaper = false;
    bool UpdateInProgress = false;

    bool TransmitComplete = false;
    bool TransmitNext = false;
    uint32_t TransmitCount = 0;
    uint32_t ImgByteIndx = 0;
    uint32_t SequenceNumber = 0;

    uint32_t ImgRowNumber = 0;
    uint32_t ImgColumNumber = 0;

    void ReadImageBinary();
    void SetupMqttClient();
    void GeneratePixels(QByteArray img, QByteArray* pixel_set);

    void DebugMsg(QString str);

    void SetButtonEnable(bool flag);

    uint8_t ePaperGetImgColor(int color);

protected:
    void closeEvent(QCloseEvent *event) override {
        QMessageBox::StandardButton resBtn = QMessageBox::question(this, "Close Confirmation",
                                                                   "Are you sure you want to close the window? Ongoing operations will be stopped.",
                                                                   QMessageBox::Yes | QMessageBox::No,
                                                                   QMessageBox::No);
        if (resBtn == QMessageBox::Yes) {
            event->accept();
        } else {
            event->ignore();
        }
    }

private slots:
    void CbOnBrokerConnected();
    void CbOnBrokerDisconnected();
    void CbOnMsgPublished(qint32 id);
    void CbOnMsgRxd(const QByteArray &message, const QMqttTopicName topic);

    void CbTransmitTimer();
    void CbUpdateImageTimer();

    void on_pbTestMqtt_clicked();
    void on_pbClearEpaper_clicked();
    void on_pbRefreshEpaper_clicked();
    void on_pbEpaperSleep_clicked();
    void on_pbDisplayImage_clicked();
    void on_pbInitEpaper_clicked();
    void on_pbInitImageMode_clicked();
    void on_pbUploadImage_clicked();
    void on_pbLoadFile_clicked();
};
#endif // MAINWINDOW_H
