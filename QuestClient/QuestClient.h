#pragma once

#include <QtWidgets/QMainWindow>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QProcess>
#include <QMenu>
#include <QEventLoop>
#include <QDateTime>
#include <QWindow>
#include <QAxObject>
#include <json.hpp>
#include <QTcpSocket>
#include "DenebTcpSocket.h"
#include "ui_QuestClient.h"

class QuestClient : public QMainWindow
{
	Q_OBJECT

public:
	QuestClient(QWidget* parent = Q_NULLPTR);

	void sendCommand(QString, std::function<void(QByteArray)> onReceived = std::function<void(QByteArray)>()) const;
	void sendSetCommand(QString name, QString attribute, QString value, bool isInstance = false)const;
	void sendSetUserAttributeCommand(QString name, QString attribute, QString value, bool isInstance = false)const;
	void sendInquireUserNumericAttributeCommand(QString name, QString attribute, std::function<void(double)> onReceived)const;
	void sendInquireUserNumericAttributeCommand(QString name, QString attribute, double&val)const;
	void sendInquireUserNumericAttributeCommand(QString name, QString attribute, int&val)const;

	void restoreNormalScene();

	void sendDefaultAgvSpeed() const;

	void configInstall();
	void signalsInstall();

	void callUpdateReport();
	void doExportReport();
private:
	static inline const char* configPath{ "application.json" };
	nlohmann::json config;
	mutable QFile logFile{ "application.log", this };
	static inline QString questPath = R"(D:\deneb\quest\quest.bat)";
	static inline int questPort = 9988;
	QTcpSocket unityServer{ this };
	QAxObject excel{ this };

	int craneFailure{ 0 };
	int luziFailure{ 0 };
	int planSimTime = 86400;

	// report data
	int countGY_XL[4]{ 0,0,0,0 }, countSH_XL[4]{ 0,0,0,0 }, countDGY_XL[4]{ 0,0,0,0 }, counFeeding[4][3]{}, countHoistUse[4]{ 0,0,0,0 };
	double craneAgvUseRate[4]{}, craneAgvUseTime[4]{}, hoistUseTime[4]{}, elevatorUseTime[4]{};
	double weightDGY_XL[4]{ 0,0,0,0 }, weightGY_XL[4], weightSH_XL[4];
	double weightLiaoCang{};

	//QProcess extraPorcess{ this };
	QTimer debugCheckTimer{ this };
	QTimer recvDataTimer{ this };
	QWindow* extraWindow{ nullptr };
	QWidget* extraWindowContainer{ nullptr };

	Ui::QuestClientClass ui;
	using Socket = int;
	DenebTcpSocket questSocket;
};
