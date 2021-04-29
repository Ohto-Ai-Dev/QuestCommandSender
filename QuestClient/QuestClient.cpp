#pragma execution_character_set("utf-8")
#include "QuestClient.h"

QuestClient::QuestClient(QWidget* parent)
	: QMainWindow(parent)
	, questSocket(this)
{
	ui.setupUi(this);

	constexpr auto configData = R"({
  "application_name": "QuestClient.exe",
  "version": "v1.0",
  "quest_bat_path":"D:\\deneb\\quest\\quest.bat",
  "quest_port":9988,
  "solution1": {
    "plan_sim_time": 172800
  },
  "solution2": {
    "use_big_grab": {
      "plan_sim_time": 345600
    },
	"use_peak_time": {
	  "plan_sim_time": 345600
	},
    "plan_sim_time": 345600
  },
  "solution3": {
    "use_big_grab": {
      "plan_sim_time": 345600
    },
	"use_peak_time": {
	  "plan_sim_time": 259200
	},
    "plan_sim_time": 345600
  },
  "wait_quest_time":1500,
  "log_to_file": false,
  "log_to_window": true
}
)";

#ifdef USE_CONFIG_FILE
	if (QFile configFile{ configPath }; !configFile.exists())
	{
		if (!configFile.open(QFile::WriteOnly))
		{
			QMessageBox::critical(this, "错误", "无法创建配置文件!");
			QApplication::quit();
			return;
		}
		configFile.write(configData);
		configFile.close();
	}
	if (QFile configFile{ configPath }; !configFile.open(QFile::ReadOnly))
	{
		QMessageBox::critical(this, "错误", "无法读取配置文件!");
		QApplication::quit();
		return;
}
	else
	{
		config = nlohmann::json::parse(configFile.readAll().toStdString());
		configFile.close();
	}
#else
	config = nlohmann::json::parse(configData);
