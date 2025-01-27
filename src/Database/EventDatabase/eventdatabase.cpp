#include "eventdatabase.h"
#include "project.h"
#include "Commands/EventDatabaseCommands/eventdatabaseediteventcommand.h"
#include "application.h"

const QString EventDatabase::EVENT_POINTERS_MIME_DATA_FORMAT = "can/events";

EventDatabase::EventDatabase(Project *project) :
    Database(project)
{
    Event::seedRandomID();
}


QJsonObject EventDatabase::toJsonObject() const
{
    QJsonObject json;

    for (int i = 0; i < m_events.length(); ++i)
    {
        m_events[i]->saveTo( project()->makeAbsolute( QString("event%1").arg( m_events[i]->randomID() ) ) );
    }

    return json;
}

Qt::DropActions EventDatabase::supportedDragActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

bool EventDatabase::restoreFromJsonObject(const QJsonObject &object)
{
    Q_UNUSED( object );

    beginResetModel();
    bool success = true;

    m_events.clear();
    QStringList filenames = QDir( project()->path() ).entryList( QStringList() << "event*" );
    filenames.removeOne( "eventDatabase" );

    for (const QString& filename : filenames)
    {
        Event* e = new Event( this );
        success &= e->loadFrom( project()->makeAbsolute( filename ) );
        m_events << e;
    }

    endResetModel();
    return success;
}

void EventDatabase::reset()
{
    beginResetModel();
    qDeleteAll( m_events );
    m_events.clear();
    endResetModel();
}

int EventDatabase::rowCount(const QModelIndex &parent) const
{
    assert( !parent.isValid() );
    return m_events.length();
}

int EventDatabase::columnCount(const QModelIndex &parent) const
{
    //    Type | Beginning | Label
    assert( !parent.isValid() );
    return 3;
}

QVariant EventDatabase::data(const QModelIndex &index, int role) const
{
    assert(!index.parent().isValid());
    int row = index.row();
    switch (index.column())
    {
    case 0:
        switch (role)
        {
        case Qt::DisplayRole:
            return Event::typeString( m_events[row]->type(), true );
        case Qt::EditRole:
            return (int) m_events[row]->type();
        default:
            return QVariant();
        }
        break;
    case 1:
        switch (role)
        {
        case Qt::DisplayRole:
            return m_events[row]->beginning();
        case Qt::EditRole:
            return QVariant::fromValue( TimeSpan( m_events[row]->timeSpan() ));
        default:
            return QVariant();
        }
        break;
    case 2:
        switch (role)
        {
        case Qt::DisplayRole:
        case Qt::EditRole:
            return m_events[row]->label();
        default:
            return QVariant();
        }
        break;
    default:
        return QVariant();
    }
}

QVariant EventDatabase::headerData(int section, Qt::Orientation orientation, int role) const
{
    Q_UNUSED(section);
    Q_UNUSED(orientation);
    Q_UNUSED(role);
    return QVariant();  // no header
}

Qt::ItemFlags EventDatabase::flags(const QModelIndex &index) const
{
    Q_UNUSED(index);
    return Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
}

void EventDatabase::notifyDataChanged(const QModelIndex &index)
{
    notifyDataChanged(index, index);
}

void EventDatabase::notifyDataChanged(const QModelIndex & start, const QModelIndex & end)
{
    emit dataChanged(start, end);
}

void EventDatabase::notifyDataChanged( const Event *event )
{
    int row = m_events.indexOf((Event*) event);
    if (row < 0)
    {
        return;
    }

    QModelIndex start = index( row, 0, QModelIndex() );
    QModelIndex end   = index( row, columnCount() - 1, QModelIndex() );

    notifyDataChanged( start, end );
}

Event* EventDatabase::eventAtIndex(const QModelIndex & index) const
{
    if (index.isValid() && index.row() < rowCount())
    {
        return m_events.at(index.row());
    }
    else
    {
        return NULL;
    }
}

bool EventDatabase::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (data(index, role) != value)
    {
        app().pushCommand( new EventDatabaseEditEventCommand( this, index, value, role ) );
    }
    return true;
}

