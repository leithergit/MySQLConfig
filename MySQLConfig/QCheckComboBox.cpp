#include "QCheckComboBox.h"

#include <QtWidgets/QLineEdit>
#include <QtGUI/QMouseEvent>
#include <QtCore/QDebug>

QCheckComboBox::QCheckComboBox(QWidget* parent)
	: QComboBox(parent)
{
	pLineEdit = new QLineEdit(this);
	pLineEdit->setReadOnly(true);
	this->setLineEdit(pLineEdit);
	this->lineEdit()->disconnect();

	//KeyPressEater *keyPressEater = new KeyPressEater(this);
	pListView = new QListView(this);
	//pListView->installEventFilter(keyPressEater);
	//pListView->installEventFilter(this);
	//setItemView(new QComboBoxListView(this));
	this->setView(pListView);
	pListView->setModel(&m_model);
	this->setModel(&m_model);

	connect(this, SIGNAL(activated(int)), this, SLOT(SlotActivated(int)));
	//connect(keyPressEater, SIGNAL(signActivated(int)), this, SLOT(SlotActivated(int)));
}

QCheckComboBox::~QCheckComboBox()
{
}

void QCheckComboBox::AddItem(const QString& str, bool bChecked /*= false*/, QVariant& userData /*= QVariant()*/)
{
	QStandardItem* item = new QStandardItem(str);
	item->setCheckable(true);
	item->setCheckState(bChecked ? Qt::Checked : Qt::Unchecked);
	item->setData(userData, Qt::UserRole + 1);
	m_model.appendRow(item);

	UpdateText();
}

void QCheckComboBox::AddItems(const QList<ItemInfo>& lstItemInfo)
{
	for (auto a : lstItemInfo)
	{
		AddItem(a.str, a.bChecked, a.userData);
	}
}

void QCheckComboBox::AddItems(const QMap<QString, bool>& mapStrChk)
{
	for (auto it = mapStrChk.begin(); it != mapStrChk.end(); ++it)
	{
		AddItem(it.key(), it.value());
	}
}

void QCheckComboBox::AddItems(const QList<QString>& lstStr)
{
	for (auto a : lstStr)
	{
		AddItem(a, false);
	}
}

void QCheckComboBox::RemoveItem(int idx)
{
	m_model.removeRow(idx);
	UpdateText();
}

void QCheckComboBox::SetItemCheck(int nIndex, bool bCheck )
{
	QStandardItem* item = m_model.item(nIndex);
	if (nullptr == item) 
		return;

	item->setCheckState(bCheck ? Qt::Checked : Qt::Unchecked);

	UpdateText();
}

void QCheckComboBox::SetItemCheck(QString strString, bool bCheck )
{
	QList<QStandardItem*> itemList = m_model.findItems(strString);
	if (itemList.size())
	{
		for each (auto var in itemList)
			var->setCheckState(bCheck ? Qt::Checked : Qt::Unchecked);
		UpdateText();
	}
}
void QCheckComboBox::Clear()
{
	m_model.clear();
	UpdateText();
}

QString QCheckComboBox::getText()
{
	//QStringList lst;
	return pLineEdit->text();
	/*if (str.isEmpty())
	{
		return lst;
	}
	else
	{
		return pLineEdit->text().split(",");
	}*/
}

QStringList QCheckComboBox::getTextList()
{
	QStringList lst;
	QString strText = pLineEdit->text();
	if (strText.isEmpty())
	{
		return lst;
	}
	else
	{
		return pLineEdit->text().split(",");
	}
}

QList<ItemInfo> QCheckComboBox::GetSelItemsInfo()
{
	QList<ItemInfo> lstInfo;
	for (int i = 0; i < m_model.rowCount(); i++)
	{
		QStandardItem* item = m_model.item(i);
		if (item->checkState() == Qt::Unchecked) continue;

		ItemInfo info;
		info.idx = i;
		info.str = item->text();
		info.bChecked = true;
		info.userData = item->data(Qt::UserRole + 1);

		lstInfo << info;
	}

	return lstInfo;
}

