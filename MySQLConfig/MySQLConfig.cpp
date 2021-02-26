#include "MySQLConfig.h"
#include "QString"
#include "QDialog"
#include "QDir"
#include "QFileDialog"
#include "QMessageBox"
#include "Utility.h"
#include "qinputdbpassword.h"
#include "QWaitCursor.h"
#include <QMenu>
QString g_strServerType[] = { QObject::tr("Master Server"),QObject::tr("Slave Server") };
QString g_strLogMode[] = { QObject::tr("FILE"),QObject::tr("TABLE"), QObject::tr("FILE,TABLE") };

MySQLConfig::MySQLConfig(QWidget *parent)
    : QDialog(parent)
{
    ui.setupUi(this);
	//setWindowIcon(QIcon(QString::fromUtf8(":/MySQLConfig/dialog-ok-2.ico")));
	IcoConnect[0].addFile(QString::fromUtf8(":/MySQLConfig/close.png"), QSize(), QIcon::Normal);	
	IcoConnect[1].addFile(QString::fromUtf8(":/MySQLConfig/open.png"), QSize(), QIcon::Normal);
	pWidgetPtrServer = new vector<QWidget*>(
		{
			ui.lineEdit_ServerID,
			ui.checkBox_Logerror,
			//ui.lineEdit_Logerror,		
			ui.comboBox_ServerType,
			ui.lineEdit_Logbin,
			//ui.comboBox_LogMode,
			ui.checkBox_LongQueryTime,
			//ui.lineEdit_LongQueryTime,
			ui.lineEdit_ServerPort,
			ui.checkBox_ExternAccess,
			//ui.lineEdit_AccessPassword,
			ui.checkBox_Generallog,
			//ui.lineEdit_Generallog,
			ui.checkBox_SlowQuerylog,
			//ui.lineEdit_SlowQuerylog,
			ui.checkBox_Expiredlogdays,
			//ui.lineEdit_Expiredlogdays,		
			
		});
	pWidgetPtrMaster = new vector<QWidget*>(
		{
			ui.comboBox_SourceDB,
			ui.comboBox_IgnoredDB,
			ui.lineEdit_ReplicationAccount,
			ui.lineEdit_ReplicationPassword,
			ui.lineEdit_LogbinFile,
			ui.lineEdit_LogbinPosition,
			ui.listWidget_SlaveHosts,
			//ui.pushButton_ApplySettings
		});
	pWidgetPtrSlave = new vector<QWidget*>(
		{
			//ui.lineEdit_MasterlogFile,
			//ui.lineEdit_MasterlogFilePos,
			ui.lineEdit_MasterHost,
			ui.lineEdit_MasterHost_Port,
			ui.lineEdit_MasterAccount,
			ui.lineEdit_MasterPassword,
			
			ui.lineEdit_ReplicationAccount_Slave,
			ui.lineEdit_ReplicationPassword_Slave,
			//ui.checkBox_IORunning,
			//ui.checkBox_SQLRunning,
			ui.lineEdit_Relaylog,
			ui.lineEdit_RelaylogIndex,
			//ui.pushButton_ApplySettings
		});
	int nItemCount = ui.comboBox_LogMode->count();
	for (int nIndex = 1; nIndex < nItemCount; nIndex++)
	{
		ui.comboBox_LogMode->setItemData(nIndex, QVariant(0), Qt::UserRole - 1);
		ui.comboBox_LogMode->setItemData(nIndex, QBrush(QColor(192, 192, 192)), Qt::BackgroundRole);
		ui.comboBox_LogMode->setItemData(nIndex, QBrush(QColor(Qt::black)), Qt::ForegroundRole);
	}
	//connect(ui.comboBox_SourceDB, SIGNAL(hidingPopup()), this, SLOT(OnComboboxSourceDBHidePopup()));
	// connect(ui.pushButtonStart, SIGNAL(clicked()), this, SLOT(OnStart()));
}
void MySQLConfig::OnStart()
{
	QWaitCursor Wait;
	std::string strService = "MySQL.T";
	CWinService Service;
	SERVICE_STATUS_PROCESS ssStatus;
	char *szServiceStatus[] = 	{
		"",
		"Stopped ",
		"Start_Pending",
		"Stop_Pending",
		"Running",
		"Continue_Pending",
		"Pause_Pending",
		"Paused"
	};
	int nResult = Service.Stop(strService.c_str(), ssStatus);
	qDebug("Stop Result = %d\tServiceStatus = %s.\n", nResult, szServiceStatus[ssStatus.dwCurrentState]);
	nResult = Service.Start(strService.c_str(), ssStatus);	
	qDebug("Start Result = %d\tServiceStatus = %s.\n", nResult, szServiceStatus[ssStatus.dwCurrentState]);
}
void MySQLConfig::OnComboboxSourceDBHidePopup()
{
	//QStandardItemModel* pModel = (QStandardItemModel *)ui.comboBox_IgnoredDB->model();
	//QStandardItem* pItem = pModel->item(2);
	//pItem->setFlags(0);
	//auto& Index = pModel->index(2, 0);
	//	
	//pModel->setData(Index, QVariant(0), Qt::UserRole - 1);
	//pModel->setData(Index, QBrush(QColor(192, 192, 192)), Qt::BackgroundRole);
	//pModel->setData(Index, QBrush(QColor(Qt::black)), Qt::ForegroundRole);
}
MySQLConfig::~MySQLConfig()
{
	if (pMySQLIni)
		delete pMySQLIni;

	if (pMySQLSettings)
		delete pMySQLSettings;

	if (pWidgetPtrServer)
		delete pWidgetPtrServer;

	if (pWidgetPtrMaster)
		delete pWidgetPtrMaster;

	if (pWidgetPtrSlave)
		delete pWidgetPtrSlave;
}
bool MySQLConfig::GetMySQLService(std::string strMySQL_Path, std::string& strMySQLService, ServiceStatus &nServiceStatus)
{
	CWinService ServiceMgr;
	ServiceInformationArray SvrConfigArray;
	ServiceMgr.GetAllServiceInformation(SvrConfigArray);
	
	QString strMySQLD_Path = QString("%1\\bin\\Mysqld.exe").arg(strMySQL_Path.c_str());
	
	int nIndex = 0;
	auto itFind = find_if(SvrConfigArray.begin(), SvrConfigArray.end(), [strMySQLD_Path,nIndex](ServiceInformationPtr p) mutable
		{
			QString strBinPath = p->ServiceConfig.lpBinaryPathName;
			return strBinPath.contains(strMySQLD_Path, Qt::CaseInsensitive);
		});
	// if the MySQL Service is installed and it's Prescess path equal to m_strMySQLPath,and it is running ,then stop it !
	if (itFind != SvrConfigArray.end())
	{
		strMySQLService = (*itFind)->EnumServiceStatus.lpServiceName;
		nServiceStatus = (ServiceStatus)(*itFind)->EnumServiceStatus.ServiceStatus.dwCurrentState;
		return true;
	}
	else
		return false;
}

