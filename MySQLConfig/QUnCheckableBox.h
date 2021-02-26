#pragma once

#include <QCheckBox>

#include<QEvent>
#include<QMouseEvent>
class QUnCheckableBox : public QCheckBox
{
	Q_OBJECT

public:
	QUnCheckableBox(QWidget* parent);
	~QUnCheckableBox();
	bool event(QEvent*);
};
