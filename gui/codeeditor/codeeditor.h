/*
    This file is part of IanniX, a graphical real-time open-source sequencer for digital art
    Copyright (C) 2025-2026 - Hypar.XYZ (https://iannix.hypar.xyz/)

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

#ifndef CODEEDITOR_H
#define CODEEDITOR_H

#include <QPlainTextEdit>
#include <QString>

namespace KSyntaxHighlighting {
    class Repository;
    class SyntaxHighlighter;
}

class LineNumberArea;

class CodeEditor : public QPlainTextEdit
{
    Q_OBJECT

public:
    explicit CodeEditor(QWidget *parent = nullptr);
    ~CodeEditor() override;

    // Language selection — pass a KSyntaxHighlighting definition name,
    // e.g. "JavaScript", "Python", "C++", "Lua", "" for no highlighting.
    void setLanguage(const QString &languageName);
    QString language() const;

    // Feature toggles (API compatible with the old JSEdit widget)
    void setTextWrapEnabled(bool enable);
    void setLineNumbersVisible(bool visible);
    void setCodeFoldingEnabled(bool enable);   // accepted; sidebar folding is a no-op
    void setBracketsMatchingEnabled(bool enable);
    void setTabStopWidth(int width);

    // Called by LineNumberArea during paint
    int  lineNumberAreaWidth() const;
    void lineNumberAreaPaintEvent(QPaintEvent *event);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateLineNumberAreaWidth(int newBlockCount);
    void updateLineNumberArea(const QRect &rect, int dy);
    void highlightCurrentLine();

private:
    void applyTheme();
    void doHighlightMatchingBrackets(QList<QTextEdit::ExtraSelection> &extras);

    LineNumberArea                         *m_lineNumberArea;
    KSyntaxHighlighting::Repository        *m_repository;
    KSyntaxHighlighting::SyntaxHighlighter *m_highlighter;
    QString                                 m_language;

    bool m_lineNumbersVisible;
    bool m_bracketsMatchingEnabled;
};

#endif // CODEEDITOR_H
