#include "ElaFooterDelegate.h"

#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>

#include "ElaApplication.h"
#include "ElaFooterModel.h"
#include "ElaListView.h"
#include "ElaNavigationNode.h"
#include "private/ElaFooterDelegatePrivate.h"
Q_PRIVATE_CREATE_Q_CPP(ElaFooterDelegate, ElaListView*, ElaListView);
ElaFooterDelegate::ElaFooterDelegate(QObject* parent)
    : QStyledItemDelegate{parent}, d_ptr(new ElaFooterDelegatePrivate())
{
    Q_D(ElaFooterDelegate);
    d->q_ptr = this;
    d->_pElaListView = nullptr;
    d->_hovergradient = new QLinearGradient(0, 0, 290, 38);
    d->_hovergradient->setColorAt(0, QColor(0xE9, 0xE9, 0xF0));
    d->_hovergradient->setColorAt(1, QColor(0xEA, 0xE9, 0xF0));
    d->_selectedgradient = new QLinearGradient(0, 0, 290, 38);
    d->_selectedgradient->setColorAt(0, QColor(0xE9, 0xE9, 0xF0));
    d->_selectedgradient->setColorAt(1, QColor(0xEA, 0xE9, 0xF0));
    d->_selectedHovergradient = new QLinearGradient(0, 0, 290, 38);
    d->_selectedHovergradient->setColorAt(0, QColor(0xEC, 0xEC, 0xF3));
    d->_selectedHovergradient->setColorAt(1, QColor(0xED, 0xEC, 0xF3));
    d->_themeMode = eApp->getThemeMode();
    connect(eApp, &ElaApplication::themeModeChanged, d, &ElaFooterDelegatePrivate::onThemeChanged);
    setProperty("lastSelectMarkTop", 10.0);
    setProperty("lastSelectMarkBottom", 10.0);
    setProperty("selectMarkTop", 10.0);
    setProperty("selectMarkBottom", 10.0);
    // Mark向上
    d->_lastSelectMarkTopAnimation = new QPropertyAnimation(this, "lastSelectMarkTop");
    connect(d->_lastSelectMarkTopAnimation, &QPropertyAnimation::valueChanged, this, [=](const QVariant& value) {
                d->_lastSelectMarkTop = value.toReal();
                d->_pElaListView->viewport()->update(); });
    d->_lastSelectMarkTopAnimation->setDuration(300);
    d->_lastSelectMarkTopAnimation->setEasingCurve(QEasingCurve::InOutSine);

    d->_selectMarkBottomAnimation = new QPropertyAnimation(this, "selectMarkBottom");
    connect(d->_selectMarkBottomAnimation, &QPropertyAnimation::valueChanged, this, [=](const QVariant& value) {
                d->_selectMarkBottom = value.toReal();
                d->_pElaListView->viewport()->update(); });
    d->_selectMarkBottomAnimation->setDuration(300);
    d->_selectMarkBottomAnimation->setEasingCurve(QEasingCurve::InOutSine);
    connect(d->_lastSelectMarkTopAnimation, &QPropertyAnimation::finished, this, [=]() {
                d->_isSelectMarkDisplay = true;
                d->_lastSelectedNode = nullptr;
                d->_selectMarkBottomAnimation->setStartValue(0);
                d->_selectMarkBottomAnimation->setEndValue(10);
                d->_selectMarkBottomAnimation->start(); });

    // Mark向下
    d->_lastSelectMarkBottomAnimation = new QPropertyAnimation(this, "lastSelectMarkBottom");
    connect(d->_lastSelectMarkBottomAnimation, &QPropertyAnimation::valueChanged, this, [=](const QVariant& value) {
                d->_lastSelectMarkBottom =  value.toReal();
                d->_pElaListView->viewport()->update(); });
    d->_lastSelectMarkBottomAnimation->setDuration(300);
    d->_lastSelectMarkBottomAnimation->setEasingCurve(QEasingCurve::InOutSine);

    d->_selectMarkTopAnimation = new QPropertyAnimation(this, "selectMarkTop");
    connect(d->_selectMarkTopAnimation, &QPropertyAnimation::valueChanged, this, [=](const QVariant& value) {
                d->_selectMarkTop =  value.toReal();
                d->_pElaListView->viewport()->update(); });
    d->_selectMarkTopAnimation->setDuration(300);
    d->_selectMarkTopAnimation->setEasingCurve(QEasingCurve::InOutSine);
    connect(d->_lastSelectMarkBottomAnimation, &QPropertyAnimation::finished, this, [=]() {
                d->_isSelectMarkDisplay = true;
                d->_lastSelectedNode = nullptr;
                d->_selectMarkTopAnimation->setStartValue(0);
                d->_selectMarkTopAnimation->setEndValue(10);
                d->_selectMarkTopAnimation->start(); });
}

