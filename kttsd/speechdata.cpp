/***************************************************** vim:set ts=4 sw=4 sts=4:
  speechdata.cpp
  This contains the SpeechData class which is in charge of maintaining
  all the data on the memory.
  It maintains queues, mutex, a wait condition and has methods to enque
  messages and warnings and manage the text that is thread safe.
  We could say that this is the common repository between the KTTSD class
  (dcop service) and the Speaker class (speaker, loads plug ins, call plug in
  functions)
  -------------------
  Copyright:
  (C) 2002-2003 by José Pablo Ezequiel "Pupeno" Fernández <pupeno@kde.org>
  (C) 2003-2004 by Olaf Schmidt <ojschmidt@kde.org>
  (C) 2004 by Gary Cramblitt <garycramblitt@comcast.net>
  -------------------
  Original author: José Pablo Ezequiel "Pupeno" Fernández
 ******************************************************************************/

/******************************************************************************
 *                                                                            *
 *    This program is free software; you can redistribute it and/or modify    *
 *    it under the terms of the GNU General Public License as published by    *
 *    the Free Software Foundation; either version 2 of the License.          *
 *                                                                            *
 ******************************************************************************/

#include <stdlib.h>

#include <kdebug.h>
#include <kglobal.h>
#include <kapplication.h>
#include <qregexp.h>
#include <qpair.h>
#include <qvaluelist.h>

#include "speechdata.h"
#include "speechdata.moc"

/**
* Constructor
* Sets text to be stopped and warnings and messages queues to be autodelete.
* Loads configuration.
*/
SpeechData::SpeechData(){
    kdDebug() << "Running: SpeechData::SpeechData()" << endl;
    // The text should be stoped at the beggining (thread safe)
    textMutex.lock();
    reading = false;
    jobCounter = 0;
    textIterator = 0;
    textJobs.setAutoDelete(true);
    textMutex.unlock();

    // Warnings queue to be autodelete  (thread safe)
    warningsMutex.lock();
    warnings.setAutoDelete(true);
    warningsMutex.unlock();

    // Messages queue to be autodelete (thread safe)
    messagesMutex.lock();
    messages.setAutoDelete(true);
    messagesMutex.unlock();

    // Load configuration
    //config = KGlobal::config();
    config = new KConfig("kttsdrc");
}

bool SpeechData::readConfig(){
    // Load configuration
    // Set the group general for the configuration of KTTSD itself (no plug ins)
    config->setGroup("General");

    // Load the configuration of the text interruption messages and sound
    textPreMsgEnabled = config->readBoolEntry("TextPreMsgEnabled", false);
    textPreMsg = config->readEntry("TextPreMsg");

    textPreSndEnabled = config->readBoolEntry("TextPreSndEnabled", false);
    textPreSnd = config->readPathEntry("TextPreSnd");

    textPostMsgEnabled = config->readBoolEntry("TextPostMsgEnabled", false);
    textPostMsg = config->readEntry("TextPostMsg");

    textPostSndEnabled = config->readBoolEntry("TextPostSndEnabled", false);
    textPostSnd = config->readPathEntry("TextPostSnd");

    // Load the configuration of the par interruption messages and sound
    parPreMsgEnabled = config->readBoolEntry("ParPreMsgEnabled", false);
    parPreMsg = config->readEntry("ParPreMsg");

    parPreSndEnabled = config->readBoolEntry("ParPreSndEnabled", false);
    parPreSnd = config->readPathEntry("ParPreSnd");

    parPostMsgEnabled = config->readBoolEntry("ParPostMsgEnabled", false);
    parPostMsg = config->readEntry("ParPostMsg");

    parPostSndEnabled = config->readBoolEntry("ParPostSndEnabled", false);
    parPostSnd = config->readPathEntry("ParPostSnd");

    if(config->hasKey("DefaultLanguage")){
        defaultTalker = config->readEntry("DefaultLanguage");
        return true;
    } else {
        return false;
    }
}
/**
* Destructor
*/
SpeechData::~SpeechData(){
    kdDebug() << "Running: SpeechData::~SpeechData()" << endl;
    delete config;
    delete textIterator;
}

/**
* Add a new warning to the queue (thread safe)
*/
void SpeechData::enqueueWarning( const QString &warning, const QString &talker, const QCString &appId ){
    kdDebug() << "Running: SpeechData::enqueueWarning( const QString &warning )" << endl;
    mlText *temp = new mlText();
    temp->text = warning;
    temp->talker = talker;
    temp->appId = appId;
    temp->seq = 1;
    warningsMutex.lock();
    warnings.enqueue( temp );
    uint count = warnings.count();
    warningsMutex.unlock();
    kdDebug() << "Adding '" << temp->text << "' with talker '" << temp->talker << "' from application " << appId << " to the warnings queue leaving a total of " << count << " items." << endl;
    newTMW.wakeOne();
}

