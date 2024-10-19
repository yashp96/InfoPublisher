#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QTextStream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    SetupMqttClient();
    ReadImageBinary();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::ReadImageBinary()
{
    QFile img("img.bin");

    if(!img.open(QIODevice::ReadOnly))
    {
        return;
    }

    DebugMsg(QString().asprintf("Image size in bytes %lld",img.size()));

    ImageBlob = img.readAll();

    GeneratePixels(ImageBlob, &ImagePixelData);

    img.close();

    /////////////////////////////////// delete below code later

    // img.setFileName("array.txt");
    // if(!img.open(QIODevice::ReadOnly | QIODevice::Text))
    // {
    //     return;
    // }

    // QString arraytext;

    // QTextStream in(&img);
    // while (!in.atEnd()) {
    //     arraytext += in.readLine();
    //     // qDebug() << line;  // Process the line (print in this case)
    // }
    // QStringList arr = arraytext.split(",");

    // DebugMsg(QString().asprintf("arr size %lld", arr.size()));

    // QByteArray samp;

    // for(quint64 i = 0; i < arr.size(); i++)
    // {
    //     uint8_t n = arr[i].toUInt();
    //     samp.append(n);
    // }

    // QFile file("samp.bin");
    // if (!file.open(QIODevice::WriteOnly)) {
    //     DebugMsg("Cannot open file for writing");
    //     return;
    // }

    // // Write the QByteArray to the file
    // file.write(samp);

    // // Close the file
    // file.close();
}

void MainWindow::SetupMqttClient()
{
    ePaperClient = new QMqttClient(this);

    ePaperClient->setHostname("test.mosquitto.org");
    ePaperClient->setPort(1883);

    QObject::connect(ePaperClient, &QMqttClient::connected, this, &MainWindow::CbOnBrokerConnected);
    QObject::connect(ePaperClient, &QMqttClient::disconnected, this, &MainWindow::CbOnBrokerDisconnected);
    QObject::connect(ePaperClient, &QMqttClient::messageSent, this, &MainWindow::CbOnMsgPublished);
    QObject::connect(ePaperClient, &QMqttClient::messageReceived, this, &MainWindow::CbOnMsgRxd);

    ePaperClient->connectToHost();
    EpaperTopic.setName("esp32EpaperDiscount/test");

    TransmitTimer = new QTimer();
    QObject::connect(TransmitTimer, &QTimer::timeout, this, &MainWindow::CbTransmitTimer);

    UpdateImgTimer = new QTimer();
    QObject::connect(UpdateImgTimer, &QTimer::timeout, this, &MainWindow::CbUpdateImageTimer);
}

void MainWindow::GeneratePixels(QByteArray img, QByteArray *pixel_set)
{
    uint16_t row = 0, col = 0, plane = 0;
    uint8_t temp_c1 = 0, temp_c2 = 0;
    uint8_t data_H = 0, data_L = 0, data = 0;

    pixel_set->clear();

    for (row = 0; row < 448;row++)
    {
        plane = 0;
        for (col = 0; col < 300; col++)
        {
            temp_c1 = img.at(row*600+plane++);
            temp_c2 = img.at(row*600+plane++);
            data_H = ePaperGetImgColor(temp_c1)<<4;
            data_L = ePaperGetImgColor(temp_c2);
            data = data_H|data_L;
            pixel_set->append(data);
        }
    }

    DebugMsg(QString().asprintf("Generated Pixel Set Size: %lld",pixel_set->size()));
}

void MainWindow::DebugMsg(QString str)
{
    QString local;
    local = ui->debugbox->toPlainText();
    local += str + "\n";
    ui->debugbox->setPlainText(local);
    ui->debugbox->moveCursor(QTextCursor::End);  // Move cursor to the end
    ui->debugbox->ensureCursorVisible();
}

