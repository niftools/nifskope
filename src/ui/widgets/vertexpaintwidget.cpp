#include "vertexpaintwidget.h"
#include "ui_vertexpaintwidget.h"
#include <algorithm>

PaintSettingsWidget::PaintSettingsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PaintSettingsWidget)
{
    ui->setupUi(this);
    supressUpdate_ = false;

    ui->opacityMode->addItems({"Color", "Alpha", "Color + Alpha"});
    ui->blendMode->setCurrentIndex(0);
    ui->blendMode->addItems({"Normal","Add","Multiply"});
    ui->blendMode->setCurrentIndex(0);

    // Hookup change events to update the paint config
    connect(ui->brushSize, SIGNAL(valueChanged(int)), this, SLOT(updateValue()));
    connect(ui->colorR, SIGNAL(valueChanged(double)), this, SLOT(updateValue()));
    connect(ui->colorG, SIGNAL(valueChanged(double)), this, SLOT(updateValue()));
    connect(ui->colorB, SIGNAL(valueChanged(double)), this, SLOT(updateValue()));
    connect(ui->colorA, SIGNAL(valueChanged(double)), this, SLOT(updateValue()));
    connect(ui->opacity, SIGNAL(valueChanged(int)), this, SLOT(updateValue()));
    connect(ui->opacityMode, SIGNAL(currentIndexChanged(int)), this, SLOT(updateValue()));
    connect(ui->blendMode, SIGNAL(currentIndexChanged(int)), this, SLOT(updateValue()));
}

void PaintSettingsWidget::setValue(const GLView::PaintSettings& value)
{
    supressUpdate_ = true;

    // Read all the settings that we can directly into controls
    ui->brushSize->setValue(value.brushSize);
    ui->colorR->setValue(value.brushColor.red());
    ui->colorG->setValue(value.brushColor.green());
    ui->colorB->setValue(value.brushColor.blue());
    ui->colorA->setValue(value.brushColor.alpha());
    ui->blendMode->setCurrentIndex((int)value.brushMode);

    // Infer opacity from brushOpacity
    int opacity = qBound<int>(std::round(
                  (value.brushOpacity.red() +
                   value.brushOpacity.green() +
                   value.brushOpacity.blue() +
                   value.brushOpacity.alpha() ) / 4.0f * 255.0f), 0, 255);

    ui->opacity->setValue(opacity);

    // Infer the paintOpacityMode from the provided brushOpacity
    if (value.brushOpacity.red() != 0 &&
        value.brushOpacity.green() != 0 &&
        value.brushOpacity.blue() != 0 &&
        value.brushOpacity.alpha() == 0)
    {
        ui->opacityMode->setCurrentIndex(0);
    }
    else if (value.brushOpacity.red() == 0 &&
             value.brushOpacity.green() == 0 &&
             value.brushOpacity.blue() == 0 &&
             value.brushOpacity.alpha() != 0)
         {
             ui->opacityMode->setCurrentIndex(1);
         }
    else
    {
        ui->opacityMode->setCurrentIndex(2);
    }

    supressUpdate_ = false;

    // With all controls populated, issue a manual updateValue
    updateValue();
}

void PaintSettingsWidget::updateValue()
{
    if (supressUpdate_)
        return;

    value_.brushColor = Color4(ui->colorR->value(),
                                 ui->colorG->value(),
                                 ui->colorB->value(),
                                 ui->colorA->value());

    value_.brushSize = (float)ui->brushSize->value();

    float opacity = (float)ui->opacity->value() / 255.0f;
    if (ui->opacityMode->currentIndex() == 0)  // Color only
    {
        value_.brushOpacity = Color4(opacity,opacity,opacity,0.0f);
    }
    else if (ui->opacityMode->currentIndex() == 1)  // Alpha only
    {
        value_.brushOpacity = Color4(0,0,0,opacity);
    }
    else if (ui->opacityMode->currentIndex() == 2)  // Color+Alpha
    {
        value_.brushOpacity = Color4(opacity,opacity,opacity,opacity);
    }

    value_.brushMode = (GLView::PaintBlendMode)ui->blendMode->currentIndex();

    emit valueChanged(value_);
}

PaintSettingsWidget::~PaintSettingsWidget()
{
    delete ui;
}