#endif

	connect(&debugCheckTimer, &QTimer::timeout, [=]
		{
			if (QFile{ ".debug" }.exists())
			{
				ui.debugPannel->show();
			}
			else
			{
				ui.debugPannel->hide();
			}
		});
	debugCheckTimer.setInterval(1000);
	debugCheckTimer.start();

	questPath = QString::fromStdString(config["quest_bat_path"].get<std::string>());
	questPort = config["quest_port"].get<int>();

	ui.agvReportTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

	if (config["log_to_file"])
	{
		logFile.open(QFile::Append);
		connect(qApp, &QApplication::lastWindowClosed, &logFile, &QFile::close);
	}
	if (!QFile(questPath).exists())
	{
		QMessageBox::warning(this, "启动失败", "Quest路径无效");
		QApplication::quit();
		return;
	}

	QProcess::execute(questPath, { "-s", QString::number(questPort) });
		
	connect(&questSocket, &DenebTcpSocket::connected, this, [=]
		{
			if (config["log_to_window"])
				ui.logBrowser->appendPlainText(QString("%1 [System] Connected.")
					.arg(QTime::currentTime().toString("hh:mm:ss")));
			if (config["log_to_file"])
			{
				logFile.write(QString("%1 [System] Connected.\n")
					.arg(QTime::currentTime().toString("hh:mm:ss"))
					.toLatin1());
				logFile.flush();
			}
		});
	connect(&questSocket, &DenebTcpSocket::connectFailed, this, [=](int code)
		{
			if (config["log_to_window"])
				ui.logBrowser->appendPlainText(QString("%1 [System] Error Occurred.(%2)")
					.arg(QTime::currentTime().toString("hh:mm:ss"))
					.arg(code));
			if (config["log_to_file"])
			{
				logFile.write(QString("%1 [System] Error Occurred.(%2)\n")
					.arg(QTime::currentTime().toString("hh:mm:ss"))
					.arg(code)
					.toLatin1());
				logFile.flush();
			}
			QMessageBox::warning(this, "错误", QString{ "Error Occurred.(%1)" }.arg(code));
		});

	connect(ui.animationMode, &QCheckBox::clicked, ui.simInterval, &QLineEdit::setEnabled);

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

	connect(ui.commandEdit, &QLineEdit::returnPressed, [=]
		{
			sendCommand(ui.commandEdit->text());
			ui.commandEdit->clear();
		});
	connect(ui.sendCommand, &QPushButton::clicked, [=]
		{
			sendCommand(ui.commandEdit->text());
			ui.commandEdit->clear();
		});

	connect(ui.updateReport, &QPushButton::clicked, [=]
		{
			ui.updateReport->setEnabled(false);
			int countGY_XL[4]{ 0,0,0,0 }, countSH_XL[4]{ 0,0,0,0 }, countDGY_XL[4]{ 0,0,0,0 }, counFeeding[4][3]{}, countHoistUse[4]{ 0,0,0,0 };
			double craneAgvUseRate[4]{}, crossTravelUseTime[4]{}, hoistUseTime[4]{}, elevatorUseTime[4]{};
			double weightDGY_XL[4]{ 0,0,0,0 }, weightGY_XL[4], weightSH_XL[4];
			auto weightLiaoCang = sendInquireUserAttributeCommand("Source_time", "liaocangtotal", true).toDouble();

			for (int i = 1; i < 4; ++i)
			{
				auto craneAgvName = QString("gd_Crane_AGV%1").arg(i + 1);
				countGY_XL[i] = sendInquireUserNumericAttributeCommand(craneAgvName, "gy_xieliao", true);
				countSH_XL[i] = sendInquireUserNumericAttributeCommand(craneAgvName, "sh_xieliao", true);
				countDGY_XL[i] = sendInquireUserNumericAttributeCommand(craneAgvName, "dgy_xieliao", true);

				counFeeding[i][0] = sendInquireUserNumericAttributeCommand(craneAgvName, "touliao1", true);
				counFeeding[i][1] = sendInquireUserNumericAttributeCommand(craneAgvName, "touliao2", true);
				counFeeding[i][2] = sendInquireUserNumericAttributeCommand(craneAgvName, "touliao3", true);

				craneAgvUseRate[i] = sendInquireUserNumericAttributeCommand(craneAgvName, "u_use_rate", true);

			}

			// #2 #4 送到#3
			if (craneFailure == 2 || craneFailure == 4)
			{
				countGY_XL[2] += countGY_XL[craneFailure - 1];
				countSH_XL[2] += countSH_XL[craneFailure - 1];
				countDGY_XL[2] += countDGY_XL[craneFailure - 1];
				counFeeding[2][0] += counFeeding[craneFailure - 1][0];
				counFeeding[2][1] += counFeeding[craneFailure - 1][1];
				counFeeding[2][2] += counFeeding[craneFailure - 1][2];
				craneAgvUseRate[2] += craneAgvUseRate[craneFailure - 1];

				countGY_XL[craneFailure - 1] = 0;
				countSH_XL[craneFailure - 1] = 0;
				countDGY_XL[craneFailure - 1] = 0;
				counFeeding[craneFailure - 1][0] = 0;
				counFeeding[craneFailure - 1][1] = 0;
				counFeeding[craneFailure - 1][2] = 0;
				craneAgvUseRate[craneFailure - 1] = 0;

			}
			// #2 
			if (craneFailure == 2 && luziFailure == 0)
			{
				counFeeding[3][1] += counFeeding[2][1]; // #3车#2炉数据放到#4车
				counFeeding[2][1] = 0;
			}

			for (int i = 1; i < 4; ++i)
			{				
				weightDGY_XL[i] = countDGY_XL[i] * (ui.useBigGrab->isChecked() ? 6 : 3.5);
				weightGY_XL[i] = countGY_XL[i] * (ui.useBigGrab->isChecked() ? 6 : 3.5);
				weightSH_XL[i] = countSH_XL[i] * 8;

				crossTravelUseTime[i] = (30000 / 2 / 700.0 * (counFeeding[i][0] + counFeeding[i][1] + counFeeding[i][2] + countGY_XL[i] + countSH_XL[i] + countDGY_XL[i])
					+ (counFeeding[i][0] + counFeeding[i][1] + counFeeding[i][2]) * 120 + (countGY_XL[i] + countDGY_XL[i]) * 150 + countSH_XL[i] * 60) / 3600.0;
				hoistUseTime[i] = 48 * (counFeeding[i][0] + counFeeding[i][1] + counFeeding[i][2] + countGY_XL[i] + countSH_XL[i] + countDGY_XL[i]) / 3600.0;

				countHoistUse[i] = counFeeding[i][0] + counFeeding[i][1] + counFeeding[i][2] + countGY_XL[i] + countSH_XL[i] + countDGY_XL[i];
				elevatorUseTime[i] = 53.5 * countHoistUse[i] / 3600.0;

			}

		
			ui.dgyTotal->setText(QString::number(weightDGY_XL[1] + weightDGY_XL[2] + weightDGY_XL[3]));
			ui.liaocangTotal->setText(QString::number(weightLiaoCang));
			ui.touluTotal->setText(QString::number(
				(counFeeding[1][0] + counFeeding[1][1] + counFeeding[1][2]
					+ counFeeding[2][0] + counFeeding[2][1] + counFeeding[2][2]
					+ counFeeding[3][0] + counFeeding[3][1] + counFeeding[3][2]) * 9
			));
			ui.xieliaoTotal->setText(QString::number(
				weightDGY_XL[0] + weightDGY_XL[1] + weightDGY_XL[2] + weightDGY_XL[3]
				+ weightGY_XL[0] + weightGY_XL[1] + weightGY_XL[2] + weightGY_XL[3]
				+ weightSH_XL[0] + weightSH_XL[1] + weightSH_XL[2] + weightSH_XL[3]
			));


			for (int i = 1; i < 4; ++i)
			{
				ui.agvReportTable->item(0, i - 1)->setText(QString::number(counFeeding[i][0]));
				ui.agvReportTable->item(0, i - 1)->setData(Qt::UserRole, counFeeding[i][0]);
				ui.agvReportTable->item(1, i - 1)->setText(QString::number(counFeeding[i][0] * 9));
				ui.agvReportTable->item(1, i - 1)->setData(Qt::UserRole, counFeeding[i][0] * 9);
				ui.agvReportTable->item(2, i - 1)->setText(QString::number(counFeeding[i][1]));
				ui.agvReportTable->item(2, i - 1)->setData(Qt::UserRole, counFeeding[i][1]);
				ui.agvReportTable->item(3, i - 1)->setText(QString::number(counFeeding[i][1] * 9));
				ui.agvReportTable->item(3, i - 1)->setData(Qt::UserRole, counFeeding[i][1] * 9);
				ui.agvReportTable->item(4, i - 1)->setText(QString::number(counFeeding[i][2]));
				ui.agvReportTable->item(4, i - 1)->setData(Qt::UserRole, counFeeding[i][2]);
				ui.agvReportTable->item(5, i - 1)->setText(QString::number(counFeeding[i][2] * 9));
				ui.agvReportTable->item(5, i - 1)->setData(Qt::UserRole, counFeeding[i][2] * 9);
				ui.agvReportTable->item(6, i - 1)->setText(QString::asprintf("%.2f%%", craneAgvUseRate[i] * 100));
				ui.agvReportTable->item(6, i - 1)->setData(Qt::UserRole, craneAgvUseRate[i]);
				ui.agvReportTable->item(7, i - 1)->setText(QString::number(craneAgvUseRate[i] * 24, 'f', 2));
				ui.agvReportTable->item(7, i - 1)->setData(Qt::UserRole, craneAgvUseRate[i] * 24);
				ui.agvReportTable->item(8, i - 1)->setText(QString::number(crossTravelUseTime[i], 'f', 2));
				ui.agvReportTable->item(8, i - 1)->setData(Qt::UserRole, crossTravelUseTime[i]);
				ui.agvReportTable->item(9, i - 1)->setText(QString::number(hoistUseTime[i], 'f', 2));
				ui.agvReportTable->item(9, i - 1)->setData(Qt::UserRole, hoistUseTime[i]);
				ui.agvReportTable->item(10, i - 1)->setText(QString::number(elevatorUseTime[i], 'f', 2));
				ui.agvReportTable->item(10, i - 1)->setData(Qt::UserRole, elevatorUseTime[i]);				
				ui.agvReportTable->item(11, i - 1)->setText(QString::number(countGY_XL[i]));
				ui.agvReportTable->item(11, i - 1)->setData(Qt::UserRole, countGY_XL[i]);
				ui.agvReportTable->item(12, i - 1)->setText(QString::number(weightGY_XL[i]));
				ui.agvReportTable->item(12, i - 1)->setData(Qt::UserRole, weightGY_XL[i]);
				ui.agvReportTable->item(13, i - 1)->setText(QString::number(countSH_XL[i]));
				ui.agvReportTable->item(13, i - 1)->setData(Qt::UserRole, countSH_XL[i]);
				ui.agvReportTable->item(14, i - 1)->setText(QString::number(weightSH_XL[i]));
				ui.agvReportTable->item(14, i - 1)->setData(Qt::UserRole, weightSH_XL[i]);
				ui.agvReportTable->item(15, i - 1)->setText(QString::number(countDGY_XL[i]));
				ui.agvReportTable->item(15, i - 1)->setData(Qt::UserRole, countDGY_XL[i]);
				ui.agvReportTable->item(16, i - 1)->setText(QString::number(weightDGY_XL[i]));
				ui.agvReportTable->item(16, i - 1)->setData(Qt::UserRole, weightDGY_XL[i]);
				ui.agvReportTable->item(17, i - 1)->setText(QString::number(countHoistUse[i]));
				ui.agvReportTable->item(17, i - 1)->setData(Qt::UserRole, countHoistUse[i]);
			}

			ui.updateReport->setEnabled(true);
		});

	connect(ui.exportReport, &QPushButton::clicked, [=]
		{
			const auto exportFile = QFileDialog::getSaveFileName(this, "导出报告"
				, QString{ "report-%1.xls" }.arg(QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss"))
				, "Excel 文件(*.xls *.xlsx)");
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
						auto var = table->item(i, j)->data(Qt::UserRole);
						auto cell = worksheet->querySubObject("Cells(int,int)", i + 3, j + 2);
						cell->dynamicCall("SetValue(const QVariant&)", var.isNull() ? "" : var);
						if (table->item(i, j)->text().back() == '%')
							cell->setProperty("NumberFormatLocal", "0.000%");
					}
				}

				worksheet->querySubObject("Cells(int,int)", table->rowCount() + 3, 1)
					->dynamicCall("SetValue(const QString&)", ui.labelDgyTotal->text());
				worksheet->querySubObject("Cells(int,int)", table->rowCount() + 3, 2)
					->dynamicCall("SetValue(const QString&)", ui.dgyTotal->text());

				worksheet->querySubObject("Cells(int,int)", table->rowCount() + 4, 1)
					->dynamicCall("SetValue(const QString&)", ui.labelLiaocangTotal->text());
				worksheet->querySubObject("Cells(int,int)", table->rowCount() + 4, 2)
					->dynamicCall("SetValue(const QString&)", ui.liaocangTotal->text());

				worksheet->querySubObject("Cells(int,int)", table->rowCount() + 5, 1)
					->dynamicCall("SetValue(const QString&)", ui.labelTouluTotal->text());
				worksheet->querySubObject("Cells(int,int)", table->rowCount() + 5, 2)
					->dynamicCall("SetValue(const QString&)", ui.touluTotal->text());

				worksheet->querySubObject("Cells(int,int)", table->rowCount() + 6, 1)
					->dynamicCall("SetValue(const QString&)", ui.labelXieliaoTotal->text());
				worksheet->querySubObject("Cells(int,int)", table->rowCount() + 6, 2)
					->dynamicCall("SetValue(const QString&)", ui.xieliaoTotal->text());
				
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
			double addSpeed = 0;
			if (!ui.solution1Choice->isChecked())
			{
				if (ui.useBigGrab->isChecked())
					addSpeed += 300;
				else
					addSpeed += 200;
			}
			sendSetCommand("gd_Crane_AGV1", "SPEED", QString::number(ui.agvSpeed1->text().toInt() + addSpeed));
			sendSetCommand("gd_Crane_AGV2", "SPEED", QString::number(ui.agvSpeed2->text().toInt() + addSpeed));
			sendSetCommand("gd_Crane_AGV3", "SPEED", QString::number(ui.agvSpeed3->text().toInt() + addSpeed));
			sendSetCommand("gd_Crane_AGV4", "SPEED", QString::number(ui.agvSpeed4->text().toInt() + addSpeed));
			sendSetCommand("gd_Crane_AGV1", "LOADED SPEED", QString::number(ui.agvSpeed1->text().toInt() + addSpeed));
			sendSetCommand("gd_Crane_AGV2", "LOADED SPEED", QString::number(ui.agvSpeed2->text().toInt() + addSpeed));
			sendSetCommand("gd_Crane_AGV3", "LOADED SPEED", QString::number(ui.agvSpeed3->text().toInt() + addSpeed));
			sendSetCommand("gd_Crane_AGV4", "LOADED SPEED", QString::number(ui.agvSpeed4->text().toInt() + addSpeed));
		});

	connect(ui.sendHoistSpeed, &QPushButton::clicked, [=]
		{
			sendSetCommand("gd_Hoist1", "SPEED", ui.hoistSpeed1->text());
			sendSetCommand("gd_Hoist2", "SPEED", ui.hoistSpeed2->text());
			sendSetCommand("gd_Hoist3", "SPEED", ui.hoistSpeed3->text());
			sendSetCommand("gd_Hoist4", "SPEED", ui.hoistSpeed4->text());
		});

	connect(ui.sendOtherScene, &QPushButton::clicked, [=]
		{
			craneFailure = 0;
			luziFailure = 0;
			if (ui.solutionCrane2Failure->isChecked()
				|| ui.solutionLuzi1Failure->isChecked())
				craneFailure = 2;
			else if (ui.solutionLuzi3Failure->isChecked())
				craneFailure = 4;

			
			if (ui.solutionLuzi1Failure->isChecked())
				luziFailure = 1;
			else if (ui.solutionLuzi3Failure->isChecked())
				luziFailure = 3;

			sendSetUserAttributeCommand("Source_time", "crane_failure", QString::number(craneFailure));
			unityServer.write(QString("crane_failure = %1").arg(craneFailure).toLatin1());

			auto reconnectElement = [=](QString fromClass, QString toClass, bool isConnect)
			{
				sendCommand(QString{ "DISCONNECT ELEMENT '%1_1' ALL INPUT" }.arg(toClass));
				if (isConnect)
					sendCommand(QString{ "CONNECT ELEMENT '%1_1' TO ELEMENT '%2_1'" }.arg(fromClass, toClass));
			};

			reconnectElement("Buffer_LK3_2", "Buffer_temp_LK3_2", luziFailure != 1);
			reconnectElement("Buffer_LK3_2", "Buffer_temp_LK3_3", luziFailure != 3);
		
			sendSetUserAttributeCommand("Source_time", "luzi_failure", QString::number(luziFailure));
			unityServer.write(QString("luzi_failure = %1").arg(luziFailure).toLatin1());

			if (craneFailure == 2 && luziFailure == 0)
			{
				sendSetCommand("gd_Crane_AGV2", "SPEED", QString::number(900));
				sendSetCommand("gd_Crane_AGV2", "LOADED SPEED", QString::number(900));
				sendSetCommand("gd_Crane_AGV3", "SPEED", QString::number(1000));
				sendSetCommand("gd_Crane_AGV4", "SPEED", QString::number(1000));
				sendSetCommand("gd_Crane_AGV3", "LOADED SPEED", QString::number(1000));
				sendSetCommand("gd_Crane_AGV4", "LOADED SPEED", QString::number(1000));
			}
		
		});

	connect(ui.restoreNormalScene, &QPushButton::clicked, this, &QuestClient::restoreNormalScene);
	
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
			ui.otherScenePannel->setEnabled(false);
		});
	connect(ui.solution2Choice, &QRadioButton::clicked, [=]
		{
			ui.useBigGrab->setEnabled(true);
			ui.usePeakTime->setEnabled(ui.useBigGrab->isChecked());
			if (!ui.usePeakTime->isEnabled())
				ui.usePeakTime->setChecked(false);
			ui.otherScenePannel->setEnabled(!ui.useBigGrab->isChecked());
		});
	connect(ui.solution3Choice, &QRadioButton::clicked, [=]
		{
			ui.useBigGrab->setEnabled(true);
			ui.usePeakTime->setEnabled(ui.useBigGrab->isChecked());
			if (!ui.usePeakTime->isEnabled())
				ui.usePeakTime->setChecked(false);
			ui.otherScenePannel->setEnabled(false);
		});
	connect(ui.useBigGrab, &QCheckBox::clicked, [=]
		{
			ui.usePeakTime->setEnabled(ui.useBigGrab->isChecked());
			if (!ui.useBigGrab->isChecked())
				ui.usePeakTime->setChecked(false);
			ui.otherScenePannel->setEnabled(ui.solution2Choice->isChecked() && !ui.useBigGrab->isChecked());
		});
	connect(ui.usePeakTime, &QPushButton::clicked, [=]
		{
			ui.otherScenePannel->setEnabled(ui.solution2Choice->isChecked() && !ui.useBigGrab->isChecked());
		});

	connect(ui.loadModel, &QPushButton::clicked, [=]
		{
			int modelChoice = 0;
			if (ui.solution1Choice->isChecked())
				modelChoice = 1;
			else if (ui.solution2Choice->isChecked())
				modelChoice = 2;
			else if (ui.solution3Choice->isChecked())
				modelChoice = 3;
			unityServer.write(QString::asprintf("model = %d", modelChoice).toLatin1());

				
			sendCommand("CLEAR ALL");
			questSocket.waitReceived();

			if (ui.solution1Choice->isChecked())
			{
				planSimTime = config["solution1"]["plan_sim_time"].get<int>();
				sendCommand(R"(READ MODEL 'D:\deneb\GDWJ1\MODELS\GDWJ.mdl')");

				sendSetCommand("gd_Crane_AGV1", "SPEED", QString::number(700));
				sendSetCommand("gd_Crane_AGV2", "SPEED", QString::number(600));
				sendSetCommand("gd_Crane_AGV3", "SPEED", QString::number(300));
				sendSetCommand("gd_Crane_AGV4", "SPEED", QString::number(600));
				sendSetCommand("gd_Crane_AGV1", "LOADED SPEED", QString::number(700));
				sendSetCommand("gd_Crane_AGV2", "LOADED SPEED", QString::number(600));
				sendSetCommand("gd_Crane_AGV3", "LOADED SPEED", QString::number(300));
				sendSetCommand("gd_Crane_AGV4", "LOADED SPEED", QString::number(600));
				sendSetCommand("gd_Hoist1", "SPEED", "700");
				sendSetCommand("gd_Hoist2", "SPEED", "700");
				sendSetCommand("gd_Hoist3", "SPEED", "700");
				sendSetCommand("gd_Hoist4", "SPEED", "700");
			}
			else
			{
				if (ui.useBigGrab->isChecked())
				{
					planSimTime = config[ui.solution2Choice->isChecked() ? "solution2" : "solution3"]
						[ui.usePeakTime->isChecked() ? "use_peak_time" : "use_big_grab"]["plan_sim_time"].get<int>();
					sendCommand(R"(READ MODEL 'D:\deneb\GDWJ2-3crane2\MODELS\GDWJ.mdl')");
				}
				else
				{
					planSimTime = config[ui.solution2Choice->isChecked() ? "solution2" : "solution3"]["plan_sim_time"].get<int>();
					sendCommand(R"(READ MODEL 'D:\deneb\GDWJ2-3crane\MODELS\GDWJ.mdl')");
				}

				sendSetUserAttributeCommand("Source_time", "mode3", QString::number(ui.solution3Choice->isChecked()));
				sendSetUserAttributeCommand("Source_time", "modetag", QString::number(ui.usePeakTime->isChecked()));

				// 方案2 3 小抓斗 950 950 950 950 = 2772 2781
				if (!ui.useBigGrab->isChecked())
				{
					auto speed = 950;
					sendSetCommand("gd_Crane_AGV1", "SPEED", QString::number(speed));
					sendSetCommand("gd_Crane_AGV2", "SPEED", QString::number(speed));
					sendSetCommand("gd_Crane_AGV3", "SPEED", QString::number(speed));
					sendSetCommand("gd_Crane_AGV4", "SPEED", QString::number(speed));
					sendSetCommand("gd_Crane_AGV1", "LOADED SPEED", QString::number(speed));
					sendSetCommand("gd_Crane_AGV2", "LOADED SPEED", QString::number(speed));
					sendSetCommand("gd_Crane_AGV3", "LOADED SPEED", QString::number(speed));
					sendSetCommand("gd_Crane_AGV4", "LOADED SPEED", QString::number(speed));
				}
				// 方案2 大抓斗
				else if (ui.solution2Choice->isChecked() && !ui.usePeakTime->isChecked())
				{
					auto speed = 570;
					sendSetCommand("gd_Crane_AGV1", "SPEED", QString::number(speed));
					sendSetCommand("gd_Crane_AGV2", "SPEED", QString::number(speed));
					sendSetCommand("gd_Crane_AGV3", "SPEED", QString::number(speed));
					sendSetCommand("gd_Crane_AGV4", "SPEED", QString::number(speed));
					sendSetCommand("gd_Crane_AGV1", "LOADED SPEED", QString::number(speed));
					sendSetCommand("gd_Crane_AGV2", "LOADED SPEED", QString::number(speed));
					sendSetCommand("gd_Crane_AGV3", "LOADED SPEED", QString::number(speed));
					sendSetCommand("gd_Crane_AGV4", "LOADED SPEED", QString::number(speed));
				}
				else if (ui.solution3Choice->isChecked() && !ui.usePeakTime->isChecked())
				{
					// ok.
				}
				else if (ui.solution2Choice->isChecked() && ui.usePeakTime->isChecked())
				{
					auto speed = 940;
					sendSetCommand("gd_Crane_AGV1", "SPEED", QString::number(speed));
					sendSetCommand("gd_Crane_AGV2", "SPEED", QString::number(speed));
					sendSetCommand("gd_Crane_AGV3", "SPEED", QString::number(speed));
					sendSetCommand("gd_Crane_AGV4", "SPEED", QString::number(speed));
					sendSetCommand("gd_Crane_AGV1", "LOADED SPEED", QString::number(speed));
					sendSetCommand("gd_Crane_AGV2", "LOADED SPEED", QString::number(speed));
					sendSetCommand("gd_Crane_AGV3", "LOADED SPEED", QString::number(speed));
					sendSetCommand("gd_Crane_AGV4", "LOADED SPEED", QString::number(speed));
				}
				else if (ui.solution3Choice->isChecked() && ui.usePeakTime->isChecked())
				{
					auto speed = 700;
					sendSetCommand("gd_Crane_AGV1", "SPEED", QString::number(speed));
					sendSetCommand("gd_Crane_AGV2", "SPEED", QString::number(speed));
					sendSetCommand("gd_Crane_AGV3", "SPEED", QString::number(speed));
					sendSetCommand("gd_Crane_AGV4", "SPEED", QString::number(speed));
					sendSetCommand("gd_Crane_AGV1", "LOADED SPEED", QString::number(speed));
					sendSetCommand("gd_Crane_AGV2", "LOADED SPEED", QString::number(speed));
					sendSetCommand("gd_Crane_AGV3", "LOADED SPEED", QString::number(speed));
					sendSetCommand("gd_Crane_AGV4", "LOADED SPEED", QString::number(speed));
				}
				restoreNormalScene();
			}
		});

	connect(&questSocket, &DenebTcpSocket::received, [=]
		{
			currentReceivedMessage = QString::fromLatin1(questSocket.read());

			if (config["log_to_window"])
				ui.logBrowser->appendPlainText(QString("%1 [Simulation] %2")
					.arg(QTime::currentTime().toString("hh:mm:ss"))
					.arg(currentReceivedMessage));
			if (config["log_to_file"])
			{
				logFile.write(QString("%1 [Simulation] %2\n")
					.arg(QTime::currentTime().toString("hh:mm:ss"))
					.arg(currentReceivedMessage).toLatin1());
				logFile.flush();
			}
		});

	unityServer.connectToHost("localhost", 9730);

	if (!unityServer.waitForConnected(3000))
		QMessageBox::warning(this, "连接失败", "前端未启动！");
	QEventLoop eventLoop{ this };
	QTimer::singleShot(config["wait_quest_time"].get<int>(), &eventLoop, &QEventLoop::quit);
	eventLoop.exec();
	
	questSocket.connectToHost("localhost", questPort);
}

