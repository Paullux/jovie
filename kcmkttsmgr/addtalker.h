//
// C++ Interface: %{MODULE}
//
// Description: 
//
//
// Author: %{AUTHOR} <%{EMAIL}>, (C) %{YEAR}
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef ADDTALKER_H
#define ADDTALKER_H

// Qt includes.
#include <qmap.h>

#include "addtalkerwidget.h"

typedef QMap<QString,QStringList> SynthToLangMap;
typedef QMap<QString,QStringList> LangToSynthMap;

class AddTalker : public AddTalkerWidget
{
    Q_OBJECT

public:
    /**
    * Constructor.
    * @param synthToLangMap     QMap of supported language codes indexed by synthesizer.
    * @param parent             Inherited KDialog parameter.
    * @param name               Inherited KDialog parameter.
    */
    AddTalker(SynthToLangMap synthToLangMap, QWidget* parent = 0, const char* name = 0, WFlags fl = 0 );

    /**
    * Destructor.
    */
    ~AddTalker();

    /**
    * Returns user's chosen language code.
    */
    QString getLanguageCode();

    /**
    * Returns user's chosen synthesizer.
    */
    QString getSynthesizer();


private:
    /**
    * Set the synthesizer-to-languages map.
    * @param synthToLang        QMap of supported language codes indexed by synthesizer.
    */
    void setSynthToLangMap(SynthToLangMap synthToLangMap);

    // Converts a language code plus optional country code to language description.
    QString AddTalker::languageCodeToLanguage(const QString &languageCode);

    // QMap of language descriptions to language codes.
    QMap<QString,QString> m_languageToLanguageCodeMap;
    // QMap of supported languages indexed by synthesizer.
    SynthToLangMap m_synthToLangMap;
    // QMap of synthesizers indexed by language code they support.
    LangToSynthMap m_langToSynthMap;

private slots:
    // Based on user's radio button selection, filters choices for language or synthesizer
    // comboboxes based on what is selected in the other combobox.
    void applyFilter();
};

#endif
