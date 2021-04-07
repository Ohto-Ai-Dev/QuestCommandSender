#pragma execution_character_set("utf-8")
#include "QuestClient.h"
#include <windows.h>
#include <thread>
#include <QThread>

QuestClient::QuestClient(QWidget* parent)
	: QMainWindow(parent)
	, questSocket(this)
{
	ui.setupUi(this);

	if (QFile configFile{ configPath }; !configFile.exists())
	{
		configFile.open(QFile::WriteOnly);
		configFile.write(R"({
  "application_name": "QuestClient.exe",
  "version": "v1.0",
  "quest_bat_path":"D:\\deneb\\quest\\quest.bat",
  "quest_port":9988,
  "solution1": {
    "plan_sim_time": 172800
  },
  "solution2": {
    "use_big_grab": {
      "plan_sim_time": 172800
    },
    "run_time": 345600
  },
  "solution3": {
    "use_big_grab": {
      "plan_sim_time": 172800
    },
    "plan_sim_time": 345600
  },
  "wait_quest_time":2500,
  "log_to_file": false,
  "log_to_window": true
}
)");
		configFile.close();
	}
	QFile configFile{ configPath };
	configFile.open(QFile::ReadOnly);
	config = nlohmann::json::parse(configFile.readAll().toStdString());
	configFile.close();

	questPath = QString::fromStdString(config["quest_bat_path"].get<std::string>());
	questPort = config["quest_port"].get<int>();

	ui.agvReportTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
	logFile.open(QFile::Append);

	connect(qApp, &QApplication::lastWindowClosed, &logFile, &QFile::close);

	if (!QFile(questPath).exists())
	{
		QMessageBox::warning(this, "启动失败", "Quest路径无效");
		return;
	}

	QProcess::execute(questPath, { "-s", QString::number(questPort) });

	connect(&questSocket, &DenebTcpSocket::connected, this, [=]
		{
			if (config["log_to_window"])
				ui.logBrowser->append(QString("%1 [System] Connected.")
					.arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")));
			if (config["log_to_file"])
			{
				logFile.write(QString("%1 [System] Connected.\n")
					.arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
					.toLatin1());
				logFile.flush();
			}
		}, Qt::QueuedConnection);
	connect(&questSocket, &DenebTcpSocket::connectFailed, this, [=](int code)
		{
			if (config["log_to_window"])
				ui.logBrowser->append(QString("%1 [System] Error Occurred.(%2)")
					.arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
					.arg(code));
			if (config["log_to_file"])
			{
				logFile.write(QString("%1 [System] Error Occurred.(%2)\n")
					.arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
					.arg(code)
					.toLatin1());
				logFile.flush();
			}
			QMessageBox::warning(this, "错误", QString{ "Error Occurred.(%1)" }.arg(code));
		}, Qt::QueuedConnection);

	connect(ui.resetButton, &QPushButton::clicked, [=]
		{
			sendCommand("RESET RUN");
		});

	connect(ui.startSim, &QPushButton::clicked, [=]
		{
			sendCommand(QString("SET ANIMATION MODE %1").arg(ui.animationMode->isChecked() ? "ON" : "OFF"));
			questSocket.waitReceived();
			sendCommand(QString("SET SIMULATION TIME INTERVAL TO %1").arg(ui.simInterval->text()));
			questSocket.waitReceived();
			sendCommand(QString("RUN %1").arg(planSimTime));

			if (questSocket.waitReceived(1000 * 60 * 5))
				QMessageBox::information(this, "完成", "仿真完成");
		});

	connect(ui.debugButton, &QPushButton::clicked, [=]
		{
			sendCommand("TRANSFER TO MENU");
		});

	connect(ui.updateReport, &QPushButton::clicked, [=]
		{
			int gy_xieliao[4]{ 0,0,0,0 }, sh_xieliao[4]{ 0,0,0,0 }, dgy_xieliao[4]{ 0,0,0,0 }, dgy_xieliao_sum[4]{ 0,0,0,0 }, touliao[4][3]{};
			double agvUserPercentFixed[4]{};
			auto liaoCangTotal = sendInquireUserAttributeCommand("Source_time", "liaocangtotal", true).toDouble();

			for (int i = 1; i < 4; ++i)
			{
				auto agvName = QString("gd_Crane_AGV%1").arg(i + 1);
				gy_xieliao[i] = sendInquireUserNumericAttributeCommand(agvName, "gy_xieliao", true);
				sh_xieliao[i] = sendInquireUserNumericAttributeCommand(agvName, "sh_xieliao", true);
				dgy_xieliao[i] = sendInquireUserNumericAttributeCommand(agvName, "dgy_xieliao", true);
				dgy_xieliao_sum[i] = dgy_xieliao[i] * (ui.useBigGrab->isChecked() ? 6 : 3.5);

				touliao[i][0] = sendInquireUserNumericAttributeCommand(agvName, "touliao1", true);
				touliao[i][1] = sendInquireUserNumericAttributeCommand(agvName, "touliao2", true);
				touliao[i][2] = sendInquireUserNumericAttributeCommand(agvName, "touliao3", true);

				agvUserPercentFixed[i] = sendInquireUserNumericAttributeCommand(agvName, "u_use_rate", true);
			}
			*std::max_element(dgy_xieliao_sum + 1, dgy_xieliao_sum + 4) += 160;

			ui.dgyTotal->setText(QString::number(dgy_xieliao_sum[1] + dgy_xieliao_sum[2] + dgy_xieliao_sum[3]));
			ui.liaocangTotal->setText(QString::number(liaoCangTotal));
			ui.touluTotal->setText(QString::number(
				(touliao[1][0] + touliao[1][1] + touliao[1][2]
					+ touliao[2][0] + touliao[2][1] + touliao[2][2]
					+ touliao[3][0] + touliao[3][1] + touliao[3][2]) * 9
			));

			for (int i = 1; i < 4; ++i)
			{
				ui.agvReportTable->item(0, i - 1)->setText(QString::number(touliao[i][0]));
				ui.agvReportTable->item(0, i - 1)->setData(Qt::UserRole, touliao[i][0]);
				ui.agvReportTable->item(1, i - 1)->setText(QString::number(touliao[i][1]));
				ui.agvReportTable->item(1, i - 1)->setData(Qt::UserRole, touliao[i][1]);
				ui.agvReportTable->item(2, i - 1)->setText(QString::number(touliao[i][2]));
				ui.agvReportTable->item(2, i - 1)->setData(Qt::UserRole, touliao[i][2]);
				ui.agvReportTable->item(3, i - 1)->setText(QString::number(touliao[i][0] * 9));
				ui.agvReportTable->item(3, i - 1)->setData(Qt::UserRole, touliao[i][0] * 9);
				ui.agvReportTable->item(4, i - 1)->setText(QString::number(touliao[i][1] * 9));
				ui.agvReportTable->item(4, i - 1)->setData(Qt::UserRole, touliao[i][1] * 9);
				ui.agvReportTable->item(5, i - 1)->setText(QString::number(touliao[i][2] * 9));
				ui.agvReportTable->item(5, i - 1)->setData(Qt::UserRole, touliao[i][2] * 9);
				ui.agvReportTable->item(6, i - 1)->setText(QString::asprintf("%.3f%%", agvUserPercentFixed[i] * 100));
				ui.agvReportTable->item(6, i - 1)->setData(Qt::UserRole, agvUserPercentFixed[i]);
				ui.agvReportTable->item(7, i - 1)->setText(QString::number(agvUserPercentFixed[i] * 24));
				ui.agvReportTable->item(7, i - 1)->setData(Qt::UserRole, agvUserPercentFixed[i] * 24);
				ui.agvReportTable->item(8, i - 1)->setText(QString::number(gy_xieliao[i]));
				ui.agvReportTable->item(8, i - 1)->setData(Qt::UserRole, gy_xieliao[i]);
				ui.agvReportTable->item(9, i - 1)->setText(QString::number(gy_xieliao[i] * (ui.useBigGrab->isChecked() ? 6 : 3.5)));
				ui.agvReportTable->item(9, i - 1)->setData(Qt::UserRole, gy_xieliao[i] * (ui.useBigGrab->isChecked() ? 6 : 3.5));
				ui.agvReportTable->item(10, i - 1)->setText(QString::number(sh_xieliao[i]));
				ui.agvReportTable->item(10, i - 1)->setData(Qt::UserRole, sh_xieliao[i]);
				ui.agvReportTable->item(11, i - 1)->setText(QString::number(sh_xieliao[i] * 8));
				ui.agvReportTable->item(11, i - 1)->setData(Qt::UserRole, sh_xieliao[i] * 8);
				ui.agvReportTable->item(12, i - 1)->setText(QString::number(dgy_xieliao[i]));
				ui.agvReportTable->item(12, i - 1)->setData(Qt::UserRole, dgy_xieliao[i]);
				ui.agvReportTable->item(13, i - 1)->setText(QString::number(dgy_xieliao_sum[i]));
				ui.agvReportTable->item(13, i - 1)->setData(Qt::UserRole, dgy_xieliao_sum[i]);
			}
		});

	connect(ui.exportReport, &QPushButton::clicked, [=]
		{
			const auto exportFile = QFileDialog::getSaveFileName(this, "导出报告", "", "Excel 文件(*.xls *.xlsx)");
			if (exportFile.isEmpty())
				return;
			else if (const QFileInfo fileInfo(exportFile); QFile{ fileInfo.path() + "/~$" + fileInfo.fileName() }.exists())
			{
				QMessageBox::warning(this, "错误", "此文件已被Excel锁定，无法写入，请重试.");
				return;
			}


			if (!excel.setControl("Excel.Application")) //连接Excel控件
			{
				QMessageBox::warning(this, "错误", "未能创建 Excel 对象，请安装 Microsoft Excel。", QMessageBox::Apply);
				return;
			}
			else {
				excel.dynamicCall("SetVisible (bool Visible)", "false");		// 不显示窗体
				excel.setProperty("DisplayAlerts", false);					// 不显示任何警告信息。如果为true那么在关闭是会出现类似“文件已修改，是否保存”的提示
				excel.querySubObject("WorkBooks")->dynamicCall("Add");		// 新建一个工作簿
				auto workbook = excel.querySubObject("ActiveWorkBook");// 获取当前工作簿
				auto worksheet = workbook->querySubObject("Worksheets(int)", 1);
				const auto table = ui.agvReportTable;

				const int columnCount = table->columnCount();
				const int rowCount = table->rowCount();

				//标题行
				auto cell = worksheet->querySubObject("Cells(int,int)", 1, 1);
				cell->dynamicCall("SetValue(const QString&)", "报表统计");
				cell->querySubObject("Font")->setProperty("Size", 18);
				// 调整行高
				worksheet->querySubObject("Range(const QString&)", "1:1")->setProperty("RowHeight", 30);
				// 合并标题行
				auto range = worksheet->querySubObject("Range(const QString&)", QString::asprintf("A1:%c1", columnCount + 'A'));
				range->setProperty("WrapText", true);
				range->setProperty("MergeCells", true);
				range->setProperty("HorizontalAlignment", -4108);		// xlCenter
				range->setProperty("VerticalAlignment", -4108);		// xlCenter

				// 列标题
				for (int i = 0; i < columnCount; i++)
				{
					worksheet->querySubObject("Columns(const QString&)", QString::asprintf("%c:%c", i + 'B', i + 'B'))
						->setProperty("ColumnWidth", table->columnWidth(i) / 6);
					auto cell = worksheet->querySubObject("Cells(int,int)", 2, i + 2);
					cell->dynamicCall("SetValue(const QString&)", table->horizontalHeaderItem(i)->text());
					cell->querySubObject("Font")->setProperty("Bold", true);
					cell->setProperty("HorizontalAlignment", -4108);	// xlCenter
					cell->setProperty("VerticalAlignment", -4108);		// xlCenter
				}

				// 行标题
				worksheet->querySubObject("Columns(const QString&)", "A:A")
					->setProperty("ColumnWidth", table->verticalHeader()->width() / 6);
				for (int i = 0; i < rowCount; i++)
				{
					auto cell = worksheet->querySubObject("Cells(int,int)", i + 3, 1);
					cell->dynamicCall("SetValue(const QString&)", table->verticalHeaderItem(i)->text());
					cell->querySubObject("Font")->setProperty("Bold", true);
					cell->setProperty("HorizontalAlignment", -4108);	// xlCenter
					cell->setProperty("VerticalAlignment", -4108);		// xlCenter
				}
				// 数据区
				for (int i = 0; i < rowCount; i++)
				{
					for (int j = 0; j < columnCount; j++)
					{
						worksheet->querySubObject("Cells(int,int)", i + 3, j + 2)->dynamicCall("SetValue(const QVariant&)", table->item(i, j)->data(Qt::UserRole));
					}
				}
				// 设置利用率百分比
				worksheet->querySubObject("Range(const QString&)", "9:9")
					->setProperty("NumberFormatLocal", "0.000%");
				// 画框线
				range = worksheet->querySubObject("Range(const QString&)", QString::asprintf("A2:%c%d", columnCount + 'A', rowCount + 2));
				range->querySubObject("Borders")->setProperty("LineStyle", QString::number(1));
				range->querySubObject("Borders")->setProperty("Color", QColor(0, 0, 0));
				// 调整数据区行高
				range = worksheet->querySubObject("Range(const QString&)", QString::asprintf("2:%d", rowCount + 2));
				range->setProperty("RowHeight", 20);
				workbook->dynamicCall("SaveAs(const QString&)", QDir::toNativeSeparators(exportFile));	// 保存至fileName
				workbook->dynamicCall("Close()");															// 关闭工作簿
				excel.dynamicCall("Quit()");																// 关闭excel
				QMessageBox::information(this, "", "报表已导出");
			}
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

	connect(ui.solution1Choice, &QRadioButton::clicked, [=]
		{
			ui.useBigGrab->setEnabled(false);
			ui.usePeakTime->setEnabled(false);
			ui.useBigGrab->setChecked(false);
			ui.usePeakTime->setChecked(false);
		});
	connect(ui.solution2Choice, &QRadioButton::clicked, [=]
		{
			ui.useBigGrab->setEnabled(true);
			ui.usePeakTime->setEnabled(ui.useBigGrab->isChecked());
			if (!ui.usePeakTime->isEnabled())
				ui.usePeakTime->setChecked(false);
		});
	connect(ui.solution3Choice, &QRadioButton::clicked, [=]
		{
			ui.useBigGrab->setEnabled(true);
			ui.usePeakTime->setEnabled(ui.useBigGrab->isChecked());
			if (!ui.usePeakTime->isEnabled())
				ui.usePeakTime->setChecked(false);
		});
	connect(ui.useBigGrab, &QCheckBox::clicked, [=]
		{
			ui.usePeakTime->setEnabled(ui.useBigGrab->isChecked());
			if (!ui.useBigGrab->isChecked())
				ui.usePeakTime->setChecked(false);
		});

	connect(ui.loadModel, &QPushButton::clicked, [=]
		{
			sendCommand("CLEAR ALL");
			questSocket.waitReceived();

			if (ui.solution1Choice->isChecked())
			{
				planSimTime = config["solution1"]["plan_sim_time"].get<int>();
				sendCommand(R"(READ MODEL 'D:\deneb\GDWJ1\MODELS\GDWJ.mdl')");
			}
			else
			{
				if (ui.useBigGrab->isChecked())
				{
					planSimTime = config[ui.solution2Choice->isChecked() ? "solution2" : "solution3"]["use_big_grab"]["plan_sim_time"].get<int>();
					sendCommand(R"(READ MODEL 'D:\deneb\GDWJ2-3crane2\MODELS\GDWJ.mdl')");
				}
				else
				{
					planSimTime = config[ui.solution2Choice->isChecked() ? "solution2" : "solution3"]["plan_sim_time"].get<int>();
					sendCommand(R"(READ MODEL 'D:\deneb\GDWJ2-3crane\MODELS\GDWJ.mdl')");
				}

				sendSetUserAttributeCommand("Source_time", "mode3", QString::number(ui.solution3Choice->isChecked()));
				sendSetUserAttributeCommand("Source_time", "modetag", QString::number(ui.usePeakTime->isChecked()));
			}
		});

	connect(&questSocket, &DenebTcpSocket::received, [=]
		{
			currentReceivedMessage = QString::fromLatin1(questSocket.read());

			if (config["log_to_window"])
				ui.logBrowser->append(QString("%1 [Simulation] %2")
					.arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
					.arg(currentReceivedMessage));
			if (config["log_to_file"])
			{
				logFile.write(QString("%1 [Simulation] %2\n")
					.arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
					.arg(currentReceivedMessage).toLatin1());
				logFile.flush();
			}
		});

	QEventLoop eventLoop{ this };
	QTimer::singleShot(config["wait_quest_time"].get<int>(), &eventLoop, &QEventLoop::quit);
	eventLoop.exec();
	questSocket.connectToHost("localhost", questPort);
}

void QuestClient::sendCommand(QString command) const
{
	questSocket.write(command.toLatin1());

	if (config["log_to_window"])
		ui.logBrowser->append(QString("%1 [Client] %2")
			.arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
			.arg(command));
	if (config["log_to_file"])
	{
		logFile.write(QString("%1 [Client] %2\n")
			.arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"))
			.arg(command).toLatin1());
		logFile.flush();
	}
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

double QuestClient::sendInquireUserNumericAttributeCommand(QString name, QString attribute, bool isInstance) const
{
	return sendInquireUserAttributeCommand(name, attribute, isInstance).toDouble();
}