void MainWindow::SetButtonEnable(bool flag)
{
    ui->pbTestMqtt->setEnabled(flag);
    ui->pbEpaperSleep->setEnabled(flag);
    ui->pbClearEpaper->setEnabled(flag);
    ui->pbRefreshEpaper->setEnabled(flag);
    ui->pbDisplayImage->setEnabled(flag);
    ui->pbInitEpaper->setEnabled(flag);
    ui->pbInitImageMode->setEnabled(flag);
    ui->pbUploadImage->setEnabled(flag);
    ui->pbLoadFile->setEnabled(flag);
}

uint8_t MainWindow::ePaperGetImgColor(int color)
{
    uint8_t ecolor = 0;

    if(color == 0xFF)
    {
        ecolor = WHITE_4BIT;
    }
    else if(color >= 0xF8 && color <= 0xFE)
    {
        ecolor = YELLOW_4BIT;
    }
    else if(color >= 0xEC && color <= 0xF5)
    {
        ecolor = ORANGE_4BIT;
    }
    else if(color >= 0x80 && color <= 0xEB)
    {
        ecolor = RED_4BIT;
    }
    else if(color >= 0x19 && color <= 0x5F) // 0x19
    {
        ecolor = GREEN_4BIT;
    }
    else if(color >= 0x03 && color <= 0x2B)
    {
        ecolor = BLUE_4BIT;
    }
    else
    {
        ecolor = BLACK_4BIT;
    }

    return ecolor;
}

/*uint8_t MainWindow::ePaperGetImgColor(int color)
{
    uint8_t ecolor = 0;
    switch(color)
    {
    case 0xFF:
        ecolor = WHITE_4BIT;
        break;
    case 0xFC:
        ecolor = YELLOW_4BIT;
        break;
    case 0xEC:
        ecolor = ORANGE_4BIT;
        break;
    case 0xE0:
        ecolor = RED_4BIT;
        break;
    case 0x35:
        ecolor = GREEN_4BIT;
        break;
    case 0x2B:
        ecolor = BLUE_4BIT;
        break;
    case 0x00:
        ecolor = BLACK_4BIT;
        break;
    default:
        // qDebug() << "default color";
        break;
    }
    return ecolor;
}*/

void MainWindow::CbOnBrokerConnected()
{
    QMqttTopicFilter myfiler;
    myfiler.setFilter("esp32EpaperDiscount/testAck");

    DebugMsg("Connected to broker!");
    ui->lblMsg->setText("Ready to Update Epaper");
    // ePaperClient->disconnectFromHost();
    ePaperClient->subscribe(myfiler);
}

void MainWindow::CbOnBrokerDisconnected()
{
    DebugMsg("Disconnected from broker!");
}

void MainWindow::CbOnMsgPublished(qint32 id)
{
    DebugMsg(QString().asprintf("Mesg. successfuly published. Ack Id: %d",id));
}

