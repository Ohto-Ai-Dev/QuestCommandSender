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

	void sendCommand(QString) const;
	void sendSetCommand(QString name, QString attribute, QString value, bool isInstance = false)const;
	QString sendInquireCommand(QString name, QString attribute, bool isInstance = false)const;
	void sendSetUserAttributeCommand(QString name, QString attribute, QString value, bool isInstance = false)const;
	QString sendInquireUserAttributeCommand(QString name, QString attribute, bool isInstance = false)const;
	double sendInquireUserNumericAttributeCommand(QString name, QString attribute, bool isInstance = false)const;

	void restoreNormalScene();

	void sendDefaultAgvSpeed() const;
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

	QTimer debugCheckTimer{ this };

	QString currentReceivedMessage{};
	Ui::QuestClientClass ui;
	using Socket = int;
	DenebTcpSocket questSocket;
};