void QuestClient::sendCommand(QString command) const
{
	questSocket.write(command.toLatin1());

	if (config["log_to_window"])
		ui.logBrowser->appendPlainText(QString("%1 [Client] %2")
			.arg(QTime::currentTime().toString("hh:mm:ss"))
			.arg(command));
	if (config["log_to_file"])
	{
		logFile.write(QString("%1 [Client] %2\n")
			.arg(QTime::currentTime().toString("hh:mm:ss"))
			.arg(command).toLatin1());
		logFile.flush();
	}
}

void QuestClient::sendSetCommand(QString name, QString attribute, QString value, bool isInstance) const
{
	sendCommand(QString{ "SET %1'%2%3' %4 TO %5" }.arg(isInstance ? "ELEMENT " : "", name, isInstance ? "_1" : "", attribute, value));
}

QString QuestClient::sendInquireCommand(QString name, QString attribute, bool isInstance) const
{
	sendCommand(QString{ "INQ %1'%2%3' %4" }.arg(isInstance ? "ELEMENT " : "", name, isInstance ? "_1" : "", attribute));
	questSocket.waitReceived();
	return currentReceivedMessage.section(QRegExp("[:;]"), 1, 1).trimmed();
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

void QuestClient::restoreNormalScene()
{
	if (craneFailure != 0 || luziFailure != 0)
	{
		craneFailure = 0;
		luziFailure = 0;
		ui.solutionCrane2Failure->setAutoExclusive(false);
		ui.solutionLuzi1Failure->setAutoExclusive(false);
		ui.solutionLuzi3Failure->setAutoExclusive(false);
		ui.solutionGYCMove->setAutoExclusive(false);
		ui.solutionCrane2Failure->setChecked(false);
		ui.solutionLuzi1Failure->setChecked(false);
		ui.solutionLuzi3Failure->setChecked(false);
		ui.solutionGYCMove->setChecked(false);
		ui.solutionCrane2Failure->setAutoExclusive(true);
		ui.solutionLuzi1Failure->setAutoExclusive(true);
		ui.solutionLuzi3Failure->setAutoExclusive(true);
		ui.solutionGYCMove->setAutoExclusive(true);

		auto reconnectElement = [=](QString fromClass, QString toClass, bool isConnect)
		{
			sendCommand(QString{ "DISCONNECT ELEMENT '%1_1' ALL INPUT" }.arg(toClass));
			if (isConnect)
				sendCommand(QString{ "CONNECT ELEMENT '%1_1' TO ELEMENT '%2_1'" }.arg(fromClass, toClass));
		};
		reconnectElement("Buffer_LK3_2", "Buffer_temp_LK3_2", true);
		reconnectElement("Buffer_LK3_2", "Buffer_temp_LK3_3", true);

		sendSetUserAttributeCommand("Source_time", "crane_failure", "0");
		sendSetUserAttributeCommand("Source_time", "luzi_failure", "0");

		unityServer.write(QString("crane_failure = 0").toLatin1());
		unityServer.write(QString("luzi_failure = 0").toLatin1());
	}
}
