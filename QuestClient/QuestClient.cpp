#pragma execution_character_set("utf-8")
#include "QuestClient.h"

QuestClient::QuestClient(QWidget* parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);


	connect(ui.chooseQuestPath, &QPushButton::clicked, [=]
		{
			auto filePath = QFileDialog::getOpenFileName(this, "Choose Quest", ui.questPath->text(), "Quest.bat(Quest.bat)");
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
		});


	connect(&questSocket, &DenebTcpSocket::connectFinished, [=](int code)
		{
			ui.connectCheck->setChecked(code == 0);
			if (code != 0)
				ui.chatBrowser->append(QString{ "System:Error Occurred.(%1)" }.arg(code));
			else
				ui.chatBrowser->append("System:Connected.");
		});

	connect(&questSocket, &DenebTcpSocket::disconnected, [=]
		{
			ui.chatBrowser->append("System:Disconnected.");
		});
	
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

	connect(ui.sendAGVSpeed, &QPushButton::clicked, [=]
		{
			sendCommand(QString("SET 'gd_Crane_AGV1' SPEED TO %1").arg(ui.agvSpeed1->text()));
			sendCommand(QString("SET 'gd_Crane_AGV2' SPEED TO %1").arg(ui.agvSpeed2->text()));
			sendCommand(QString("SET 'gd_Crane_AGV3' SPEED TO %1").arg(ui.agvSpeed3->text()));
			sendCommand(QString("SET 'gd_Crane_AGV4' SPEED TO %1").arg(ui.agvSpeed4->text()));
			sendCommand(QString("SET 'gd_Crane_AGV1' LOADED SPEED TO %1").arg(ui.agvSpeed1->text()));
			sendCommand(QString("SET 'gd_Crane_AGV2' LOADED SPEED TO %1").arg(ui.agvSpeed2->text()));
			sendCommand(QString("SET 'gd_Crane_AGV3' LOADED SPEED TO %1").arg(ui.agvSpeed3->text()));
			sendCommand(QString("SET 'gd_Crane_AGV4' LOADED SPEED TO %1").arg(ui.agvSpeed4->text()));

			sendCommand(QString("SET 'gd_Hoist1' SPEED TO %1").arg(ui.hoistSpeed1->text()));
			sendCommand(QString("SET 'gd_Hoist2' SPEED TO %1").arg(ui.hoistSpeed2->text()));
			sendCommand(QString("SET 'gd_Hoist3' SPEED TO %1").arg(ui.hoistSpeed3->text()));
			sendCommand(QString("SET 'gd_Hoist4' SPEED TO %1").arg(ui.hoistSpeed4->text()));
		});

	connect(ui.chooseModel, &QPushButton::clicked, [=]
		{
			auto filePath = QFileDialog::getOpenFileName(this, "Choose Model", ui.modelPath->text(), "Model Files(*.mdl);;All Files(*.*)");
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
			if (QMessageBox::question(this, "确认放弃控制权", "放弃控制权以便直接使用QUEST软件，但无法继续使用本软件发送指令！") == QMessageBox::Yes)
				sendCommand("TRANSFER TO MENU");
		});

	connect(&questSocket, &DenebTcpSocket::received, [=]
		{
			ui.chatBrowser->append(QString{ "Quest: %1" }.arg(QString::fromLatin1(questSocket.recv())));
		});

	
}

void QuestClient::sendCommand(QString command) const
{
	ui.chatBrowser->append(QString{ "Client: %1 [%2]" }.arg(command, questSocket.send(command.toLatin1()) ? "SUCCESS" : "FAILED"));
}