#ifndef ACTIONVIEW_H
#define ACTIONVIEW_H

#include "spellbook.h"

#include <QWidget>
#include <QVariant>
#include <QScrollArea>
#include <QVBoxLayout>

struct ActionItem
{
    QString title;
    bool expanded;
    QList<BatchProperty> properties;
    QList<QWidget*> widgets;
};

class ActionView : public QWidget
{
    Q_OBJECT
public:
    explicit ActionView(QWidget *parent = 0);
    ~ActionView();
    void addItem(QString title, QList<BatchProperty> contentType = QList<BatchProperty>());
    QList<BatchProperty> data(int index);
    ActionItem item(int index);
    void setData(int index, QString key, QVariant value);
    int count();
    QString title(int index);
    void clear();

private:
    QVBoxLayout *mainLayout;
    QList<ActionItem> content;
    QList<QWidget*> contentWidgets;

signals:

public slots:
    void hideShowContent();
    void deleteItem();

    void lineEditChanged(QString text);
    void checkboxChanged(bool state);
    void intSpinBoxChanged(int value);
    void doubleSpinBoxChanged(double value);
};

#endif // ACTIONVIEW_H