/**
* Pop (get and erase) a warning from the queue (thread safe)
*/
mlText SpeechData::dequeueWarning(){
    kdDebug() << "Running: SpeechData::dequeueWarning()" << endl;
    warningsMutex.lock();
    mlText *temp = warnings.dequeue();
    uint count = warnings.count();
    warningsMutex.unlock();
    kdDebug() << "Removing '" << temp->text << "' with talker '" << temp->talker << "' from the warnings queue leaving a total of " << count << " items." << endl;
    return *temp;
}

/**
* Are there any Warnings (thread safe)
*/
bool SpeechData::warningInQueue(){
    kdDebug() << "Running: SpeechData::warningInQueue() const" << endl;
    warningsMutex.lock();
    bool temp = !warnings.isEmpty();
    warningsMutex.unlock();
    if(temp){
        kdDebug() << "The warnings queue is NOT empty" << endl;
    } else {
        kdDebug() << "The warnings queue is empty" << endl;
    }
    return temp;
}

/**
* Add a new message to the queue (thread safe)
*/
void SpeechData::enqueueMessage( const QString &message, const QString &talker, const QCString& appId ){
    kdDebug() << "Running: SpeechData::enqueueMessage" << endl;
    mlText *temp = new mlText();
    temp->text = message;
    temp->talker = talker;
    temp->appId = appId;
    temp->seq = 1;
    messagesMutex.lock();
    messages.enqueue( temp );
    uint count = messages.count();
    messagesMutex.unlock();
    kdDebug() << "Adding '" << temp->text << "' with talker '" << temp->talker << "' from application " << appId << " to the messages queue leaving a total of " << count << " items." << endl;
    newTMW.wakeOne();
}

/**
* Pop (get and erase) a message from the queue (thread safe)
*/
mlText SpeechData::dequeueMessage(){
    kdDebug() << "Running: SpeechData::dequeueMessage()" << endl;
    messagesMutex.lock();
    mlText *temp = messages.dequeue();
    uint count = warnings.count();
    messagesMutex.unlock();
    kdDebug() << "Removing '" << temp->text << "' with talker '" << temp->talker << "' from the messages queue leaving a total of " << count << " items." << endl;
    return *temp;
}

/**
* Are there any Message (thread safe)
*/
bool SpeechData::messageInQueue(){
    kdDebug() << "Running: SpeechData::messageInQueue() const" << endl;
    messagesMutex.lock();
    bool temp = !messages.isEmpty();
    messagesMutex.unlock();
    if(temp){
        kdDebug() << "The messages queue is NOT empty" << endl;
    } else {
        kdDebug() << "The messages queue is empty" << endl;
    }
    return temp;
}

/**
* Queues a text job. (thread safe)
*/
uint SpeechData::setText( const QString &text, const QString &talker, const QCString &appId ){
    kdDebug() << "Running: SpeechData::setText" << endl;
    // There has to be a better way
    kdDebug() << "I'm getting: " << endl << text << " from application " << appId << endl;
    // See if app has specified a custom sentence delimiter and use it, otherwise use default.
    QRegExp sentenceDelimiter;
    if (sentenceDelimiters.find(appId) != sentenceDelimiters.end())
        sentenceDelimiter = QRegExp(sentenceDelimiters[appId]);
    else
        sentenceDelimiter = QRegExp("([\\.\\?\\!\\:\\;])\\s");
    QString temp = text;
    // Append sentence delimiters with newline.
    temp.replace(sentenceDelimiter, "\\1\n");
    // Remove leading spaces.
    temp.replace(QRegExp("\\n[ \\t]+"), "\n");
    // Remove trailing spaces.
    temp.replace(QRegExp("[ \\t]+\\n"), "\n");
    // Remove excess paragraph separators.
    temp.replace(QRegExp("\n\n\n+"),"\n\n");
    QStringList tempList = QStringList::split('\n', temp, true);
/*
    // This should be something better, like "[a-zA-Z]\. " (a regexp of course) The dot (.) is used for more than ending a sentence.
    temp.replace('.', '\n');
    QStringList tempList = QStringList::split('\n', temp, true);
*/
    
//    for ( QStringList::Iterator it = tempList.begin(); it != tempList.end(); ++it ) {
//        kdDebug() << "'" << *it << "'" << endl;
//    }

    textMutex.lock();
    if (talker != NULL)
        textTalker = talker;
    else
        textTalker = defaultTalker;
    mlJob* job = new mlJob;
    uint jobNum = ++jobCounter;
    job->jobNum = jobNum;
    job->appId = appId;
    job->talker = textTalker;
    job->state = kspeech::jsQueued;
    job->seq = 0;
    job->sentences = tempList;
    // Save current pointer in job queue.
    int currentJobIndex = textJobs.at();
    textJobs.append(job);
    // Restore current pointer in job queue, if there is no current pointer, set it to the job
    // we just added.
    if (currentJobIndex >= 0)
        textJobs.at(currentJobIndex);
    else
        textJobs.first();
    textMutex.unlock();
    emit textSet(appId, jobNum);
    return jobNum;
}

