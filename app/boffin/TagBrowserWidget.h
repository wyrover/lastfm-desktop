#ifndef TAG_BROWSER_WIDGET_H
#define TAG_BROWSER_WIDGET_H

#include <boost/function.hpp>
#include <QStringList>
#include <QWidget>

class QItemSelection;
class TagCloudView;
class PlaydarTagCloudModel;
class SideBySideLayout;
class HistoryWidget;
class PlaydarConnection;
class PlaylistModel;
class PlaylistWidget;

#include "PlaydarConnection.h"
#include "PlaydarTagCloudModel.h"
#include <QSortFilterProxyModel>

class RelevanceFilter : public QSortFilterProxyModel
{
    Q_OBJECT;

public:
    RelevanceFilter()
        :m_req(0)
        ,m_showAll(true)
    {
    }

    void resetFilter()
    {
        m_map.clear();
        m_showAll = true;
        invalidateFilter();
    }

    void setRqlFilter(PlaydarConnection* playdar, QStringList tags)
    {
	    m_map.clear();

        QString rql;
        {
	        QStringList t;
            foreach(QString s, tags) {
    	        t << "tag:\"" + s + '"';
                m_map.insert(s, 1.0);
            }
	        rql = t.join(" and ");
        }

        m_min = FLT_MAX;
	    m_max = FLT_MIN;
        if (m_req) {
            m_req->disconnect(this);
        }
	    m_req = playdar->boffinTagcloud( rql );
	    connect(m_req, SIGNAL(tagItem(BoffinTagItem)), SLOT(onTagItem(BoffinTagItem)));

        invalidateFilter();
    }

    void showRelevant(bool bShowRelevant)
    {
        if (bShowRelevant == m_showAll) {
            m_showAll = !m_showAll;
            invalidateFilter();
        }
    }

    bool showingRelevant()
    {
        return !m_showAll;
    }

    bool showingAll()
    {
        return m_showAll;
    }

protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex & source_parent) const
    {
        if (m_showAll) 
            return true;

        QModelIndex i = sourceModel()->index(source_row, 0, source_parent);
        const QString tagname = i.data().toString();
        return m_map.contains(tagname);
    }

    // we own the RelevanceRole.  :)
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const 
    {
        if (role == PlaydarTagCloudModel::RelevanceRole) {
            const QString tagname = mapToSource(index).data().toString();
        	QMap<QString, float>::const_iterator i = m_map.find(tagname);
            float result;
            if (i == m_map.end()) {
        		result = 0;
            } else if (m_min == m_max) {
                result = 1.0;
            } else {
                const float lw = i.value();
                result = (lw - m_min) / (m_max - m_min);
            }
            return QVariant::fromValue<float>(result);
        }

        return QSortFilterProxyModel::data(index, role);
    }

private slots:
    void onTagItem(const BoffinTagItem& tagitem)
    {
        const QString& tag = tagitem.m_name;
        float lw = log(tagitem.m_weight);
        if (m_map.contains(tag)) {
            lw = (m_map[tag] += lw);
        } else {
            m_map[tag] = lw;
            invalidateFilter();
        }

        if (lw < m_min) {
		    m_min = lw;
	    }
	    if (lw > m_max) {
		    m_max = lw;
	    }
    }

private:
    BoffinTagRequest* m_req;
    bool m_showAll;
    QMap<QString, float> m_map;
    float m_min, m_max;
};


class TagBrowserWidget : public QWidget
{
    Q_OBJECT

public:
    TagBrowserWidget(PlaydarConnection*, QWidget* parent = 0);

    static QString rql(const QStringList& in);

signals:
    void selectionChanged();

private slots:
    void onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
    void onHistoryClicked(int position, const QString& text);
    void onFilterClicked();

private:
    QStringList selectedTags() const;

    HistoryWidget* m_history;

    TagCloudView* m_view;
    RelevanceFilter* m_filter;              // sits between model and view
    PlaydarTagCloudModel* m_tagCloudModel;

    PlaylistWidget* m_playlistWidget;
    PlaylistModel* m_playlistModel;

    QStringList m_tags;
    PlaydarConnection* m_playdar;
};

#endif
