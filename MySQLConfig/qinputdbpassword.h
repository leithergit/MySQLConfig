#ifndef QINPUTDBPASSWORD_H
#define QINPUTDBPASSWORD_H

#include <QDialog>
#include <QDir>
#include <QProcess>
#include <QTcpSocket>
#include <QHostAddress>
#include <windows.h>
#include <QThread>
#include <QDebug>
#include "Utility.h"
#include "WinService.h"
namespace Ui {
class QInputDBPassword;
}

class QMySQLDThread :public QThread
{
public:
	QString m_strMySQLPath;
	QMySQLDThread(QString strMySQLPath)
		:m_strMySQLPath(strMySQLPath)
	{
	}
	~QMySQLDThread()
	{
		requestInterruption();
		quit();
		wait();
	}
	void run()
	{
		QString strMySQLD = m_strMySQLPath;
		strMySQLD += "\\bin\\mysqld.exe";
		if (!QFileInfo::exists(strMySQLD))
			return;
		QProcess Process(0);

		QString strDefaultSetting = QString("--defaults-file=\"%1\\My.ini\"").arg(m_strMySQLPath);
		//QStringList strArgs;

		// strArgs << strDefaultSetting << "--skip-grant-tables";// << "--explicit_defaults_for_timestamp";
		//strArgs << "--skip-grant-tables";

		QString strAppWorkPath = m_strMySQLPath + "\\bin";
		std::string strArgs = strDefaultSetting.toStdString().c_str();
		strArgs += " --skip-grant-tables";
		strArgs += " --explicit_defaults_for_timestamp";
		//Process.setWorkingDirectory(strAppWorkPath);
		//Process.setReadChannel(QProcess::StandardOutput);
		//QProcess* pProcess = &Process;
		/*connect(pProcess, &QProcess::readyReadStandardOutput, [pProcess]() {
			TraceMsgA("%s.\n", pProcess->readAllStandardOutput().toStdString().c_str());
			});*/
		int nResult = RunWinProcess(strMySQLD.toStdString().c_str(), strArgs.c_str(), strAppWorkPath.toStdString().c_str());
		//Process.start(strMySQLD, strArgs);
		//QProcess::ProcessState nState;
		//QProcess::ExitStatus  nStatus;
		//QProcess::ProcessError nError;
		//QByteArray Out = Process.readAll();
		//if (Process.waitForStarted(-1))
		//{
		//	QByteArray In = Process.readAll();
		//	nState = Process.state();
		//	Process.waitForFinished(-1);
		//	nStatus = Process.exitStatus();
		//	return;
		//}
		//else
		//{
		//	nState = Process.state();
		//	nError = Process.error();
		//	return;
		//}

		return;
	}
};
class QInputDBPassword : public QDialog
{
	Q_OBJECT

public:
	explicit QInputDBPassword(QString strMySQLPath, unsigned short nMySQLPort, QWidget* parent = nullptr);
	~QInputDBPassword();
	QString GetPassword();

	QMySQLDThread* m_pMySQLThread = nullptr;
	CWinService ServiceMgr;;
	void StartMySQLWithoutAuthenticaion()
	{
		/*if (m_pMySQLThread)
			delete m_pMySQLThread;
		m_pMySQLThread = new QMySQLDThread(m_strMySQLPath);
		m_pMySQLThread->start();
		while (!m_pMySQLThread->isRunning())
		{
			QThread::sleep(50);
		}*/
		QString strMySQLD = m_strMySQLPath;
		strMySQLD += "\\bin\\mysqld.exe";
		if (!QFileInfo::exists(strMySQLD))
			return;
		
		QString strDefaultSetting = QString("--defaults-file=\"%1\\My.ini\"").arg(m_strMySQLPath);
		QString strAppWorkPath = m_strMySQLPath + "\\bin";
		std::string strArgs = strDefaultSetting.toStdString().c_str();
		strArgs += " --skip-grant-tables";
		strArgs += " --explicit_defaults_for_timestamp";
		
		int nResult = RunWinProcess(strMySQLD.toStdString().c_str(), strArgs.c_str(), strAppWorkPath.toStdString().c_str());
	}
	
    bool ShutDownMySQL(QString strMySQLPath)
    {
		QTcpSocket TcpClient;
		TcpClient.connectToHost("127.0.0.1", m_nMySQLPort);
		if (!TcpClient.waitForConnected(100))
		{// the MySql Service not start
            return true;
		}

        QString strMySQLAdmin = strMySQLPath;
        strMySQLAdmin += "\\bin\\mysqladmin.exe";
        if (!QFileInfo::exists(strMySQLAdmin))
            return false;
        QProcess Process;
        QStringList strArgs;
        //  -uroot -p='' shutdown
        strArgs << "-uroot" << "-p=''" << "shutdown";
        Process.start(strMySQLAdmin, strArgs);
        Process.waitForStarted(-1);
        Process.waitForFinished(-1);
		if (m_pMySQLThread)
		{
			m_pMySQLThread->requestInterruption();
			while (m_pMySQLThread->isRunning())
			{
				QThread::msleep(100);
			}
			TraceMsgA("%s %d MySQLD Thread is exit.\n", __FUNCTION__, __LINE__);
		}
		//do
		//{
		//	QTcpSocket TcpClient;
		//	TcpClient.connectToHost("127.0.0.1", m_nMySQLPort);
		//	if (!TcpClient.waitForConnected(100))
		//	{// the MySql Service not start
		//		break;
		//	}
		//	QThread::sleep(500);
		//} while (true);
        return true;
      }
private slots:
    void on_pushButton_OK_clicked();

    void on_pushButton_Cancel_clicked();

    void on_pushButton_ResetPassword_clicked();    

    void on_checkBox_HidePassword_stateChanged(int arg1);

    bool ResetMySQLPassword(QString strNewPassword,QString &strMessage);

private:
    Ui::QInputDBPassword *ui;
    QString m_strPassword;
    QString m_strMySQLService;
    QString m_strMySQLPath;
    unsigned short  m_nMySQLPort = 3306;
};

#endif // QINPUTDBPASSWORD_H
