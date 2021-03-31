#pragma once

#include <QtWidgets/QMainWindow>
#include <thread>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QProcess>
#include "DenebTcpSocket.h"
#include "ui_QuestClient.h"

class QuestClient : public QMainWindow
{
	Q_OBJECT

public:
	QuestClient(QWidget* parent = Q_NULLPTR);

	void sendCommand(QString) const;
private:
	Ui::QuestClientClass ui;
	using Socket = int;
	DenebTcpSocket questSocket;
};
