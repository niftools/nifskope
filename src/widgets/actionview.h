#ifndef ACTIONVIEW_H
#define ACTIONVIEW_H

#include <QWidget>
#include <QVariant>
#include <QScrollArea>
#include <QVBoxLayout>

struct ActionItem
{
    QString title;
    bool expanded;
    QMap<QString, QVariant> properties;
};

class ActionView : public QWidget
{
    Q_OBJECT
public:
    explicit ActionView(QWidget *parent = 0);
    ~ActionView();
    void addItem(QString title, QMap<QString, QVariant::Type> contentType = QMap<QString, QVariant::Type>());
    QMap<QString, QVariant> data(int index);
    int count();
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
};

#endif // ACTIONVIEW_H
