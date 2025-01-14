#include "ElaWindowPrivate.h"

#include <QImage>
#include <QPropertyAnimation>
#include <QStackedWidget>
#include <QTimer>
#include <QVBoxLayout>

#include "ElaAppBar.h"
#include "ElaAppBarPrivate.h"
#include "ElaApplication.h"
#include "ElaNavigationBar.h"
#include "ElaThemeAnimationWidget.h"
#include "ElaWindow.h"
ElaWindowPrivate::ElaWindowPrivate(QObject* parent)
    : QObject{parent}
{
}

ElaWindowPrivate::~ElaWindowPrivate()
{
}

void ElaWindowPrivate::onNavigationButtonClicked()
{
    int contentMargin = _contentsMargins;
    int appBarHeight = _appBar->height();
    if (_isWMClickedAnimationFinished)
    {
        _resetWindowLayout(true);
        _navigationBar->setDisplayMode(ElaNavigationType::Maximal, false);
        _navigationBar->move(-_navigationBar->width(), _navigationBar->pos().y());
        _navigationBar->resize(_navigationBar->width(), _centerStackedWidget->height());
        QPropertyAnimation* navigationMoveAnimation = new QPropertyAnimation(_navigationBar, "pos");
        connect(navigationMoveAnimation, &QPropertyAnimation::finished, this, [=]() {
            _isNavigationBarExpanded = true;
        });
        navigationMoveAnimation->setEasingCurve(QEasingCurve::InOutSine);
        navigationMoveAnimation->setDuration(300);
        navigationMoveAnimation->setStartValue(_navigationBar->pos());
        navigationMoveAnimation->setEndValue(QPoint(contentMargin, appBarHeight + contentMargin));
        navigationMoveAnimation->start(QAbstractAnimation::DeleteWhenStopped);
        _isWMClickedAnimationFinished = false;
    }
}

void ElaWindowPrivate::onWMWindowClickedEvent(QVariantMap data)
{
    ElaAppBarType::WMMouseActionType actionType = data.value("WMClickType").value<ElaAppBarType::WMMouseActionType>();
    if (actionType == ElaAppBarType::WMLBUTTONDBLCLK || actionType == ElaAppBarType::WMLBUTTONUP || actionType == ElaAppBarType::WMNCLBUTTONDOWN)
    {
        if (ElaApplication::containsCursorToItem(_navigationBar))
        {
            return;
        }
        if (_isNavigationBarExpanded)
        {
            QPropertyAnimation* navigationMoveAnimation = new QPropertyAnimation(_navigationBar, "pos");
            connect(navigationMoveAnimation, &QPropertyAnimation::finished, this, [=]() {
                _isWMClickedAnimationFinished = true;
            });
            navigationMoveAnimation->setEasingCurve(QEasingCurve::InOutSine);
            navigationMoveAnimation->setDuration(300);
            navigationMoveAnimation->setStartValue(_navigationBar->pos());
            navigationMoveAnimation->setEndValue(QPoint(-_navigationBar->width() - 5, 35));
            navigationMoveAnimation->start(QAbstractAnimation::DeleteWhenStopped);
            _isNavigationBarExpanded = false;
        }
    }
}

void ElaWindowPrivate::onThemeReadyChange()
{
    Q_Q(ElaWindow);
    // 主题变更绘制窗口
    _appBar->setIsOnlyAllowMinAndClose(true);
    if (!_animationWidget)
    {
        _animationWidget = new ElaThemeAnimationWidget(q);
        connect(_animationWidget, &ElaThemeAnimationWidget::animationFinished, this, [=]() {
            _appBar->setIsOnlyAllowMinAndClose(false);
            _animationWidget = nullptr;
        });
        _animationWidget->move(0, 0);
        ElaApplication* app = eApp;
        _animationWidget->setOldWindowBackground(q->grab(q->rect()).toImage());
        if (app->getThemeMode() == ElaApplicationType::Light)
        {
            _windowLinearGradient->setColorAt(0, QColor(0x1A, 0x1A, 0x1A));
            _windowLinearGradient->setColorAt(1, QColor(0x1A, 0x1A, 0x1A));
            app->setThemeMode(ElaApplicationType::Dark);
        }
        else
        {
            _windowLinearGradient->setColorAt(0, QColor(0xF2, 0xF2, 0xF9));
            _windowLinearGradient->setColorAt(1, QColor(0xF9, 0xEF, 0xF6));
            app->setThemeMode(ElaApplicationType::Light);
        }
        _animationWidget->setNewWindowBackground(q->grab(q->rect()).toImage());
        QPoint centerPos = q->mapFromGlobal(QCursor::pos());
        _animationWidget->setCenter(centerPos);
        qreal topLeftDis = _distance(centerPos, QPoint(0, 0));
        qreal topRightDis = _distance(centerPos, QPoint(q->width(), 0));
        qreal bottomLeftDis = _distance(centerPos, QPoint(0, q->height()));
        qreal bottomRightDis = _distance(centerPos, QPoint(q->width(), q->height()));
        QList<qreal> disList{topLeftDis, topRightDis, bottomLeftDis, bottomRightDis};
        std::sort(disList.begin(), disList.end());
        _animationWidget->setEndRadius(disList[3]);
        _animationWidget->resize(q->width(), q->height());
        _animationWidget->startAnimation(_pThemeChangeTime);
        _animationWidget->show();
    }
}

