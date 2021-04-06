#pragma once

#include <QtWidgets/QMainWindow>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QProcess>
#include <QMenu>
#include <QDateTime>
#include <QAxObject>
#include <json.hpp>
#include "DenebTcpSocket.h"
#include "ui_QuestClient.h"

class QuestClient : public QMainWindow
{
	Q_OBJECT

public:
	QuestClient(QWidget* parent = Q_NULLPTR);

	void sendCommand(QString) const;
	void sendSetCommand(QString name, QString attribute, QString value, bool isInstance = false)const;
	void sendSetUserAttributeCommand(QString name, QString attribute, QString value, bool isInstance = false)const;
	QString sendInquireUserAttributeCommand(QString name, QString attribute, bool isInstance = false)const;
	double sendInquireUserNumericAttributeCommand(QString name, QString attribute, bool isInstance = false)const;

private:
	static inline const char* configPath{ "application.json" };
	nlohmann::json config;
	mutable QFile logFile{ "application.log", this };
	static inline QString questPath = R"(D:\deneb\quest\quest.bat)";
	static inline int questPort = 9988;

	int planSimTime = 86400;

	QString currentReceivedMessage{};
	Ui::QuestClientClass ui;
	using Socket = int;
	DenebTcpSocket questSocket;
};