/**
* Given an appId, returns the first job with that appId.
* @param appId          The DCOP senderId of the application.  NULL if kttsd.
* @return               Pointer to the text job.
* If no such job, returns 0.
* If appId is NULL, returns the first job in the queue.
* Does not change textJobs.current().
*/
mlJob* SpeechData::findFirstJobByAppId(const QCString& appId)
{
    if (appId == NULL)
        return textJobs.getFirst();
    else
    {
        QPtrListIterator<mlJob> it(textJobs);
        for ( ; it.current(); ++it )
        {
            if (it.current()->appId == appId)
            {
                return it.current();
            }
        }
        return 0;
    }
}

/**
* Given an appId, returns the last (most recently queued) job with that appId.
* @param appId          The DCOP senderId of the application.  NULL if kttsd.
* @return               Pointer to the text job.
* If no such job, returns 0.
* If appId is NULL, returns the last job in the queue.
* Does not change textJobs.current().
*/
mlJob* SpeechData::findLastJobByAppId(const QCString& appId)
{
    if (appId == NULL)
        return textJobs.getLast();
    else
    {
        QPtrListIterator<mlJob> it(textJobs);
        for (it.toLast() ; it.current(); --it )
        {
            if (it.current()->appId == appId)
            {
                return it.current();
            }
        }
        return 0;
    }
}

/**
* Given an appId, returns the last (most recently queued) job with that appId,
* or if no such job, the last (most recent) job in the queue.
* @param appId          The DCOP senderId of the application.  NULL if kttsd.
* @return               Pointer to the text job.
* If no such job, returns 0.
* If appId is NULL, returns the last job in the queue.
* Does not change textJobs.current().
*/
mlJob* SpeechData::findAJobByAppId(const QCString& appId)
{
    mlJob* job = findLastJobByAppId(appId);
    if (!job) job = textJobs.getLast();
    return job;
}

/**
* Given a jobNum, returns the first job with that jobNum.
* @return               Pointer to the text job.
* If no such job, returns 0.
* Does not change textJobs.current().
*/
mlJob* SpeechData::findJobByJobNum(const uint jobNum)
{
    QPtrListIterator<mlJob> it(textJobs);
    for ( ; it.current(); ++it )
    {
        if (it.current()->jobNum == jobNum)
        {
            return it.current();
        }
    }
    return 0;
}

/**
* Given a jobNum, returns the appId of the application that owns the job.
* @param jobNum         Job number of the text job.
* @return               appId of the job.
* If no such job, returns "".
* Does not change textJobs.current().
*/
QCString SpeechData::getAppIdByJobNum(const uint jobNum)
{
    QCString appId;
    mlJob* job = findJobByJobNum(jobNum);
    if (job) appId = job->appId;
    return appId;
}

/**
* Delete a text job (thread safe)
*/
void SpeechData::removeText(const uint jobNum, const QCString& appId){
    kdDebug() << "Running: SpeechData::removeText" << endl;
    textMutex.lock();
    mlJob* removeJob = 0;
    uint removeJobNum = 0;
    QCString removeAppId;    // The appId of the removed (and stopped) job.
    uint stoppedJobNum = 0;
    bool startAnotherJob = false;
    uint startedJobNum = 0;
    // If jobNum is zero, find the first job owned by the appId.
    // If appId is NULL, the request is coming from kttsd, which is allowed to delete any job.
    if (jobNum) removeJob = findJobByJobNum(jobNum); else removeJob = findAJobByAppId(appId);
    if (removeJob)
    {
        // Verify that app is allowed to delete the job.
        removeAppId = removeJob->appId;
//        if (appId == NULL || removeAppId == appId)
        {
            removeJobNum = removeJob->jobNum;
            // If removing the current job, delete the text iterator.
            if (textJobs.current() == removeJob)
            {
                // Start another job after deleting this one.
                startAnotherJob = true;
                // If currently speaking this job, stop it.
                if (reading)
                {
                    stoppedJobNum = removeJob->jobNum;
                    reading = false;
                }
                delete textIterator;
                textIterator = 0;
            }
            // Delete the job.
            textJobs.removeRef(removeJob);
        }
    }
    textMutex.unlock();
    if (stoppedJobNum) emit textStopped(removeAppId, stoppedJobNum);
    if (removeJobNum) emit textRemoved(removeAppId, removeJobNum);
    if (startAnotherJob) startedJobNum = startNextJob();
    if (startedJobNum) emit textStarted(getAppIdByJobNum(startedJobNum), startedJobNum);
}

