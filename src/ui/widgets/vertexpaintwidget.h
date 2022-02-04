#ifndef VERTEXPAINTWIDGET_H
#define VERTEXPAINTWIDGET_H

#include <QWidget>
#include <glview.h>

namespace Ui {
class PaintSettingsWidget;
}

class PaintSettingsWidget final : public QWidget
{
	Q_OBJECT

public:
	explicit PaintSettingsWidget(QWidget *parent = nullptr);
	~PaintSettingsWidget();

public slots:
	void setValue( const GLView::PaintSettings& value );

signals:
	void valueChanged( const GLView::PaintSettings& value );

protected slots:
	void setColorFromHex();
	void setColorFromWheel();
	void setPreviewFromValue();
	void setHexFromColor();
	void setWheelFromColor();
	void updateValue();

private:
	Ui::PaintSettingsWidget *ui;
	GLView::PaintSettings value_;
	bool supressUpdate_;
	bool supressHexUpdate_;
	bool supressWheelUpdate_;
};

#endif // VERTEXPAINTWIDGET_H
