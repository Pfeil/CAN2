#include "mainwindow.h"
#include "application.h"
#include "Dialogs/tagdialog.h"
#include <QTranslator>
#include "configurable.h"
#include <QMessageBox>
#include <QLibraryInfo>
#include "global.h"

int main(int argc, char *argv[])
{
    Application app( argc, argv );

    QString localeCode = MainWindow::config["locale"].toString();
    QLocale locale( localeCode );
    QLocale::setDefault( locale );
    QString language = QLocale::languageToString( locale.language() );

    bool useQtTranslator = (localeCode != "en_US");

    QTranslator qtTranslator;
    // load qt-translator
    if (useQtTranslator)
    {
        if (qtTranslator.load(":/translations/qt_" + localeCode))
        {
            if (!Application::installTranslator( &qtTranslator ))
            {
                qWarning() << "failed to install translator.";
            }
        }
        else
        {
            qWarning() << "failed to load translator qt.";
        }
    }

    QTranslator qtBaseTranslator;
    // load qtbase-translator
    if (useQtTranslator)
    {
        if (qtBaseTranslator.load(":/translations/qtbase_" + localeCode))
        {
            if (!Application::installTranslator( &qtBaseTranslator ))
            {
                qWarning() << "failed to install translator.";
            }
        }
        else
        {
            qWarning() << "failed to load translator qtbase.";
        }
    }

    // load qtmultimedia-translator
    QTranslator qtMultimediaTranslator;
    if (useQtTranslator)
    {
        if (qtMultimediaTranslator.load(":/translations/qtmultimedia_" + localeCode))
        {
            if (!Application::installTranslator( &qtMultimediaTranslator ))
            {
                qWarning() << "failed to install translator qtmultimedia.";
            }
        }
        else
        {
            qWarning() << "failed to load translator.";
        }
    }

    // load translator
    QTranslator translator;
    if (translator.load(":/translations/can2_" + localeCode))
    {
        if (!Application::installTranslator( &translator ))
        {
            qWarning() << "failed to install translator.";
        }
    }
    else
    {
        qWarning() << "cannot load translation file for " << language;
    }


    app.fileIndex().restore();

    MainWindow m;
    m.show();

    return app.exec();
}