/**
* Start the next speakable job on the queue.
* @return               Job number of the started text job.
* Should not be called when another job is active.
* Leaves textJobs.current() pointing to the started job.
* If no job started, textJobs.current() becomes 0.
* Sets up the text iterator for the job.
* Returns 0 if no speakable jobs.
*/
uint SpeechData::startNextJob()
{
    kdDebug() << "startNextJob: startNextJob called" << endl;
    bool wasNotLocked = textMutex.tryLock();
    uint startedJobNum = 0;
    mlJob* job;
    mlJob* currentJob = textJobs.current();
    if (currentJob) kdDebug() << "startNextJob: the current job is " << currentJob->jobNum << endl;
    reading = false;
    for (job = textJobs.first(); (job); job = textJobs.next())
    {
        kdDebug() << "startNextJob: job " << job->jobNum << " has state " << job->state << endl;
        // If job was paused, remain paused.
//        if (job->state == kspeech::jsPaused) break;
        // Start the job if it is speakable.
        if (job->state == kspeech::jsSpeakable)
        {
            startedJobNum = job->jobNum;
            kdDebug() << "startNextJob: job " << job->jobNum << " is startable " << endl;
            // If started job is not the current job, set up text iterator.
            if (job != currentJob)
            {
                kdDebug() << "startNextJob: setting up text interator" << endl;
                delete textIterator;
                textIterator = new QStringList::Iterator(job->sentences.begin());
                // When resuming a previously paused job, position iterator at paused position.
                int cnt = 0;
                for ( ; cnt < job->seq; ++cnt) ++*textIterator;
            }
            job->state = kspeech::jsSpeaking;
            reading = true;
            break;
        }
    }
    if (wasNotLocked) textMutex.unlock();
    kdDebug() << "startNextJob: returning job number " << startedJobNum << endl;
    return startedJobNum;
}

/**
* Delete expired jobs.  At most, one finished job is kept on the queue.
* (thread safe)
* @param finishedJobNum Job number of a job that just finished.
* The just finished job is not deleted, but any other finished jobs are.
* Does not change the textJobs.current() pointer.
*/
void SpeechData::deleteExpiredJobs(const uint finishedJobNum)
{
    // Save current pointer.
    textMutex.lock();
    mlJob* currentJob = textJobs.current();
    typedef QPair<QCString, uint> removedJob;
    typedef QValueList<removedJob> removedJobsList;
    removedJobsList removedJobs;
    // Walk through jobs and delete any other finished jobs.
    for (mlJob* job = textJobs.first(); (job); job = textJobs.next())
    {
        if (job->jobNum != finishedJobNum and job->state == kspeech::jsFinished)
        {
            removedJobs.append(removedJob(job->appId, job->jobNum));
            textJobs.removeRef(job);
        }
    }
    // Restore current pointer in job queue.
    if (currentJob) textJobs.findRef(currentJob);
    textMutex.unlock();
    // Emit signals for removed jobs.
    removedJobsList::iterator it;
    for (it = removedJobs.begin(); it != removedJobs.end() ; ++it)
    {
        QCString appId = (*it).first;
        uint jobNum = (*it).second;
        textRemoved(appId, jobNum);
    }
}

