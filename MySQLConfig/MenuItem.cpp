#include "menuitem.h"

MenuItem::MenuItem(int menuId, const QString &menuName, bool isChecked)
{
	this->menuId = menuId;
	this->menuName = menuName;
	this->isChecked = isChecked;
}

MenuItem::MenuItem()
{
	this->menuId = -1;
	this->menuName = QString("");
	this->isChecked = false;
}

MenuItem::MenuItem(const MenuItem &menuItem)
{
	this->menuId = menuItem.getMenuId();
	this->menuName = menuItem.getMenuName();
	this->isChecked = menuItem.getIsChecked();
}

int MenuItem::getMenuId() const
{
	return menuId;
}

void MenuItem::setMenuId(int value)
{
	menuId = value;
}

QString MenuItem::getMenuName() const
{
	return menuName;
}

void MenuItem::setMenuName(const QString &value)
{
	menuName = value;
}

bool MenuItem::getIsChecked() const
{
	return isChecked;
}

void MenuItem::setIsChecked(bool value)
{
	isChecked = value;
}