void MainWindow::CbOnMsgRxd(const QByteArray &message, const QMqttTopicName topic)
{
    double completion_val = 0.0;

    QString str = QString().asprintf("Rx'd command %X %X %X %X",
    (uint8_t)message[EPAPER_SOF_INDX], (uint8_t)message[EPAPER_CMD_INDX],
    (uint8_t)message[EPAPER_DATA_SEQ_H_INDX], (uint8_t)message[EPAPER_DATA_SEQ_L_INDX]);

    DebugMsg(QString().asprintf("rx'd cmd length %lld",message.length()));
    DebugMsg(str);

    epaper_indices_t rcd = (epaper_indices_t)message[EPAPER_CMD_INDX];

    switch(rcd)
    {
        case EPAPER_CLEAR:
            if(UpdateEpaper)
            {
                UpdateInProgress = false;
                LastStage = EPAPER_CLEAR;
                NextStage = EPAPER_INIT_IMG;
                DebugMsg("Rx'd EPAPER_CLEAR");
            }
        break;
        case EPAPER_INIT:
            if(UpdateEpaper)
            {
                UpdateInProgress = false;
                NextStage = EPAPER_CLEAR;
            }
        break;

        case EPAPER_REFRESH:
            if(LastStage == EPAPER_TX_IMG && UpdateEpaper)
            {
                UpdateInProgress = false;
                NextStage = EPAPER_SLEEP;
            }
        break;

        case EPAPER_SLEEP:
            if(UpdateEpaper)
            {
                UpdateImgTimer->stop();
                UpdateInProgress = false;
                NextStage = EPAPER_INIT;
                LastStage = EPAPER_SLEEP;
                UpdateEpaper = false;
                ui->lblMsg->setText("Epaper Successfully Updated");
                SetButtonEnable(true);
            }
        break;

        case EPAPER_INIT_IMG:
            if(UpdateEpaper)
            {
                UpdateInProgress = false;
                LastStage = EPAPER_INIT_IMG;
                NextStage = EPAPER_TX_IMG;
            }
        break;

        case EPAPER_TX_IMG:
            TransmitCount++;
            if(TransmitCount == IMG_TRANSFER_NUMBERS)
            {
                TransmitComplete = true;
                DebugMsg(QString().asprintf("Transmit Complete Txn Count %d",TransmitCount));
                if(UpdateEpaper)
                {
                    UpdateInProgress = false;
                    LastStage = EPAPER_TX_IMG;
                    NextStage = EPAPER_REFRESH;
                }
            }

            if(TransmitCount < IMG_TRANSFER_NUMBERS)
            {
                TransmitNext = true;
                DebugMsg(QString().asprintf("TransmitCount %d | ImgByteIndx %d",TransmitCount,ImgByteIndx));
            }
            completion_val = (double(TransmitCount * 1.0) / (double)IMG_TRANSFER_NUMBERS) * 100;
            DebugMsg(QString().asprintf("%d%%", (int)completion_val));
            ui->progbarImgTx->setValue((int)completion_val);
        break;

        case EPAPER_TEST:
            DebugMsg("Rx'd EPAPER_TEST");
        break;

        default:
            DebugMsg("Reached Default");
        break;
    }

}

void MainWindow::CbTransmitTimer()
{
    QByteArray cmd;

    if(TransmitNext)
    {
        SequenceNumber++;
        cmd.append((uint8_t)EPAPER_SOF);
        cmd.append((uint8_t)EPAPER_TX_IMG);
        cmd.append((uint8_t)SequenceNumber >> 8);
        cmd.append((uint8_t)SequenceNumber);
        cmd.append(ImagePixelData.mid(ImgByteIndx, IMG_BIN_TX_BYTES));
        ImgByteIndx += IMG_BIN_TX_BYTES;
        ePaperClient->publish(EpaperTopic, cmd);
        TransmitNext = false;
    }

    if(TransmitComplete && UpdateEpaper == false)
    {
        SetButtonEnable(true);
        TransmitComplete = false;
        TransmitTimer->stop();
    }
}

void MainWindow::CbUpdateImageTimer()
{
    if(UpdateEpaper && !UpdateInProgress)
    {
        UpdateInProgress = true;
        switch(NextStage)
        {
        case EPAPER_INIT:
            DebugMsg("EPAPER_INIT");
            // ui->pbInitEpaper->click();
            on_pbInitEpaper_clicked();
            break;
        case EPAPER_REFRESH:
            DebugMsg("EPAPER_REFRESH");
            // ui->pbRefreshEpaper->click();
            on_pbRefreshEpaper_clicked();
            break;
        case EPAPER_SLEEP:
            DebugMsg("EPAPER_SLEEP");
            // ui->pbEpaperSleep->click();
            on_pbEpaperSleep_clicked();
            break;
        case EPAPER_INIT_IMG:
            DebugMsg("EPAPER_INIT_IMG");
            // ui->pbInitImageMode->click();
            on_pbInitImageMode_clicked();
            break;
        case EPAPER_TX_IMG:
            DebugMsg("EPAPER_TX_IMG");
            // ui->pbDisplayImage->click();
            on_pbDisplayImage_clicked();
            break;
        case EPAPER_CLEAR:
            DebugMsg("EPAPER_CLEAR");
            // ui->pbClearEpaper->click();
            on_pbClearEpaper_clicked();
            break;
        default:
            break;
        }
    }
}

