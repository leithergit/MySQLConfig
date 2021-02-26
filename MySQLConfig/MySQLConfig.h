#pragma once

#include <QtWidgets/QDialog>
#include "ui_MySQLConfig.h"
#include <string>
#include <QSettings>
#include <MySQLAgent.h>
#include "WinService.h"
#include "IniFile.h"

enum ServerType
{
	Server_Unknown = -1,
	Server_Master = 0,
	Server_Slave = 1
};

enum LogMode
{
    Log_FILE = 0,
    Log_TABLE = 1,
    Log_FILE_TABLE = 2
};

class MySQLConfig : public QDialog
{
    Q_OBJECT

public:
    MySQLConfig(QWidget *parent = Q_NULLPTR);
    ~MySQLConfig();
    bool LoadSettings(QString strMySQLPath);
    bool SaveSettings(QString strMySQLPath, QString& strMessage);
    int  nCurSlaveHost = -1;
    void SetDefaultMaster();
    void SetDefaultSlave();
    static bool GetMySQLService(std::string strMySQL_Path,  std::string& strMySQLService, ServiceStatus& nServiceStatus);

private slots:
    void on_pushButton_Browse_clicked();

    void on_pushButton_Connect_clicked();

    void on_comboBox_ServerType_currentIndexChanged(int index);

    void on_pushButton_ApplySettings_clicked();   

    void on_checkBox_Logbin_stateChanged(int arg1);

    void on_checkBox_Generallog_stateChanged(int arg1);

    void on_checkBox_Logerror_stateChanged(int arg1);

    void on_checkBox_LongQueryTime_stateChanged(int arg1);

    void on_checkBox_SlowQuerylog_stateChanged(int arg1);

    void on_checkBox_ExternAccess_stateChanged(int arg1);
    
    void on_checkBox_Expiredlogdays_stateChanged(int arg1);

    void on_listWidget_SlaveHosts_customContextMenuRequested(const QPoint &pos);

    void OnAddHost();

    void OnDelHost();

    void OnClearHost();   

    void on_pushButton_Cancel_clicked();

    bool ConfigureMaster();

    bool ConfigureSlave();

    bool TestMySQLService(QString &strService, ServiceStatus& nStatus);

    void OnComboboxSourceDBHidePopup();

    void OnStart();
private:
    Ui::MySQLConfigClass ui;
    QIcon IcoConnect[2];
    CIniFile* pMySQLIni = nullptr;
    QSettings* pMySQLSettings = nullptr;
    QString     strMySQLPath;
    ServerType  nServerType = Server_Unknown;
    QString     strDBPassword = "";
    bool        bCheckLogbin = false;
    bool        bCheckGenerallog = false;
    bool        bChecklogError = false;
    bool        bChecklongQueryTime = false;
    bool        bCheckSlowQuerylog = false;
    bool        bCheckExpiredlogDays = false;
    bool        bCheckExternAccess = false;

    QString strLogMode;
    QString strInstalledPath;
    int nServerID;
    int nServerPort;
    QString strLogbinFile;
	QString strGeneralLog;
	QString strLogError;
	int nLongQueryTime;
	QString strSlowQuerylog;
	int nExpiredlogDays;
	QString strExternAccessPassword;

    // Master Server Variables
    QString strSourceDB;
    QString strIgorenDB;
    QString strReplicationAccount;
    QString strReplicationPassword;
    QStringList strSlaveHostList;

    // Slave Server Variables
    QString strMasterHost;
    int nMasterPort;
    QString strMasterLogFile;
    int nMasterlogPos;
    QString strRelayLog;
    QString strRelayLogIndex;
	QString strMasterAccount;
	QString strMasterPassword;
    vector<QWidget*>* pWidgetPtrServer = nullptr;
    vector<QWidget*>* pWidgetPtrMaster = nullptr;
    vector<QWidget*>* pWidgetPtrSlave = nullptr;
   
	 
};

//#if defined _M_IX86
//#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
//#elif defined _M_X64
//#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
//#else
//#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
//#endif