void ElaWindowPrivate::onDisplayModeChanged()
{
    switch (_pNavigationBarDisplayMode)
    {
    case ElaNavigationType::Auto:
    {
        _appBar->setWindowButtonFlag(ElaAppBarType::NavigationButtonHint, false);
        break;
    }
    case ElaNavigationType::Minimal:
    {
        _navigationBar->setDisplayMode(ElaNavigationType::Minimal, false);
        _appBar->setWindowButtonFlag(ElaAppBarType::NavigationButtonHint);
        break;
    }
    case ElaNavigationType::Compact:
    {
        _navigationBar->setDisplayMode(ElaNavigationType::Compact, false);
        _appBar->setWindowButtonFlag(ElaAppBarType::NavigationButtonHint, false);
        break;
    }
    case ElaNavigationType::Maximal:
    {
        _navigationBar->setDisplayMode(ElaNavigationType::Maximal, false);
        _appBar->setWindowButtonFlag(ElaAppBarType::NavigationButtonHint, false);
        break;
    }
    }
}

void ElaWindowPrivate::onThemeModeChanged(ElaApplicationType::ThemeMode themeMode)
{
    if (eApp->getThemeMode() == ElaApplicationType::Light)
    {
        _windowLinearGradient->setColorAt(0, QColor(0xF2, 0xF2, 0xF9));
        _windowLinearGradient->setColorAt(1, QColor(0xF9, 0xEF, 0xF6));
    }
    else
    {
        _windowLinearGradient->setColorAt(0, QColor(0x1A, 0x1A, 0x1A));
        _windowLinearGradient->setColorAt(1, QColor(0x1A, 0x1A, 0x1A));
    }
}

void ElaWindowPrivate::onNavigationNodeClicked(ElaNavigationType::NavigationNodeType nodeType, QString nodeKey)
{
    int nodeIndex = _routeMap.value(nodeKey);
    if (nodeIndex == -1)
    {
        // 页脚没有绑定页面
        return;
    }
    if (_navigationTargetIndex == nodeIndex || _centerStackedWidget->count() <= nodeIndex)
    {
        return;
    }
    _navigationTargetIndex = nodeIndex;
    QTimer::singleShot(180, this, [=]() {
        QWidget* currentWidget = _centerStackedWidget->widget(nodeIndex);
        QPropertyAnimation* currentWidgetAnimation = new QPropertyAnimation(currentWidget, "pos");
        currentWidgetAnimation->setEasingCurve(QEasingCurve::InOutCubic);
        currentWidgetAnimation->setDuration(280);
        QPoint currentWidgetPos = currentWidget->pos();
        currentWidgetPos.setY(10);
        currentWidgetAnimation->setEndValue(currentWidgetPos);
        currentWidgetPos.setY(currentWidgetPos.y() + 60);
        currentWidgetAnimation->setStartValue(currentWidgetPos);
        _centerStackedWidget->setCurrentIndex(nodeIndex);
        currentWidgetAnimation->start(QAbstractAnimation::DeleteWhenStopped); });
}

void ElaWindowPrivate::onNavigationNodeAdded(ElaNavigationType::NavigationNodeType nodeType, QString nodeKey, QWidget* page)
{
    if (nodeType == ElaNavigationType::PageNode)
    {
        _routeMap.insert(nodeKey, _centerStackedWidget->count());
        _centerStackedWidget->addWidget(page);
    }
    else
    {
        if (page)
        {
            _routeMap.insert(nodeKey, _centerStackedWidget->count());
            _centerStackedWidget->addWidget(page);
        }
        else
        {
            _routeMap.insert(nodeKey, -1);
        }
    }
}

qreal ElaWindowPrivate::_distance(QPoint point1, QPoint point2)
{
    return std::sqrt((point1.x() - point2.x()) * (point1.x() - point2.x()) + (point1.y() - point2.y()) * (point1.y() - point2.y()));
}

void ElaWindowPrivate::_resetWindowLayout(bool isAnimation)
{
    if (isAnimation)
    {
        while (_centerLayout->count() > 0)
        {
            _centerLayout->takeAt(0);
        }
        if (_mainLayout->count() == 2)
        {
            _mainLayout->addStretch();
        }
    }
    else
    {
        if (_centerLayout->count() == 0)
        {
            _navigationBar->setDisplayMode(ElaNavigationType::Minimal, false);
            _centerLayout->addWidget(_navigationBar);
            _centerLayout->addWidget(_centerStackedWidget);
        }
        if (_mainLayout->count() == 3)
        {
            _mainLayout->takeAt(2);
        }
    }
}