/**
* Get a sentence to speak it.
*/
mlText SpeechData::getSentenceText()
{
    kdDebug() << "Running: QString getSentenceText()" << endl;
    textMutex.lock();
    mlText* temp = new mlText;
    uint startedJobNum = 0;
    QCString startedAppId;
    uint finishedJobNum = 0;
    QCString finishedAppId;
    uint resumedJobNum = 0;
    QCString resumedAppId;
    // If not speaking a job, start first speakable job on queue.
    if (!reading) startedJobNum = startNextJob();
    mlJob* job = textJobs.current();
    if (job)
    {
        kdDebug() << "getSentenceText: current job number " << job->jobNum << " has state " << job->state << endl;
        // If job is paused, unpause it.
        if (job->state == kspeech::jsPaused)
        {
            job->state = kspeech::jsSpeaking;
            resumedJobNum = job->jobNum;
            resumedAppId = job->appId;
        }
        
        // If we run out of sentences, finish the job and mark it deleteable.
        if (*textIterator == job->sentences.end())
        {
            kdDebug() << "getSentenceText: Job finished" << endl;
            finishedJobNum = job->jobNum;
            finishedAppId = job->appId;
            reading = false;
            delete textIterator;
            textIterator = 0;
            job->state = kspeech::jsFinished;
            startedJobNum = startNextJob();
            job = textJobs.current();
        }
        // Still have a job to do?
        if (job)
        {
            if (startedJobNum) startedAppId = job->appId;
            kdDebug() << "getSentenceText: retrieving next sentence" << endl;
            // Get the next sentence.
            temp->text = **textIterator;
            ++*textIterator;
            temp->appId = job->appId;
            temp->talker = job->talker;
            temp->jobNum = job->jobNum;
            temp->seq = ++(job->seq);
            kdDebug() << "getSentenceText: returning seq number " << temp->seq << endl;
        }
    }
    textMutex.unlock();
    // Delete expired jobs from queue.
    if (finishedJobNum) deleteExpiredJobs(finishedJobNum);
    if (resumedJobNum) emit textResumed(resumedAppId, resumedJobNum);
    if (finishedJobNum) emit textFinished(finishedAppId, finishedJobNum);
    if (startedJobNum) emit textStarted(startedAppId, startedJobNum);
    kdDebug() << "getSentenceText: returning" << endl;
    return *temp;
 }

/**
* Sets the GREP pattern that will be used as the sentence delimiter.
* @param delimiter      A valid GREP pattern.
* @param appId          The DCOP senderId of the application.  NULL if kttsd.
*
* The default delimiter is
  @verbatim
     ([\\.\\?\\!\\:\\;])\\s
  @endverbatim
*
* Note that backward slashes must be escaped.
*
* Changing the sentence delimiter does not affect other applications.
* @see sentenceparsing
*/
void SpeechData::setSentenceDelimiter(const QString &delimiter, const QCString appId)
{
    delimiterMutex.lock();
    sentenceDelimiters[appId] = delimiter;
    delimiterMutex.unlock();
}

/**
* Get the number of sentences in a text job.
* (thread safe)
* @param jobNum         Job number of the text job.
*                       If zero, applies to the last job queued by the application,
*                       but if no such job, applies to the last job queued by any application.
* @param appId          The DCOP senderId of the application.  NULL if kttsd.
* @return               The number of sentences in the job.  -1 if no such job.
*
* The sentences of a job are given sequence numbers from 1 to the number returned by this
* method.  The sequence numbers are emitted in the sentenceStarted and sentenceFinished signals.
*/
int SpeechData::getTextCount(const uint jobNum, const QCString& appId)
{
    textMutex.lock();
    mlJob* job;
    if (jobNum) job = findJobByJobNum(jobNum); else job = findAJobByAppId(appId);
    int temp;
    if (job)
        temp = job->sentences.count();
    else
        temp = -1;
    textMutex.unlock();
    return temp;
}

/**
* Get the job number of the current text job.
* (thread safe)
* @return               Job number of the current text job. 0 if no jobs.
*
* Note that the current job may not be speaking.  @see isSpeaking.  @see getTextJobState.
*/
uint SpeechData::getCurrentTextJob()
{
    textMutex.lock();
    uint temp;
    mlJob* job = textJobs.current();
    if (job)
        temp = job->jobNum;
    else
        temp = 0;
    textMutex.unlock();
    return temp;
}
        
/**
* Get the number of jobs in the text job queue.
* (thread safe)
* @return               Number of text jobs in the queue.  0 if none.
*/
uint SpeechData::getTextJobCount()
{
    textMutex.lock();
    uint temp = textJobs.count();
    textMutex.unlock();
    return temp;
}

/**
* Get a comma-separated list of text job numbers in the queue.
* @return               Comma-separated list of text job numbers in the queue.
*/
QString SpeechData::getTextJobNumbers()
{
    textMutex.lock();
    QString jobs;
    QPtrListIterator<mlJob> it(textJobs);
    for ( ; it.current(); ++it )
    {
        if (jobs != "") jobs.append(",");
        jobs.append(QString::number(it.current()->jobNum));
    }
    textMutex.unlock();
    return jobs;
}
        
/**
* Get the state of a text job.
* (thread safe)
* @param jobNum         Job number of the text job.
*                       If zero, applies to the last job queued by the application,
*                       but if no such job, applies to the last job queued by any application.
* @param appId          The DCOP senderId of the application.  NULL if kttsd.
* @return               State of the job. -1 if invalid job number.
*/
int SpeechData::getTextJobState(const uint jobNum, const QCString& appId)
{
    textMutex.lock();
    mlJob* job;
    if (jobNum) job = findJobByJobNum(jobNum); else job = findAJobByAppId(appId);
    int temp;
    if (job)
        temp = job->state;
    else
        temp = -1;
    textMutex.unlock();
    return temp;
}

