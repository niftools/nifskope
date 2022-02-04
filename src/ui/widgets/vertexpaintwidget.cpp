#include "vertexpaintwidget.h"
#include "ui_vertexpaintwidget.h"
#include <algorithm>

enum OpacityMode : int
{
	Color = 0,
	Alpha = 1,
	ColorAndAlpha = 2
};

PaintSettingsWidget::PaintSettingsWidget(QWidget *parent) :
	QWidget(parent),
	ui(new Ui::PaintSettingsWidget)
{
	ui->setupUi(this);
	supressUpdate_ = false;
	supressHexUpdate_ = false;
	supressWheelUpdate_ = false;

	ui->opacityMode->addItems({"Color", "Alpha", "Color + Alpha"});
	ui->blendMode->setCurrentIndex(0);
	ui->blendMode->addItems({"Normal","Add","Multiply"});
	ui->blendMode->setCurrentIndex(OpacityMode::Color);
	setHexFromColor();
	setPreviewFromValue();
	setWheelFromColor();

	// Hookup change events to handle sync of the hex and double color reps
	connect(ui->colorR, SIGNAL(valueChanged(double)), this, SLOT(setHexFromColor()));
	connect(ui->colorG, SIGNAL(valueChanged(double)), this, SLOT(setHexFromColor()));
	connect(ui->colorB, SIGNAL(valueChanged(double)), this, SLOT(setHexFromColor()));
	connect(ui->colorA, SIGNAL(valueChanged(double)), this, SLOT(setHexFromColor()));
	connect(ui->colorR, SIGNAL(valueChanged(double)), this, SLOT(setWheelFromColor()));
	connect(ui->colorG, SIGNAL(valueChanged(double)), this, SLOT(setWheelFromColor()));
	connect(ui->colorB, SIGNAL(valueChanged(double)), this, SLOT(setWheelFromColor()));
	connect(ui->colorA, SIGNAL(valueChanged(double)), this, SLOT(setWheelFromColor()));
	connect(ui->colorHex, SIGNAL(textChanged(QString)), this, SLOT(setColorFromHex()));
	connect(ui->colorWheel, SIGNAL(sigColor(QColor)), this, SLOT(setColorFromWheel()));

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

static int colorDoubleToInt(double value)
{
   return qBound<int>((int)std::round(value*255.0), 0, 255);
}

static double colorIntToDouble(int value)
{
   return qBound<double>((double)value/255.0, 0.0, 1.0);
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
	setHexFromColor();
	setWheelFromColor();
	ui->blendMode->setCurrentIndex((int)value.brushMode);

	// Infer the opacity from the provided brushOpacity
	if (value.brushOpacity.red() != 0 &&
		value.brushOpacity.green() != 0 &&
		value.brushOpacity.blue() != 0 &&
		value.brushOpacity.alpha() == 0)
	{
		ui->opacityMode->setCurrentIndex(OpacityMode::Color);
		ui->opacity->setValue(colorDoubleToInt(value.brushOpacity.red()));
	}
	else if (value.brushOpacity.red() == 0 &&
			 value.brushOpacity.green() == 0 &&
			 value.brushOpacity.blue() == 0 &&
			 value.brushOpacity.alpha() != 0)
		 {
			 ui->opacityMode->setCurrentIndex(OpacityMode::Alpha);
			 ui->opacity->setValue(colorDoubleToInt(value.brushOpacity.alpha()));
		 }
	else
	{
		ui->opacityMode->setCurrentIndex(OpacityMode::ColorAndAlpha);
		ui->opacity->setValue(colorDoubleToInt(value.brushOpacity.red()));
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
	if (ui->opacityMode->currentIndex() == OpacityMode::Color)  // Color only
	{
		value_.brushOpacity = Color4(opacity,opacity,opacity,0.0f);
	}
	else if (ui->opacityMode->currentIndex() == OpacityMode::Alpha)  // Alpha only
	{
		value_.brushOpacity = Color4(0,0,0,opacity);
	}
	else if (ui->opacityMode->currentIndex() == OpacityMode::ColorAndAlpha)  // Color+Alpha
	{
		value_.brushOpacity = Color4(opacity,opacity,opacity,opacity);
	}

	value_.brushMode = (GLView::PaintBlendMode)ui->blendMode->currentIndex();

	// Update some internal stuff too...
	setPreviewFromValue();

	emit valueChanged(value_);
}

void PaintSettingsWidget::setColorFromHex()
{
	QString hex = ui->colorHex->text();
	bool ok;

	supressHexUpdate_ = true;

	// Red
	if (hex.length() >= 2)
	{
		int i = hex.midRef(0,2).toInt(&ok, 16);
		if (ok)
			ui->colorR->setValue(colorIntToDouble(i));
	}
	else
	{
		ui->colorR->setValue(0.0);
		ui->colorG->setValue(0.0);
		ui->colorB->setValue(0.0);
		ui->colorA->setValue(1.0);
	}

	// Green
	if (hex.length() >= 4)
	{
		int i = hex.midRef(2,2).toInt(&ok, 16);
		if (ok)
			ui->colorG->setValue(colorIntToDouble(i));
	}
	else
	{
		ui->colorG->setValue(0.0);
		ui->colorB->setValue(0.0);
		ui->colorA->setValue(1.0);
	}

	// Blue
	if (hex.length() >= 6)
	{
		int i = hex.midRef(4,2).toInt(&ok, 16);
		if (ok)
			ui->colorB->setValue(colorIntToDouble(i));
	}
	else
	{
		ui->colorB->setValue(0.0);
		ui->colorA->setValue(1.0);
	}

	// Alpha
	if (hex.length() >= 8)
	{
		int i = hex.midRef(6,2).toInt(&ok, 16);
		if (ok)
			ui->colorA->setValue(colorIntToDouble(i));
	}
	else
	{
		ui->colorA->setValue(1.0);
	}

	supressHexUpdate_ = false;
}

void PaintSettingsWidget::setHexFromColor()
{
	if (supressHexUpdate_)
		return;

	// Convert the color components from doubles [0.0 , 1.0] to ints [0 , 255]
	int r = colorDoubleToInt(ui->colorR->value());
	int g = colorDoubleToInt(ui->colorG->value());
	int b = colorDoubleToInt(ui->colorB->value());
	int a = colorDoubleToInt(ui->colorA->value());

	// Format the rgba int components into an 8 character hex string like FFAA00FF
	QString result = QString("%1").arg(r, 2, 16, QLatin1Char('0')).toUpper() +
					 QString("%1").arg(g, 2, 16, QLatin1Char('0')).toUpper() +
					 QString("%1").arg(b, 2, 16, QLatin1Char('0')).toUpper() +
					 QString("%1").arg(a, 2, 16, QLatin1Char('0')).toUpper();
	ui->colorHex->setText(result);
}

void PaintSettingsWidget::setColorFromWheel()
{
	supressWheelUpdate_ = true;

	QColor color = ui->colorWheel->getColor();
	ui->colorR->setValue(color.redF());
	ui->colorG->setValue(color.greenF());
	ui->colorB->setValue(color.blueF());

	supressWheelUpdate_ = false;
}

void PaintSettingsWidget::setWheelFromColor()
{
	if (supressWheelUpdate_)
		return;

	int r = colorDoubleToInt(ui->colorR->value());
	int g = colorDoubleToInt(ui->colorG->value());
	int b = colorDoubleToInt(ui->colorB->value());
	int a = colorDoubleToInt(ui->colorA->value());

	ui->colorWheel->setColor(QColor::fromRgb(r,g,b,a));
}

void PaintSettingsWidget::setPreviewFromValue()
{
	QPalette pal = QPalette();
	pal.setColor(QPalette::Button, QColor::fromRgbF(value_.brushColor.red(),
													value_.brushColor.green(),
													value_.brushColor.blue(),
													1.0f));
	ui->colorPreview->setPalette(pal);

	QPalette palA = QPalette();
	palA.setColor(QPalette::Button, QColor::fromRgbF(value_.brushColor.red(),
													value_.brushColor.green(),
													value_.brushColor.blue(),
													value_.brushColor.alpha()));
	ui->colorPreviewAlpha->setPalette(palA);
}

PaintSettingsWidget::~PaintSettingsWidget()
{
	delete ui;
}
