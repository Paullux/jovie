/***************************************************** vim:set ts=4 sw=4 sts=4:
  Generic Talker Chooser Filter Configuration class.
  -------------------
  Copyright:
  (C) 2005 by Gary Cramblitt <garycramblitt@comcast.net>
  -------------------
  Original author: Gary Cramblitt <garycramblitt@comcast.net>

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
 ******************************************************************************/

// TalkerChooser includes.
#include "talkerchooserconf.h"
#include "talkerchooserconf.moc"

// Qt includes.


// KDE includes.
#include <klocale.h>
#include <klineedit.h>
#include <kdialog.h>
#include <kcombobox.h>
#include <kpushbutton.h>
#include <kconfig.h>
#include <kstandarddirs.h>
#include <kregexpeditorinterface.h>
#include <kfiledialog.h>
#include <kservicetypetrader.h>

// KTTS includes.
#include "selecttalkerdlg.h"

/**
* Constructor
*/
TalkerChooserConf::TalkerChooserConf( QWidget *parent, const QVariantList & args) :
    KttsFilterConf(parent, args)
{
    Q_UNUSED(args);
    // kDebug() << "TalkerChooserConf::TalkerChooserConf: Running";

    // Create configuration widget.
    setupUi(this);

    // Determine if kdeutils Regular Expression Editor is installed.
    m_reEditorInstalled = !KServiceTypeTrader::self()->query(QLatin1String( "KRegExpEditor/KRegExpEditor" )).isEmpty();
    reEditorButton->setEnabled(m_reEditorInstalled);

    connect(nameLineEdit, SIGNAL(textChanged(QString)),
            this, SLOT(configChanged()));
    connect(reLineEdit, SIGNAL(textChanged(QString)),
            this, SLOT(configChanged()));
    connect(reEditorButton, SIGNAL(clicked()),
            this, SLOT(slotReEditorButton_clicked()));
    connect(appIdLineEdit, SIGNAL(textChanged(QString)),
            this, SLOT(configChanged()));
    connect(talkerButton, SIGNAL(clicked()),
            this, SLOT(slotTalkerButton_clicked()));

    connect(loadButton, SIGNAL(clicked()),
            this, SLOT(slotLoadButton_clicked()));
    connect(saveButton, SIGNAL(clicked()),
            this, SLOT(slotSaveButton_clicked()));
    connect(clearButton, SIGNAL(clicked()),
            this, SLOT(slotClearButton_clicked()));

    // Set up defaults.
    defaults();
}

/**
* Destructor.
*/
TalkerChooserConf::~TalkerChooserConf(){
    // kDebug() << "TalkerChooserConf::~TalkerChooserConf: Running";
}

void TalkerChooserConf::load(KConfig* c, const QString& configGroup){
    // kDebug() << "TalkerChooserConf::load: Running";
    KConfigGroup config( c, configGroup );
    nameLineEdit->setText( config.readEntry( "UserFilterName", nameLineEdit->text() ) );
    reLineEdit->setText(
            config.readEntry("MatchRegExp", reLineEdit->text()) );
    appIdLineEdit->setText(
            config.readEntry("AppIDs", appIdLineEdit->text()) );

    m_talkerCode = TalkerCode(config.readEntry("TalkerCode"), false);
    // Legacy settings.
    QString s = config.readEntry( "LanguageCode" );
    if (!s.isEmpty()) m_talkerCode.setLanguage(s);
    s = config.readEntry( "SynthInName" );
    //if (!s.isEmpty()) m_talkerCode.setPlugInName(s);
    s = config.readEntry( "Gender" );
    //if (!s.isEmpty()) m_talkerCode.setGender(s);
    s = config.readEntry( "Volume" );
    //if (!s.isEmpty()) m_talkerCode.setVolume(s);
    s = config.readEntry( "Rate" );
    //if (!s.isEmpty()) m_talkerCode.setRate(s);

    talkerLineEdit->setText(m_talkerCode.getTranslatedDescription());
}

void TalkerChooserConf::save(KConfig* c, const QString& configGroup){
    // kDebug() << "TalkerChooserConf::save: Running";
    KConfigGroup config( c, configGroup );
    config.writeEntry( "UserFilterName", nameLineEdit->text() );
    config.writeEntry( "MatchRegExp", reLineEdit->text() );
    config.writeEntry( "AppIDs", appIdLineEdit->text().remove(QLatin1Char( ' ' )) );
    config.writeEntry( "TalkerCode", m_talkerCode.getTalkerCode());
}