void MySQLConfig::on_pushButton_Browse_clicked()
{
    QDir dir;
    QString strPath = QFileDialog::getExistingDirectory(this, tr("Select the Directory of MySQL"), "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    strPath.replace("/", "\\");
  
    QString strINI = QString("%1\\My.ini").arg(strPath);
    if (!QFileInfo::exists(strINI))
    {
        QMessageBox::critical(nullptr, tr("Error"), tr("Can't find My.ini in the selected directory!"), QMessageBox::Abort);
        return;
    }
	strMySQLPath = strPath;
    ui.lineEdit_InstalledPath->setText(strPath);
   	
	/*QWaitCursor Wait;*/
	LoadSettings(strPath);
	ui.pushButton_Connect->setEnabled(true);
}

bool MySQLConfig::TestMySQLService(QString& strService, ServiceStatus& nServiceStatus)
{
	unsigned short nMySQLPort = ui.lineEdit_ServerPort->text().toShort();
	strMySQLPath = ui.lineEdit_InstalledPath->text();
	std::string strMySQLService;
	QString strMessage;
	bool bFoundMySQLService = GetMySQLService(strMySQLPath.toStdString(), strMySQLService, nServiceStatus);
	if (!bFoundMySQLService)
	{
		QMessageBox::critical(nullptr, tr("Error"), tr("MySQL Service is not installed,Please install it!"), QMessageBox::Ok);
		return false;
	}

	QTcpSocket TcpClient;
	TcpClient.connectToHost("127.0.0.1", nMySQLPort);

	if (!TcpClient.waitForConnected(100))
	{// the MySql Service not start
		CWinService Service;
		SERVICE_STATUS_PROCESS ssStatus;
		if (!Service.Start(strMySQLService.c_str(), ssStatus) ||
			ssStatus.dwCurrentState == Running)
		{
			strMessage = QString(tr("Failed in starting %1 Service,Please check the service setting!")).arg(strMySQLService.c_str());
			QMessageBox::critical(nullptr, tr("Error"), strMessage, QMessageBox::Ok);
			return false;
		}
	}
	strService = strMySQLService.c_str();
	return true;
}
void MySQLConfig::on_pushButton_Connect_clicked()
{
	try
	{
		QString strService;
		ServiceStatus nServiceStatus;

		if (!TestMySQLService(strService, nServiceStatus))
			return;
		unsigned short nMySQLPort = ui.lineEdit_ServerPort->text().toShort();
		strMySQLPath = ui.lineEdit_InstalledPath->text();
		CMySQLAgent DBConnector;
		
		QInputDBPassword InpudDlg(strMySQLPath, nMySQLPort, this);
		if (InpudDlg.exec() == QDialog::Accepted)
		{
			QWaitCursor Wait;
			strDBPassword = InpudDlg.GetPassword();
			int nResult = DBConnector.Connect("127.0.0.1", "root", strDBPassword.toStdString().c_str());	// Access denied,may Account or password error
			if (!nResult)
			{
				ui.pushButton_Connect->setIcon(IcoConnect[1]);
				for each (auto var in *pWidgetPtrServer)
					var->setEnabled(true);

				// get some information of mysql;
				CMyResult res = DBConnector.Query("show databases");
				if (res.RowCount())
				{
					ui.comboBox_SourceDB->clear();
					ui.comboBox_IgnoredDB->clear();
					do
					{
						char* pDatabase = res["Database"];
						ui.comboBox_SourceDB->AddItem(pDatabase);
						ui.comboBox_IgnoredDB->AddItem(pDatabase);
					} while (++res);
				}
				res = DBConnector.Query("show variables like 'server_id'");
				if (res.RowCount())
				{
					char* pVarName = res["Variable_name"];
					if (strcmp(pVarName, "server_id") != 0)
					{
						// set global server_id=2;
						char* pServerID = res["Value"];
						if (pServerID)
							ui.lineEdit_ServerID->setText(pServerID);
					}
				}
				res = DBConnector.Query("show master status");
				if (res.RowCount())
				{
					char* pLogbin = res["File"];
					int	  nLogPos = res["Position"];
					char* pDBList = res["Binlog_Do_DB"];
					char* pIgnoreDBList = res["Binlog_Ignore_DB"];
					ui.lineEdit_LogbinFile->setText(pLogbin);
					ui.lineEdit_LogbinPosition->setText(QString("%1").arg(nLogPos));

					if (strlen(pDBList))
					{
						QStringList Dblist = QString(pDBList).split(',');
						for each (auto var in Dblist)
							ui.comboBox_SourceDB->SetItemCheck(var);
					}

					if (strlen(pIgnoreDBList))
					{
						QStringList IgnoreDBList = QString(pIgnoreDBList).split(',');
						for each (auto var in IgnoreDBList)
							ui.comboBox_SourceDB->SetItemCheck(var);
					}
				}
				res = DBConnector.Query("show slave status");
				if (res.RowCount())
				{
					char* pMasterHost = res["Master_Host"];
					char* pMasterUser = res["Master_User"];
					char* pMasterPort = res["Master_Port"];
					char* pMasterlogFile = res["Master_Log_File"];
					char* pReadMasterPos = res["Read_Master_Log_Pos"];
					char* pRelaylogFile = res["Relay_Log_File"];
					//int		nRelaylogPos	 = res["Relay_log_Pos"];
					char* pRelayMasterLogFile = res["Relay_Master_Log_File"];
					char* pSlaveIORunning = res["Slave_IO_Running"];
					char* pSlaveSQLRunning = res["Slave_SQL_Running"];
					ui.lineEdit_Relaylog->setText(pMasterHost);
					if (pMasterHost)
						ui.lineEdit_MasterHost->setText(pMasterHost);
					if (pMasterPort)
						ui.lineEdit_MasterHost_Port->setText(pMasterPort);
					if (pMasterUser)
						ui.lineEdit_ReplicationAccount_Slave->setText(pMasterUser);

	/*				if (pMasterlogFile)
						ui.lineEdit_MasterlogFile->setText(pMasterlogFile);

					if (pReadMasterPos)
						ui.lineEdit_MasterlogFilePos->setText(pReadMasterPos);*/

					if (pRelaylogFile)
						ui.lineEdit_Relaylog->setText(pRelaylogFile);
				}
				res = DBConnector.Query("show variables like '%%relay_log_index%%'");
				if (res.RowCount())
				{
					char* VarName = res["Variable_name"];
					char* pRelaylogIndex = res["Value"];
					if (strcmp(VarName,"relay_log_index") != 0 && pRelaylogIndex)
					{
						ui.lineEdit_RelaylogIndex->setText(pRelaylogIndex);
					}
				}
			}
			else
			{
				ui.pushButton_Connect->setIcon(IcoConnect[0]);
				QString strError = DBConnector.GetErrorMsg();
				QMessageBox::critical(nullptr, tr("Error"), strError, QMessageBox::Abort);
				return;
			}
		}

	}
	catch (CMySQLException& e)
	{
		QMessageBox::critical(nullptr, tr("Error"), e.what(), QMessageBox::Abort);
	}
	catch (std::exception& e)
	{
		QMessageBox::critical(nullptr, tr("Error"), e.what(), QMessageBox::Abort);
	}
}

bool MySQLConfig::LoadSettings(QString strMySQLPath)
{
	QSettings MySQLSettings(QString("%1\\My.ini").arg(strMySQLPath), QSettings::IniFormat);
	
	ui.lineEdit_Logbin->setEnabled(false);
	ui.lineEdit_Generallog->setEnabled(false);
	ui.checkBox_Generallog->setChecked(false);
	ui.checkBox_Logerror->setChecked(false);
	ui.lineEdit_Logerror->setEnabled(false);
	ui.lineEdit_SlowQuerylog->setEnabled(false);
	ui.checkBox_SlowQuerylog->setChecked(false);
	ui.lineEdit_Expiredlogdays->setEnabled(false);
	ui.checkBox_Expiredlogdays->setChecked(false);

	MySQLSettings.beginGroup("mysqld");
	QStringList keyList = MySQLSettings.allKeys();
	foreach (QString strKey,keyList)
	{
		TraceMsgA("Key = %s\tValue = %s\n", strKey.toStdString().c_str(), MySQLSettings.value(strKey).toString().toStdString().c_str());
		QVariant varValue;
		if (strKey =="server-id")
		{
			varValue = MySQLSettings.value(strKey);
			ui.lineEdit_ServerID->setText(varValue.toString());
		}

		varValue = MySQLSettings.value("port");
		ui.lineEdit_ServerPort->setText(varValue.toString());

		if (strKey =="log-output")
		{
			varValue = MySQLSettings.value(strKey);
			int nLogmode = ui.comboBox_LogMode->findText(varValue.toString());
			ui.comboBox_LogMode->setCurrentIndex(nLogmode);
		}
		//ui.comboBox_LogMode->setEnabled(false);
		if (strKey =="log-bin")
		{
			varValue = MySQLSettings.value(strKey);
			ui.lineEdit_Logbin->setText(varValue.toString());			
			ui.lineEdit_Logbin->setEnabled(true);
		}

		if (strKey =="general-log")
		{
			varValue = MySQLSettings.value(strKey);
			bool bGeneralLog = varValue.toBool();
			ui.lineEdit_Generallog->setEnabled(bGeneralLog);
			ui.checkBox_Generallog->setChecked(bGeneralLog);
		}
		

		if (strKey =="general_log_file")
		{
			varValue = MySQLSettings.value(strKey);
			ui.lineEdit_Generallog->setText(varValue.toString());
		}

		if (strKey =="log-error")
		{
			varValue = MySQLSettings.value(strKey);
			ui.lineEdit_Logerror->setText(varValue.toString());
			ui.checkBox_Logerror->setChecked(true);
			ui.lineEdit_Logerror->setEnabled(true);
		}
		
		if (strKey =="long_query_time")
		{
			ui.checkBox_LongQueryTime->setChecked(true);
			ui.lineEdit_LongQueryTime->setEnabled(true);
			varValue = MySQLSettings.value(strKey);
			ui.lineEdit_LongQueryTime->setText(varValue.toString());
		}
		
		if (strKey =="slow-query-log")
		{
			varValue = MySQLSettings.value(strKey);
			bool bSlowQueylog = varValue.toBool();
			ui.lineEdit_SlowQuerylog->setEnabled(bSlowQueylog);
			ui.checkBox_SlowQuerylog->setChecked(bSlowQueylog);
		}
		
		if (strKey =="slow_query_log_file")
		{
			varValue = MySQLSettings.value(strKey);
			ui.lineEdit_SlowQuerylog->setText(varValue.toString());
		}


		if (strKey =="expire-logs-days")
		{
			varValue = MySQLSettings.value(strKey);
			ui.lineEdit_Expiredlogdays->setText(varValue.toString());
			ui.lineEdit_Expiredlogdays->setEnabled(true);
			ui.checkBox_Expiredlogdays->setChecked(true);
		}

		if (strKey == "relay-log")
		{
			varValue = MySQLSettings.value(strKey);
			ui.lineEdit_Relaylog->setText(varValue.toString());
		}

		if (strKey == "relay-log-index")
		{
			varValue = MySQLSettings.value(strKey);
			ui.lineEdit_RelaylogIndex->setText(varValue.toString());
		}
	}
	MySQLSettings.endGroup();
	return true;
}

bool MySQLConfig::SaveSettings(QString strInstalledPath,QString &strMessage)
{
	QString strIniFile = QString("%1\\My.ini").arg(strInstalledPath);
	if (pMySQLIni)
		delete pMySQLIni;
	pMySQLIni = new CIniFile(strIniFile.toStdString());
	if (!pMySQLIni)
	{
		strMessage = QString(tr("Failed to Load file '%s'.")).arg(strIniFile);
		return false;
	}

	pMySQLIni->EnterSection("mysqld");
	pMySQLIni->WriteKey( "port", nServerPort);
	pMySQLIni->WriteKey( "server-id", nServerID);
	if (strLogbinFile.size())
		pMySQLIni->WriteKey("log-bin", strLogbinFile.toStdString());
	else
		pMySQLIni->EraseKey("log-bin");

	if (bChecklogError)
		pMySQLIni->WriteKey("log-error", strLogError.toStdString());

	pMySQLIni->WriteKey("log-output", strLogMode.toStdString());	
	pMySQLIni->WriteKey("general-log", bCheckGenerallog ? 1 : 0);
	if (strGeneralLog.size())
		pMySQLIni->WriteKey("general_log_file", strGeneralLog.toStdString());
	else
		pMySQLIni->EraseKey("general_log_file");

	pMySQLIni->WriteKey("slow-query-log", bCheckSlowQuerylog ? 1 : 0);
	if (strSlowQuerylog.size())
		pMySQLIni->WriteKey("slow_query_log_file", strSlowQuerylog.toStdString());
	else
		pMySQLIni->EraseKey("slow_query_log_file");

	if (bChecklongQueryTime)
		pMySQLIni->WriteKey("long_query_time", nLongQueryTime);
	else
		pMySQLIni->EraseKey("long_query_time");

	if (bCheckExpiredlogDays)
		pMySQLIni->WriteKey("expire-logs-days", nExpiredlogDays);
	else
		pMySQLIni->EraseKey("expire-logs-days");
	
	if (nServerType == Server_Master)
	{
		pMySQLIni->WriteKey("binlog-do-db", strSourceDB.toStdString());
		if (strIgorenDB.size())
			pMySQLIni->WriteKey("binlog-ignore-db", strIgorenDB.toStdString());
	}
	else if (nServerType == Server_Slave)
	{
		pMySQLIni->WriteKey("relay-log-index", strRelayLogIndex.toStdString());
		pMySQLIni->WriteKey("relay-log", strRelayLog.toStdString());
	}
	pMySQLIni->LeaveSection();
	return true;
}

void MySQLConfig::SetDefaultMaster()
{
	if (ui.lineEdit_Logbin->text().size() < 1)
		ui.lineEdit_Logbin->setText("Master_bin");	
	
	ui.checkBox_Generallog->setChecked(false);
	if (!ui.lineEdit_Generallog->text().size())
		ui.lineEdit_Generallog->setText("General.log");

	ui.checkBox_Logerror->setChecked(true);	
	if (!ui.lineEdit_Logerror->text().size())
		ui.lineEdit_Logerror->setText("Master_Error.log");

	ui.checkBox_LongQueryTime->setChecked(true);
	ui.lineEdit_LongQueryTime->setText("30");

	ui.checkBox_SlowQuerylog->setChecked(true);
	if (!ui.lineEdit_SlowQuerylog->text().size())
		ui.lineEdit_SlowQuerylog->setText("SlowQuery.log");

	ui.checkBox_Expiredlogdays->setChecked(true);
	if (!ui.lineEdit_Expiredlogdays->text().size())
		ui.lineEdit_Expiredlogdays->setText("30");

	ui.checkBox_ExternAccess->setChecked(true);

	ui.lineEdit_AccessPassword->setText("Mago&Zpmc@2020");

	ui.lineEdit_ReplicationAccount->setText("SlaveHost");

	ui.lineEdit_ReplicationPassword->setText("Mago&Zpmc@2020");
}

void MySQLConfig::SetDefaultSlave()
{
	if (ui.lineEdit_Logbin->text().size() < 1)
		ui.lineEdit_Logbin->setText("Slave_bin");
	
	ui.checkBox_Generallog->setChecked(false);
	if (!ui.lineEdit_Generallog->text().size())
		ui.lineEdit_Generallog->setText("General.log");

	ui.checkBox_Logerror->setChecked(true);
	if (ui.lineEdit_Logerror->text().size() < 1)
		ui.lineEdit_Logerror->setText("Slave_Error.log");

	ui.checkBox_LongQueryTime->setChecked(true);
	ui.lineEdit_LongQueryTime->setText("30");

	ui.checkBox_SlowQuerylog->setChecked(true);
	if (ui.lineEdit_SlowQuerylog->text().size() < 1)
		ui.lineEdit_SlowQuerylog->setText("SlowQuery.log");

	ui.checkBox_Expiredlogdays->setChecked(true);

	if (ui.lineEdit_Expiredlogdays->text().size() < 1)
		ui.lineEdit_Expiredlogdays->setText("30");

	ui.checkBox_ExternAccess->setChecked(true);

	ui.lineEdit_AccessPassword->setText("Mago&Zpmc@2020");

	ui.lineEdit_MasterAccount->setText("root");
	if (ui.lineEdit_Relaylog->text().size() < 1)
		ui.lineEdit_Relaylog->setText("Relay_log.bin");

	if (ui.lineEdit_RelaylogIndex->text().size() < 1)
	ui.lineEdit_RelaylogIndex->setText("Relay_log_Index.bin");

	ui.lineEdit_ReplicationAccount_Slave->setText("SlaveHost");

	ui.lineEdit_ReplicationPassword_Slave->setText("Mago&Zpmc@2020");

	ui.lineEdit_MasterHost_Port->setText("3306");
}

void MySQLConfig::on_comboBox_ServerType_currentIndexChanged(int index)
{
	nServerType = (ServerType)index;
	ui.comboBox_LogMode->setCurrentIndex(0);
	if (nServerType == Server_Master)
	{
		for each (auto var in *pWidgetPtrMaster)
			var->setEnabled(true);

		for each (auto var in *pWidgetPtrSlave)
			var->setEnabled(false);
		ui.pushButton_ApplySettings->setEnabled(true);
		SetDefaultMaster();
		ui.tabWidget->setCurrentIndex(0);
	}
	else if (nServerType == Server_Slave)
	{
		for each (auto var in *pWidgetPtrMaster)
			var->setEnabled(false);

		for each (auto var in *pWidgetPtrSlave)
			var->setEnabled(true);
		ui.pushButton_ApplySettings->setEnabled(true);
		SetDefaultSlave();
		ui.tabWidget->setCurrentIndex(1);
	}
}

bool MySQLConfig::ConfigureMaster()
{
	QString strMessage;
	strSourceDB = ui.comboBox_SourceDB->getText();
	if (!strSourceDB.size())
	{
		QMessageBox::information(nullptr, tr("Information"), tr("Please select source database for Master Server!"), QMessageBox::Ok);
		ui.comboBox_SourceDB->setFocus();
		return false;
	}
	QStringList strSourceDBList = strSourceDB.split(",");

	strIgorenDB = ui.comboBox_IgnoredDB->getText();
	if (strIgorenDB.size())
	{
		QStringList strIgorenDBList = strIgorenDB.split(",");
		for each (auto var in strIgorenDBList)
		{
			QStringList Result = strSourceDBList.filter(var, Qt::CaseInsensitive);
			if (Result.size())
			{
				QMessageBox::information(nullptr, tr("Information"), tr("There is some conficts between 'Source Database' and 'Ignored Database',please check them!"), QMessageBox::Abort);
				return false;
			}
		}
	}
	strReplicationAccount = ui.lineEdit_ReplicationAccount->text();
	if (!strReplicationAccount.size())
	{
		QMessageBox::information(nullptr, tr("Information"), tr("Please Replication Account for Master Server!"), QMessageBox::Ok);
		ui.lineEdit_ReplicationAccount->setFocus();
		return false;
	}
	strReplicationPassword = ui.lineEdit_ReplicationPassword->text();
	if (!strReplicationPassword.size())
	{
		QMessageBox::information(nullptr, tr("Information"), tr("Please Replication Password for Master Server!"), QMessageBox::Ok);
		ui.lineEdit_ReplicationPassword->setFocus();
		return false;
	}
	int nCount = ui.listWidget_SlaveHosts->count();
	if (!nCount)
	{
		QMessageBox::information(nullptr, tr("Information"), tr("Please Add some slave hosts in Slave Host list!"), QMessageBox::Ok);
		ui.listWidget_SlaveHosts->setFocus();
		return false;
	}

	strSlaveHostList.clear();
	for (int nRow = 0; nRow < nCount; nRow++)
	{
		QString strItem = ui.listWidget_SlaveHosts->item(nRow)->text();
		if (!strItem.size() || !IsValidIPAddressA(strItem.toStdString().c_str()))
		{
			strMessage = QString(tr("The %d item in Slave Hosts list is invalid,Please input a Host IP Address as '172.16.20.100'!")).arg(nRow + 1);
			QMessageBox::information(nullptr, tr("Information"), strMessage, QMessageBox::Ok);
			ui.listWidget_SlaveHosts->setFocus();
			return false;
		}
		strSlaveHostList.push_back(strItem);
	}
	if (!strSlaveHostList.size())
	{
		QMessageBox::information(nullptr, tr("Information"), tr("Please Add some slave hosts in Slave Host list!"), QMessageBox::Ok);
		ui.listWidget_SlaveHosts->setFocus();
		return false;
	}
	SaveSettings(strInstalledPath, strMessage);
	QString strMySQLService;
	ServiceStatus nServiceStatus;
	QWaitCursor Wait;
	TestMySQLService(strMySQLService, nServiceStatus);
	
	CWinService Service;
	SERVICE_STATUS_PROCESS ssStatus;
	if (strMySQLService.size() )
	{
		int nResult = 0;
		if (nServiceStatus == Running)
		{
			nResult = Service.Restart(strMySQLService.toStdString().c_str(), ssStatus);
		}
		else
		{
			nResult = Service.Start(strMySQLService.toStdString().c_str(), ssStatus);
		}
		char szError[1024] = { 0 };
		if (nResult || ssStatus.dwCurrentState != Running)
		{
			FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				nResult,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPSTR)szError,
				1024,
				NULL);
			strMessage = QString(tr("Failed in restarting Service '%1':%2")).arg(strMySQLService).arg(szError);
			Wait.Restore();
			QMessageBox::critical(nullptr, tr("Information"), strMessage, QMessageBox::Abort);
			return false;
		}
	}
	
	try
	{
		CMySQLAgent DBConnector;
		QString strSQL;
		
		int nResult = DBConnector.Connect("127.0.0.1", "root", strDBPassword.toStdString().c_str());
		if (nResult)	// return a non zero value indicated a failure!
		{
			strMessage = DBConnector.GetErrorMsg();
			QMessageBox::information(nullptr, tr("Information"), strMessage, QMessageBox::Ok);
			return false;
		}
		if (bCheckExternAccess)
		{
			strSQL = QString("GRANT ALL PRIVILEGES ON *.* TO 'root'@'%' IDENTIFIED BY 'Mago&Zpmc@2020' WITH GRANT OPTION").arg(strDBPassword);

			DBConnector.ExecuteSQLString(strSQL.toStdString().c_str());
			DBConnector.ExecuteSQLString("flush privileges");
		}
		// grant all slave host replication privileges
		for each (auto strHost in strSlaveHostList)
		{
			strSQL = QString("GRANT REPLICATION SLAVE ON *.* to '%1'@'%2' IDENTIFIED by '%3'").arg(strReplicationAccount).arg(strHost).arg(strReplicationPassword);
			DBConnector.ExecuteSQLString(strSQL.toStdString().c_str());
			DBConnector.ExecuteSQLString("flush privileges");
		}

		CMyResult res = DBConnector.Query("show master status");
		if (res.RowCount())
		{
			char* pLogbin = res["File"];
			int	  nLogPos = res["Position"];
			char* pDBList = res["Binlog_Do_DB"];
			char* pIgnoreDBList = res["Binlog_Ignore_DB"];
			ui.lineEdit_LogbinFile->setText(pLogbin);
			ui.lineEdit_LogbinPosition->setText(QString("%1").arg(nLogPos));

			if (strlen(pDBList))
			{
				QStringList Dblist = QString(pDBList).split(',');
				for each (auto var in Dblist)
					ui.comboBox_SourceDB->SetItemCheck(var);
			}

			if (strlen(pIgnoreDBList))
			{
				QStringList IgnoreDBList = QString(pIgnoreDBList).split(',');
				for each (auto var in IgnoreDBList)
					ui.comboBox_SourceDB->SetItemCheck(var);
			}
		}
		return true;
	}
	catch (std::exception& e)
	{
		QMessageBox::information(nullptr, tr("Exception"), e.what(), QMessageBox::Ok);
		return false;
	}
}

