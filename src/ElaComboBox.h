#ifndef ELACOMBOBOX_H
#define ELACOMBOBOX_H

#include <QComboBox>

#include "stdafx.h"

class ElaComboBoxPrivate;
class ElaComboBox : public QComboBox
{
    Q_OBJECT
    Q_Q_CREATE(ElaComboBox);
    Q_PROPERTY_CREATE_Q_H(int, BorderRadius)
public:
    explicit ElaComboBox(QWidget* parent = nullptr);
    ~ElaComboBox();
    void setMaxVisibleItems(int maxItems);

protected:
    void paintEvent(QPaintEvent* e) override;
    virtual void showPopup() override;
    virtual void hidePopup() override;
};

#endif // ELACOMBOBOX_H
