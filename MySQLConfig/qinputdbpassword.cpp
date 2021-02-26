#include "qinputdbpassword.h"
#include "ui_qinputdbpassword.h"
#include "Utility.h"
#include "QWaitCursor.h"
#include "WinService.h"
#include <QMessageBox>
#include <QSettings>
#include <commctrl.h>
#include <QTcpSocket>
#include "MySQLAgent.h"

QInputDBPassword::QInputDBPassword(QString strMySQLPath,unsigned short nMySQLPort,QWidget *parent) :
    QDialog(parent),
    m_strMySQLPath(strMySQLPath),
	m_nMySQLPort(nMySQLPort),
    ui(new Ui::QInputDBPassword)
{
    ui->setupUi(this);
	QStringList arguments = QApplication::arguments();
    //if (arguments.size()< 2)
    //{
    //    ui->pushButton_ResetPassword->setEnabled(false);
    //}
    //else
    //    m_strMySQLService = arguments[1];
	Button_SetElevationRequiredState(HWND(ui->pushButton_ResetPassword->winId()), TRUE);
	
}

QInputDBPassword::~QInputDBPassword()
{
    delete ui;
}

void QInputDBPassword::on_pushButton_OK_clicked()
{
    m_strPassword = ui->lineEdit_Password->text();
    QDialog::accept();
}

void QInputDBPassword::on_pushButton_Cancel_clicked()
{
    QDialog::reject();
}

