#include "actionview.h"

#include <QPushButton>
#include <QFormLayout>
#include <QLineEdit>
#include <QIntValidator>
#include <QCheckBox>
#include <QSpinBox>

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

void ActionView::addItem(QString title, QList<BatchProperty> contentType)
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
        for(BatchProperty property : contentType){

            switch(property.value.type()){
            case QVariant::Int:{
                QSpinBox *input = new QSpinBox();
                input->setRange(0, std::numeric_limits<int>::max());
                input->setValue(property.value.toInt());
                input->setProperty("index", actionItem.widgets.count());
                contentLayout->addRow(property.name, input);
                actionItem.widgets.append(input);
                actionItem.properties.append(property);
                connect(input, SIGNAL(valueChanged(int)), this, SLOT(intSpinBoxChanged(int)));
            }
            break;
            case QVariant::Double:{
                QDoubleSpinBox *input = new QDoubleSpinBox();
                input->setRange(-10e+4, 10e+4);
                input->setProperty("index", actionItem.widgets.count());
                input->setValue(property.value.toDouble());
                contentLayout->addRow(property.name, input);
                actionItem.widgets.append(input);
                actionItem.properties.append(property);
                connect(input, SIGNAL(valueChanged(double)), this, SLOT(doubleSpinBoxChanged(double)));
            }
            break;
            case QVariant::String:{
                QLineEdit *input = new QLineEdit();
                input->setProperty("index", actionItem.widgets.count());
                input->setText(property.value.toString());
                contentLayout->addRow(property.name, input);
                actionItem.widgets.append(input);
                actionItem.properties.append(property);
                connect(input, SIGNAL(textEdited(QString)), this, SLOT(lineEditChanged(QString)));
            }
            break;
            case QVariant::Bool:{
                QCheckBox *input = new QCheckBox();
                input->setProperty("index", actionItem.widgets.count());
                input->setChecked(property.value.toBool());
                contentLayout->addRow(property.name, input);
                actionItem.widgets.append(input);
                actionItem.properties.append(property);
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

QList<BatchProperty> ActionView::data(int index)
{
    return content[index].properties;
}

ActionItem ActionView::item(int index)
{
    return content[index];
}

void ActionView::setData(int index, QString key, QVariant value)
{
    BatchProperty *property;
    QWidget* widget;
    for(BatchProperty &prop : content[index].properties){
        if(prop.name == key){
            property = &prop;
            widget = content[index].widgets[content[index].properties.indexOf(prop)];
        }
    }

    switch(property->value.type()){
    case QVariant::Int:
        dynamic_cast<QSpinBox*>(widget)->setValue(value.toInt());
    break;
    case QVariant::Double:
        dynamic_cast<QDoubleSpinBox*>(widget)->setValue(value.toDouble());
    break;
    case QVariant::String:
        dynamic_cast<QLineEdit*>(widget)->setText(value.toString());
    break;
    case QVariant::Bool:
        dynamic_cast<QCheckBox*>(widget)->setChecked(value.toBool());
    break;
    default:
    break;
    }
    property->value = value;
}

int ActionView::count()
{
    return content.count();
}

QString ActionView::title(int index)
{
    return content[index].title;
}

void ActionView::clear()
{
    content.clear();
    for(QWidget* widget : contentWidgets){
        delete widget;
    }
    contentWidgets.clear();
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
    content[contentWidgets.indexOf(item)].properties[sender()->property("index").toInt()].value = text;
}

void ActionView::checkboxChanged(bool state)
{
    QWidget *item = dynamic_cast<QWidget*>(sender()->parent()->parent());
    content[contentWidgets.indexOf(item)].properties[sender()->property("index").toInt()].value = state;
}

void ActionView::intSpinBoxChanged(int value)
{
    QWidget *item = dynamic_cast<QWidget*>(sender()->parent()->parent());
    content[contentWidgets.indexOf(item)].properties[sender()->property("index").toInt()].value = value;
}

void ActionView::doubleSpinBoxChanged(double value)
{
    QWidget *item = dynamic_cast<QWidget*>(sender()->parent()->parent());
    content[contentWidgets.indexOf(item)].properties[sender()->property("index").toInt()].value = value;
}