bool MySQLConfig::ConfigureSlave()
{
	QString strMessage;
	strMasterHost = ui.lineEdit_MasterHost->getIP();
	if (!IsValidIPAddressA(strMasterHost.toStdString().c_str()))
	{
		QMessageBox::information(nullptr, tr("Information"), tr("Please Input a valid IP Address for Master Server Host!"), QMessageBox::Ok);
		ui.lineEdit_MasterHost->setFocus();
		return false;
	}

	nMasterPort = ui.lineEdit_MasterHost_Port->text().toInt();
	if (nMasterPort <= 0 || nMasterPort >= 65535)
	{
		QMessageBox::information(nullptr, tr("Information"), tr("The Master Server Port must be a value between 1~65535,please input a valid value!"), QMessageBox::Ok);
		ui.lineEdit_MasterHost->setFocus();
		return false;
	}

	strMasterAccount = ui.lineEdit_MasterAccount->text();
	if (!strMasterAccount.size())
	{
		QMessageBox::information(nullptr, tr("Information"), tr("Please Input Master Account for Slave Server!"), QMessageBox::Ok);
		ui.lineEdit_MasterAccount->setFocus();
		return false;
	}

	strMasterPassword = ui.lineEdit_MasterPassword->text();
	if (!strMasterPassword.size())
	{
		QMessageBox::information(nullptr, tr("Information"), tr("Please Input Master Password for Slave Server!"), QMessageBox::Ok);
		ui.lineEdit_MasterPassword->setFocus();
		return false;
	}

	strReplicationAccount = ui.lineEdit_ReplicationAccount_Slave->text();
	if (!strReplicationAccount.size())
	{
		QMessageBox::information(nullptr, tr("Information"), tr("Please Input Replication Account for Slave Server!"), QMessageBox::Ok);
		ui.lineEdit_ReplicationAccount_Slave->setFocus();
		return false;
	}

	strReplicationPassword = ui.lineEdit_ReplicationPassword_Slave->text();
	if (!strReplicationAccount.size())
	{
		QMessageBox::information(nullptr, tr("Information"), tr("Please Input Replication Password for Slave Server!"), QMessageBox::Ok);
		ui.lineEdit_ReplicationPassword_Slave->setFocus();
		return false;
	}

	/*strMasterLogFile = ui.lineEdit_MasterlogFile->text();
	if (!strMasterLogFile.size())
	{
		QMessageBox::information(nullptr, tr("Information"), tr("Please Input Master log File for Slave Server!"), QMessageBox::Ok);
		ui.lineEdit_MasterlogFile->setFocus();
		return false;
	}*/

	/*nMasterlogPos = ui.label_MasterlogFilePos->text().toInt();
	if (!nMasterlogPos)
	{
		QMessageBox::information(nullptr, tr("Information"), tr("Please Input Master log File Pos for Slave Server!"), QMessageBox::Ok);
		ui.label_MasterlogFilePos->setFocus();
		return false;
	}*/

	/*if ((!strMasterPassword.size() || strMasterPassword.size()) &&
		(strMasterLogFile.size() || !nMasterlogPos))
	{
		QMessageBox::information(nullptr, tr("Information"), tr("You have neither input the account and password for the Master server nor input the Master log file and Master log Pos,the slave server can't be configuration cannot continue!"), QMessageBox::Ok);
		return false;
	}*/
	strRelayLog = ui.lineEdit_Relaylog->text();
	if (!strRelayLog.size())
	{
		QMessageBox::information(nullptr, tr("Information"), tr("Please Input Relay log for Slave Server!"), QMessageBox::Ok);
		ui.lineEdit_Relaylog->setFocus();
		return false;
	}
	QString strRelayLogIndex = ui.label_RelaylogIndex->text();
	if (!strRelayLogIndex.size())
	{
		QMessageBox::information(nullptr, tr("Information"), tr("Please Input Relay log Index for Slave Server!"), QMessageBox::Ok);
		ui.label_RelaylogIndex->setFocus();
		return false;
	}

	SaveSettings(strInstalledPath, strMessage);
	QString strMySQLService;
	ServiceStatus nServiceStatus;
	QWaitCursor Wait;
	TestMySQLService(strMySQLService, nServiceStatus);

	CWinService Service;
	SERVICE_STATUS_PROCESS ssStatus;
	if (strMySQLService.size())
	{
		int nResult = 0;
		if (nServiceStatus == Running)
		{
			nResult = Service.Restart(strMySQLService.toStdString().c_str(), ssStatus);
		}
		else
		{
			nResult = Service.Start(strMySQLService.toStdString().c_str(), ssStatus);
		}
		char szError[1024] = { 0 };
		if (nResult || ssStatus.dwCurrentState != Running)
		{
			FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				nResult,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
				(LPSTR)szError,
				1024,
				NULL);
			strMessage = QString(tr("Failed in restarting Service '%1':%2")).arg(strMySQLService).arg(szError);
			Wait.Restore();
			QMessageBox::critical(nullptr, tr("Information"), strMessage, QMessageBox::Abort);
			return false;
		}
	}
	try
	{
		CMySQLAgent MasterServer;
		CMySQLAgent DBConnector;
		QString strSQL;
		int nResult = 0;
		CMyResult res;
		// the user has input account and password for master server ,then get the master log file and log pos by itsself
		
		nResult = MasterServer.Connect(strMasterHost.toStdString().c_str(),
			strMasterAccount.toStdString().c_str(),
			strMasterPassword.toStdString().c_str(),
			nullptr,
			nMasterPort);
		if (nResult)	// return a non zero value indicated a failure!
		{
			QString strError = DBConnector.GetErrorMsg();
			strMessage = QString("Failed in connect to Master Server %1@%2:%3(%4)").arg(strMasterAccount).arg(strMasterHost).arg(nMasterPort).arg(strError);
			QMessageBox::information(nullptr, tr("Information"), strMessage, QMessageBox::Ok);
			return false;
		}
		res = MasterServer.Query("show master status");
		if (res.RowCount())
		{
			strMasterLogFile = (char*)res["File"];
			ui.lineEdit_MasterlogFile->setText(strMasterLogFile);
			nMasterlogPos = res["Position"];
			ui.lineEdit_MasterlogFilePos->setText(QString("%1").arg(nMasterlogPos));
		}
		
		nResult = DBConnector.Connect("127.0.0.1",
			"root",
			strDBPassword.toStdString().c_str(),
			nullptr,
			nServerPort);
		if (nResult)	// return a non zero value indicated a failure!
		{
			QString strError = DBConnector.GetErrorMsg();
			strMessage = QString("Failed in connect to Server %1@127.0.0.1:%2(%3)").arg(strMasterAccount).arg(nMasterPort).arg(strError);
			QMessageBox::information(nullptr, tr("Information"), strMessage, QMessageBox::Ok);
			return false;
		}
		if (bCheckExternAccess)
		{
			strSQL = QString("GRANT ALL PRIVILEGES ON *.* TO 'root'@'%' IDENTIFIED BY '%1' WITH GRANT OPTION;").arg(strDBPassword);
			DBConnector.ExecuteSQLString(strSQL.toStdString().c_str());
			DBConnector.ExecuteSQLString("flush privileges");
		}
		/*
		stop slave;
		change master to master_host='192.168.58.138',master_port,master_user='SlaveHost',master_password='Mago&Zpmc@2020', master_log_file='Master-bin.000004',master_log_pos=120;
		Start slave;
		*/

		strSQL = QString("change master to master_host='%1',"
			"master_port=%2,"
			"master_user='%3',"
			"master_password='%4', "
			"master_log_file='%5',"
			"master_log_pos=%6")
			.arg(strMasterHost)
			.arg(nMasterPort)
			.arg(strReplicationAccount)
			.arg(strReplicationPassword)
			.arg(strMasterLogFile)
			.arg(nMasterlogPos);
		DBConnector.ExecuteSQLString("stop slave");
		DBConnector.ExecuteSQLString(strSQL.toStdString().c_str());
		DBConnector.ExecuteSQLString("start slave");

		res = DBConnector.Query("show slave status");
		if (res.RowCount())
		{
			char* pSlaveIORunning = res["Slave_IO_Running"];
			if (pSlaveIORunning)
				ui.lineEdit_SlaveIO_Running->setText(pSlaveIORunning);
			//ui.checkBox_IORunning->setChecked(strcmp(pSlaveIORunning, "Yes") == 0);

			char* pSlaveSQLRunning = res["Slave_SQL_Running"];
			//ui.checkBox_SQLRunning->setChecked(strcmp(pSlaveSQLRunning, "Yes") == 0);
			if (pSlaveSQLRunning)
				ui.lineEdit_SlaveSQL_Running->setText(pSlaveSQLRunning);

		}
		return true;
	}
	catch (std::exception& e)
	{
		QMessageBox::information(nullptr, tr("Exception"), e.what(), QMessageBox::Ok);
		return false;
	}
}

