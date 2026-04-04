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

#include "codeeditor.h"

#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/SyntaxHighlighter>
#include <KSyntaxHighlighting/Definition>
#include <KSyntaxHighlighting/Theme>

#include <QPainter>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QScrollBar>
#include <QTextBlock>
#include <QGuiApplication>
#include <QPalette>
#include <QFont>

// ---------------------------------------------------------------------------
// LineNumberArea — left gutter that draws line numbers
// ---------------------------------------------------------------------------

class LineNumberArea : public QWidget
{
public:
    explicit LineNumberArea(CodeEditor *editor)
        : QWidget(editor), m_editor(editor) {}

    QSize sizeHint() const override
    {
        return QSize(m_editor->lineNumberAreaWidth(), 0);
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        m_editor->lineNumberAreaPaintEvent(event);
    }

private:
    CodeEditor *m_editor;
};

// ---------------------------------------------------------------------------
// CodeEditor
// ---------------------------------------------------------------------------

CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
    , m_lineNumberArea(new LineNumberArea(this))
    , m_repository(new KSyntaxHighlighting::Repository)
    , m_highlighter(new KSyntaxHighlighting::SyntaxHighlighter(document()))
    , m_lineNumbersVisible(true)
    , m_bracketsMatchingEnabled(true)
{
    QFont f(QStringLiteral("Monospace"));
    f.setStyleHint(QFont::TypeWriter);
    f.setPointSize(10);
    setFont(f);

    setLineWrapMode(QPlainTextEdit::NoWrap);

    connect(this, &QPlainTextEdit::blockCountChanged,
            this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &QPlainTextEdit::updateRequest,
            this, &CodeEditor::updateLineNumberArea);
    connect(this, &QPlainTextEdit::cursorPositionChanged,
            this, &CodeEditor::highlightCurrentLine);

    applyTheme();
    updateLineNumberAreaWidth(0);
    highlightCurrentLine();
}

CodeEditor::~CodeEditor()
{
    delete m_repository;
}

// ---------------------------------------------------------------------------
// Language / highlighting
// ---------------------------------------------------------------------------

void CodeEditor::setLanguage(const QString &languageName)
{
    m_language = languageName;

    if (languageName.isEmpty()) {
        m_highlighter->setDefinition(KSyntaxHighlighting::Definition());
        return;
    }

    const auto def = m_repository->definitionForName(languageName);
    m_highlighter->setDefinition(def);
}

QString CodeEditor::language() const
{
    return m_language;
}

void CodeEditor::applyTheme()
{
    const auto palette = QGuiApplication::palette();
    const auto theme   = m_repository->themeForPalette(palette);
    m_highlighter->setTheme(theme);

    if (theme.isValid()) {
        QPalette p = this->palette();
        p.setColor(QPalette::Base,
                   QColor(QRgb(theme.editorColor(KSyntaxHighlighting::Theme::BackgroundColor))));
        p.setColor(QPalette::Text,
                   QColor(QRgb(theme.textColor(KSyntaxHighlighting::Theme::Normal))));
        setPalette(p);
    }
}

// ---------------------------------------------------------------------------
// Line number area
// ---------------------------------------------------------------------------

int CodeEditor::lineNumberAreaWidth() const
{
    if (!m_lineNumbersVisible)
        return 0;

    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    return 6 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
}

void CodeEditor::updateLineNumberAreaWidth(int /*newBlockCount*/)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        m_lineNumberArea->scroll(0, dy);
    else
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent *e)
{
    QPlainTextEdit::resizeEvent(e);
    const QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(),
                                        lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), palette().color(QPalette::Window).darker(108));
    painter.setPen(palette().color(QPalette::Mid));

    QTextBlock block   = firstVisibleBlock();
    int blockNumber    = block.blockNumber();
    int top    = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            painter.drawText(0, top,
                             m_lineNumberArea->width() - 3,
                             fontMetrics().height(),
                             Qt::AlignRight,
                             QString::number(blockNumber + 1));
        }
        block  = block.next();
        top    = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}

