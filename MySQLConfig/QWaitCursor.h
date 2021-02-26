#pragma once
#include <QtWidgets/QApplication>
class QWaitCursor
{
public:
	QWaitCursor()
	{
		QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	}
	~QWaitCursor()
	{
		QGuiApplication::restoreOverrideCursor();
	}
	void Restore()
	{
		QGuiApplication::restoreOverrideCursor();
	}
};