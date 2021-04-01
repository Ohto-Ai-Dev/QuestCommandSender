#pragma execution_character_set("utf-8")
#include "QuestClient.h"

QuestClient::QuestClient(QWidget* parent)
	: QMainWindow(parent)
	, questSocket(this)
{
	ui.setupUi(this);

	connect(ui.chooseQuestPath, &QPushButton::clicked, [=]
		{
			const auto filePath = QFileDialog::getOpenFileName(this, "Choose Quest", ui.questPath->text(), "Quest.bat(Quest.bat)");
			if (!filePath.isEmpty())
			{
				ui.questPath->setText(filePath);
			}
		});

	connect(ui.runQuest, &QPushButton::clicked, [=]
		{
			if (!QFile(ui.questPath->text()).exists())
			{
				QMessageBox::warning(this, "启动失败", "Quest路径无效");
			}
			QProcess::execute(ui.questPath->text(), { "-s", ui.port->text() });
			QTimer::singleShot(500, [=] {
				questSocket.connectToHost(ui.hostame->text(), ui.port->text().toInt());
				});
		});

	connect(ui.closeQuest, &QPushButton::clicked, [=]
		{
			if (questSocket.write("EXIT"))
			{
				questSocket.disconnectToHost();
			}		
		});
	
	connect(&questSocket, &DenebTcpSocket::connectFailed, this, [=](int code)
		{
			ui.connectCheck->setChecked(questSocket.isConnected());
			appendSystemMessage(QString{ "Error Occurred.(%1)" }.arg(code));
		}, Qt::QueuedConnection);

	connect(&questSocket, &DenebTcpSocket::connected, this, [=]
		{
			ui.connectCheck->setChecked(questSocket.isConnected());
			appendSystemMessage("Connected.");
		}, Qt::QueuedConnection);
	connect(&questSocket, &DenebTcpSocket::disconnected, this, [=]
		{
			ui.connectCheck->setChecked(questSocket.isConnected());
			appendSystemMessage("Disconnected.");
		}, Qt::QueuedConnection);

	connect(ui.connectCheck, &QCheckBox::clicked, [=](bool checked)
		{
			if (checked)
			{
				questSocket.connectToHost(ui.hostame->text(), ui.port->text().toInt());
			}
			else
			{
				questSocket.disconnectToHost();
			}
		});

	connect(ui.sendCommand, &QPushButton::clicked, [=]
		{
			sendCommand(ui.command->text());
			ui.command->clear();
		});

	connect(ui.startSim, &QPushButton::clicked, [=]
		{
			sendCommand(QString("SET SIMULATION TIME INTERVAL TO %1").arg(ui.simInterval->text()));
			sendCommand(QString("RUN %1").arg(ui.simtime->text()));
		});

	connect(ui.resetSim, &QPushButton::clicked, [=]
		{
			sendCommand("RESET RUN");
		});

	connect(ui.continueSim, &QPushButton::clicked, [=]
		{
			sendCommand(QString("SET SIMULATION TIME INTERVAL TO %1").arg(ui.simInterval->text()));
			sendCommand(QString("CONTINUE FOR %1").arg(ui.simtime->text()));
		});


	connect(ui.chooseModel, &QPushButton::clicked, [=]
		{
			const auto filePath = QFileDialog::getOpenFileName(this, "Choose Model", ui.modelPath->text(), "Model Files(*.mdl);;All Files(*.*)");
			if (!filePath.isEmpty())
			{
				ui.modelPath->setText(filePath);
			}
		});

	connect(ui.readModel, &QPushButton::clicked, [=]
		{
			sendCommand(QString("READ MODEL '%1'").arg(ui.modelPath->text()));
		});

	connect(ui.transferToMenu, &QPushButton::clicked, [=]
		{
			sendCommand("TRANSFER TO MENU");
		});

	connect(ui.updateReport, &QPushButton::clicked, [=]
		{
			auto route_flag = sendInquireUserAttributeCommand("Source_TL1", "route_flag", true);
			ui.reportBrowser->append(QString("Source_TL1_1.route_flag = %1").arg(route_flag));
		});

	connect(ui.quickCommand, &QPushButton::clicked, [=]
		{
			if (ui.quickCommandMode->currentText() == "设置")
				sendSetUserAttributeCommand(ui.element->text(), ui.attrib->text()
					, ui.value->text(), ui.elementMode->currentText() == "实例");
			else
				ui.value->setText(sendInquireUserAttributeCommand(ui.element->text(), ui.attrib->text()
					, ui.elementMode->currentText() == "实例"));
		});

	connect(ui.sendAGVSpeed, &QPushButton::clicked, [=]
		{
			sendSetCommand("gd_Crane_AGV1", "SPEED", ui.agvSpeed1->text());
			sendSetCommand("gd_Crane_AGV2", "SPEED", ui.agvSpeed2->text());
			sendSetCommand("gd_Crane_AGV3", "SPEED", ui.agvSpeed3->text());
			sendSetCommand("gd_Crane_AGV4", "SPEED", ui.agvSpeed4->text());
			sendSetCommand("gd_Crane_AGV1", "LOADED SPEED", ui.agvSpeed1->text());
			sendSetCommand("gd_Crane_AGV2", "LOADED SPEED", ui.agvSpeed2->text());
			sendSetCommand("gd_Crane_AGV3", "LOADED SPEED", ui.agvSpeed3->text());
			sendSetCommand("gd_Crane_AGV4", "LOADED SPEED", ui.agvSpeed4->text());

			sendSetCommand("gd_Hoist1", "SPEED", ui.hoistSpeed1->text());
			sendSetCommand("gd_Hoist2", "SPEED", ui.hoistSpeed2->text());
			sendSetCommand("gd_Hoist3", "SPEED", ui.hoistSpeed3->text());
			sendSetCommand("gd_Hoist4", "SPEED", ui.hoistSpeed4->text());
		});
	
	connect(ui.sendTime, &QPushButton::clicked, [=]
		{
			sendSetCommand("Buffer_LK3_2_upload", "LOAD TIME", ui.loadTime->text());
			sendSetCommand("Buffer_LK3_2_upload2", "LOAD TIME", ui.loadTime->text());
			sendSetCommand("Buffer_LK5_2_upload", "LOAD TIME", ui.loadTime->text());
			sendSetCommand("Dec_LK1_DL_upload", "LOAD TIME", ui.loadTime->text());
			sendSetCommand("Dec_LK2_DL_upload", "LOAD TIME", ui.loadTime->text());
			sendSetCommand("Dec_LK3_DL_upload", "LOAD TIME", ui.loadTime->text());
			sendSetCommand("Dec_LK4_DL_upload", "LOAD TIME", ui.loadTime->text());

			sendSetCommand("Dec_LK1_DL_download", "UNLOAD TIME", ui.unloadDLTime->text());
		
			sendSetCommand("Buffer_LK3_2_download", "UNLOAD TIME", ui.unloadTime->text());
			sendSetCommand("Buffer_LK3_2_download2", "UNLOAD TIME", ui.unloadTime->text());
			sendSetCommand("Buffer_LK5_2_download", "UNLOAD TIME", ui.unloadTime->text());
		});

	connect(ui.sendLevel, &QPushButton::clicked, [=]
		{
			auto toBlockCount = [](QString num) {return QString::number(ceil(num.toDouble() * 30 / 9 * 13)); };
			sendSetUserAttributeCommand("Buffer_TL_1", "hightest", toBlockCount(ui.levelH1->text()));
			sendSetUserAttributeCommand("Buffer_TL_2", "hightest", toBlockCount(ui.levelH2->text()));
			sendSetUserAttributeCommand("Buffer_TL_3", "hightest", toBlockCount(ui.levelH3->text()));
			sendSetUserAttributeCommand("Buffer_TL_1", "lowest", toBlockCount(ui.levelL1->text()));
			sendSetUserAttributeCommand("Buffer_TL_2", "lowest", toBlockCount(ui.levelL3->text()));
			sendSetUserAttributeCommand("Buffer_TL_3", "lowest", toBlockCount(ui.levelL3->text()));
		});

	connect(ui.sendError, &QPushButton::clicked, [=]
		{
			sendSetUserAttributeCommand("Source_TL1", "ljfailure", ui.ljdError->text());
			sendSetUserAttributeCommand("Source_TL1", "lufailure", ui.luError->text());
		});

	connect(&questSocket, &DenebTcpSocket::received, [=]
		{
			currentReceivedMessage = QString::fromLatin1(questSocket.read());
			appendQuestMessage(currentReceivedMessage);
		});
}