QString QCheckComboBox::GetItemText(int idx)
{
	if (idx < 0 || idx >= m_model.rowCount())
	{
		return QString("");
	}

	return m_model.item(idx)->text();
}

ItemInfo QCheckComboBox::GetItemInfo(int idx)
{
	ItemInfo info;
	if (idx < 0 || idx >= m_model.rowCount())
	{
		return info;
	}

	QStandardItem* item = m_model.item(idx);
	info.idx = idx;
	info.str = item->text();
	info.bChecked = (item->checkState() == Qt::Checked);
	info.userData = item->data(Qt::UserRole + 1);

	return info;
}

//bool QCheckComboBox::eventFilter(QObject *watched, QEvent *event)
//{
//	if (event->type() == QEvent::MouseButtonPress) {
//		QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
//		if (mouseEvent->button() == Qt::LeftButton) {
//			QListView *listView = qobject_cast<QListView*>(watched);
//			if (nullptr != listView) {
//				int row = listView->currentIndex().row();
//				if (row != -1) {
//					SlotActivated(row);
//				}
//			}
//			return true;
//		}
//	}
//	return QComboBox::eventFilter(watched, event);
//}

//void QCheckComboBox::showPopup()
//{
//	emit showingPopup();
//	QComboBox::showPopup();
//}

void QCheckComboBox::hidePopup()
{
	int width = this->view()->width();
	int height = this->view()->height();
	int x = QCursor::pos().x() - mapToGlobal(geometry().topLeft()).x() + geometry().x();
	int y = QCursor::pos().y() - mapToGlobal(geometry().topLeft()).y() + geometry().y();

	QRect rectView(0, this->height(), width, height);
	if (!rectView.contains(x, y))
	{
		emit hidingPopup();
		QComboBox::hidePopup();
	}
}

void QCheckComboBox::mousePressEvent(QMouseEvent* event)
{
	QComboBox::mousePressEvent(event);
	event->accept();
}

void QCheckComboBox::mouseReleaseEvent(QMouseEvent* event)
{
	QMouseEvent* m = event;
	if (isVisible() &&
		pListView->rect().contains(m->pos()) &&
		pListView->currentIndex().isValid()
		/*&& !blockMouseReleaseTimer.isActive() && !ignoreEvent*/
		&& (pListView->currentIndex().flags() & Qt::ItemIsEnabled)
		&& (pListView->currentIndex().flags() & Qt::ItemIsSelectable))
	{
		//emit itemSelected(pListView->currentIndex());
		qDebug("%s item = %d.\n", __FUNCTION__, pListView->currentIndex().row());
		return;
	}
	//event->accept();
}

void QCheckComboBox::mouseMoveEvent(QMouseEvent* event)
{
	QComboBox::mouseMoveEvent(event);
	event->accept();
}

void QCheckComboBox::UpdateText()
{
	QStringList lstTxt;
	for (int i = 0; i < m_model.rowCount(); ++i)
	{
		QStandardItem* item = m_model.item(i);
		if (item->checkState() == Qt::Unchecked) continue;

		lstTxt << item->text();
	}

	pLineEdit->setText(lstTxt.join(","));
	pLineEdit->setToolTip(lstTxt.join("\n"));
}

void QCheckComboBox::SlotActivated(int idx)
{
	QStandardItem* item = m_model.item(idx);
	if (nullptr == item)
		return;
	if ((item->flags() & Qt::ItemIsEnabled) &&
		(item->flags()& Qt::ItemIsSelectable))
	{
		Qt::CheckState state = (item->checkState() == Qt::Checked) ? Qt::Unchecked : Qt::Checked;
		item->setCheckState(state);

		UpdateText();
	}
	this->view();
}