void MainWindow::on_pbTestMqtt_clicked()
{
    QByteArray cmd;
    uint16_t num = 1234;
    cmd.append((uint8_t)EPAPER_SOF);
    cmd.append((uint8_t)EPAPER_TEST);
    cmd.append((uint8_t)(num >> 8));
    cmd.append((uint8_t)num);
    ePaperClient->publish(EpaperTopic, cmd);
}

void MainWindow::on_pbClearEpaper_clicked()
{
    QByteArray cmd;
    cmd.append((uint8_t)EPAPER_SOF);
    cmd.append((uint8_t)EPAPER_CLEAR);
    cmd.append((uint8_t)0);
    ePaperClient->publish(EpaperTopic, cmd);
    ui->lblMsg->setText("Clearing Epaper...");
}

void MainWindow::on_pbRefreshEpaper_clicked()
{
    QByteArray cmd;
    cmd.append((uint8_t)EPAPER_SOF);
    cmd.append((uint8_t)EPAPER_REFRESH);
    cmd.append((uint8_t)0);
    ePaperClient->publish(EpaperTopic, cmd);
    ui->lblMsg->setText("Refreshing Epaper...");
}

void MainWindow::on_pbEpaperSleep_clicked()
{
    QByteArray cmd;
    cmd.append((uint8_t)EPAPER_SOF);
    cmd.append((uint8_t)EPAPER_SLEEP);
    cmd.append((uint8_t)0);
    ePaperClient->publish(EpaperTopic, cmd);
    ui->lblMsg->setText("Setting low power");
}

void MainWindow::on_pbDisplayImage_clicked()
{
    SetButtonEnable(false);

    TransmitComplete = false;
    TransmitCount = 0;
    ImgByteIndx = 0;
    SequenceNumber = 0;
    TransmitNext = true;
    TransmitTimer->start(100);
    ui->lblMsg->setText("Transfering Image Data...");
}

void MainWindow::on_pbInitEpaper_clicked()
{
    QByteArray cmd;
    cmd.append((uint8_t)EPAPER_SOF);
    cmd.append((uint8_t)EPAPER_INIT);
    cmd.append((uint8_t)0);
    ePaperClient->publish(EpaperTopic, cmd);
    ui->lblMsg->setText("Initializing Epaper...");
}

void MainWindow::on_pbInitImageMode_clicked()
{
    QByteArray cmd;
    cmd.append((uint8_t)EPAPER_SOF);
    cmd.append((uint8_t)EPAPER_INIT_IMG);
    cmd.append((uint8_t)0);
    ePaperClient->publish(EpaperTopic, cmd);
}

void MainWindow::on_pbUploadImage_clicked()
{
    NextStage = EPAPER_INIT;
    UpdateEpaper = true;
    UpdateInProgress = false;
    UpdateImgTimer->start(200);
    SetButtonEnable(false);
}


void MainWindow::on_pbLoadFile_clicked()
{
   QString fp =  ui->lePath->text();

    if(fp == "")
   {
       ui->lblMsg->setText("path can't be blank");
       return;
   }

   if(QFile(fp).exists())
   {
       QFile img(fp);
       if(!img.open(QIODevice::ReadOnly))
       {
           ui->lblMsg->setText("Unable to Locate File");
           return;
       }
       DebugMsg(QString().asprintf("Image size in bytes %lld",img.size()));
       ImageBlob = img.readAll();
       GeneratePixels(ImageBlob, &ImagePixelData);
       ui->lblMsg->setText("File Loaded");
   }
   else
   {
       ui->lblMsg->setText("Unable to Locate File");
   }
}

