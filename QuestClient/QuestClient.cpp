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
			if (QMessageBox::warning(this, "确认", "确认关闭Quest？", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
			{
				if (questSocket.write("EXIT"))
				{
					questSocket.disconnectToHost();
				}
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
			sendCommand(QString("SET ANIMATION MODE %1").arg(ui.animationMode->isChecked() ? "ON" : "OFF"));
			sendCommand(QString("SET SIMULATION TIME INTERVAL TO %1").arg(ui.simInterval->text()));
			sendCommand(QString("RUN %1").arg(ui.simtime->text()));
		});
	
	connect(ui.resetSim, &QPushButton::clicked, [=]
		{
			sendCommand("RESET RUN");
		});

	connect(ui.continueSim, &QPushButton::clicked, [=]
		{
			sendCommand(QString("SET ANIMATION MODE %1").arg(ui.animationMode->isChecked() ? "ON" : "OFF"));
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
			sendCommand("CLEAR ALL");
			sendCommand(QString("READ MODEL '%1'").arg(ui.modelPath->text()));

			if (ui.bigChoice->isChecked()|| ui.bigChoiceDump->isChecked())
				sendSetUserAttributeCommand("Source_time", "modetag", QString::number(ui.bigChoiceDump->isChecked()));
		});

	connect(ui.transferToMenu, &QPushButton::clicked, [=]
		{
			sendCommand("TRANSFER TO MENU");
		});

	connect(ui.updateReport, &QPushButton::clicked, [=]
		{
			auto maxGyNum = sendInquireUserAttributeCommand("Source_time", "max_gy_num", true).toDouble();
			auto maxShNum = sendInquireUserAttributeCommand("Source_time", "max_sh_num", true).toDouble();
			auto liaoCangTotal = sendInquireUserAttributeCommand("Source_time", "liaocangtotal", true).toDouble();


			auto gy_xieliao2 = sendInquireUserAttributeCommand("gd_Crane_AGV2", "gy_xieliao", true).toDouble();
			auto sh_xieliao2 = sendInquireUserAttributeCommand("gd_Crane_AGV2", "sh_xieliao", true).toDouble();
			auto dgy_xieliao2 = sendInquireUserAttributeCommand("gd_Crane_AGV2", "dgy_xieliao", true).toDouble();
			auto touliao1_2 = sendInquireUserAttributeCommand("gd_Crane_AGV2", "touliao1", true).toDouble();
			auto touliao2_2 = sendInquireUserAttributeCommand("gd_Crane_AGV2", "touliao2", true).toDouble();
			auto touliao3_2 = sendInquireUserAttributeCommand("gd_Crane_AGV2", "touliao3", true).toDouble();

			auto gy_xieliao3 = sendInquireUserAttributeCommand("gd_Crane_AGV3", "gy_xieliao", true).toDouble();
			auto sh_xieliao3 = sendInquireUserAttributeCommand("gd_Crane_AGV3", "sh_xieliao", true).toDouble();
			auto dgy_xieliao3 = sendInquireUserAttributeCommand("gd_Crane_AGV3", "dgy_xieliao", true).toDouble();
			auto touliao1_3 = sendInquireUserAttributeCommand("gd_Crane_AGV3", "touliao1", true).toDouble();
			auto touliao2_3 = sendInquireUserAttributeCommand("gd_Crane_AGV3", "touliao2", true).toDouble();
			auto touliao3_3 = sendInquireUserAttributeCommand("gd_Crane_AGV3", "touliao3", true).toDouble();

			auto gy_xieliao4 = sendInquireUserAttributeCommand("gd_Crane_AGV4", "gy_xieliao", true).toDouble();
			auto sh_xieliao4 = sendInquireUserAttributeCommand("gd_Crane_AGV4", "sh_xieliao", true).toDouble();
			auto dgy_xieliao4 = sendInquireUserAttributeCommand("gd_Crane_AGV4", "dgy_xieliao", true).toDouble();
			auto touliao1_4 = sendInquireUserAttributeCommand("gd_Crane_AGV4", "touliao1", true).toDouble();
			auto touliao2_4 = sendInquireUserAttributeCommand("gd_Crane_AGV4", "touliao2", true).toDouble();
			auto touliao3_4 = sendInquireUserAttributeCommand("gd_Crane_AGV4", "touliao3", true).toDouble();

			auto simTime = sendInquireUserAttributeCommand("Source_time", "u_sim_time", true).toDouble();
			auto firstSimTime = sendInquireUserAttributeCommand("Source_time", "u_first_use_time", true).toDouble();
			if (simTime == 0.0)
				simTime = 0.000001;
			double agvBusyProc[4], agvBusyTime[4], agvBusyWaitTime[4], agvLoadTime[4], agvUnloadTime[4], agvEmptyTravelTime[4], agvFirstUseTime[4];
			double agvUsePercent[4], agvUserPercentFixed[4];

			for (int i = 0; i < 4; ++i)
			{
				agvBusyProc[i] = sendInquireUserAttributeCommand(QString("gd_Crane_AGV%1").arg(i + 1), "u_busy_proc_time", true).toDouble();
				agvBusyTime[i] = sendInquireUserAttributeCommand(QString("gd_Crane_AGV%1").arg(i + 1), "u_busy_time", true).toDouble();
				agvBusyWaitTime[i] = sendInquireUserAttributeCommand(QString("gd_Crane_AGV%1").arg(i + 1), "u_busy_wait_time", true).toDouble();
				agvLoadTime[i] = sendInquireUserAttributeCommand(QString("gd_Crane_AGV%1").arg(i + 1), "u_load_time", true).toDouble();
				agvUnloadTime[i] = sendInquireUserAttributeCommand(QString("gd_Crane_AGV%1").arg(i + 1), "u_unload_time", true).toDouble();
				agvEmptyTravelTime[i] = sendInquireUserAttributeCommand(QString("gd_Crane_AGV%1").arg(i + 1), "u_empty_travel_time", true).toDouble();
				agvFirstUseTime[i] = sendInquireUserAttributeCommand(QString("gd_Crane_AGV%1").arg(i + 1), "u_first_use_time", true).toDouble();
				agvUsePercent[i] = (agvLoadTime[i] + agvUnloadTime[i] + agvEmptyTravelTime[i]) * 3600 / simTime;

				if (simTime > 86400)
					agvUserPercentFixed[i] = (agvLoadTime[i] + agvUnloadTime[i] + agvEmptyTravelTime[i] - agvFirstUseTime[i]) * 3600 / (simTime - firstSimTime);
				else
					agvUserPercentFixed[i] = 0;
			}

			if (ui.bigChoice->isChecked() || ui.bigChoiceDump->isChecked())
			{
				maxGyNum *= 6;
				maxShNum *= 6;
			}
			else
			{
				maxGyNum *= 3.5;
				maxShNum *= 3.5;
			}

			ui.reportBrowser->append(QString(R"([%0]Report
料仓统计 %1t
送往左侧工业仓垃圾 %2t
送往左侧生活仓垃圾 %3t

行车	工业卸料	生活卸料 大工业仓卸料	投料1	投料2	投料3	利用率
AGV2	%4	%5	%6	%7	%8	%9	%10
AGV3	%11	%12	%13	%14	%15	%16	%17
AGV4	%18	%19	%20	%21	%22	%23	%24

投料口	数量
料口1	%25t
料口2	%26t
料口3	%27t

)")
.arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
.arg(liaoCangTotal).arg(maxGyNum).arg(maxShNum)
.arg(gy_xieliao2).arg(sh_xieliao2).arg(dgy_xieliao2).arg(touliao1_2).arg(touliao2_2).arg(touliao3_2).arg(agvUserPercentFixed[1])
.arg(gy_xieliao3).arg(sh_xieliao3).arg(dgy_xieliao3).arg(touliao1_3).arg(touliao2_3).arg(touliao3_3).arg(agvUserPercentFixed[2])
.arg(gy_xieliao4).arg(sh_xieliao4).arg(dgy_xieliao4).arg(touliao1_4).arg(touliao2_4).arg(touliao3_4).arg(agvUserPercentFixed[3])
.arg((touliao1_2 + touliao1_3 + touliao1_4) * 9)
.arg((touliao2_2 + touliao2_3 + touliao2_4) * 9)
.arg((touliao3_2 + touliao3_3 + touliao3_4) * 9)
);
		});

	connect(ui.reportBrowser, &QTextBrowser::customContextMenuRequested, [=]
		{
			QMenu menu{ this };
			menu.addAction("全选", ui.reportBrowser, &QTextBrowser::selectAll);
			menu.addAction("复制", ui.reportBrowser, &QTextBrowser::copy);
			menu.addAction("清除", ui.reportBrowser, &QTextBrowser::clear);
			menu.exec(QCursor::pos());
		});

	connect(ui.exportReport, &QPushButton::clicked, [=]
		{
			QFile exportFile = QFileDialog::getSaveFileName(this, "导出报告", "", "文本文件(*.txt)");
		if(exportFile.exists())
		{
			exportFile.open(QFile::WriteOnly);
			exportFile.write(ui.reportBrowser->toPlainText().toLatin1());
			exportFile.close();
		}
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

	connect(ui.solution1Choice, &QCheckBox::clicked, [=]
		{
			ui.modelPath->setEnabled(false);
			ui.chooseModel->setEnabled(false);
			ui.modelPath->setText(R"(D:\deneb\GDWJ1\MODELS\GDWJ.mdl)");
		});
	connect(ui.littleChoice, &QCheckBox::clicked, [=]
		{
			ui.modelPath->setEnabled(false);
			ui.chooseModel->setEnabled(false);
			ui.modelPath->setText(R"(D:\deneb\GDWJ2-3crane\MODELS\GDWJ.mdl)");
		});
	connect(ui.bigChoice, &QCheckBox::clicked, [=]
		{
			ui.modelPath->setEnabled(false);
			ui.chooseModel->setEnabled(false);
			ui.modelPath->setText(R"(D:\deneb\GDWJ2-3crane2\MODELS\GDWJ.mdl)");
		});
	connect(ui.bigChoiceDump, &QCheckBox::clicked, [=]
		{
			ui.modelPath->setEnabled(false);
			ui.chooseModel->setEnabled(false);
			ui.modelPath->setText(R"(D:\deneb\GDWJ2-3crane2\MODELS\GDWJ.mdl)");
		});
	connect(ui.customChoice, &QCheckBox::clicked, [=]
		{
			ui.modelPath->setEnabled(true);
			ui.chooseModel->setEnabled(true);
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