/**
* This function is called to set the settings in the module to sensible
* default values. It gets called when hitting the "Default" button. The
* default values should probably be the same as the ones the application
* uses when started without a config file.  Note that defaults should
* be applied to the on-screen widgets; not to the config file.
*/
void TalkerChooserConf::defaults(){
    // kDebug() << "TalkerChooserConf::defaults: Running";
    // Default name.
    nameLineEdit->setText( i18n("Talker Chooser") );
    // Default regular expression is blank.
    reLineEdit->clear( );
    // Default App ID is blank.
    appIdLineEdit->clear();
    // Default to using default Talker.
    m_talkerCode = TalkerCode( QString(), false );
    talkerLineEdit->setText( m_talkerCode.getTranslatedDescription() );
}

/**
 * Indicates whether the plugin supports multiple instances.  Return
 * False if only one instance of the plugin can be configured.
 * @return            True if multiple instances are possible.
 */
bool TalkerChooserConf::supportsMultiInstance() { return true; }

/**
 * Returns the name of the plugin.  Displayed in Filters tab of KTTSMgr.
 * If there can be more than one instance of a filter, it should return
 * a unique name for each instance.  The name should be TalkerCode::translated for
 * the user if possible.  If the plugin is not correctly configured,
 * return an empty string.
 * @return          Filter instance name.
 */
QString TalkerChooserConf::userPlugInName()
{
    if (talkerLineEdit->text().isEmpty()) return QString();
    if (appIdLineEdit->text().isEmpty() &&
        reLineEdit->text().isEmpty()) return QString();
    QString instName = nameLineEdit->text();
    if (instName.isEmpty()) return QString();
    return instName;
}

void TalkerChooserConf::slotReEditorButton_clicked()
{
    // Show Regular Expression Editor dialog if it is installed.
    if ( !m_reEditorInstalled ) return;
    KDialog *editorDialog =
        KServiceTypeTrader::createInstanceFromQuery<KDialog>( QLatin1String( "KRegExpEditor/KRegExpEditor" ));
    if ( editorDialog )
    {
        // kdeutils was installed, so the dialog was found.  Fetch the editor interface.
        KRegExpEditorInterface *reEditor =
            qobject_cast<KRegExpEditorInterface *>(editorDialog);
        Q_ASSERT( reEditor ); // This should not fail!// now use the editor.
        reEditor->setRegExp( reLineEdit->text() );
        int dlgResult = editorDialog->exec();
        if ( dlgResult == QDialog::Accepted )
        {
            QString re = reEditor->regExp();
            reLineEdit->setText( re );
        }
        delete editorDialog;
    } else return;
}

void TalkerChooserConf::slotTalkerButton_clicked()
{
    QString talkerCode = m_talkerCode.getTalkerCode();
    QPointer<SelectTalkerDlg> dlg = new SelectTalkerDlg(this, "selecttalkerdialog", i18n("Select Talker"), talkerCode, true);
    int dlgResult = dlg->exec();
    if ( dlgResult != KDialog::Accepted ) return;
    m_talkerCode = TalkerCode( dlg->getSelectedTalkerCode(), false );
    talkerLineEdit->setText( m_talkerCode.getTranslatedDescription() );
    configChanged();
    delete dlg;
}

void TalkerChooserConf::slotLoadButton_clicked()
{
    QStringList dataDirs = KGlobal::dirs()->findAllResources("data", QLatin1String( "kttsd/talkerchooser/" ));
    QString dataDir;
    if (!dataDirs.isEmpty()) dataDir = dataDirs.last();
    QString filename = KFileDialog::getOpenFileName(
        dataDir,
        QLatin1String( "*rc|" ) + i18n( "Talker Chooser Config (*rc)" ),
        this,
        QLatin1String( "talkerchooser_loadfile" ));
    if ( filename.isEmpty() ) return;
    KConfig* cfg = new KConfig( filename );
    load( cfg, QLatin1String( "Filter" ) );
    delete cfg;
    configChanged();
}

void TalkerChooserConf::slotSaveButton_clicked()
{
    QString filename = KFileDialog::getSaveFileName(
        KGlobal::dirs()->saveLocation( "data" ,QLatin1String( "kttsd/talkerchooser/" ), false ) ,
       QLatin1String( "*rc|" ) + i18n( "Talker Chooser Config (*rc)" ),
        this,
        QLatin1String( "talkerchooser_savefile" ));
    if ( filename.isEmpty() ) return;
    KConfig* cfg = new KConfig( filename );
    save( cfg, QLatin1String( "Filter" ) );
    delete cfg;
}

void TalkerChooserConf::slotClearButton_clicked()
{
    nameLineEdit->setText( QString() );
    reLineEdit->setText( QString() );
    appIdLineEdit->setText( QString() );
    m_talkerCode = TalkerCode( QString(), false );
    talkerLineEdit->setText( m_talkerCode.getTranslatedDescription() );
    configChanged();
}