bool QInputDBPassword::ResetMySQLPassword(QString strNewPassword,QString &strMessage)
{
	try
	{
		QString strPassword = ui->lineEdit_NewPassword->text();
		SERVICE_STATUS_PROCESS ssStatus;
		int nResult = 0;
		char szError[1024] = { 0 };
		
		CMySQLAgent DBConnector;
		nResult = DBConnector.Connect("127.0.0.1", "root", "");	// Access denied,may Account or password error
		if (!nResult)			// return a non zero value indicated a failure!
		{
			try
			{
				ULONGLONG nAffect = DBConnector.ExecuteSQL("update mysql.user set mysql.user.password=password('%s') where mysql.user.user='root'",strNewPassword.toStdString().c_str());
				return true;
			}
			catch (CMySQLException  e)
			{
				strMessage = e.what();
				return false;
			}
		}
		else
		{
			strMessage = DBConnector.GetErrorMsg();
			return false;
		}

	}
	catch (std::exception& e)
	{
		strMessage = e.what();
		return false;
		//QMessageBox::critical(nullptr, tr("Exception"), e.what(), QMessageBox::Ok);
	}
}
void QInputDBPassword::on_pushButton_ResetPassword_clicked()
{
	try
	{
		if (QMessageBox::warning(nullptr,
			tr("Warning"),
			tr("MySQL Service will be restarted while resetting password,Press yes to to continue!"),
			QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
			return;
		QWaitCursor Wait;
		QString strNewPassword = ui->lineEdit_NewPassword->text();
		if (strNewPassword.size() < 6)
		{
			Wait.Restore();
			QMessageBox::information(nullptr, tr("Information"), tr("To Reset password,you have to enter a new password that not less than 6 characters! "), QMessageBox::Ok);
			return;
		}
	
		QString strMessage;
		ui->progressBar->setRange(1, 100);
		ui->progressBar->setValue(1);
			
		ui->progressBar->setFormat(tr("Searching system services..."));
		bool bServiceStop = false;
		std::string strMySQLService = "";
		ServiceInformationArray SvrConfigArray;
		ServiceMgr.GetAllServiceInformation(SvrConfigArray);
		int nProgress = 5;
		ui->progressBar->setValue(nProgress);
			
		if (SvrConfigArray.size())
		{
			ui->progressBar->setFormat(tr("[5%]Try Match MySQL Service..."));
			QString strBinPath = m_strMySQLPath + "\\bin\\Mysqld.exe";
			char szBinPath[1024] = { 0 };
			strcpy(szBinPath, m_strMySQLPath.toStdString().c_str());
			auto itFind = find_if(SvrConfigArray.begin(), SvrConfigArray.end(), [szBinPath](ServiceInformationPtr p)
				{
					return strstr(p->ServiceConfig.lpBinaryPathName, szBinPath) != nullptr;
				});
			// if the MySQL Service is installed and it's Prescess path equal to m_strMySQLPath,and it is running ,then stop it !
			if (itFind != SvrConfigArray.end())
			{
				strMySQLService = (*itFind)->EnumServiceStatus.lpServiceName;
				nProgress = 15;
				ui->progressBar->setValue(nProgress);
				ui->progressBar->setFormat(QString(tr("[1%]Succeed in matching MySQL Service,Try to stop it...")).arg(nProgress));
				if ((*itFind)->EnumServiceStatus.ServiceStatus.dwCurrentState == Running)
				{
					SERVICE_STATUS_PROCESS ssStatus;
					int nResult = ServiceMgr.Stop((*itFind)->EnumServiceStatus.lpServiceName, ssStatus);
					if (nResult )
					{							
						char szError[1024] = { 0 };
						FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
							NULL,
							nResult,
							MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
							(LPSTR)szError,
							1024,
							NULL);
						strMessage = QString("Failed to stop Service %1:%2").arg((*itFind)->EnumServiceStatus.lpServiceName).arg(szError);
						ui->progressBar->setFormat(tr("Operation aborted!"));
						ui->progressBar->setValue(100);
						Wait.Restore();
						QMessageBox::critical(nullptr, tr("Information"), strMessage, QMessageBox::Abort);
							
					}
					bServiceStop = true;
				}
				nProgress = 30;
				ui->progressBar->setFormat(QString(tr("[%1%]MySQL Service has been stopped,Try to Update Password...!")).arg(nProgress));
				ui->progressBar->setValue(nProgress);
			}
			else
			{
				nProgress = 30;
				ui->progressBar->setFormat(QString(tr("[30%]Failed matching MySQL Service,Try to Update Password...")).arg(nProgress));
				ui->progressBar->setValue(nProgress);
			}
		}
		/*if (!ShutDownMySQL(m_strMySQLPath))
		{
			QMessageBox::information(nullptr, tr("Information"), tr("Failed Stop MySQL Service ,can't find file MySQLAdmin.exe!"), QMessageBox::Ok);
			return;
		}*/
		StartMySQLWithoutAuthenticaion();
		nProgress = 40;
		ui->progressBar->setValue(nProgress);
			
		if (ResetMySQLPassword(strNewPassword, strMessage))
		{
			nProgress = 45;
			ui->progressBar->setValue(nProgress);
			ui->progressBar->setFormat(QString(tr("[1%]Succeed in updating MySQL Password,Try to clean up...")).arg(nProgress));
			
			ShutDownMySQL(m_strMySQLPath);
			nProgress = 50;
			ui->progressBar->setValue(nProgress);
			ui->progressBar->setFormat(QString(tr("[%1%]Clean up is Completed,Try to restart MySQL Service...")).arg(nProgress));
			

			if (strMySQLService.size() && bServiceStop)
			{
				DWORD dwT1 = timeGetTime();
				int nProgress = 50;
				DWORD dwWaitTime = 4000;
				int nLastDiff = 0;
				DWORD dwT2 = 0;
				while (true)
				{
					dwT2 = timeGetTime();
					if ((dwT2- dwT1) >= dwWaitTime)
						break;
					else
					{
						int nDiff = (dwT2 - dwT1) / 100;
						if (nDiff > nLastDiff)
						{
							//qDebug("%s Progress = %d.\n", __FUNCTION__, 50 + nDiff);
							ui->progressBar->setValue(50 + nDiff);
							ui->progressBar->setFormat(QString(tr("[%1%]Clean up is Completed,Try to restart MySQL Service...")).arg(50 + nDiff));
							
							nLastDiff = nDiff;
						}
						Sleep(50);
					}
				}
				
				SERVICE_STATUS_PROCESS ssStatus;
				int nResult = ServiceMgr.Start(strMySQLService.c_str(),ssStatus);
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
					strMessage = QString(tr("Failed in restarting Service '%1':%2")).arg(strMySQLService.c_str()).arg(szError);
					Wait.Restore();
					QMessageBox::critical(nullptr, tr("Information"), strMessage, QMessageBox::Abort);
					return ;
				}
				nProgress = 95;
				ui->progressBar->setValue(nProgress);
				ui->progressBar->setFormat(QString(tr("[%1%]MySQL Service has been started...")).arg(nProgress));
				
				CMySQLAgent DBConnector;
				nResult = DBConnector.Connect("127.0.0.1", "root", strNewPassword.toStdString().c_str());
				if (nResult)	// return a non zero value indicated a failure!
				{
					strMessage = DBConnector.GetErrorMsg();
					return ;
				}
			}
			ui->lineEdit_Password->setText(strNewPassword);
			ui->progressBar->setValue(100);
			ui->progressBar->setFormat(tr("The password has been reset!"));
			
			Wait.Restore();
			QMessageBox::information(nullptr, tr("Information"), tr("The password has been reset! "), QMessageBox::Ok);
		}
		else
		{
			Wait.Restore();
			QMessageBox::warning(nullptr, tr("Information"), strMessage, QMessageBox::Ok);
		}
	}
	catch (DWORD errCode)
	{
		TraceMsgA("%s Exception code:%d", __FUNCTION__, errCode);
	}   
   
}

void QInputDBPassword::on_checkBox_HidePassword_stateChanged(int arg1)
{
	if (arg1 == 0)
	{
        ui->lineEdit_NewPassword->setEchoMode(QLineEdit::Normal);
        ui->lineEdit_Password->setEchoMode(QLineEdit::Normal);
	}
    else
    {
        ui->lineEdit_NewPassword->setEchoMode(QLineEdit::Password);
        ui->lineEdit_Password->setEchoMode(QLineEdit::Password);
    }
}

QString QInputDBPassword::GetPassword()
{
    return m_strPassword;
}