ElaFooterDelegate::~ElaFooterDelegate()
{
}

void ElaFooterDelegate::navigationNodeStateChange(QVariantMap data)
{
    Q_D(ElaFooterDelegate);
    if (data.contains("SelectMarkChanged"))
    {
        d->_lastSelectedNode = data.value("LastSelectedNode").value<ElaNavigationNode*>();
        ElaNavigationNode* selectedNode = data.value("SelectedNode").value<ElaNavigationNode*>();
        bool direction = d->_compareItemY(selectedNode, d->_lastSelectedNode);
        d->_lastSelectMarkTop = 10;
        d->_lastSelectMarkBottom = 10;
        d->_selectMarkTop = 10;
        d->_selectMarkBottom = 10;
        if (direction)
        {
            d->_lastSelectMarkTopAnimation->setStartValue(10);
            d->_lastSelectMarkTopAnimation->setEndValue(0);
            d->_lastSelectMarkTopAnimation->start();
            d->_lastSelectMarkBottomAnimation->stop();
            d->_selectMarkTopAnimation->stop();
            d->_isSelectMarkDisplay = false;
        }
        else
        {
            d->_lastSelectMarkBottomAnimation->setStartValue(10);
            d->_lastSelectMarkBottomAnimation->setEndValue(0);
            d->_lastSelectMarkBottomAnimation->start();
            d->_lastSelectMarkTopAnimation->stop();
            d->_selectMarkBottomAnimation->stop();
            d->_isSelectMarkDisplay = false;
        }
    }
}

void ElaFooterDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    Q_D(const ElaFooterDelegate);
    QStyleOptionViewItem viewOption(option);
    initStyleOption(&viewOption, index);
    ElaFooterModel* model = dynamic_cast<ElaFooterModel*>(const_cast<QAbstractItemModel*>(index.model()));
    ElaNavigationNode* node = index.data(Qt::UserRole).value<ElaNavigationNode*>();
    if (option.state.testFlag(QStyle::State_HasFocus))
    {
        viewOption.state &= ~QStyle::State_HasFocus;
    }
    QStyledItemDelegate::paint(painter, viewOption, index);
    // 背景绘制
    QRect itemRect = option.rect;
    painter->save();
    painter->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);
    itemRect.setTop(itemRect.top() + 2);
    itemRect.setBottom(itemRect.bottom() - 2);
    QPainterPath path;
    path.addRoundedRect(itemRect, 8, 8);
    if (option.showDecorationSelected && (option.state & QStyle::State_Selected))
    {
        if (option.state & QStyle::State_MouseOver)
        {
            // 选中时覆盖
            painter->fillPath(path, *d->_selectedHovergradient);
        }
        else
        {
            // 选中
            painter->fillPath(path, *d->_selectedgradient);
        }
    }
    else
    {
        if (option.state & QStyle::State_MouseOver)
        {
            // 覆盖时颜色
            painter->fillPath(path, *d->_hovergradient);
        }
    }
    painter->restore();

    painter->save();
    painter->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);
    itemRect = option.rect;
    // 图标绘制
    if (index.row() == 0)
    {
        painter->setPen(d->_themeMode == ElaApplicationType::Light ? QColor(0xD2, 0xD2, 0xD2) : QColor(0x50, 0x50, 0x50));

        painter->drawLine(itemRect.x(), itemRect.y() + 1, itemRect.x() + itemRect.width(), itemRect.y() + 1);
    }
    painter->setPen(d->_themeMode == ElaApplicationType::Light ? Qt::black : Qt::white);
    if (node->getAwesome() != ElaIconType::None)
    {
        QFont iconFont = QFont("ElaAwesome");
        iconFont.setPixelSize(17);
        painter->setFont(iconFont);
        painter->drawText(itemRect.x() + 11, itemRect.y() + 26, QChar((unsigned short)node->getAwesome()));
    }
    // 展开图标 KeyPoints
    if (node->getIsExpanderNode())
    {
        if (node->getIsHasChild())
        {
            if (node->getIsExpanded())
            {
                // 展开
                painter->drawText(247, itemRect.y() + 26, QChar((unsigned short)ElaIconType::AngleUp));
            }
            else
            {
                // 未展开
                painter->drawText(247, itemRect.y() + 26, QChar((unsigned short)ElaIconType::AngleDown));
            }
        }
        if (node->getIsChildHasKeyPoints())
        {
            painter->save();
            painter->setPen(Qt::NoPen);
            painter->setBrush(QColor(0xFF, 0x4D, 0x4F));
            painter->drawEllipse(QPoint(264, itemRect.y() + 12), 3, 3);
            painter->restore();
        }
    }
    else
    {
        int keyPoints = node->getKeyPoints();
        if (keyPoints)
        {
            // KeyPoints
            painter->save();
            painter->setPen(Qt::NoPen);
            painter->setBrush(Qt::white);
            painter->drawEllipse(QPoint(255, itemRect.y() + itemRect.height() / 2), 10, 10);
            painter->setBrush(QColor(0xFF, 0x4D, 0x4F));
            painter->drawEllipse(QPoint(255, itemRect.y() + itemRect.height() / 2), 9, 9);
            painter->setPen(QPen(Qt::white, 2));
            QFont font = QFont("Microsoft YaHei", 10);
            font.setHintingPreference(QFont::PreferNoHinting);
            font.setBold(true);
            if (keyPoints > 99)
            {
                keyPoints = 99;
            }
            if (keyPoints > 9)
            {
                font.setPixelSize(11);
            }
            else
            {
                font.setPixelSize(12);
            }
            painter->setFont(font);
            painter->drawText(keyPoints > 9 ? 248 : 251, itemRect.y() + itemRect.height() / 2 + 4, QString::number(keyPoints));
            painter->restore();
        }
    }

    // 文字绘制
    QFont titlefont("Microsoft YaHei", 10);
    titlefont.setHintingPreference(QFont::PreferNoHinting);
    painter->setFont(titlefont);
    painter->setPen(d->_themeMode == ElaApplicationType::Light ? Qt::black : Qt::white);
    painter->drawText(itemRect.x() + 37, itemRect.y() + 25, node->getNodeTitle());
    // 选中特效
    if (d->_isSelectMarkDisplay && (node == model->getSelectedNode()))
    {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0x0E, 0x6F, 0xC3));
        painter->drawRoundedRect(QRectF(itemRect.x() + 3, itemRect.y() + d->_selectMarkTop, 3, itemRect.height() - d->_selectMarkTop - d->_selectMarkBottom), 3, 3);
    }
    if (node == d->_lastSelectedNode)
    {
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0x0E, 0x6F, 0xC3));
        painter->drawRoundedRect(QRectF(itemRect.x() + 3, itemRect.y() + d->_lastSelectMarkTop, 3, itemRect.height() - d->_lastSelectMarkTop - d->_lastSelectMarkBottom), 3, 3);
    }
    painter->restore();
}

QSize ElaFooterDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    size.setHeight(40);
    return size;
}
