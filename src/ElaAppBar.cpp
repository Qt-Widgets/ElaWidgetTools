#include "ElaAppBar.h"

#include <Windows.h>
#include <dwmapi.h>
#include <windowsx.h>

#include <QApplication>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QScreen>
#include <QTimer>
#include <QVBoxLayout>

#include "Def.h"
#include "ElaApplication.h"
#include "ElaEventBus.h"
#include "ElaIconButton.h"
#include "private/ElaAppBarPrivate.h"
#include "qdebug.h"
Q_PROPERTY_CREATE_Q_CPP(ElaAppBar, bool, IsStayTop)
Q_PROPERTY_CREATE_Q_CPP(ElaAppBar, bool, IsFixedSize)
Q_PROPERTY_CREATE_Q_CPP(ElaAppBar, bool, IsDefaultClosed)
Q_PROPERTY_CREATE_Q_CPP(ElaAppBar, bool, IsOnlyAllowMinAndClose)
#if (QT_VERSION == QT_VERSION_CHECK(6, 5, 3) || QT_VERSION == QT_VERSION_CHECK(6, 6, 0))
[[maybe_unused]] static inline void setShadow(HWND hwnd)
{
    const MARGINS shadow = {1, 0, 0, 0};
    typedef HRESULT(WINAPI * DwmExtendFrameIntoClientAreaPtr)(HWND hWnd, const MARGINS* pMarInset);
    HMODULE module = LoadLibraryW(L"dwmapi.dll");
    if (module)
    {
        DwmExtendFrameIntoClientAreaPtr dwm_extendframe_into_client_area_;
        dwm_extendframe_into_client_area_ = reinterpret_cast<DwmExtendFrameIntoClientAreaPtr>(GetProcAddress(module, "DwmExtendFrameIntoClientArea"));
        if (dwm_extendframe_into_client_area_)
        {
            dwm_extendframe_into_client_area_(hwnd, &shadow);
        }
    }
}
#endif