// ---------------------------------------------------------------------------
// Current-line highlight + bracket matching
// ---------------------------------------------------------------------------

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extras;

    if (!isReadOnly()) {
        QTextEdit::ExtraSelection sel;
        sel.format.setBackground(palette().color(QPalette::AlternateBase));
        sel.format.setProperty(QTextFormat::FullWidthSelection, true);
        sel.cursor = textCursor();
        sel.cursor.clearSelection();
        extras << sel;
    }

    if (m_bracketsMatchingEnabled)
        doHighlightMatchingBrackets(extras);

    setExtraSelections(extras);
}

void CodeEditor::doHighlightMatchingBrackets(QList<QTextEdit::ExtraSelection> &extras)
{
    static const QString openBrackets  = QStringLiteral("{([");
    static const QString closeBrackets = QStringLiteral("})]");

    const int  pos     = textCursor().position();
    const QChar ch     = document()->characterAt(pos);
    const QChar chPrev = (pos > 0) ? document()->characterAt(pos - 1) : QChar();

    int  bracketPos    = -1;
    bool searchForward = false;
    QChar openCh, closeCh;

    auto trySetup = [&](QChar c, int p) -> bool {
        int idx = openBrackets.indexOf(c);
        if (idx >= 0) {
            openCh = openBrackets[idx];
            closeCh = closeBrackets[idx];
            bracketPos = p;
            searchForward = true;
            return true;
        }
        idx = closeBrackets.indexOf(c);
        if (idx >= 0) {
            openCh = openBrackets[idx];
            closeCh = closeBrackets[idx];
            bracketPos = p;
            searchForward = false;
            return true;
        }
        return false;
    };

    if (!trySetup(ch, pos))
        trySetup(chPrev, pos - 1);

    if (bracketPos < 0)
        return;

    int depth    = 0;
    int matchPos = -1;
    const int docLen = document()->characterCount();

    if (searchForward) {
        for (int i = bracketPos; i < docLen; ++i) {
            const QChar c = document()->characterAt(i);
            if (c == openCh)       ++depth;
            else if (c == closeCh) { if (--depth == 0) { matchPos = i; break; } }
        }
    } else {
        for (int i = bracketPos; i >= 0; --i) {
            const QChar c = document()->characterAt(i);
            if (c == closeCh)     ++depth;
            else if (c == openCh) { if (--depth == 0) { matchPos = i; break; } }
        }
    }

    const QColor matchColor = QColor(Qt::green).lighter(160);
    const QColor errorColor = QColor(Qt::red).lighter(160);
    const QColor color      = (matchPos >= 0) ? matchColor : errorColor;

    auto makeSelection = [&](int p) {
        QTextEdit::ExtraSelection s;
        s.format.setBackground(color);
        s.cursor = textCursor();
        s.cursor.setPosition(p);
        s.cursor.setPosition(p + 1, QTextCursor::KeepAnchor);
        return s;
    };

    extras << makeSelection(bracketPos);
    if (matchPos >= 0)
        extras << makeSelection(matchPos);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void CodeEditor::setTextWrapEnabled(bool enable)
{
    setLineWrapMode(enable ? QPlainTextEdit::WidgetWidth : QPlainTextEdit::NoWrap);
}

void CodeEditor::setLineNumbersVisible(bool visible)
{
    m_lineNumbersVisible = visible;
    m_lineNumberArea->setVisible(visible);
    updateLineNumberAreaWidth(0);
}

void CodeEditor::setCodeFoldingEnabled(bool /*enable*/)
{
    // No-op: KSyntaxHighlighting::SyntaxHighlighter exposes startsFoldingRegion()
    // and findFoldingRegionEnd() for a future sidebar-folding implementation.
}

void CodeEditor::setBracketsMatchingEnabled(bool enable)
{
    m_bracketsMatchingEnabled = enable;
    highlightCurrentLine();
}

void CodeEditor::setTabStopWidth(int width)
{
    QPlainTextEdit::setTabStopWidth(width);
}