void MySQLConfig::on_pushButton_ApplySettings_clicked()
{
	if (QMessageBox::warning(nullptr,
		tr("Warning"),
		tr("MySQL Service will be restarted while applying settings,Press yes to to continue!"),
		QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
		return;
	
	if (nServerType == Server_Unknown)
	{
		QMessageBox::information(nullptr, tr("Information"), tr("Plz select a Servertype to continue! "), QMessageBox::Ok);
		ui.comboBox_ServerType->setFocus();
		return;
	}
	strInstalledPath = ui.lineEdit_InstalledPath->text();
	strInstalledPath.replace("/", "\\");

	QString strINI = QString("%1\\My.ini").arg(strInstalledPath);
	QFileInfo fi(strINI);
	if (!fi.exists(strINI) || fi.isDir())
	{
		QMessageBox::information(nullptr, tr("Information"), tr("Can't find My.ini in the selected directory!"), QMessageBox::Ok);
		return ;
	}

	nServerID = ui.lineEdit_ServerID->text().toInt();
	
	if (nServerID < 1)
	{
		QMessageBox::information(nullptr, tr("Information"), tr("The Server ID must be a value above Zero,please input a valid value!"), QMessageBox::Ok);
		return ;
	}
	nServerPort = ui.lineEdit_ServerPort->text().toInt();
	if (nServerPort <= 0 || nServerPort >= 65535)
	{
		QMessageBox::information(nullptr, tr("Information"), tr("The Server Port must be a value between 1~65535,please input a valid value!"), QMessageBox::Ok);
		return ;
	}
	if (ui.comboBox_LogMode->currentIndex() != 0)
	{
		QMessageBox::information(nullptr, tr("Information"), tr("The log output only support FILE mode!"), QMessageBox::Ok);
		return;
	}
	QString strMessage = "";
	QTcpSocket TcpClient;
	TcpClient.connectToHost("127.0.0.1", nServerPort);
	if (!TcpClient.waitForConnected(100))
	{// the MySql Service not start
		strMessage = QString(tr("Failed to connect MySQL Server 127.0.0.1:%1,it may not started!")).arg(nServerPort);
		QMessageBox::information(nullptr, tr("Information"), strMessage, QMessageBox::Ok);
		return;
	}

	strLogMode = ui.comboBox_LogMode->currentText();
	strLogbinFile = ui.lineEdit_Logbin->text();
	strGeneralLog = ui.lineEdit_Generallog->text();
	strLogError = ui.lineEdit_Logerror->text();
	nLongQueryTime = ui.lineEdit_LongQueryTime->text().toInt();
	strSlowQuerylog = ui.lineEdit_SlowQuerylog->text();
	nExpiredlogDays = ui.lineEdit_Expiredlogdays->text().toInt();
	strExternAccessPassword = ui.lineEdit_AccessPassword->text();
	if (!strLogbinFile.size())
	{
		QMessageBox::information(nullptr, tr("Information"), tr("Please input a valid file name for log bin!"), QMessageBox::Ok);
		ui.lineEdit_Logbin->setFocus();
		return;
	}
	if (bCheckGenerallog && !strGeneralLog.size())
	{
		QMessageBox::information(nullptr, tr("Information"), tr("You have enable General log,but you didn't input a file name for it!"), QMessageBox::Ok);
		ui.lineEdit_Generallog->setFocus();
		return;
	}

	if (bChecklogError && !strLogError.size())
	{
		QMessageBox::information(nullptr, tr("Information"), tr("You have enable Log error,but you didn't input a file name for it!"), QMessageBox::Ok);
		ui.lineEdit_Logerror->setFocus();
		return ;
	}
	if (bChecklongQueryTime && !nLongQueryTime)
	{
		QMessageBox::information(nullptr, tr("Information"), tr("You have enable Long Query Time,but you  didn't input a valid value for it!"), QMessageBox::Ok);
		ui.lineEdit_LongQueryTime->setFocus();
		return;
	}
	if (bCheckSlowQuerylog && !strSlowQuerylog.size())
	{
		QMessageBox::information(nullptr, tr("Information"), tr("You have enable Slow Query log,but you didn't input a file name for it!"), QMessageBox::Ok);
		ui.lineEdit_SlowQuerylog->setFocus();
		return;
	}
	if (bCheckExpiredlogDays && !nExpiredlogDays)
	{
		QMessageBox::information(nullptr, tr("Information"), tr("You have enable Expired log days,but you  didn't input a valid value for it!"), QMessageBox::Ok);
		ui.lineEdit_Expiredlogdays->setFocus();
		return;
	}

	if (bCheckExternAccess && strExternAccessPassword.size()< 6)
	{
		QMessageBox::information(nullptr, tr("Information"), tr("You have enable External Access,then you have to input a password that not less than 6 characters!"), QMessageBox::Ok);
		ui.lineEdit_AccessPassword->setFocus();
		return;
	}
	
	if (nServerType == Server_Master)
	{
		if (!ConfigureMaster())
			return;
	}
	else if (nServerType == Server_Slave)
	{
		if (!ConfigureSlave())
			return;
	}
	/*if (!SaveSettings(strInstalledPath, strMessage))
	{
		QMessageBox::information(nullptr, tr("Information"),strMessage, QMessageBox::Ok);
		return;
	}*/
}

void MySQLConfig::on_pushButton_Cancel_clicked()
{
	QDialog::reject();
}

void MySQLConfig::on_checkBox_Logbin_stateChanged(int arg1)
{
	bCheckLogbin = arg1 == Qt::Checked;
		
	ui.lineEdit_Logbin->setEnabled(arg1 == Qt::Checked);
	ui.lineEdit_Logbin->setFocus();
}

void MySQLConfig::on_checkBox_Generallog_stateChanged(int arg1)
{
	bCheckGenerallog = arg1 == Qt::Checked;
		
	ui.lineEdit_Generallog->setEnabled(arg1 == Qt::Checked);
	ui.lineEdit_Generallog->setFocus();
}

void MySQLConfig::on_checkBox_Logerror_stateChanged(int arg1)
{
	bChecklogError = arg1 == Qt::Checked;
		
	ui.lineEdit_Logerror->setEnabled(arg1 == Qt::Checked);
	ui.lineEdit_Logerror->setFocus();
}

void MySQLConfig::on_checkBox_LongQueryTime_stateChanged(int arg1)
{
	bChecklongQueryTime = arg1 == Qt::Checked;
		
	ui.lineEdit_LongQueryTime->setEnabled(arg1 == Qt::Checked);
	ui.lineEdit_LongQueryTime->setFocus();
}

void MySQLConfig::on_checkBox_SlowQuerylog_stateChanged(int arg1)
{
	bCheckSlowQuerylog = arg1 == Qt::Checked;
		
	ui.lineEdit_SlowQuerylog->setEnabled(arg1 == Qt::Checked);
	ui.lineEdit_SlowQuerylog->setFocus();
}

void MySQLConfig::on_checkBox_Expiredlogdays_stateChanged(int arg1)
{
	bCheckExpiredlogDays = arg1 == Qt::Checked;
		
	ui.lineEdit_Expiredlogdays->setEnabled(arg1 == Qt::Checked);
	ui.lineEdit_Expiredlogdays->setFocus();
}

void MySQLConfig::on_checkBox_ExternAccess_stateChanged(int arg1)
{
	bCheckExternAccess = arg1 == Qt::Checked;
	ui.lineEdit_AccessPassword->setEnabled(arg1 == Qt::Checked);
	ui.lineEdit_AccessPassword->setFocus();
}

void MySQLConfig::on_listWidget_SlaveHosts_customContextMenuRequested(const QPoint &pos)
{
	int nItems = ui.listWidget_SlaveHosts->count();
	QMenu *popMenu = new QMenu( this );
    QAction *pActionAdd = new QAction(tr("Add"), this);
    QAction *pActionDel = new QAction(tr("Delete"), this);
    QAction *pActionClear = new QAction(tr("Clear"), this);
    popMenu->addAction( pActionAdd );
    popMenu->addAction( pActionDel );
    popMenu->addAction( pActionClear );
	connect( pActionAdd, SIGNAL(triggered()), this, SLOT( OnAddHost()) );
    connect( pActionDel, SIGNAL(triggered()), this, SLOT( OnDelHost()) );
    connect( pActionClear, SIGNAL(triggered()), this, SLOT( OnClearHost()) );
    QListWidgetItem* curItem = ui.listWidget_SlaveHosts->itemAt( pos );
	if (curItem == NULL)
	{
		pActionDel->setEnabled(false);
		nCurSlaveHost = -1;
	}
	else
		nCurSlaveHost = ui.listWidget_SlaveHosts->row(curItem);
	if (nItems == 0)
		pActionClear->setEnabled(false);
   
    popMenu->exec( QCursor::pos() );

    delete pActionAdd;
    delete pActionDel;
    delete pActionClear;
    delete popMenu;
}

void MySQLConfig::OnAddHost()
{
	QListWidgetItem* pItem = new QListWidgetItem("");

	ui.listWidget_SlaveHosts->addItem(pItem);
	ui.listWidget_SlaveHosts->setCurrentItem(pItem);
	pItem->setFlags(pItem->flags() | Qt::ItemIsEditable| Qt::ItemIsEnabled );
	QModelIndex index = ui.listWidget_SlaveHosts->model()->index(ui.listWidget_SlaveHosts->row(pItem), 0);
	
	//QPersistentModelIndex persistent = index;
	//ui.listWidget_SlaveHosts->activated(index);
	//emit activated(persistent);
	//emit doubleClicked(index);
	//connect(this, SIGNAL(itemDoubleClicked), this, SLOT(on_listWidget_SlaveHosts_itemDoubleClicked));
}

void MySQLConfig::OnDelHost()
{
	int nRespond = QMessageBox::warning(NULL, tr("Warning"),tr("Are you sure to delete the selected items?"),	QMessageBox::Yes | QMessageBox::No,	QMessageBox::No);

	if (nRespond != QMessageBox::Yes)
		return;

	QModelIndexList SelectedItems = ui.listWidget_SlaveHosts->selectionModel()->selectedRows();
	// delete the list item in reversed sequece
	for (auto it = --SelectedItems.end(); it != --SelectedItems.begin(); it--)
		ui.listWidget_SlaveHosts->takeItem(it->row());
	
}

void MySQLConfig::OnClearHost()
{
	int nRespond = QMessageBox::warning(NULL, tr("Warning"), tr("Are you sure to clear the list?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

	if (nRespond != QMessageBox::Yes)
		return;
	ui.listWidget_SlaveHosts->clear();
}