ElaAppBar::ElaAppBar(QWidget* parent)
    : QWidget{parent}, d_ptr(new ElaAppBarPrivate())
{
    Q_D(ElaAppBar);
    d->_buttonFlags = ElaAppBarType::RouteBackButtonHint | ElaAppBarType::StayTopButtonHint | ElaAppBarType::ThemeChangeButtonHint | ElaAppBarType::MinimizeButtonHint | ElaAppBarType::MaximizeButtonHint | ElaAppBarType::CloseButtonHint;
    window()->setAttribute(Qt::WA_Mapped);
    d->q_ptr = this;
    d->_pIsStayTop = false;
    d->_pIsFixedSize = false;
    d->_pIsDefaultClosed = true;
    d->_pIsOnlyAllowMinAndClose = false;
    d->_currentWinID = window()->winId();
#if (QT_VERSION == QT_VERSION_CHECK(6, 5, 3) || QT_VERSION == QT_VERSION_CHECK(6, 6, 0))
    window()->setWindowFlags((window()->windowFlags()) | Qt::WindowMinimizeButtonHint | Qt::FramelessWindowHint);
    window()->installEventFilter(this);
    setShadow((HWND)(window()->winId()));
#endif
    QGuiApplication::instance()->installNativeEventFilter(this);
    setMouseTracking(true);
    setFixedHeight(30);
    setObjectName("ElaAppBar");
    setStyleSheet("#ElaAppBar{background-color:transparent;}");
    d->_routeBackButton = new ElaIconButton(ElaIconType::ArrowLeft, 18, 40, 30, this);
    d->_routeBackButton->setEnabled(false);
    // 路由跳转
    connect(d->_routeBackButton, &QPushButton::clicked, this, &ElaAppBar::routeBackButtonClicked);

    // 导航栏展开按钮
    d->_navigationButton = new ElaIconButton(ElaIconType::Bars, 16, 40, 30, this);
    d->_navigationButton->setVisible(false);
    // 展开导航栏
    connect(d->_navigationButton, &ElaIconButton::clicked, this, [this]() { Q_EMIT navigationButtonClicked(); });

    // 设置置顶
    d->_stayTopButton = new ElaIconButton(ElaIconType::ArrowUpToArc, 15, 40, 30, this);
    d->_stayTopButton->setLightHoverColor(QColor(0xEF, 0xE6, 0xED));
    connect(d->_stayTopButton, &ElaIconButton::clicked, this, [=]() { this->setIsStayTop(!this->getIsStayTop()); });
    connect(this, &ElaAppBar::pIsStayTopChanged, d, &ElaAppBarPrivate::onStayTopButtonClicked);
    d->_titleLabel = new QLabel(this);
    QFont textfont("Microsoft YaHei", 10);
    textfont.setHintingPreference(QFont::PreferNoHinting);
    d->_titleLabel->setFont(textfont);

    // 主题变更
    d->_themeChangeButton = new ElaIconButton(ElaIconType::MoonStars, 15, 40, 30, this);
    d->_themeChangeButton->setLightHoverColor(QColor(0xEF, 0xE6, 0xED));
    connect(d->_themeChangeButton, &ElaIconButton::clicked, this, [this]() { Q_EMIT themeChangeButtonClicked(); });
    connect(eApp, &ElaApplication::themeModeChanged, this, [=](ElaApplicationType::ThemeMode themeMode) { d->_onThemeModeChange(themeMode); });

    d->_minButton = new ElaIconButton(ElaIconType::Dash, 12, 40, 30, this);
    d->_minButton->setLightHoverColor(QColor(0xEF, 0xE6, 0xED));
    connect(d->_minButton, &ElaIconButton::clicked, d, &ElaAppBarPrivate::onMinButtonClicked);
    d->_maxButton = new ElaIconButton(ElaIconType::Square, 13, 40, 30, this);
    d->_maxButton->setLightHoverColor(QColor(0xEF, 0xE6, 0xED));
    connect(d->_maxButton, &ElaIconButton::clicked, d, &ElaAppBarPrivate::onMaxButtonClicked);
    d->_closeButton = new ElaIconButton(ElaIconType::Xmark, 17, 40, 30, this);
    d->_closeButton->setLightHoverColor(QColor(0xC4, 0x2B, 0x1C));
    d->_closeButton->setDarkHoverColor(QColor(0xC4, 0x2B, 0x1C));
    connect(d->_closeButton, &ElaIconButton::clicked, d, &ElaAppBarPrivate::onCloseButtonClicked);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    QHBoxLayout* operateButtonLayout = new QHBoxLayout();
    operateButtonLayout->setContentsMargins(0, 0, 0, 0);
    operateButtonLayout->setSpacing(0);
    operateButtonLayout->addWidget(d->_routeBackButton);
    operateButtonLayout->addWidget(d->_navigationButton);
    operateButtonLayout->addSpacing(10);
    operateButtonLayout->addWidget(d->_titleLabel);
    operateButtonLayout->addStretch();
    operateButtonLayout->addWidget(d->_stayTopButton);
    operateButtonLayout->addWidget(d->_themeChangeButton);
    operateButtonLayout->addWidget(d->_minButton);
    operateButtonLayout->addWidget(d->_maxButton);
    operateButtonLayout->addWidget(d->_closeButton);
    mainLayout->addLayout(operateButtonLayout);

    HWND hwnd = reinterpret_cast<HWND>(window()->winId());

    DWORD style = ::GetWindowLongPtr(hwnd, GWL_STYLE);
    if (d->_pIsFixedSize)
    {
        //切换DPI处理
        ::SetWindowLongPtr(hwnd, GWL_STYLE, style | WS_THICKFRAME);
        for (int i = 0; i < qApp->screens().count(); ++i)
        {
            connect(qApp->screens().at(i), &QScreen::logicalDotsPerInchChanged, this, [=] { SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_FRAMECHANGED); });
        }
    }
    else
    {
        ::SetWindowLongPtr(hwnd, GWL_STYLE, style | WS_MAXIMIZEBOX | WS_THICKFRAME);
    }
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);

    //主屏幕变更处理
    connect(qApp, &QApplication::primaryScreenChanged, this, [=]() {
        ::SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
        ::RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
    });
    d->_lastScreen = qApp->screenAt(window()->geometry().center());
}

ElaAppBar::~ElaAppBar()
{
    QGuiApplication::instance()->removeNativeEventFilter(this);
}

void ElaAppBar::setWindowTitle(QString title)
{
    Q_D(ElaAppBar);
    d->_titleLabel->setText(title);
}