/**
* @param jobNum         Job number of the text job.
*                       If zero, applies to the last job queued by the application,
*                       but if no such job, applies to the last job queued by any application.
* @param appId          The DCOP senderId of the application.  NULL if kttsd.
* @return               A QDataStream containing information about the job.
*                       Blank if no such job.
*
* The stream contains the following elements:
*   - int state         Job state.
*   - QCString appId    DCOP senderId of the application that requested the speech job.
*   - QString talker    Language code in which to speak the text.
*   - int seq           Current sentence being spoken.  Sentences are numbered starting at 1.
*   - int sentenceCount Total number of sentences in the job.
*
* The following sample code will decode the stream:
          @verbatim
            QByteArray jobInfo = getTextJobInfo(jobNum);
            QDataStream stream(jobInfo, IO_ReadOnly);
            int state;
            QCString appId;
            QString talker;
            int seq;
            int sentenceCount;
            stream >> state;
            stream >> appId;
            stream >> talker;
            stream >> seq;
            stream >> sentenceCount;
          @endverbatim
*/
QByteArray SpeechData::getTextJobInfo(const uint jobNum, const QCString& appId)
{
    textMutex.lock();
    mlJob* job;
    if (jobNum) job = findJobByJobNum(jobNum); else job = findAJobByAppId(appId);
    QByteArray temp;
    if (job)
    {
        QDataStream stream(temp, IO_WriteOnly);
        stream << job->state;
        stream << job->appId;
        stream << job->talker;
        stream << job->seq;
        stream << job->sentences.count();
    }
    textMutex.unlock();
    return temp;
}

/**
* Return a sentence of a job.
* @param jobNum         Job number of the text job.
*                       If zero, applies to the last job queued by the application,
*                       but if no such job, applies to the last job queued by any application.
* @param seq            Sequence number of the sentence.
* @param appId          The DCOP senderId of the application.  NULL if kttsd.
* @return               The specified sentence in the specified job.  If not such
*                       job or sentence, returns "".
*/
QString SpeechData::getTextJobSentence(const uint jobNum, const uint seq, const QCString& appId)
{
    textMutex.lock();
    mlJob* job;
    if (jobNum) job = findJobByJobNum(jobNum); else job = findAJobByAppId(appId);
    QString temp;
    if (job)
    {
        temp = job->sentences[seq - 1];
    }
    textMutex.unlock();
    return temp;
}
       
/**
* Determine if kttsd is currently speaking any text jobs.
* (thread safe)
* @return               True if currently speaking any text jobs.
*/
bool SpeechData::currentlyReading(){
    kdDebug() << "Running: SpeechData::currentlyReading()" << endl;
    textMutex.lock();
    bool temp = reading;
    textMutex.unlock();
    return temp;
}
bool SpeechData::isSpeakingText() { return currentlyReading(); }

/**
* Pause playing text (thread safe)
*/
void SpeechData::pauseText(const uint jobNum, const QCString& appId){
    kdDebug() << "Running: SpeechData::pauseText" << endl;
    textMutex.lock();
    mlJob* job = 0;
    uint pausedJobNum = 0;
    QCString pausedAppId;
    if (jobNum) job = findJobByJobNum(jobNum); else job = findAJobByAppId(appId);
    if (job)
    {
//        if (appId == NULL or job->appId == appId)
        {
            job->state = kspeech::jsPaused;
            // If we paused the current job, stop speaking.
            if (job == textJobs.current())
            {
                pausedJobNum = job->jobNum;
                pausedAppId = job->appId;
                reading = false;
            }
        }
    }
    textMutex.unlock();
    if (pausedJobNum) emit textPaused(pausedAppId, pausedJobNum);
}

/**
* Stop playing text and go to the begining (thread safe)
*/
void SpeechData::stopText(const uint jobNum, const QCString& appId){
    kdDebug() << "Running: SpeechData::stopText" << endl;
    textMutex.lock();
    mlJob* job = 0;
    uint stoppedJobNum = 0;
    QCString stoppedAppId;
    if (jobNum) job = findJobByJobNum(jobNum); else job = findAJobByAppId(appId);
    if (job)
    {
//        if (appId == NULL or job->appId == appId)
        {
            // Rewind the stopped job to the beginning and mark it not speakable.
            job->seq = 0;
            job->state = kspeech::jsQueued;
            // If we stopped the current job, stop speaking.
            if (job == textJobs.current())
            {
                reading = false;
                stoppedJobNum = job->jobNum;
                stoppedAppId = job->appId;
                delete textIterator;
                textIterator = new QStringList::Iterator(job->sentences.begin());
            }
        }
    }
    textMutex.unlock();
    if (stoppedJobNum) emit textStopped(stoppedAppId, stoppedJobNum);
}

