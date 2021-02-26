#include "QUnCheckableBox.h"

QUnCheckableBox::QUnCheckableBox(QWidget* parent)
	: QCheckBox(parent)
{
}

QUnCheckableBox::~QUnCheckableBox()
{
}
bool QUnCheckableBox::event(QEvent* ev)
{
	if (ev->type() == QEvent::MouseButtonPress ||
		ev->type() == QEvent::MouseButtonRelease||
		ev->type() == QEvent::MouseButtonDblClick)
	{
		return true;
	}
	if (ev->type() == QEvent::KeyPress ||
		ev->type() == QEvent::KeyRelease)
		return true;

	return QWidget::event(ev);
}
