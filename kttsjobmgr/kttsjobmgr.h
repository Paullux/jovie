//
// C++ Interface: kttsjobmgr
//
// Description: 
//
//
// Author: Gary Cramblitt <garycramblitt@comcast.net>, (C) 2004
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef _KTTSJOBMGRPART_H_
#define _KTTSJOBMGRPART_H_

#include <kparts/browserextension.h>
#include <klibloader.h>

#include "kspeech_stub.h"
#include "kspeechsink.h"

class KAboutData;
class KInstance;
class KttsJobMgrBrowserExtension;
class KListView;
class QListViewItem;
class KToolBar;
class QLabel;

class KttsJobMgrFactory : public KLibFactory
{
    Q_OBJECT
public:
    KttsJobMgrFactory() {};
    virtual ~KttsJobMgrFactory();

    virtual QObject* createObject(QObject* parent = 0, const char* name = 0,
                            const char* classname = "QObject",
                            const QStringList &args = QStringList());

    static KInstance *instance();
    static KAboutData *aboutData();

private:
    static KInstance *s_instance;
};

class KttsJobMgrPart: 
    public KParts::ReadOnlyPart,
    public kspeech_stub,
    virtual public KSpeechSink
{
    Q_OBJECT
public:
    KttsJobMgrPart(QWidget *parent, const char *name);
    virtual ~KttsJobMgrPart();

protected:
    virtual bool openFile();
    virtual bool closeURL();
    /**
    * Set up toolbar and menu.
    */
    void setupActions();
    
    /** DCOP Methods connected to DCOP Signals emitted by KTTSD. */
    
    /**
    * This signal is emitted when KTTSD starts or restarts after a call to reinit.
    */
    // ASYNC kttsdStarted(bool) { };
    /**
    * This signal is emitted just before KTTSD exits.
    */
    // ASYNC kttsdExiting(bool) { };
    
    /**
    * This signal is emitted when the speech engine/plugin encounters a marker in the text.
    * @param appId          DCOP application ID of the application that queued the text.
    * @param markerName     The name of the marker seen.
    * @see markers
    */
    ASYNC markerSeen(const QCString& appId, const QString& markerName);
    /**
    * This signal is emitted whenever a sentence begins speaking.
    * @param appId          DCOP application ID of the application that queued the text.
    * @param jobNum         Job number of the text job.
    * @param seq            Sequence number of the text.
    * @see getTextCount
    */
    ASYNC sentenceStarted(const QCString& appId, const uint jobNum, const uint seq);
    /**
    * This signal is emitted when a sentence has finished speaking.
    * @param appId          DCOP application ID of the application that queued the text.
    * @param jobNum         Job number of the text job.
    * @param seq            Sequence number of the text.
    * @see getTextCount
    */        
    ASYNC sentenceFinished(const QCString& appId, const uint jobNum, const uint seq);
    
    /**
    * This signal is emitted whenever a new text job is added to the queue.
    * @param appId          The DCOP senderId of the application that created the job.  NULL if kttsd.
    * @param jobNum         Job number of the text job.
    */
    ASYNC textSet(const QCString& appId, const uint jobNum);

    /**
    * This signal is emitted whenever speaking of a text job begins.
    * @param appId          The DCOP senderId of the application that created the job.  NULL if kttsd.
    * @param jobNum         Job number of the text job.
    */
    ASYNC textStarted(const QCString& appId, const uint jobNum);
    /**
    * This signal is emitted whenever a text job is finished.  The job has
    * been marked for deletion from the queue and will be deleted when another
    * job reaches the Finished state. (Only one job in the text queue may be
    * in state Finished at one time.)  If @ref startText or @ref resumeText is
    * called before the job is deleted, it will remain in the queue for speaking.
    * @param appId          The DCOP senderId of the application that created the job.  NULL if kttsd.
    * @param jobNum         Job number of the text job.
    */
    ASYNC textFinished(const QCString& appId, const uint jobNum);
    /**
    * This signal is emitted whenever a speaking text job stops speaking.
    * @param appId          The DCOP senderId of the application that created the job.  NULL if kttsd.
    * @param jobNum         Job number of the text job.
    */
    ASYNC textStopped(const QCString& appId, const uint jobNum);
    /**
    * This signal is emitted whenever a speaking text job is paused.
    * @param appId          The DCOP senderId of the application that created the job.  NULL if kttsd.
    * @param jobNum         Job number of the text job.
    */
    ASYNC textPaused(const QCString& appId, const uint jobNum);
    /**
    * This signal is emitted when a text job, that was previously paused, resumes speaking.
    * @param appId          The DCOP senderId of the application that created the job.  NULL if kttsd.
    * @param jobNum         Job number of the text job.
    */
    ASYNC textResumed(const QCString& appId, const uint jobNum);
    /**
    * This signal is emitted whenever a text job is deleted from the queue.
    * The job is no longer in the queue when this signal is emitted.
    * @param appId          The DCOP senderId of the application that created the job.  NULL if kttsd.
    * @param jobNum         Job number of the text job.
    */
    ASYNC textRemoved(const QCString& appId, const uint jobNum);

private slots:
    /**
    * This slot is connected to the Job List View selectionChanged signal.
    */
    void slot_selectionChanged(QListViewItem* item);
    /**
    * Slots connected to toolbar buttons.
    */
    void slot_job_hold();
    void slot_job_resume();
    void slot_job_restart();
    void slot_job_remove();
    void slot_job_move();
    void slot_job_change_talker();
    void slot_speak_clipboard();
    void slot_speak_file();
    void slot_refresh();
    void slot_job_prev_par();
    void slot_job_prev_sen();
    void slot_job_next_sen();
    void slot_job_next_par();

private:
    /**
    * @enum JobListViewColumn
    * Columns in the Job List View.
    */
    enum JobListViewColumn
    {
        jlvcJobNum = 0,               /**< Job Number. */
        jlvcOwner = 1,                /**< AppId of job owner */
        jlvcLanguage = 2,             /**< Job language code */
        jlvcState = 3,                /**< Job State */
        jlvcPosition = 4,             /**< Current sentence of job. */
        jlvcSentences = 5,            /**< Number of sentences in job. */
        jlvcFiller = 6
    };
    
    /**
    * Convert a KTTSD job state integer into a display string.
    * @param state          KTTSD job state
    * @return               Display string for the state.
    */
    QString stateToStr(int state);
    
    /**
    * Get the Job Number of the currently-selected job in the Job List View.
    * @return               Job Number of currently-selected job.
    *                       0 if no currently-selected job.
    */
    uint getCurrentJobNum();
    
    /**
    * Given a Job Number, returns the Job List View item containing the job.
    * @param jobNum         Job Number.
    * @return               QListViewItem containing the job or 0 if not found.
    */
    QListViewItem* findItemByJobNum(const uint jobNum);
    
    /**
    * Enables or disables all the job-related buttons on the toolbar.
    * @param enable        True to enable the job-related butons.  False to disable.
    */
    void enableJobActions(bool enable);
    
    /**
    * Refresh display of a single job in the JobListView.
    * @param jobNum         Job Number.
    */
    void refreshJob(uint jobNum);
    
    /**
    * Fill the Job List View.
    */
    void refreshJobListView();
    
    /**
    * If nothing selected in Job List View and list not empty, select top item.
    * If nothing selected and list is empty, disable job buttons on toolbar.
    */
    void autoSelectInJobListView();
    
    /**
    * Job List View
    */
    KListView* m_jobListView;
    KttsJobMgrBrowserExtension *m_extension;
    
    /**
    * Current sentence box.
    */
    QLabel* m_currentSentence;

    /**
    * Toolbars.
    */    
    KToolBar* m_toolBar1;
    KToolBar* m_toolBar2;
    KToolBar* m_toolBar3;
    
    /**
    * This flag is set to True whenever we want to select the next job that
    * is announced in a textSet signal.
    */
    bool selectOnTextSet;
};

class KttsJobMgrBrowserExtension : public KParts::BrowserExtension
{
    Q_OBJECT
    friend class KttsJobMgrPart;
public:
    KttsJobMgrBrowserExtension(KttsJobMgrPart *parent);
    virtual ~KttsJobMgrBrowserExtension();
};

#endif    // _KTTSJOBMGRPART_H_