/**
* Start text job at the beginning (thread safe)
*/
void SpeechData::startText(const uint jobNum, const QCString& appId){
    kdDebug() << "Running: SpeechData::startText" << endl;
    textMutex.lock();
    mlJob* job = 0;
    uint startedJobNum = 0;
    if (jobNum) job = findJobByJobNum(jobNum); else job = findAJobByAppId(appId);
    if (job)
    {
        kdDebug() << "starting job number " << job->jobNum << endl;
//        if (appId == NULL or job->appId == appId)
        {
            // Mark the job speakable and rewind to the beginning.
            job->state = kspeech::jsSpeakable;
            job->seq = 0;
            // If this is the current job, reset the text iterator.
            if (job == textJobs.current())
            {
                delete textIterator;
                textIterator = new QStringList::Iterator(job->sentences.begin());
            }
            // If not already speaking, start a job (maybe this one).
            if (!reading) startedJobNum = startNextJob();
        }
    }
    textMutex.unlock();
    if (startedJobNum)
    {
        kdDebug() << "started job number is " << startedJobNum << endl;
        // Did we start or resume the new job?
        job = textJobs.current();
        QCString startedAppId = job->appId;
        if (job->seq == 0)
            emit textStarted(startedAppId, startedJobNum);
        else
            emit textResumed(startedAppId, startedJobNum);
        newTMW.wakeOne();
    }
}

/**
* Resume text job if paused, otherwise start at the beginning
*/
void SpeechData::resumeText(const uint jobNum, const QCString& appId){
    kdDebug() << "Running: SpeechData::resumeText" << endl;
    textMutex.lock();
    mlJob* job = 0;
    uint startedJobNum = 0;
    if (jobNum) job = findJobByJobNum(jobNum); else job = findAJobByAppId(appId);
    if (job)
    {
//        if (appId == NULL || job->appId == appId)
        {
            // If job is not paused, it is the same as calling startText.
            if (job->state != kspeech::jsPaused)
            {
                textMutex.unlock();
                startText(jobNum, appId);
                return;
            }
            else
            {
                // Mark the job speakable.
                job->state = kspeech::jsSpeakable;
                // If not speaking, start a job (maybe this one).
                if (!reading) startedJobNum = startNextJob();
            }
        }
    }
    textMutex.unlock();
    if (startedJobNum)
    {
        // Did we start or resume the new job?
        job = textJobs.current();
        QCString startedAppId = job->appId;
        if (job->seq == 0)
            emit textStarted(startedAppId, startedJobNum);
        else
            emit textResumed(startedAppId, startedJobNum);
        newTMW.wakeOne();
    }
}

/**
* Change the talker for a text job.
* @param jobNum         Job number of the text job.
*                       If zero, applies to the last job queued by the application,
*                       but if no such job, applies to the last job queued by any application.
* @param talker         New code for the language to be spoken in.  Example "en".
*                       If NULL, defaults to the user's default talker.
*                       If no plugin has been configured for the specified language code,
*                       defaults to the user's default talker.
* @param appId          The DCOP senderId of the application.  NULL if kttsd.
*/
void SpeechData::changeTextTalker(const uint jobNum, const QString &talker, const QCString& appId)
{
    textMutex.lock();
    mlJob* job = 0;
    if (jobNum) job = findJobByJobNum(jobNum); else job = findAJobByAppId(appId);
    if (job)
    {
        if (talker != "")
            job->talker = talker;
        else
            job->talker = defaultTalker;
    }
    textMutex.unlock();
}