bool EventDatabase::setData_(const QModelIndex &index, const QVariant &value, int role)
{
    assert(!index.parent().isValid());

    Event* event = eventAtIndex(index);
    assert( event );
    if (role == Qt::EditRole)
    {
        switch (index.column())
        {
        case 0:
            switch (value.toInt())
            {
            case 0:
                event->setType( Event::Rehearsal );
                break;
            case 1:
                event->setType( Event::Gig );
                break;
            case 2:
                event->setType( Event::Other );
                break;
            default:
                qWarning() << "Did not expect type " << value.toInt();
            }
            break;
        case 1:
            event->setTimeSpan( value.value<TimeSpan>() );
            break;
        case 2:
            event->setLabel( value.toString() );
            break;
        }

        emit dataChanged( index, index );
        return true;
    }

    return false;
}

QVariant EventDatabase::data( const int row, const int column, const int role)
{
    return data( index(row, column, QModelIndex()), role );
}

void EventDatabase::appendEvent(Event *event)
{
    insertEvent(event, rowCount());
}

void EventDatabase::insertEvent(Event *event, const int index)
{
    m_tmpEventBuffer.append(event);
    assert( insertRows( index, 1, QModelIndex() ));
}

bool EventDatabase::insertRows(int row, int count, const QModelIndex &parent)
{
    assert(!parent.isValid());
    assert( m_tmpEventBuffer.size() == count );

    beginInsertRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i)
    {
        m_events.insert( row + i, m_tmpEventBuffer[i] );
        emit eventAdded( row + i, m_tmpEventBuffer[i] );
    }
    m_tmpEventBuffer.clear();
    endInsertRows();

    return true;
}

int EventDatabase::removeEvent(Event* event)
{
    int index;
    if ( (index = m_events.indexOf(event)) < 0 )
    {
        WARNING << "EventDatabase does not contain event " << event;
    }
    else
    {
        assert( removeRows(index, 1, QModelIndex()) );
    }
    return index;
}

bool EventDatabase::removeRows(int row, int count, const QModelIndex &parent)
{
    assert(!parent.isValid());
    assert(m_tmpEventBuffer.isEmpty());

    beginRemoveRows(parent, row, row + count - 1);
    for (int i = 0; i < count; ++i)
    {
        m_events.removeAt(row + i);
        emit eventRemoved(row + i );
    }
    endRemoveRows();

    return true;
}

bool EventDatabase::moveRows(const QModelIndex &sourceParent, int sourceRow, int count, const QModelIndex &destinationParent, int destinationChild)
{
    assert( !sourceParent.isValid() );
    assert( !destinationParent.isValid() );

    destinationChild = qBound( 0, destinationChild, m_events.length() - count );

    int diff = destinationChild - sourceRow;
    if (diff > 0)
    {
        assert( beginMoveRows( sourceParent, sourceRow, sourceRow + count - 1, destinationParent, destinationChild + 1 ) );
        for (int i = sourceRow + count - 1; i >= sourceRow; --i)
        {
            m_events.insert(i + diff, m_events.takeAt(i));
        }
        endMoveRows();
    }
    else if (diff < 0)
    {
        assert( beginMoveRows( sourceParent, sourceRow, sourceRow + count - 1, destinationParent, destinationChild ) );
        for (int i = sourceRow; i <= sourceRow + count - 1; ++i)
        {
            m_events.insert(i + diff, m_events.takeAt(i));
        }
        endMoveRows();
    }

    return true;
}

void EventDatabase::moveRow(int sourceRow, int targetRow)
{
    assert( moveRows( QModelIndex(), sourceRow, 1, QModelIndex(), targetRow ) );
}

QModelIndex EventDatabase::indexOfEvent(const Event *event) const
{
    int row = -1;
    for (int i = 0; i < m_events.length() && row < 0; ++i)
    {
        if (event == m_events[i])
        {
            row = i;
        }
    }
    assert( row >= 0 );

    return createIndex( row, 0 );
}

QMimeData* EventDatabase::mimeData(const QModelIndexList &indexes) const
{
    QMimeData* mime = new QMimeData();

    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);

    QList<qintptr> ptrs;
    for (const QModelIndex& index : indexes)
    {
        if (index.column() != 0)
        {
            // we want only one index per row.
            continue;
        }
        Event* event = eventAtIndex(index);
        ptrs << qintptr(event);
    }
    stream << ptrs;

    mime->setData( EVENT_POINTERS_MIME_DATA_FORMAT, data);
    return mime;
}


