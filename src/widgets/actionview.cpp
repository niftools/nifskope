#include "actionview.h"

#include <QPushButton>
#include <QFormLayout>
#include <QLineEdit>
#include <QIntValidator>
#include <QCheckBox>

#include <limits>


ActionView::ActionView(QWidget *parent) : QWidget(parent)
{
    mainLayout = new QVBoxLayout();
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setMargin(0);
    this->setLayout(mainLayout);
}

ActionView::~ActionView()
{
}

void ActionView::addItem(QString title, QMap<QString, QVariant::Type> contentType)
{
    QWidget *itemContainer = new QWidget();//Container needed to get the content later
    QVBoxLayout *layout = new QVBoxLayout();
    layout->setMargin(0);
    itemContainer->setLayout(layout);
    //Header
    QHBoxLayout *headerLayout = new QHBoxLayout();
    headerLayout->setMargin(0);
    headerLayout->setSpacing(0);
    QPushButton *titleButton = new QPushButton("▼"+title);
    titleButton->setStyleSheet("background-color: #3399FF; border:none; text-align: left; font: bold");
    titleButton->setFixedHeight(20);
    connect(titleButton, SIGNAL(pressed()), this, SLOT(hideShowContent()));
    QPushButton *deleteButton = new QPushButton("▬");
    deleteButton->setStyleSheet("background-color: #3399FF; border:none; color: red");
    deleteButton->setFixedSize(20, 20);
    connect(deleteButton, SIGNAL(pressed()), this, SLOT(deleteItem()));
    //Content
    headerLayout->addWidget(titleButton);
    headerLayout->addWidget(deleteButton);
    layout->addLayout(headerLayout);
    QWidget *containerWidget = new QWidget();//Container needed for hiding/showing
    containerWidget->setObjectName("content");
    QFormLayout *contentLayout = new QFormLayout();
    containerWidget->setLayout(contentLayout);
    contentLayout->setMargin(9);
    layout->addWidget(containerWidget);
    ActionItem actionItem;
    actionItem.title = title;
    actionItem.expanded = true;

    if(contentType.count() == 0)
        contentLayout->addRow(tr("No properties available"), new QWidget());
    else
        for(int i = 0; i < contentType.count(); i++){
            QString key = contentType.firstKey();
            QVariant::Type value = contentType.value(key);
            contentType.remove(key);
            actionItem.properties.insert(key, QVariant());

            switch(value){
            case QVariant::Int:{
                QLineEdit *input = new QLineEdit();
                input->setValidator( new QIntValidator(std::numeric_limits<int>::min(), std::numeric_limits<int>::max()));
                input->setProperty("name", key);
                contentLayout->addRow(key, input);
                connect(input, SIGNAL(textChanged(QString)), this, SLOT(lineEditChanged(QString)));
            }
            break;
            case QVariant::String:{
                QLineEdit *input = new QLineEdit();
                input->setProperty("name", key);
                contentLayout->addRow(key, input);
                connect(input, SIGNAL(textChanged(QString)), this, SLOT(lineEditChanged(QString)));
            }
            break;
            case QVariant::Bool:{
                QCheckBox *input = new QCheckBox();
                input->setProperty("name", key);
                contentLayout->addRow(key, input);
                connect(input, SIGNAL(clicked(bool)), this, SLOT(checkboxChanged(bool)));
            }
            break;
            default:
            break;
            }
        }

    mainLayout->addWidget(itemContainer);
    contentWidgets.append(itemContainer);
    content.append(actionItem);
}

QMap<QString, QVariant> ActionView::data(int index)
{
    return content[index].properties;
}

int ActionView::count()
{
    return content.count();
}

void ActionView::hideShowContent()
{
    QWidget *contentWidget = sender()->parent()->findChild<QWidget*>("content");
    QString text = dynamic_cast<QPushButton*>(sender())->text().remove(0,1);
    if(contentWidget->isHidden()){
        contentWidget->show();
        ((QPushButton*)sender())->setText("▼" + text);
        content[contentWidgets.indexOf(dynamic_cast<QWidget*>(sender()->parent()))].expanded = true;
    } else {
        contentWidget->hide();
        ((QPushButton*)sender())->setText("►" + text);
        content[contentWidgets.indexOf(dynamic_cast<QWidget*>(sender()->parent()))].expanded = false;
    }
}

void ActionView::deleteItem()
{
    content.removeAt(contentWidgets.indexOf(dynamic_cast<QWidget*>(sender()->parent())));
    contentWidgets.removeAt(contentWidgets.indexOf(dynamic_cast<QWidget*>(sender()->parent())));
    delete sender()->parent();
}

void ActionView::lineEditChanged(QString text)
{
    QWidget *item = dynamic_cast<QWidget*>(sender()->parent()->parent());
    content[contentWidgets.indexOf(item)].properties.insert(sender()->property("name").toString(), text);
}

void ActionView::checkboxChanged(bool state)
{
    QWidget *item = dynamic_cast<QWidget*>(sender()->parent()->parent());
    content[contentWidgets.indexOf(item)].properties.insert(sender()->property("name").toString(), state);
}
