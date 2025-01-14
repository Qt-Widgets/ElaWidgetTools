#ifndef ELASUGGESTBOXPRIVATE_H
#define ELASUGGESTBOXPRIVATE_H

#include <QObject>
#include <QSize>
#include <QVariantMap>

#include "Def.h"
#include "stdafx.h"
class ElaSuggestion : public QObject
{
    Q_OBJECT
    Q_PROPERTY_CREATE(ElaIconType, ElaIcon)
    Q_PROPERTY_CREATE(QString, SuggestText)
    Q_PROPERTY_CREATE(QVariantMap, SuggestData)
public:
    explicit ElaSuggestion(QObject* parent = nullptr);
    ~ElaSuggestion();
};

class ElaLineEdit;
class ElaNavigationNode;
class ElaSuggestModel;
class ElaSuggestView;
class ElaSuggestDelegate;
class ElaSuggestBox;
class ElaShadowWidget;
class ElaSuggestBoxPrivate : public QObject
{
    Q_OBJECT
    Q_D_CREATE(ElaSuggestBox)
    Q_PROPERTY_CREATE_D(int, BorderRadius)
    Q_PROPERTY_CREATE_D(Qt::CaseSensitivity, CaseSensitivity)
public:
    explicit ElaSuggestBoxPrivate(QObject* parent = nullptr);
    ~ElaSuggestBoxPrivate();
    Q_SLOT void onSearchEditTextEdit(const QString& searchText);
    Q_SLOT void onSearchViewClicked(const QModelIndex& index);

private:
    QVector<ElaSuggestion*> _suggestionVector;
    ElaShadowWidget* _shadowWidget{nullptr};
    ElaLineEdit* _searchEdit{nullptr};
    ElaSuggestModel* _searchModel{nullptr};
    ElaSuggestView* _searchView{nullptr};
    ElaSuggestDelegate* _searchDelegate{nullptr};
    QSize _lastSize{-1, -1};
    bool _isExpandAnimationFinished{true};
    bool _isCloseAnimationFinished{true};
    void _startSizeAnimation(QSize newSize);
    void _startExpandAnimation();
    void _startCloseAnimation();
};

#endif // ELASUGGESTBOXPRIVATE_H
