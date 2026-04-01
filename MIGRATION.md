# IanniX Migration Guide

This document catalogs every deprecated API, Qt4 remnant, vendored library, and
modernization target in the IanniX codebase.  It is the authoritative reference
for the Qt5 cleanup and future Qt6 migration.

The Qt5 build currently produces **~787 compiler warnings**, almost all of which
are `-Wdeprecated-declarations`.  Each section below maps to a category of work.

---

## Table of Contents

1. [QtScript to QJSEngine](#1-qtscript-to-qjsengine)
2. [QDesktopWidget to QScreen](#2-qdesktopwidget-to-qscreen)
3. [QString::SplitBehavior to Qt::SplitBehavior](#3-qstringsplitbehavior-to-qtsplitbehavior)
4. [QWheelEvent::delta to angleDelta](#4-qwheeleventdelta-to-angledelta)
5. [QMouseEvent::pos to position](#5-qmouseeventpos-to-position)
6. [QTime as timer to QElapsedTimer](#6-qtime-as-timer-to-qelapsedtimer)
7. [qrand/qsrand to QRandomGenerator](#7-qrandqsrand-to-qrandomgenerator)
8. [QPainter deprecated hints](#8-qpainter-deprecated-hints)
9. [QTextOption::setTabStop and setTabStopWidth](#9-qtextoptionsettabstop-and-settabstopwidth)
10. [QFontMetrics::width to horizontalAdvance](#10-qfontmetricswidth-to-horizontaladvance)
11. [QTextCodec (removed in Qt6)](#11-qtextcodec-removed-in-qt6)
12. [QRegExp to QRegularExpression](#12-qregexp-to-qregularexpression)
13. [Qt4 preprocessor branches](#13-qt4-preprocessor-branches)
14. [OpenGL immediate mode and QGLWidget](#14-opengl-immediate-mode-and-qglwidget)
15. [std::auto_ptr to std::unique_ptr](#15-stdauto_ptr-to-stdunique_ptr)
16. [register storage class](#16-register-storage-class)
17. [SIGNAL/SLOT macros to function pointers](#17-signalslot-macros-to-function-pointers)
18. [qSort to std::sort](#18-qsort-to-stdsort)
19. [QVariant::Type enum](#19-qvarianttype-enum)
20. [Vendored libraries](#20-vendored-libraries)
21. [Summary matrix](#21-summary-matrix)

---

## 1. QtScript to QJSEngine

**Impact: CRITICAL -- blocks Qt6 migration.**

Qt5Script (`QScriptEngine`) is deprecated in Qt 5.x and entirely removed in Qt6.
The replacement is `QJSEngine` from the Qml module.

### Files affected

| File | Usage |
|------|-------|
| `objects/nxdocument.h` | `#include <QScriptEngine>`, member `QScriptEngine scriptEngine`, `QScriptValue script`, callback handles |
| `objects/nxdocument.cpp` | `scriptEngine.newQObject(this)`, `scriptEngine.evaluate(...)`, `scriptEngine.globalObject()`, `.uncaughtExceptionBacktrace()`, `.call(QScriptValue(), QScriptValueList() << ...)` |
| `objects/nxcurve.h` | `#include <QScriptEngine>`, `#include <QScriptValue>` (transitive) |
| `messages/message.h` | `QScriptValue` members, `QScriptEngine *messageScriptEngine` |
| `messages/message.cpp` | `messageScriptEngine->evaluate(...)`, `messageScriptValue.setProperty(...)`, `messageScriptResult.isError()/.toString()/.toNumber()` |
| `messages/messagemanager.h` | `static QScriptEngine *scriptEngine` |
| `messages/messagemanager.cpp` | Stores and passes script engine pointer |
| `iannix.h` | `QScriptEngine messageScriptEngine` member |
| `iannix.cpp` | `messageScriptEngine.globalObject()`, `messageScriptEngine.evaluate(...)` |

### Migration path

- Replace `QScriptEngine` with `QJSEngine`.
- Replace `QScriptValue` with `QJSValue`.
- `QScriptValueList` does not exist in QJSEngine -- pass arguments via `QJSValueList`.
- `engine.newQObject(this)` becomes `engine.newQObject(this)` (API similar but ownership differs).
- `engine.evaluate(code)` stays similar; error handling changes (no `uncaughtExceptionBacktrace()`; use `QJSValue::isError()` and `.property("stack")`).
- `value.call(QScriptValue(), args)` becomes `value.call(args)`.
- CMake: replace `Qt5::Script` with `Qt5::Qml` (or `Qt6::Qml`).

---

## 2. QDesktopWidget to QScreen

**Impact: HIGH -- QDesktopWidget removed in Qt6.**

### Files affected

| File | Line(s) | Usage |
|------|---------|-------|
| `gui/uiview.h` | 50 | `QDesktopWidget *fullscreenDisplays` member |
| `gui/uiview.cpp` | 56, 147, 206-222, 267 | `QApplication::desktop()`, `screenGeometry()`, `screenCount()`, `screenNumber()` |
| `gui/uimessagebox.cpp` | 59, 92, 114, 135, 149, 178 | `QApplication::desktop()->screenGeometry()` |
| `gui/uisplashscreen.cpp` | 32 | `QApplication::desktop()->screenGeometry()` |
| `transport/uieditor.cpp` | 34, 52 | `QApplication::desktop()->screenGeometry()` / `availableGeometry()` |
| `transport/uiabout.cpp` | 32 | `QApplication::desktop()->screenGeometry()` |
| `interfaces/extoscpatternask.cpp` | 62 | `QApplication::desktop()->screenGeometry()` |
| `transport/uieditor.h` | 29 | `#include <QDesktopWidget>` |
| `transport/uiabout.h` | 28 | `#include <QDesktopWidget>` |
| `interfaces/extoscpatternask.h` | 29 | `#include <QDesktopWidget>` |
| `gui/uisplashscreen.h` | 30 | `#include <QDesktopWidget>` |
| `gui/uimessagebox.h` | 29 | `#include <QDesktopWidget>` |

### Migration path

```cpp
// Before
QApplication::desktop()->screenGeometry()

// After
QGuiApplication::primaryScreen()->geometry()
// or for specific screen:
QGuiApplication::screens().at(i)->geometry()
```

- `screenCount()` becomes `QGuiApplication::screens().size()`.
- `screenNumber(point)` becomes `QGuiApplication::screenAt(point)`.
- Remove the `fullscreenDisplays` member; use `QGuiApplication::screens()` directly.
- Remove all `#include <QDesktopWidget>`, add `#include <QScreen>` and `#include <QGuiApplication>`.

---

## 3. QString::SplitBehavior to Qt::SplitBehavior

**Impact: MEDIUM -- largest single source of build warnings (~200+).**

`QString::SkipEmptyParts` and `QString::KeepEmptyParts` are deprecated in Qt 5.14+
in favor of `Qt::SkipEmptyParts` and `Qt::KeepEmptyParts`.

### Files affected (25+ files)

| File | Approx. call sites |
|------|-------------------|
| `transport/transport.h` | 2 |
| `transport/transport.cpp` | 5 |
| `objects/nxobject.h` | 9 |
| `objects/nxdocument.cpp` | 1 |
| `objects/nxcurve.h` | 6 |
| `objects/nxcurve.cpp` | 3 |
| `objects/nxcursor.h` | 3 |
| `objects/nxcursor.cpp` | 1 |
| `interfaces/interfacetcp.cpp` | 1 |
| `interfaces/interfaceserial.cpp` | 1 |
| `interfaces/interfacehttp.cpp` | 5 |
| `interfaces/interfaceosc.cpp` | 1 |
| `interfaces/interfaceudp.cpp` | 2 |
| `interfaces/extoscpatterneditor.cpp` | 4 |
| `interfaces/qwebsockets/websocket.cpp` | 2 |
| `interfaces/qwebsockets/handshakerequest.cpp` | 6 |
| `iannix.cpp` | 1 |
| `gui/uiview.cpp` | 1 |
| `gui/uiinspector.cpp` | 3 |
| `gui/uihelp.cpp` | 2 |
| `abstractionsgl.h` | 1 |

### Migration path

Global search-and-replace:

```
QString::SkipEmptyParts  ->  Qt::SkipEmptyParts
QString::KeepEmptyParts  ->  Qt::KeepEmptyParts
```

Both exist since Qt 5.14, so this is safe for all Qt 5.14+ builds.

---

## 4. QWheelEvent::delta to angleDelta

**Impact: LOW -- removed in Qt6.**

### Files affected

| File | Line | Code |
|------|------|------|
| `render/uirender.cpp` | 651, 654-655 | `event->delta() / 150.0F` |
| `gui/qjsedit/jsedit.cpp` | 870 | `e->delta() / 20` |

### Migration path

```cpp
// Before
event->delta()
// After
event->angleDelta().y()
```

---

## 5. QMouseEvent::pos to position

**Impact: LOW -- deprecated in Qt 5.15, removed in Qt6.**

### Files affected

| File | Lines |
|------|-------|
| `render/uirender.cpp` | 659, 661, 763, 798, 800 |
| `items/uitreeviewwidget.cpp` | 35 |
| `gui/qjsedit/jsedit.cpp` | 480-481 |

### Migration path

```cpp
// Before
event->pos()       // returns QPoint
// After
event->position()  // returns QPointF (Qt 5.15+, required in Qt6)
```

---

## 6. QTime as timer to QElapsedTimer

**Impact: LOW -- removed in Qt6.**

### Files affected

| File | Lines | Usage |
|------|-------|-------|
| `iannix.cpp` | 336 | `QTime::start()` |
| `iannix.cpp` | 366 | `QTime::elapsed()` |

### Migration path

Replace `QTime` timer usage with `QElapsedTimer`:

```cpp
// Before
QTime timer;
timer.start();
int ms = timer.elapsed();

// After
QElapsedTimer timer;
timer.start();
qint64 ms = timer.elapsed();
```

---

## 7. qrand/qsrand to QRandomGenerator

**Impact: LOW -- removed in Qt6.**

### Files affected

| File | Lines |
|------|-------|
| `iannix.cpp` | 168-169 |

### Migration path

```cpp
// Before
qsrand(QDateTime::currentMSecsSinceEpoch());
int r = qrand();

// After
quint32 r = QRandomGenerator::global()->generate();
```

---

## 8. QPainter deprecated hints

**Impact: LOW.**

### Files affected

| File | Line | Deprecated API |
|------|------|----------------|
| `abstractionsgl.cpp` | 696 | `QPainter::HighQualityAntialiasing` |

### Migration path

Remove `QPainter::HighQualityAntialiasing` from the flags -- `QPainter::Antialiasing`
alone provides the same behavior since Qt 5.0.

---

## 9. QTextOption::setTabStop and setTabStopWidth

**Impact: LOW.**

### Files affected

| File | Lines | API |
|------|-------|-----|
| `abstractionsgl.cpp` | 705, 752 | `QTextOption::setTabStop(40)` |
| `transport/uieditor.cpp` | 45, 51 | `QPlainTextEdit::setTabStopWidth(...)` |
| `messages/messagemanagerlog.ui` | (generated) | `setTabStopWidth` in UI properties |

### Migration path

```cpp
// Before
textOption.setTabStop(40);
editor->setTabStopWidth(width);

// After (Qt 5.10+)
textOption.setTabStopDistance(40);
editor->setTabStopDistance(width);
```

For the `.ui` file, edit the property in Qt Designer or hand-edit the XML.

---

## 10. QFontMetrics::width to horizontalAdvance

**Impact: LOW.**

### Files affected

| File | Lines |
|------|-------|
| `gui/qjsedit/jsedit.cpp` | 982, 986 |

### Migration path

```cpp
// Before
metrics.width(ch)
// After (Qt 5.11+)
metrics.horizontalAdvance(ch)
```

---

## 11. QTextCodec (removed in Qt6)

**Impact: MEDIUM -- requires Qt6 Core5Compat or rewrite.**

### Files affected

| File | Usage |
|------|-------|
| `iannixapp.cpp` | `#include <QTextCodec>`, `setCodecForTr`, `setCodecForLocale`, `setCodecForCStrings` (inside `#ifdef QT4` -- already dead code) |
| `interfaces/qwebsockets/dataprocessor.h` | `QTextCodec *`, `QTextCodec::ConverterState *` members |
| `interfaces/qwebsockets/dataprocessor.cpp` | `QTextCodec::codecForName("UTF-8")`, `ConverterState` |
| `interfaces/qwebsockets/websocket.cpp` | `QTextCodec::codecForName("UTF-8")`, `ConverterState` |

### Migration path

- `iannixapp.cpp`: The QTextCodec calls are already in `#ifdef QT4` -- remove the entire block.
- `qwebsockets/`: This is vendored code. If replacing with Qt WebSockets module (see
  [Section 20](#20-vendored-libraries)), the issue disappears. If keeping the vendored
  code, replace `QTextCodec::codecForName("UTF-8")` with `QStringDecoder(QStringDecoder::Utf8)`
  (Qt6) or simply use `QString::fromUtf8()`.

---

## 12. QRegExp to QRegularExpression

**Impact: MEDIUM -- QRegExp removed in Qt6.**

### Files affected

| File | Lines | Usage |
|------|-------|-------|
| `interfaces/interfacehttp.cpp` | 183 | `split(QRegExp("[ \\r\\n][ \\r\\n]*"))` |
| `interfaces/qwebsockets/websocket.cpp` | 811-816 | `QRegExp(regExpStatusLine)`, `regExp.indexIn(...)` |
| `interfaces/qextserialport/qextserialport_win.cpp` | 68-70 | `QRegExp` for COM port matching |
| `interfaces/qextserialport/qextserialenumerator_win.cpp` | 181-184 | `QRegExp` for VID/PID parsing |
| `interfaces/qextserialport/qextserialenumerator.cpp` | 37 | `#include <QRegExp>` (unused) |

### Migration path

```cpp
// Before
QRegExp rx("pattern");
str.contains(rx);
rx.indexIn(str);
rx.cap(n);

// After
QRegularExpression re("pattern");
auto match = re.match(str);
match.hasMatch();
match.captured(n);

// For split:
str.split(QRegularExpression("[ \\r\\n]+"))
```

---

## 13. Qt4 preprocessor branches

**Impact: MEDIUM -- dead code that should be removed for clarity.**

Every `#ifdef QT4` block is unreachable (CMake defines `QT5`, never `QT4`).  Remove
the `QT4` branches and keep only the `else` (Qt5) code.

### Files affected (14 blocks across 10 files)

| File | Lines | Qt4 code being guarded |
|------|-------|------------------------|
| `render/uirenderpreview.h` | 45-48 | `QGLWidget` vs `QOpenGLWidget` base class |
| `render/uirender.cpp` | 159-163 | `QDesktopServices::storageLocation` vs `QStandardPaths` |
| `render/uirender.cpp` | 185-196 | `renderPixmap` / `grabFrameBuffer` |
| `messages/message.cpp` | 626-632, 660-666, 702-708 | `url.addQueryItem` vs `QUrlQuery` (3 blocks) |
| `items/uitreeview.cpp` | 182-186 | `header()->setResizeMode` vs `setSectionResizeMode` |
| `items/uifileitem.cpp` | 121-125 | `QDesktopServices::storageLocation` |
| `interfaces/interfacehttp.h` | 60-64 | `incomingConnection(int)` vs `(qintptr)` |
| `interfaces/interfacehttp.cpp` | 159-173, 192-196 | Same + `url.queryItems()` vs `QUrlQuery` |
| `interfaces/interfacetcp.h` | 49-53 | `incomingConnection(int)` vs `(qintptr)` |
| `interfaces/interfacetcp.cpp` | 108-126 | Same |
| `iannixapp.cpp` | 33-37 | `QTextCodec::setCodecForTr` etc. |
| `iannixapp.cpp` | 98-102 | `QDesktopServices::storageLocation` |

Also remove `#ifdef QT5` guards (4 files: `render/uirenderpreview.h`,
`misc/application.h`, `iannix.cpp`, `iannixapp.cpp`) -- the code inside
becomes unconditional.

---

## 14. OpenGL immediate mode and QGLWidget

**Impact: HIGH -- major rendering rewrite for modern OpenGL.**

### QGLWidget (deprecated, removed in Qt6)

The codebase already has dual paths controlled by `USE_GLWIDGET` vs
`USE_OPENGLWIDGET`. In the default Qt5 build, `USE_OPENGLWIDGET` is active
and `QOpenGLWidget` is used. The `QGLWidget` code paths remain as dead code.

**Files with QGLWidget code (inside `#ifdef USE_GLWIDGET`):**
`abstractionsgl.h`, `abstractionsgl.cpp`, `misc/application.h`,
`misc/application.cpp`, `misc/options.h`, `render/uirender.cpp`,
`render/uirenderpreview.h`, `render/uirenderpreview.cpp`,
`render/uirendersyphon.h`, `interfaces/extkinectmanager.h`

**Action:** Remove all `#ifdef USE_GLWIDGET` branches after confirming
`QOpenGLWidget` path works on all platforms.

### Immediate-mode OpenGL (glBegin/glEnd)

This is the largest rendering concern.  The codebase uses legacy OpenGL 1.x
fixed-function pipeline extensively.

| File | Primitives used |
|------|-----------------|
| `render/uirender.cpp` | `GL_QUADS`, `GL_LINES` (~8 blocks) |
| `render/uirenderpreview.cpp` | `GL_QUADS` |
| `objects/nxtrigger.cpp` | `GL_QUADS`, `GL_POLYGON`, `GL_LINE_LOOP` + display lists |
| `objects/nxcurve.cpp` | `GL_LINE_LOOP`, `GL_LINE_STRIP`, `GL_LINES`, `GL_QUADS` + display lists |
| `objects/nxcursor.cpp` | Multiple primitives + display lists |
| `abstractionsgl.cpp` | `GL_QUADS`, `GL_LINE_LOOP` |
| `interfaces/extkinectmanager.cpp` | `GL_LINE_STRIP` |

Also uses display lists (`glGenLists`, `glNewList`, `glCallList`) in
`nxtrigger.cpp`, `nxcurve.cpp`, `nxcursor.cpp`.

**Migration path:** Replace with `QOpenGLBuffer` + `QOpenGLShaderProgram` +
`QOpenGLVertexArrayObject`, or a minimal vertex-buffer abstraction.  This is a
large refactor -- consider doing it module by module (render first, then objects).

---

## 15. std::auto_ptr to std::unique_ptr

**Impact: LOW -- C++17 removes auto_ptr.**

### Files affected (vendored muParser)

| File | Line |
|------|------|
| `geometry/qmuparser/muParserBase.h` | 291 |
| `geometry/qmuparser/muParserToken.h` | 72 |
| `geometry/qmuparser/muParserTokenReader.cpp` | 150 |

### Migration path

Replace `std::auto_ptr<T>` with `std::unique_ptr<T>` and add `std::move()` at
transfer points.  Alternatively, upgrade to a newer muParser release.

---

## 16. register storage class

**Impact: LOW -- illegal in C++17.**

### Files affected

| File | Line |
|------|------|
| `geometry/nxpolygon.cpp` | 62 |

### Migration path

Remove the `register` keyword.

---

## 17. SIGNAL/SLOT macros to function pointers

**Impact: LOW priority but WIDESPREAD -- improves type safety and performance.**

The entire codebase uses string-based `connect(obj, SIGNAL(...), obj, SLOT(...))`
syntax.  This works in both Qt5 and Qt6, so it is not a blocker, but the modern
`connect(obj, &Class::signal, obj, &Class::slot)` form catches errors at compile
time.

### Files affected

Approximately **30+ files** use macro-based connections, including:
`iannix.cpp`, `misc/options.cpp`, `gui/uiview.cpp`, `render/uirender.cpp`,
`objects/nxdocument.cpp`, `objects/nxtrigger.cpp`, `items/uitreeview.cpp`,
`items/uifileitem.cpp`, all `interfaces/interface*.cpp`, vendored
`qextserialport/*.cpp`, `qwebsockets/*.cpp`, `zeroconf/*.cpp`,
`gui/qjsedit/jsedit.cpp`, `abstractionsgl.cpp`.

### Migration path

Convert file by file.  The vendored libraries are lower priority since they may
be replaced entirely (see [Section 20](#20-vendored-libraries)).

---

## 18. qSort to std::sort

**Impact: LOW.**

### Files affected

| File | Line |
|------|------|
| `interfaces/qextserialport/qextserialenumerator_win.cpp` | 233 |
| `interfaces/interfaceosc.cpp` | 211 |

### Migration path

```cpp
// Before
qSort(container.begin(), container.end(), comparator);
// After
std::sort(container.begin(), container.end(), comparator);
```

---

## 19. QVariant::Type enum

**Impact: LOW.**

### Files affected

| File | Line | Usage |
|------|------|-------|
| `items/uitreedelegate.cpp` | 132 | `data(index).canConvert(QVariant::Color)` |

### Migration path

```cpp
// Before (deprecated in Qt6)
data.canConvert(QVariant::Color)
// After
data.canConvert<QColor>()
```

---

## 20. Vendored libraries

### qextserialport (interfaces/qextserialport/)

- **Age:** 2000-2011, "QESP2.0"
- **Used by:** `interfaces/interfaceserial.h/.cpp` only
- **Replacement:** `QSerialPort` + `QSerialPortInfo` (Qt 5.1+, `Qt5::SerialPort`)
- **Coupling:** Low-medium.  Isolated to one interface class.
- **Effort:** Moderate -- API mapping for baud/parity/flow/DTR/RTS/enumeration.

### qwebsockets (interfaces/qwebsockets/)

- **Age:** Early Qt5 era, no TLS, no subprotocols
- **Used by:** `interfaces/interfacehttp.h/.cpp` only
- **Replacement:** `QWebSocket` + `QWebSocketServer` (Qt 5.3+, `Qt5::WebSockets`)
- **Coupling:** Medium.  Confined to `InterfaceHttp` but involves signal/slot rewiring.
- **Effort:** Moderate -- different class names and connection semantics.

### qrtmidi (interfaces/qrtmidi/)

- **Age:** RtMidi 2.0.1, 2003-2012
- **Used by:** `interfaces/interfacemidi.h/.cpp`
- **Replacement:** System RtMidi package (latest 6.x) -- same API family, much newer.
- **Coupling:** High for MIDI.  All device open/send/receive goes through this.
- **Effort:** Low if API is compatible; check for breaking changes between 2.x and 6.x.

### qmuparser (geometry/qmuparser/)

- **Age:** muParser 2.2.5, 2015
- **Used by:** `objects/nxcurve.h/.cpp` -- core curve equation evaluation
- **Replacement:** Newer muParser from upstream, or alternative (exprtk, tinyexpr).
- **Coupling:** High -- deeply integrated into parametric curve math.
- **Effort:** Low if upgrading muParser in-place; high if switching library.
- **Note:** Contains `std::auto_ptr` (see [Section 15](#15-stdauto_ptr-to-stdunique_ptr)).

### artnet (interfaces/artnet/)

- **Age:** libartnet, 2004-2007
- **Used by:** **Nothing** -- compiled on macOS but no app code references it.
- **Replacement:** Remove or wire up if Art-Net support is desired.
- **Coupling:** None.
- **Effort:** Zero to remove.

### jsedit (gui/qjsedit/)

- **Age:** Ofi Labs X2 JSEdit, 2010 (Ariya Hidayat)
- **Used by:** `transport/uieditor.ui` and `transport/uieditor.cpp`
- **Replacement:** `QSyntaxHighlighter` + `QPlainTextEdit`, or KSyntaxHighlighting/QScintilla.
- **Coupling:** Low -- single widget in the script editor.
- **Effort:** Low-medium.

### qffmpeg (gui/qffmpeg/) -- optional, USE_FFMPEG

- **Age:** QTFFmpegWrapper 2009-2010; vendored headers are FFmpeg ~0.10 era (libavcodec 53)
- **Used by:** `render/uirender.h` (when FFMPEG_INSTALLED defined)
- **Replacement:** Link against system FFmpeg with updated wrapper, or use Qt Multimedia.
- **Coupling:** Medium when enabled.
- **Effort:** High -- vendored API is ancient vs modern FFmpeg 6.x/7.x.
- **Note:** Include path inconsistency: `uirender.h` includes `interfaces/qffmpeg/QVideoEncoder.h`
  but files are in `gui/qffmpeg/`.

---

## 21. Summary matrix

| # | Issue | Severity | Files | Blocks Qt6? | Suggested phase |
|---|-------|----------|-------|-------------|-----------------|
| 1 | QtScript -> QJSEngine | Critical | 9 | **Yes** | Phase 1 |
| 2 | QDesktopWidget -> QScreen | High | 12 | **Yes** | Phase 1 |
| 3 | QString::SplitBehavior | Medium | 25+ | No (compat) | Phase 2 |
| 4 | QWheelEvent::delta | Low | 2 | **Yes** | Phase 2 |
| 5 | QMouseEvent::pos | Low | 3 | **Yes** | Phase 2 |
| 6 | QTime -> QElapsedTimer | Low | 1 | **Yes** | Phase 2 |
| 7 | qrand/qsrand | Low | 1 | **Yes** | Phase 2 |
| 8 | QPainter hints | Low | 1 | No | Phase 2 |
| 9 | setTabStop/Width | Low | 3 | No | Phase 2 |
| 10 | QFontMetrics::width | Low | 1 | No | Phase 2 |
| 11 | QTextCodec | Medium | 4 | **Yes** | Phase 2 |
| 12 | QRegExp | Medium | 5 | **Yes** | Phase 2 |
| 13 | Qt4 branches | Medium | 10 | No | Phase 2 |
| 14 | OpenGL legacy | High | 8 | Partial | Phase 3 |
| 15 | std::auto_ptr | Low | 3 | Compiler err | Phase 2 |
| 16 | register keyword | Low | 1 | Compiler err | Phase 2 |
| 17 | SIGNAL/SLOT macros | Low | 30+ | No | Phase 3 |
| 18 | qSort | Low | 2 | **Yes** | Phase 2 |
| 19 | QVariant::Type | Low | 1 | No | Phase 2 |
| 20 | Vendored libs | Varies | -- | Indirect | Phase 3 |

### Recommended phases

**Phase 1 -- Qt6 blockers (critical path):**
Migrate QtScript to QJSEngine, replace QDesktopWidget with QScreen.

**Phase 2 -- Deprecation cleanup:**
Remove Qt4 branches, fix all deprecated API calls (split behavior, wheel/mouse
events, timer, random, font metrics, tab stops, QRegExp, auto_ptr, register,
qSort, QVariant).  This should reduce build warnings from ~787 to near zero.

**Phase 3 -- Modernization:**
Replace vendored libraries with Qt/system equivalents, modernize OpenGL
rendering, convert SIGNAL/SLOT to function-pointer connections.