/**
* Move a text job down in the queue so that it is spoken later.
* @param jobNum         Job number of the text job.
*                       If zero, applies to the last job queued by the application,
*                       but if no such job, applies to the last job queued by any application.
* @param appId          The DCOP senderId of the application.  NULL if kttsd.
*
* If the job is currently speaking, it is paused.
* If the next job in the queue is speakable, it begins speaking.
*/
void SpeechData::moveTextLater(const uint jobNum, const QCString& appId)
{
    kdDebug() << "Running: SpeechData::moveTextLater" << endl;
    // Save current pointer.
    textMutex.lock();
    mlJob* currentJob = textJobs.current();
    uint pausedJobNum = 0;
    uint startedJobNum = 0;
    QCString pausedAppId;
    mlJob* job;
    if (jobNum) job = findJobByJobNum(jobNum); else job = findAJobByAppId(appId);
    if (job)
    {
        uint movedJobNum = job->jobNum;
        // Get index of the job.
        uint index = textJobs.findRef(job);
        // If job is currently speaking, pause it.
        if (job->state == kspeech::jsSpeaking)
        {
            pausedJobNum = job->jobNum;
            pausedAppId = job->appId;
            reading = false;
            job->state = kspeech::jsPaused;
        }
        // Move job down one position in the queue.
        kdDebug() << "In SpeechData::moveTextLater, moving jobNum " << movedJobNum << endl;
        if (textJobs.insert(index + 2, job)) textJobs.take(index);
        
        // Restore current pointer in job queue.
        if (currentJob) textJobs.findRef(currentJob);
        // Start another job, maybe the one we just moved.
        startedJobNum = startNextJob();
    }
    textMutex.unlock();
    if (pausedJobNum) emit textPaused(pausedAppId, pausedJobNum);
    if (startedJobNum)
    {
        kdDebug() << "started job number is " << startedJobNum << endl;
        // Did we start or resume the new job?
        job = textJobs.current();
        QCString startedAppId = job->appId;
        if (job->seq == 0)
            emit textStarted(startedAppId, startedJobNum);
        else
            emit textResumed(startedAppId, startedJobNum);
        newTMW.wakeOne();
    }
}

/**
* Jump to the previous paragraph (thread safe)
*/
void SpeechData::prevParText(const uint jobNum, const QCString& appId){
    kdDebug() << "Running: SpeechData::prevParText" << endl;
    textMutex.lock();
    mlJob* job = 0;
    if (jobNum) job = findJobByJobNum(jobNum); else job = findAJobByAppId(appId);
    if (job)
    {
//        if (appId == NULL or job->appId == appId)
        {
            QStringList::Iterator* it;
            if (job == textJobs.current())
                it = textIterator;
            else
                it = new QStringList::Iterator(job->sentences.at(job->seq - 1));
            if (*it != job->sentences.begin())
            {
                --job->seq;
                --*it;
            }
            while(*it != job->sentences.begin() and **it != "")
            {
                --job->seq;
                --*it;
            }
            if (job != textJobs.current()) delete it;
        }
    }
    textMutex.unlock();
}

/**
* Jump to the previous sentence (thread safe)
*/
void SpeechData::prevSenText(const uint jobNum, const QCString& appId){
    kdDebug() << "Running: SpeechData::prevSenText" << endl;
    textMutex.lock();
    mlJob* job = 0;
    if (jobNum) job = findJobByJobNum(jobNum); else job = findAJobByAppId(appId);
    if (job)
    {
//        if (appId == NULL or job->appId == appId)
        {
            QStringList::Iterator* it;
            if (job == textJobs.current())
                it = textIterator;
            else
                it = new QStringList::Iterator(job->sentences.at(job->seq - 1));
            if (*it != job->sentences.begin())
            {
                --job->seq;
                --*it;
            }
            if (job != textJobs.current()) delete it;
        }
    }
    textMutex.unlock();
}

/**
* Next sentence (thread safe)
*/
void SpeechData::nextSenText(const uint jobNum, const QCString& appId){
    kdDebug() << "Running: SpeechData::nextSenText" << endl;
    textMutex.lock();
    mlJob* job = 0;
    if (jobNum) job = findJobByJobNum(jobNum); else job = findAJobByAppId(appId);
    if (job)
    {
//        if (appId == NULL or job->appId == appId)
        {
            QStringList::Iterator* it;
            if (job == textJobs.current())
                it = textIterator;
            else
                it = new QStringList::Iterator(job->sentences.at(job->seq - 1));
            if (*it != job->sentences.end())
            {
                ++job->seq;
                ++*it;
            }
            if (job != textJobs.current()) delete it;
        }
    }
    textMutex.unlock();
}

/**
* Next paragrah (thread safe)
*/
void SpeechData::nextParText(const uint jobNum, const QCString& appId){
    kdDebug() << "Running: SpeechData::nextParText" << endl;
    textMutex.lock();
    mlJob* job = 0;
    if (jobNum) job = findJobByJobNum(jobNum); else job = findAJobByAppId(appId);
    if (job)
    {
//        if (appId == NULL or job->appId == appId)
        {
            QStringList::Iterator* it;
            if (job == textJobs.current())
                it = textIterator;
            else
                it = new QStringList::Iterator(job->sentences.at(job->seq - 1));
            while (*it != job->sentences.end() and **it != "")
            {
                ++job->seq;
                ++*it;
            }
            if (job != textJobs.current()) delete it;
        }
    }
    textMutex.unlock();
}