QString ElaAppBar::getWindowTitle() const
{
    return d_ptr->_titleLabel->text();
}

void ElaAppBar::setWindowButtonFlag(ElaAppBarType::ButtonType buttonFlag, bool isEnable)
{
    Q_D(ElaAppBar);
    if (isEnable)
    {
        setWindowButtonFlags(d->_buttonFlags | buttonFlag);
    }
    else
    {
        setWindowButtonFlags(d->_buttonFlags & ~buttonFlag);
    }
}

void ElaAppBar::setWindowButtonFlags(ElaAppBarType::ButtonFlags buttonFlags)
{
    Q_D(ElaAppBar);
    d->_buttonFlags = buttonFlags;
    d->_routeBackButton->setVisible(d->_buttonFlags.testFlag(ElaAppBarType::RouteBackButtonHint));
    d->_navigationButton->setVisible(d->_buttonFlags.testFlag(ElaAppBarType::NavigationButtonHint));
    d->_stayTopButton->setVisible(d->_buttonFlags.testFlag(ElaAppBarType::StayTopButtonHint));
    d->_themeChangeButton->setVisible(d->_buttonFlags.testFlag(ElaAppBarType::ThemeChangeButtonHint));
    d->_minButton->setVisible(d->_buttonFlags.testFlag(ElaAppBarType::MinimizeButtonHint));
    d->_maxButton->setVisible(d->_buttonFlags.testFlag(ElaAppBarType::MaximizeButtonHint));
    d->_closeButton->setVisible(d->_buttonFlags.testFlag(ElaAppBarType::CloseButtonHint));
}

ElaAppBarType::ButtonFlags ElaAppBar::getWindowButtonFlags() const
{
    return d_ptr->_buttonFlags;
}

void ElaAppBar::setRouteBackButtonEnable(bool isEnable)
{
    Q_D(ElaAppBar);
    d->_routeBackButton->setEnabled(isEnable);
}

void ElaAppBar::closeWindow()
{
    Q_D(const ElaAppBar);
    eApp->setIsApplicationClosed(true);
    QRect windowRect = window()->geometry();
    // 抵消setFixedSize导致的移动偏航
    window()->setMinimumSize(0, 0);
    window()->setMaximumSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
    QPropertyAnimation* closeGeometryAnimation = new QPropertyAnimation(window(), "geometry");
    QPropertyAnimation* closeOpacityAnimation = new QPropertyAnimation(window(), "windowOpacity");
    connect(closeGeometryAnimation, &QPropertyAnimation::finished, this, [=]() { window()->close(); });
    closeGeometryAnimation->setStartValue(windowRect);
    qreal minmumWidth = (d->_titleLabel->width() + 320);
    closeGeometryAnimation->setEndValue(QRect(windowRect.x() + (windowRect.width() - minmumWidth) / 2, windowRect.y() + (windowRect.height() / 2) - 145, minmumWidth, 290));
    closeGeometryAnimation->setDuration(300);
    closeGeometryAnimation->setEasingCurve(QEasingCurve::InOutSine);

    closeOpacityAnimation->setStartValue(1);
    closeOpacityAnimation->setEndValue(0);
    closeOpacityAnimation->setDuration(300);
    closeOpacityAnimation->setEasingCurve(QEasingCurve::InOutSine);
    closeGeometryAnimation->start(QAbstractAnimation::DeleteWhenStopped);
    closeOpacityAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}

