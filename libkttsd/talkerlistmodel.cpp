/***************************************************** vim:set ts=4 sw=4 sts=4:
  Widget for listing Talkers.  Based on QTreeView.
  -------------------
  Copyright : (C) 2005 by Gary Cramblitt <garycramblitt@comcast.net>
  -------------------
  Original author: Gary Cramblitt <garycramblitt@comcast.net>
  Current Maintainer: Gary Cramblitt <garycramblitt@comcast.net>
 ******************************************************************************/

/******************************************************************************
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*******************************************************************************/

// Qt includes.
#include <QStringList>

// KDE includes.
#include "klocale.h"
#include "kconfig.h"
#include "kdebug.h"

// TalkerListWidget includes.
#include "talkerlistmodel.h"
#include "talkerlistmodel.moc"

// ----------------------------------------------------------------------------

TalkerListModel::TalkerListModel(TalkerCode::TalkerCodeList talkers, QObject *parent) :
    QAbstractListModel(parent),
    m_talkerCodes(talkers),
    m_highestTalkerId(0)
{
}

int TalkerListModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return m_talkerCodes.count();
    else
        return 0;
}

int TalkerListModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 7;
}

QModelIndex TalkerListModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!parent.isValid())
        return createIndex(row, column, 0);
    else
        return QModelIndex();
}

QModelIndex TalkerListModel::parent(const QModelIndex & index ) const 
{
    Q_UNUSED(index);
    return QModelIndex();
}

QVariant TalkerListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (index.row() < 0 || index.row() >= m_talkerCodes.count())
        return QVariant();

    if (index.column() < 0 || index.column() >= 7)
        return QVariant();

    if (role == Qt::DisplayRole)
        return dataColumn(m_talkerCodes.at(index.row()), index.column());
    else
        return QVariant();
}

QVariant TalkerListModel::dataColumn(const TalkerCode& talkerCode, int column) const
{
    switch (column)
    {
        case 0: return talkerCode.id(); break;
        case 1: return talkerCode.languageCodeToLanguage(talkerCode.fullLanguageCode()); break;
        case 2: return talkerCode.TalkerDesktopEntryNameToName(talkerCode.desktopEntryName()); break;
        case 3: return talkerCode.voice(); break;
        case 4: return talkerCode.translatedGender(talkerCode.gender()); break;
        case 5: return talkerCode.translatedVolume(talkerCode.volume()); break;
        case 6: return talkerCode.translatedRate(talkerCode.rate()); break;
    }
    return QVariant();
}

Qt::ItemFlags TalkerListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::ItemIsEnabled;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant TalkerListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
    switch (section)
    {
        case 0: return i18n("ID");
        case 1: return i18n("Language");
        case 2: return i18n("Synthesizer");
        case 3: return i18n("Voice Code");
        case 4: return i18n("Gender");
        case 5: return i18n("Volume");
        case 6: return i18n("Rate");
    };

    return QVariant();
}

bool TalkerListModel::removeRow(int row, const QModelIndex & parent)
{
    beginRemoveRows(parent, row, row);
    m_talkerCodes.removeAt(row);
    for (int i = 0; i < m_talkerCodes.count(); ++i)
        if (m_talkerCodes[i].id().toInt() > m_highestTalkerId)
            m_highestTalkerId = m_talkerCodes[i].id().toInt();
    endRemoveRows();
    return true;
}

void TalkerListModel::setDatastore(TalkerCode::TalkerCodeList talkers)
{
    m_talkerCodes = talkers;
    m_highestTalkerId = 0;
    for (int i = 0; i < talkers.count(); ++i)
        if (talkers[i].id().toInt() > m_highestTalkerId) m_highestTalkerId = talkers[i].id().toInt();
    emit reset();
}

TalkerCode TalkerListModel::getRow(int row) const
{
    if (row < 0 || row >= rowCount()) return TalkerCode();
    return m_talkerCodes[row];
}

bool TalkerListModel::appendRow(TalkerCode& talker)
{
    if (talker.id().toInt() > m_highestTalkerId) m_highestTalkerId = talker.id().toInt();
    beginInsertRows(QModelIndex(), m_talkerCodes.count(), m_talkerCodes.count());
    m_talkerCodes.append(talker);
    endInsertRows();
    return true;
}

bool TalkerListModel::updateRow(int row, TalkerCode& talker)
{
    for (int i = 0; i < m_talkerCodes.count(); ++i)
        if (m_talkerCodes[i].id().toInt() > m_highestTalkerId)
            m_highestTalkerId = m_talkerCodes[i].id().toInt();
    m_talkerCodes.replace(row, talker);
    emit dataChanged(index(row, 0, QModelIndex()), index(row, columnCount()-1, QModelIndex()));
    return true;
}

bool TalkerListModel::swap(int i, int j)
{
    m_talkerCodes.swap(i, j);
    emit dataChanged(index(i, 0, QModelIndex()), index(j, columnCount()-1, QModelIndex()));
    return true;
}

void TalkerListModel::clear()
{
    m_talkerCodes.clear();
    m_highestTalkerId = 0;
    emit reset();
}

void TalkerListModel::loadTalkerCodesFromConfig(KConfig* config)
{
    // Clear the model and view.
    clear();
    // Iterate through list of the TalkerCode IDs.
    QStringList talkerIDsList = config->readEntry("TalkerIDs", QStringList(), ',');
    if (!talkerIDsList.isEmpty())
    {
        QStringList::ConstIterator itEnd = talkerIDsList.constEnd();
        for (QStringList::ConstIterator it = talkerIDsList.constBegin(); it != itEnd; ++it)
        {
            QString talkerID = *it;
            kDebug() << "TalkerListWidget::loadTalkerCodes: talkerID = " << talkerID << endl;
            config->setGroup(QString("Talker_") + talkerID);
            QString talkerCode = config->readEntry("TalkerCode");
            TalkerCode tc = TalkerCode(talkerCode, true);
            kDebug() << "TalkerCodeWidget::loadTalkerCodes: talkerCode = " << talkerCode << endl;
            tc.setId(talkerID);
            QString desktopEntryName = config->readEntry("DesktopEntryName", QString());
            tc.setDesktopEntryName(desktopEntryName);
            appendRow(tc);
        }
    }
}