void QuestClient::sendCommand(QString command) const
{
	appendClientMessage(QString{ "%1 [%2]" }.arg(command, questSocket.write(command.toLatin1()) ? "SUCCESS" : "FAILED"));
}

void QuestClient::sendSetCommand(QString name, QString attribute, QString value, bool isInstance) const
{
	sendCommand(QString{ "SET %1'%2%3' %4 TO %5" }.arg(isInstance ? "ELEMENT " : "", name, isInstance ? "_1" : "", attribute, value));
}

void QuestClient::sendSetUserAttributeCommand(QString name, QString attribute, QString value, bool isInstance) const
{
	sendCommand(QString{ "SET %1'%2%3' ATTRIB '%4' TO %5" }.arg(isInstance ? "ELEMENT " : "", name, isInstance ? "_1" : "", attribute, value));
}

QString QuestClient::sendInquireUserAttributeCommand(QString name, QString attribute, bool isInstance) const
{
	sendCommand(QString{ "INQ %1'%2%3' ATTRIB '%4'" }.arg(isInstance ? "ELEMENT " : "", name, isInstance ? "_1" : "", attribute));
	questSocket.waitReceived();
	return currentReceivedMessage.section(QRegExp("[:;]"), 1, 1).trimmed();
}

void QuestClient::appendClientMessage(QString msg) const
{
	ui.chatBrowser->append(QString{ "[Client]: %1" }.arg(msg));
}

void QuestClient::appendSystemMessage(QString msg) const
{
	ui.chatBrowser->append(QString{ "[System]: %1" }.arg(msg));
}

void QuestClient::appendQuestMessage(QString msg) const
{
	ui.chatBrowser->append(QString{ "[Quest]: %1" }.arg(msg));
}