#if (QT_VERSION == QT_VERSION_CHECK(6, 5, 3) || QT_VERSION == QT_VERSION_CHECK(6, 6, 0))
[[maybe_unused]] bool ElaAppBar::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::WindowActivate)
    {
        HWND hwnd = reinterpret_cast<HWND>(window()->winId());
        DWORD style = ::GetWindowLongPtr(hwnd, GWL_STYLE);
        bool hasCaption = (style & WS_CAPTION) == WS_CAPTION;
        if (!hasCaption)
        {
            QTimer::singleShot(15, this, [=] { ::SetWindowLongPtr(hwnd, GWL_STYLE, style | WS_CAPTION); });
        }
    }
    return QObject::eventFilter(obj, event);
}
#endif

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
bool ElaAppBar::nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result)
#else
bool ElaAppBar::nativeEventFilter(const QByteArray& eventType, void* message, long* result)
#endif
{
    Q_D(ElaAppBar);
    if ((eventType != "windows_generic_MSG") || !message)
    {
        return false;
    }
    const auto msg = static_cast<const MSG*>(message);
    const HWND hwnd = msg->hwnd;
    if (!hwnd || !msg)
    {
        return false;
    }
    const qint64 wid = reinterpret_cast<qint64>(hwnd);
    if (wid != d->_currentWinID)
    {
        return false;
    }
    const UINT uMsg = msg->message;
    const WPARAM wParam = msg->wParam;
    const LPARAM lParam = msg->lParam;
    switch (uMsg)
    {
    case WM_WINDOWPOSCHANGING:
    {
        WINDOWPOS* wp = reinterpret_cast<WINDOWPOS*>(lParam);
        if (wp != nullptr && (wp->flags & SWP_NOSIZE) == 0)
        {
            wp->flags |= SWP_NOCOPYBITS;
            *result = ::DefWindowProcW(hwnd, uMsg, wParam, lParam);
            return true;
        }
        return false;
    }
    case WM_SIZE:
    {
        if (wParam == SIZE_RESTORED)
        {
            d->_changeMaxButtonAwesome(false);
        }
        else if (wParam == SIZE_MAXIMIZED)
        {
            d->_changeMaxButtonAwesome(true);
        }
        return false;
    }
    case WM_NCCALCSIZE:
    {
#if (QT_VERSION == QT_VERSION_CHECK(6, 5, 3) || QT_VERSION == QT_VERSION_CHECK(6, 6, 0))
        if (wParam == FALSE)
        {
            return false;
        }
        if (::IsZoomed(hwnd))
        {
            window()->setContentsMargins(8, 8, 8, 8);
        }
        else
        {
            window()->setContentsMargins(0, 0, 0, 0);
        }
        *result = 0;
        return true;
#else
        if (wParam == FALSE)
        {
            return false;
        }
        RECT* clientRect = &((NCCALCSIZE_PARAMS*)(lParam))->rgrc[0];
        if (!::IsZoomed(hwnd))
        {
            clientRect->top -= 1;
            clientRect->bottom -= 1;
        }
        else
        {
            const LRESULT hitTestResult = ::DefWindowProcW(hwnd, WM_NCCALCSIZE, wParam, lParam);
            if ((hitTestResult != HTERROR) && (hitTestResult != HTNOWHERE))
            {
                *result = static_cast<long>(hitTestResult);
                return true;
            }
            // qDebug() << clientRect->left << clientRect->top << clientRect->bottom << clientRect->right;
            auto geometry = window()->screen()->geometry();
            clientRect->top = geometry.top();
        }
        *result = WVR_REDRAW;
        return true;
#endif
    }
    case WM_MOVE:
    {
        QScreen* currentScreen = qApp->screenAt(window()->geometry().center());
        if (currentScreen && currentScreen != d->_lastScreen)
        {
            if (d->_lastScreen)
            {
                ::SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED);
                ::RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
            }
            d->_lastScreen = currentScreen;
        }
        break;
    }
    case WM_NCHITTEST:
    {
        if (d->_containsCursorToItem(d->_maxButton))
        {
            if (*result == HTNOWHERE)
            {
                if (!d->_isHoverMaxButton)
                {
                    d->_isHoverMaxButton = true;
                    d->_maxButton->setIsSelected(true);
                }
                *result = HTZOOM;
            }
            return true;
        }
        else
        {
            if (d->_isHoverMaxButton)
            {
                d->_isHoverMaxButton = false;
                d->_maxButton->setIsSelected(false);
            }
        }
        POINT nativeLocalPos{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
        ::ScreenToClient(hwnd, &nativeLocalPos);
        RECT clientRect{0, 0, 0, 0};
        ::GetClientRect(hwnd, &clientRect);
        auto clientWidth = clientRect.right - clientRect.left;
        auto clientHeight = clientRect.bottom - clientRect.top;
        bool left = nativeLocalPos.x < d->_margins;
        bool right = nativeLocalPos.x > clientWidth - d->_margins;
        bool top = nativeLocalPos.y < d->_margins;
        bool bottom = nativeLocalPos.y > clientHeight - d->_margins;
        *result = 0;
        if (!d->_pIsOnlyAllowMinAndClose && !d->_pIsFixedSize && !window()->isFullScreen() && !window()->isMaximized())
        {
            if (left && top)
            {
                *result = HTTOPLEFT;
            }
            else if (left && bottom)
            {
                *result = HTBOTTOMLEFT;
            }
            else if (right && top)
            {
                *result = HTTOPRIGHT;
            }
            else if (right && bottom)
            {
                *result = HTBOTTOMRIGHT;
            }
            else if (left)
            {
                *result = HTLEFT;
            }
            else if (right)
            {
                *result = HTRIGHT;
            }
            else if (top)
            {
                *result = HTTOP;
            }
            else if (bottom)
            {
                *result = HTBOTTOM;
            }
        }
        if (0 != *result)
        {
            return true;
        }
        if (d->_containsCursorToItem(this))
        {
            *result = HTCAPTION;
            return true;
        }
        *result = HTCLIENT;
        return true;
    }
    case WM_LBUTTONDBLCLK:
    {
        QVariantMap postData;
        postData.insert("WMClickType", ElaAppBarType::WMLBUTTONDBLCLK);
        ElaEventBus::getInstance()->post("WMWindowClicked", postData);
        return false;
    }
    case WM_LBUTTONDOWN:
    {
        QVariantMap postData;
        postData.insert("WMClickType", ElaAppBarType::WMLBUTTONDOWN);
        ElaEventBus::getInstance()->post("WMWindowClicked", postData);
        return false;
    }
    case WM_LBUTTONUP:
    {
        QVariantMap postData;
        postData.insert("WMClickType", ElaAppBarType::WMLBUTTONUP);
        ElaEventBus::getInstance()->post("WMWindowClicked", postData);
        return false;
    }
    case WM_NCLBUTTONDOWN:
    {
        QVariantMap postData;
        postData.insert("WMClickType", ElaAppBarType::WMNCLBUTTONDOWN);
        ElaEventBus::getInstance()->post("WMWindowClicked", postData);
        if (d->_containsCursorToItem(d->_maxButton) || d->_pIsOnlyAllowMinAndClose)
        {
            return true;
        }
        break;
    }
    case WM_NCLBUTTONUP:
    {
        QVariantMap postData;
        postData.insert("WMClickType", ElaAppBarType::WMNCLBUTTONDOWN);
        ElaEventBus::getInstance()->post("WMWindowClicked", postData);
        if (d->_containsCursorToItem(d->_maxButton) && !d->_pIsOnlyAllowMinAndClose)
        {
            d->onMaxButtonClicked();
            return true;
        }
        break;
    }
    case WM_NCLBUTTONDBLCLK:
    {
        if (!d->_pIsOnlyAllowMinAndClose)
        {
            return false;
        }
        return true;
    }
    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* minmaxInfo = reinterpret_cast<MINMAXINFO*>(lParam);
        RECT rect;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
        minmaxInfo->ptMinTrackSize.x = (d->_titleLabel->width() + 320) * qApp->devicePixelRatio();
        minmaxInfo->ptMinTrackSize.y = 290 * qApp->devicePixelRatio();
        minmaxInfo->ptMaxPosition.x = rect.left;
        minmaxInfo->ptMaxPosition.y = rect.top;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        auto pixelRatio = window()->devicePixelRatio();
        auto geometry = window()->screen()->availableGeometry();
        minmaxInfo->ptMaxSize.x = qRound(geometry.width() * pixelRatio);
        minmaxInfo->ptMaxSize.y = qRound(geometry.height() * pixelRatio);
#endif
        return true;
    }
    case WM_NCACTIVATE:
    {
        *result = TRUE;
        return true;
    }
    case WM_NCRBUTTONDOWN:
    {
        if (wParam == HTCAPTION && !d->_pIsOnlyAllowMinAndClose)
        {
            d->_showSystemMenu(QCursor::pos());
        }
        break;
    }
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        if ((GetAsyncKeyState(VK_MENU) & 0x8000) && (GetAsyncKeyState(VK_SPACE) & 0x8000) && !d->_pIsOnlyAllowMinAndClose)
        {
            auto pos = window()->geometry().topLeft();
            d->_showSystemMenu(QPoint(pos.x(), pos.y() + this->height()));
        }
        break;
    }
    }
    return QWidget::nativeEvent(eventType, message, result);
}
