/***************************************************** vim:set ts=4 sw=4 sts=4:
  Description: 
    Filters text, applying each configured Filter in turn.
    Runs asynchronously, emitting Finished() signal when all Filters have run.

  Copyright:
  (C) 2005 by Gary Cramblitt <garycramblitt@comcast.net>
  -------------------
  Original author: Gary Cramblitt <garycramblitt@comcast.net
 ******************************************************************************/

/******************************************************************************
 *                                                                            *
 *    This program is free software; you can redistribute it and/or modify    *
 *    it under the terms of the GNU General Public License as published by    *
 *    the Free Software Foundation; either version 2 of the License.          *
 *                                                                            *
 ******************************************************************************/

// KDE includes.
#include <kdebug.h>
#include <kconfig.h>
#include <ktrader.h>
#include <kparts/componentfactory.h>

// FilterMgr includes.
#include "filtermgr.h"
#include "filtermgr.moc"

/**
 * Constructor.
 */
FilterMgr::FilterMgr( QObject *parent, const char *name) :
    KttsFilterProc(parent, name) 
{
    // kdDebug() << "FilterMgr::FilterMgr: Running" << endl;
    m_state = fsIdle;
}

/**
 * Destructor.
 */
FilterMgr::~FilterMgr()
{
    // kdDebug() << "FilterMgr::~FilterMgr: Running" << endl;
    if ( m_state == fsFiltering )
        stopFiltering();
    m_filterList.setAutoDelete( TRUE );
    m_filterList.clear();
}

/**
 * Loads and initializes the filters.
 * @param config          Settings object.
 * @return                False if FilterMgr is not ready to filter.
 */
bool FilterMgr::init(KConfig *config, const QString& /*configGroup*/)
{
    // Load each of the filters and initialize.
    config->setGroup("General");
    QStringList filterIDsList = config->readListEntry("FilterIDs", ',');
    // kdDebug() << "FilterMgr::init: FilterIDs = " << filterIDsList << endl;
    if ( !filterIDsList.isEmpty() )
    {
        QStringList::ConstIterator itEnd = filterIDsList.constEnd();
        for (QStringList::ConstIterator it = filterIDsList.constBegin(); it != itEnd; ++it)
        {
            QString filterID = *it;
            QString groupName = "Filter_" + filterID;
            config->setGroup( groupName );
            QString filterPlugInName = config->readEntry( "PlugInName" );
            if (config->readBoolEntry("Enabled"))
            {
                // kdDebug() << "FilterMgr::init: filterID = " << filterID << endl;
                KttsFilterProc* filterProc = loadFilterPlugin( filterPlugInName );
                if ( filterProc )
                {
                    filterProc->init( config, groupName );
                    m_filterList.append( filterProc );
                }
            }
        }
    }
    return true;
}

/**
 * Returns True if this filter is a Sentence Boundary Detector.
 * If so, the filter should implement @ref setSbRegExp() .
 * @return          True if this filter is a SBD.
 */
/*virtual*/ bool FilterMgr::isSBD() { return true; }

/**
 * Returns True if the plugin supports asynchronous processing,
 * i.e., supports asyncConvert method.
 * @return                        True if this plugin supports asynchronous processing.
 *
 * If the plugin returns True, it must also implement @ref getState .
 * It must also emit @ref filteringFinished when filtering is completed.
 * If the plugin returns True, it must also implement @ref stopFiltering .
 * It must also emit @ref filteringStopped when filtering has been stopped.
 */
/*virtual*/ bool FilterMgr::supportsAsync() { return true; }

/** 
 * Synchronously convert text.
 * @param inputText         Input text.
 * @param talkerCode        TalkerCode structure for the talker that KTTSD intends to
 *                          use for synthing the text.  Useful for extracting hints about
 *                          how to filter the text.  For example, languageCode.
 * @param appId             The DCOP appId of the application that queued the text.
 *                          Also useful for hints about how to do the filtering.
 * @return                  Converted text.
 */
QString FilterMgr::convert(const QString& inputText, TalkerCode* talkerCode, const QCString& appId)
{
    m_text = inputText;
    m_talkerCode = talkerCode;
    m_appId = appId;
    m_filterIndex = -1;
    m_filterProc = 0;
    m_state = fsFiltering;
    m_async = false;
    while ( m_state == fsFiltering )
        nextFilter();
    return m_text;
}

/**
 * Aynchronously convert input.
 * @param inputText         Input text.
 * @param talkerCode        TalkerCode structure for the talker that KTTSD intends to
 *                          use for synthing the text.  Useful for extracting hints about
 *                          how to filter the text.  For example, languageCode.
 * @param appId             The DCOP appId of the application that queued the text.
 *                          Also useful for hints about how to do the filtering.
 *
 * When the input text has been converted, filteringFinished signal will be emitted
 * and caller can retrieve using getOutput();
*/
bool FilterMgr::asyncConvert(const QString& inputText, TalkerCode* talkerCode, const QCString& appId)
{
    m_text = inputText;
    m_talkerCode = talkerCode;
    m_appId = appId;
    m_filterIndex = -1;
    m_filterProc = 0;
    m_state = fsFiltering;
    m_async = true;
    nextFilter();
    return true;
}

