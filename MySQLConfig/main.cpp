#include "MySQLConfig.h"
#include <QtWidgets/QApplication>
#include "MySQLAgent.h"

int main(int argc, char *argv[])
{
	//auto it = find(keys.begin(), keys.end(), "skip-grant-tables");
	
	/*Test the MySQL API for MySQL private statements */
    /*try
    {
		 CMySQLAgent MySQLAgent("127.0.0.1", "root", "Mago&Zpmc@2020");
		 LONGLONG nAffectRows = MySQLAgent.ExecuteSQL("update mysql.user set password=password('Test1234')");
		 CMyResult res = MySQLAgent.Query("show master status");
		 if (res.RowCount())
		 {
			 char *File = (char*)res["File"];
			 int	Position = res["Position"];
			 char *pDB = (char*)res["Binlog_Do_DB"];
			 char *pIgdb = (char*)res["Binlog_Ignore_DB"];
			 char *Grid = (char*)res["Executed_Gtid_Set"];
		 }
		 MySQLAgent.ExecuteSQLString("GRANT ALL PRIVILEGES ON* .*TO 'root'@'%'IDENTIFIED BY 'Mago&Zpmc@2020' WITH GRANT OPTION");
		 MySQLAgent.ExecuteSQLString("GRANT ALL PRIVILEGES ON* .*TO 'root'@'127.0.0.1'IDENTIFIED BY 'Mago&Zpmc@2020' WITH GRANT OPTION");
		 MySQLAgent.ExecuteSQLString("GRANT ALL PRIVILEGES ON* .*TO 'root'@'localhost'IDENTIFIED BY 'Mago&Zpmc@2020' WITH GRANT OPTION");
		 MySQLAgent.ExecuteSQLString("GRANT ALL PRIVILEGES ON* .*TO 'root'@'::1'IDENTIFIED BY 'Mago&Zpmc@2020' WITH GRANT OPTION");
		 MySQLAgent.ExecuteSQLString("flush privileges");

    }
	catch (CMySQLException& e)
	{
		string strException = e.what_;
		printf("Exception:%s\n", strException.c_str());
	}
	catch (std::exception& e)
	{
		printf("Exception:%s\n", e.what());
	}
	*/
   
    QApplication a(argc, argv);
    MySQLConfig w;
    w.show();
    return a.exec();
}
