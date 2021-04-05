#pragma once

#include <QtWidgets/QMainWindow>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QProcess>
#include <QMenu>
#include <QDateTime>
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

protected:
	void appendClientMessage(QString) const;
	void appendSystemMessage(QString) const;
	void appendQuestMessage(QString) const;
private:
	QString currentReceivedMessage{};
	Ui::QuestClientClass ui;
	using Socket = int;
	DenebTcpSocket questSocket;
};