// Finishes up with current filter (if any) and goes on to the next filter.
void FilterMgr::nextFilter()
{
    if ( m_filterProc )
    {
        m_text = m_filterProc->getOutput();
        m_filterProc->ackFinished();
        if ( m_async )
            disconnect( m_filterProc, SIGNAL(filteringFinished()), this, SLOT(slotFilteringFinished()) );
        if ( m_filterProc->wasModified() && m_filterProc->isSBD() )
        {
            m_state = fsFinished;
            // Post an event which will be later emitted as a signal.
            QCustomEvent* ev = new QCustomEvent(QEvent::User + 301);
            QApplication::postEvent(this, ev);
            return;
        }
    }
    ++m_filterIndex;
    if ( m_filterIndex == static_cast<int>(m_filterList.count()) )
    {
        m_state = fsFinished;
        // Post an event which will be later emitted as a signal.
        QCustomEvent* ev = new QCustomEvent(QEvent::User + 301);
        QApplication::postEvent(this, ev);
        return;
    }
    m_filterProc = m_filterList.at(m_filterIndex);
    m_filterProc->setSbRegExp( m_re );
    if ( m_async )
    {
        // kdDebug() << "FilterMgr::nextFilter: calling asyncConvert on filter " << m_filterIndex << endl;
        connect( m_filterProc, SIGNAL(filteringFinished()), this, SLOT(slotFilteringFinished()) );
        if ( !m_filterProc->asyncConvert( m_text, m_talkerCode, m_appId ) )
        {
            disconnect( m_filterProc, SIGNAL(filteringFinished()), this, SLOT(slotFilteringFinished()) );
            m_filterProc = 0;
            nextFilter();
        }
    } else
        if ( !m_filterProc->convert( m_text, m_talkerCode, m_appId ) )
        {
            m_filterProc = 0;
            nextFilter();
        }
}

// Received when each filter finishes.
void FilterMgr::slotFilteringFinished()
{
    // kdDebug() << "FilterMgr::slotFilteringFinished: received signal from filter " << m_filterIndex << endl;
    nextFilter();
}

bool FilterMgr::event ( QEvent * e )
{
    if ( e->type() == (QEvent::User + 301) )
    {
        // kdDebug() << "FilterMgr::event: emitting filteringFinished signal." << endl;
        emit filteringFinished();
        return TRUE;
    }
    if ( e->type() == (QEvent::User + 302) )
    {
        // kdDebug() << "FilterMgr::event: emitting filteringStopped signal." << endl;
        emit filteringStopped();
        return TRUE;
    }
    else return FALSE;
}

/**
 * Waits for filtering to finish.
 */
void FilterMgr::waitForFinished()
{
    if ( m_state != fsFiltering ) return;
    disconnect(m_filterProc, SIGNAL(filteringFinished()), this, SLOT(slotFilteringFinished()) );
    m_async = false;
    m_filterProc->waitForFinished();
    while ( m_state == fsFiltering )
        nextFilter();
}

/**
 * Returns the state of the FilterMgr.
 */
int FilterMgr::getState() { return m_state; }

/**
 * Returns the filtered output.
 */
QString FilterMgr::getOutput()
{
    return m_text;
}

/**
 * Acknowledges the finished filtering.
 */
void FilterMgr::ackFinished()
{
    m_state = fsIdle;
}

/**
 * Stops filtering.  The filteringStopped signal will emit when filtering
 * has in fact stopped.
 */
void FilterMgr::stopFiltering()
{
    if ( m_state != fsFiltering ) return;
    if ( m_async )
        disconnect( m_filterProc, SIGNAL(filteringFinished()), this, SLOT(slotFilteringFinished()) );
    m_filterProc->stopFiltering();
    m_state = fsIdle;
    QCustomEvent* ev = new QCustomEvent(QEvent::User + 302);
    QApplication::postEvent(this, ev);
}

/**
 * Set Sentence Boundary Regular Expression.
 * This method will only be called if the application overrode the default.
 *
 * @param re            The sentence delimiter regular expression.
 */
/*virtual*/ void FilterMgr::setSbRegExp(const QString& re)
{
    m_re = re;
}

// Loads the processing plug in for a named filter plug in.
KttsFilterProc* FilterMgr::loadFilterPlugin(const QString& plugInName)
{
    // kdDebug() << "FilterMgr::loadFilterPlugin: Running"<< endl;

    // Get list of plugins.
    KTrader::OfferList offers = KTrader::self()->query("KTTSD/FilterPlugin");

    // Iterate thru the offers to find the plug in that matches the name.
    for(unsigned int i=0; i < offers.count() ; ++i){
        // Compare the plug in to be loaded with the entry in offers[i]
        // kdDebug() << "Comparing " << offers[i]->plugInName() << " to " << synthName << endl;
        if(offers[i]->name() == plugInName)
        {
            // When the entry is found, load the plug in
            // First create a factory for the library
            KLibFactory *factory = KLibLoader::self()->factory(offers[i]->library().latin1());
            if(factory){
                // If the factory is created successfully, instantiate the KttsFilterConf class for the
                // specific plug in to get the plug in configuration object.
                int errorNo;
                KttsFilterProc *plugIn =
                        KParts::ComponentFactory::createInstanceFromLibrary<KttsFilterProc>(
                        offers[i]->library().latin1(), NULL, offers[i]->library().latin1(),
                QStringList(), &errorNo);
                if(plugIn){
                    // If everything went ok, return the plug in pointer.
                    return plugIn;
                } else {
                    // Something went wrong, returning null.
                    kdDebug() << "FilterMgr::loadFilterPlugin: Unable to instantiate KttsFilterProc class for plugin " << plugInName << " error: " << errorNo << endl;
                    return NULL;
                }
            } else {
                // Something went wrong, returning null.
                kdDebug() << "FilterMgr::loadFilterPlugin: Unable to create Factory object for plugin " << plugInName << endl;
                return NULL;
            }
            break;
        }
    }
    // The plug in was not found (unexpected behaviour, returns null).
    kdDebug() << "FilterMgr::loadFilterPlugin: KTrader did not return an offer for plugin " << plugInName << endl;
    return NULL;
}

