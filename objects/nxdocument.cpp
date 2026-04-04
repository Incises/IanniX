/*
    This file is part of IanniX, a graphical real-time open-source sequencer for digital art
    Copyright (C) 2010-2015 - IanniX Association (https://www.iannix.org/)
    Copyright (C) 2025-2026 - Hypar.XYZ (https://iannix.hypar.xyz/)

    This file was originally written by Guillaume Jacquemin.
    
    IanniX is a free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "nxdocument.h"

namespace {
QStringList scriptErrorDetails(const QJSValue &errorValue) {
    QStringList errors;
    if(errorValue.isError()) {
        const QString stack = errorValue.property("stack").toString().trimmed();
        if(!stack.isEmpty())
            errors = stack.split('\n', Qt::SkipEmptyParts);

        const QString message = errorValue.property("message").toString().trimmed();
        if(!message.isEmpty() && !errors.contains(message))
            errors << message;
    }
    return errors;
}

int scriptErrorLine(const QJSValue &errorValue) {
    if(errorValue.isError()) {
        bool ok = false;
        const int lineNumber = errorValue.property("lineNumber").toVariant().toInt(&ok);
        if(ok)
            return lineNumber;
    }
    return -1;
}

QJSValue callScriptFunction(QJSValue function, const QJSValueList &arguments = QJSValueList()) {
    if(function.isCallable())
        return function.call(arguments);
    return QJSValue();
}
}

NxDocument::NxDocument(ApplicationCurrent *parent, UiFileItem *_fileItem) :
    QObject(parent) {
    fileItem = _fileItem;
    if(fileItem) {
        connect(fileItem, SIGNAL(askFileClose()),  SLOT(askFileClose()));
        connect(fileItem, SIGNAL(askFileOpen()),   SLOT(askFileOpen()));
        connect(fileItem, SIGNAL(askFileReload()), SLOT(askFileReload()));
        connect(fileItem, SIGNAL(askFileSave()),   SLOT(askFileSave()));
    }
    skipClose = false;
    variable = 0;
    setCurrentObject(0);
    setCurrentGroup(0);
    currentCurve = 0;
    snapshotsIndex = 0;
    isLoaded = false;
}


const QString NxDocument::serialize() const {
    QString retour;
    QString prefix = "\trun(\"", postfix = "\");\n";

    if(NxObjectDispatchProperty::source == ExecuteSourceGui) {
        //Textures
        QMapIterator<QString, UiRenderTexture*> textureIterator(*Render::textures);
        while (textureIterator.hasNext()) {
            textureIterator.next();
            UiRenderTexture *texture = textureIterator.value();
            if(texture->filename.exists()) {
                QString filename = getScriptFile().absoluteDir().relativeFilePath(texture->filename.absoluteFilePath());
                retour += prefix + QString("%1 %2 %3  %4 %5 %6 %7").arg(COMMAND_TEXTURE).arg(textureIterator.key()).arg(texture->mapping.topLeft().x()).arg(texture->mapping.topLeft().y()).arg(texture->mapping.bottomRight().x()).arg(texture->mapping.bottomRight().y()).arg(filename) + postfix;
            }
        }

        //Colors
        QMapIterator<QString, QColor> colorIterator(*Render::colors);
        while (colorIterator.hasNext()) {
            colorIterator.next();
            if(!((Render::defaultColors.contains(colorIterator.key())) && (Render::defaultColors.value(colorIterator.key()) == colorIterator.value()))) {
                QColor color = colorIterator.value();
                retour += prefix + QString("%1 %2  %3 %4 %5 %6").arg(COMMAND_GLOBAL_COLOR).arg(colorIterator.key()).arg(color.red()).arg(color.green()).arg(color.blue()).arg(color.alpha()) + postfix;
            }
        }

        retour += COMMAND_END;
    }

    //Browse groups
    foreach(NxGroup *group, groups)
        retour += group->serialize();

    return retour;
}

void NxDocument::pushSnapshot() {
    NxObjectDispatchProperty::source = ExecuteSourceCopyPaste;
    if(snapshotsIndex < snapshots.count())
        snapshots.replace(snapshotsIndex, Application::current->serialize());
    else
        snapshots.append(Application::current->serialize());
    snapshotsIndex++;
}
void NxDocument::popSnapshot(bool revert) {
    bool canDo = false;
    if((revert) && (snapshotsIndex < snapshots.count()-1)) {
        snapshotsIndex++;
        canDo = true;
    }
    else if ((!revert) && (snapshotsIndex > 0) && (snapshotsIndex == snapshots.count())) {
        pushSnapshot();
        snapshotsIndex--;
        snapshotsIndex--;
        canDo = true;
    }
    else if ((!revert) && (snapshotsIndex > 0)) {
        snapshotsIndex--;
        canDo = true;
    }
    if(canDo) {
        clear();
        NxObjectDispatchProperty::source = ExecuteSourceGui;
        source = NxObjectDispatchProperty::source;
        Application::current->executeAsScript(snapshots.at(snapshotsIndex));
    }
}


void NxDocument::open() {
    if(true) {
        clear();
        open(true);

        NxObjectDispatchProperty::source = ExecuteSourceGui;
        source = NxObjectDispatchProperty::source;
        initialContent = Application::current->serialize();
    }
}
void NxDocument::open(bool configure) {
    isLoaded = false;

    if(!skipClose)
        Application::current->getMainWindow()->setWindowTitle(tr("IanniX") + QString(" / %1").arg(getScriptFile().baseName()));

    //Open the script
    QJSValue scriptFunctions = scriptEngine.newQObject(this);
    script = scriptEngine.globalObject();

    //Map specials features/keywords/functions
    script.setProperty("mouseX", QJSValue(mousePos.x()));
    script.setProperty("mouseY", QJSValue(mousePos.y()));
    script.setProperty("iannix", scriptFunctions);
    script.setProperty("nx",     scriptFunctions);

    //GUI to ask the user variables
    if(!variable)
        variable = new ExtScriptVariableAsk(Application::current->getMainWindow());

    //Open the script file (if its a file)
    QFileInfo file = getScriptFile();
    if(!file.exists())
        file = QFileInfo(Application::pathTools.absoluteFilePath() + "/Score template.iannix");
    QFile scriptFileContent(file.absoluteFilePath());
    if(scriptFileContent.open(QIODevice::ReadOnly | QIODevice::Text)) {
        //Read file
        scriptContent = scriptFileContent.readAll();
        scriptFileContent.close();

        //Load
        if(getScriptFile().suffix().toLower() == "nxscore") {
            QStringList paste = scriptContent.split(COMMAND_END, Qt::SkipEmptyParts);
            foreach(const QString & command, paste)
                Application::current->execute(command, ExecuteSourceGui);
        }
        else {
            QJSValue scriptReturn = scriptEvaluate(scriptContent, false);

            //Extract function
            if(getScriptFile().suffix().toLower() == "iannix") {
                scriptMakeWithScript       = script.property("makeWithScript");
                scriptOnIncomingMessage    = script.property("onIncomingMessage");
                scriptAskUserForParameters = script.property("askUserForParameters");
            }
            else {
                scriptOnIncomingMessage    = script.property("onMessage");
                scriptMakeWithScript       = script.property("onCreate");
                scriptAskUserForParameters = script.property("onConfigure");
            }
            scriptMadeThroughGUI           = script.property("madeThroughGUI");
            scriptAlterateWithScript       = script.property("alterateWithScript");
            scriptMadeThroughInterfaces    = script.property("madeThroughInterfaces");


            //Extract errors
            QStringList errors = scriptErrorDetails(scriptReturn);
            if(errors.count())  Transport::editor->scriptError(errors, scriptErrorLine(scriptReturn));
            else                Transport::editor->scriptError(QStringList(), -1);


            //Call the "askUserForParameters()" function
            if(configure) {
                callScriptFunction(scriptAskUserForParameters);
                Application::current->pushSnapshot();
            }

            //Ask variables to user and sets the variable in the script
            QList<ExtScriptVariable*> variables = variable->ask();
            if(variable->result()) {
                foreach(const ExtScriptVariable *variable, variables) {
                    if(variable->isDefFloat())  script.setProperty(variable->getValue(), QJSValue(variable->getDefFloat()));
                    else                        script.setProperty(variable->getValue(), QJSValue(variable->getDefStr()));
                }

                //Call the functions
                source = ExecuteSourceScript;
                callScriptFunction(scriptMakeWithScript);
                source = ExecuteSourceGui;
                callScriptFunction(scriptMadeThroughGUI);
                source = ExecuteSourceNetwork;
                callScriptFunction(scriptMadeThroughInterfaces);
                source = ExecuteSourceScript;
                callScriptFunction(scriptAlterateWithScript);

                isLoaded = true;
            }
        }
    }

    if(fileItem)
        fileItem->setIcon(0, UiFileItem::iconFileOpened);
    updateCode(true, configure);
}
void NxDocument::updateCode(bool fromFile, bool raiseWindow) {
    if(!skipClose)
        Transport::editor->setContent(getContent(fromFile), raiseWindow);
}
void NxDocument::save() {
    QString scoreContent = getContent(false);
    QFile scriptFileContent(getScriptFile().absoluteFilePath());
    if(scriptFileContent.open(QIODevice::WriteOnly | QIODevice::Text)) {
        scriptFileContent.write(qPrintable(scoreContent));
        scriptFileContent.close();
    }
    Application::current->getMainWindow()->setWindowTitle(tr("IanniX") + QString(" / %1").arg(getScriptFile().baseName()));
    if(fileItem)
        fileItem->setIcon(0, UiFileItem::iconFileOpened);
}
const QString NxDocument::getContent(bool fromFile) {
    QString scoreContent;

    //Load IanniX Score
    if(fromFile) {
        QFileInfo file = getScriptFile();
        if(!file.exists())
            file = QFileInfo(Application::pathTools.absoluteFilePath() + "/Score template.iannix");

        QFile scoreTemplateFile(file.absoluteFilePath());
        if(scoreTemplateFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            scoreContent = scoreTemplateFile.readAll();
            scoreTemplateFile.close();
        }
    }
    else
        scoreContent = Transport::editor->getContent();

    //Locate functions
    NxObjectDispatchProperty::source = ExecuteSourceGui;
    remplaceInFunction(&scoreContent, "//GUI: NEVER EVER REMOVE THIS LINE\n", Application::current->serialize());
    NxObjectDispatchProperty::source = ExecuteSourceNetwork;
    remplaceInFunction(&scoreContent, "//INTERFACES: NEVER EVER REMOVE THIS LINE\n", Application::current->serialize());
    remplaceInFunction(&scoreContent, " *\t//APP VERSION: NEVER EVER REMOVE THIS LINE\n", QString(" *\tMade with IanniX %1").arg(QCoreApplication::applicationVersion()));

    return scoreContent;
}

void NxDocument::remplaceInFunction(QString *content, const QString &delimiter, const QString &data) {
    qint32 startReplace = content->indexOf(delimiter, 0) + delimiter.length();
    qint32 endReplace   = content->indexOf(delimiter, startReplace);
    if((startReplace < endReplace) && (startReplace >= 0) && (endReplace >= 0))
        *content = content->left(startReplace) + data + "\n" + content->mid(endReplace);
}




const QString NxDocument::loadLibrary() {
    QString scriptContent = "";

    QFileInfoList scriptDirs = QDir(Application::pathTools.absoluteFilePath() + "/").entryInfoList(QStringList() << "*.js", QDir::Files | QDir::NoDotAndDotDot);
    foreach(const QFileInfo & scriptFile, scriptDirs) {
        QFile scriptFileContent(scriptFile.absoluteFilePath());
        if(scriptFileContent.open(QIODevice::ReadOnly | QIODevice::Text)) {
            //Read file
            scriptContent += scriptFileContent.readAll();
            scriptFileContent.close();
        }
    }
    return scriptContent;
}

QJSValue NxDocument::scriptEvaluate(const QString &scriptContent, bool _createNewObjectIfExists) {
    createNewObjectIfExists = _createNewObjectIfExists;
    return scriptEngine.evaluate(scriptContent + loadLibrary());
}

QJSValue NxDocument::createScriptArgumentsArray(const QStringList &arguments) {
    QJSValue values = scriptEngine.newArray(arguments.count());
    for(quint32 argumentIndex = 0 ; argumentIndex < (quint32)arguments.count() ; argumentIndex++)
        values.setProperty(argumentIndex, QJSValue(arguments.at(argumentIndex)));
    return values;
}

QString NxDocument::incomingMessage(const MessageIncomming &source, bool needOutput, bool) {
    if(scriptOnIncomingMessage.isCallable()) {
        QJSValueList arguments;
        arguments << QJSValue(source.protocol)
                  << QJSValue(source.host)
                  << QJSValue(source.port.toString())
                  << QJSValue(source.destination)
                  << createScriptArgumentsArray(source.arguments);
        const QJSValue result = scriptOnIncomingMessage.call(arguments);
        if(needOutput)
            return result.toString();
    }
    return QString();
}

void NxDocument::askFileOpen() {
    askFileOpen(true);
}
void NxDocument::askFileOpen(bool mode) {
    qDebug("==> OPEN (mode %d) %s", mode, qPrintable(getScriptFile().absoluteFilePath()));
    if(mode)    restoreDefaults();
    else        skipClose = true;
    open();
}
void NxDocument::askFileSave() {
    save();
}
void NxDocument::askFileReload() {
    open(false);
}
void NxDocument::askFileClose() {
    if((fileItem) && (!skipClose)) {
        NxObjectDispatchProperty::source = ExecuteSourceGui;
        if(initialContent != Application::current->serialize()) {
            int rep = (new UiMessageBox())->display(tr("Score file"), tr("Do you want to save changes before closing score?"), QDialogButtonBox::Yes | QDialogButtonBox::No);
            if(rep)
                fileItem->askForSave(fileItem, false);
        }
    }
    if(fileItem)
            fileItem->setIcon(0, UiFileItem::iconFile);
    clear();
}
void NxDocument::restoreDefaults() {
    Render::defaultColors.insert("background_texture_tint"          , QColor(255, 255, 255, 255));
    Render::defaultColors.insert("darktheme_background"             , QColor(  0,   0,   0, 255));
    Render::defaultColors.insert("darktheme_grid"                   , QColor(255, 255, 255,  35));
    Render::defaultColors.insert("darktheme_axis"                   , QColor(255, 255, 255,  25));
    Render::defaultColors.insert("darktheme_gui_gridSnap"           , QColor( 90,  25,  15, 255));
    Render::defaultColors.insert("darktheme_gui_axisSnap"           , QColor( 90,  25,  15, 255));
    Render::defaultColors.insert("darktheme_gui_selection"          , QColor(255, 255, 255,  40));
    Render::defaultColors.insert("darktheme_gui_object_selection"   , QColor(255, 240,  35, 255));
    Render::defaultColors.insert("darktheme_gui_object_hover"       , QColor( 35, 255, 165, 255));
    Render::defaultColors.insert("darktheme_cursor_active"          , QColor(255,  80,  30, 255));
    Render::defaultColors.insert("darktheme_cursor_inactive"        , QColor(255, 255, 255,  92));
    Render::defaultColors.insert("darktheme_trigger_active"         , QColor(  0, 185, 255, 255));
    Render::defaultColors.insert("darktheme_trigger_inactive"       , QColor(255, 255, 255, 92));
    Render::defaultColors.insert("darktheme_curve_active"           , QColor(255, 255, 255, 175));
    Render::defaultColors.insert("darktheme_curve_inactive"         , QColor(255, 255, 255,  92));
    Render::defaultColors.insert("darktheme_simple_curve_active"    , QColor(115, 159,  89, 255));
    Render::defaultColors.insert("darktheme_simple_curve_inactive"  , QColor(115, 159,  89, 255));
    Render::defaultColors.insert("lighttheme_background"            , QColor(242, 241, 237, 255));
    Render::defaultColors.insert("lighttheme_grid"                  , QColor(  0,   0,   0,  15));
    Render::defaultColors.insert("lighttheme_axis"                  , QColor(  0,   0,   0,  10));
    Render::defaultColors.insert("lighttheme_gui_gridSnap"          , QColor(255, 190, 190, 255));
    Render::defaultColors.insert("lighttheme_gui_axisSnap"          , QColor(255, 190, 190, 255));
    Render::defaultColors.insert("lighttheme_gui_selection"         , QColor(  0,   0,   0,  40));
    Render::defaultColors.insert("lighttheme_gui_object_selection"  , QColor(  0,  15, 220, 255));
    Render::defaultColors.insert("lighttheme_gui_object_hover"      , QColor(220,   0,  90, 255));
    Render::defaultColors.insert("lighttheme_cursor_active"         , QColor(255,  80,  30, 255));
    Render::defaultColors.insert("lighttheme_cursor_inactive"       , QColor(  0,   0,   0,  92));
    Render::defaultColors.insert("lighttheme_trigger_active"        , QColor(  0, 185, 255, 255));
    Render::defaultColors.insert("lighttheme_trigger_inactive"      , QColor(  0,   0,   0,  92));
    Render::defaultColors.insert("lighttheme_curve_active"          , QColor(  0,   0,   0, 175));
    Render::defaultColors.insert("lighttheme_curve_inactive"        , QColor(  0,   0,   0,  92));
    Render::defaultColors.insert("lighttheme_simple_curve_active"   , QColor(115, 159,  89, 255));
    Render::defaultColors.insert("lighttheme_simple_curve_inactive" , QColor(115, 159,  89, 255));
    Render::colors->clear();
    QMapIterator<QString, QColor> colorIterator(Render::defaultColors);
    while (colorIterator.hasNext()) {
        colorIterator.next();
        Render::colors->insert(colorIterator.key(), colorIterator.value());
    }

    Application::render->loadTexture(new UiRenderTexture("background",       QFileInfo("filename"), NxRect(-4, 4, 8, -8)));
    Application::render->loadTexture(new UiRenderTexture("trigger_active",   QFileInfo("filename"), NxRect(-1, 1, 2, -2)));
    Application::render->loadTexture(new UiRenderTexture("trigger_inactive", QFileInfo("filename"), NxRect(-1, 1, 2, -2)));
    Application::render->loadTexture(new UiRenderTexture("cursor_active",    QFileInfo("filename"), NxRect(-1, 1, 2, -2)));
    Application::render->loadTexture(new UiRenderTexture("cursor_inactive",  QFileInfo("filename"), NxRect(-1, 1, 2, -2)));
}
