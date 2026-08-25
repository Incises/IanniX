# IanniX Migration Guide

This document catalogs every deprecated API, vendored library, and modernization
target in the IanniX codebase.  It is the authoritative reference for the ongoing
Qt5 cleanup and future Qt6 migration.

The Qt5 build currently compiles with **zero warnings**.
Each section below maps to a category of work.  Items marked **DONE** have been
verified clean by a full rebuild with clang against the current source tree.

---

## Table of Contents

1. [QtScript to QJSEngine](#1-qtscript-to-qjsengine) ✅
2. [QDesktopWidget to QScreen](#2-qdesktopwidget-to-qscreen) ✅
3. [QString::SplitBehavior to Qt::SplitBehavior](#3-qstringsplitbehavior-to-qtsplitbehavior) ✅
4. [QWheelEvent::delta to angleDelta](#4-qwheeleventdelta-to-angledelta) ✅
5. [QMouseEvent::pos to position](#5-qmouseeventpos-to-position) ✅
6. [QTime as timer to QElapsedTimer](#6-qtime-as-timer-to-qelapsedtimer) ✅
7. [qrand/qsrand to QRandomGenerator](#7-qrandqsrand-to-qrandomgenerator) ✅
8. [QPainter deprecated hints](#8-qpainter-deprecated-hints) ✅
9. [QTextOption::setTabStop and setTabStopWidth](#9-qtextoptionsettabstop-and-settabstopwidth) ✅
10. [QFontMetrics::width to horizontalAdvance](#10-qfontmetricswidth-to-horizontaladvance) ✅
11. [QTextCodec (removed in Qt6)](#11-qtextcodec-removed-in-qt6) ✅
12. [QRegExp to QRegularExpression](#12-qregexp-to-qregularexpression) ✅
13. [Qt4 preprocessor branches](#13-qt4-preprocessor-branches) ✅
14. [OpenGL immediate mode and QGLWidget](#14-opengl-immediate-mode-and-qglwidget)
15. [std::auto_ptr to std::unique_ptr](#15-stdauto_ptr-to-stdunique_ptr) ✅
16. [register storage class](#16-register-storage-class) ✅
17. [SIGNAL/SLOT macros to function pointers](#17-signalslot-macros-to-function-pointers)
18. [qSort to std::sort](#18-qsort-to-stdsort) ✅
19. [QVariant::Type enum](#19-qvarianttype-enum) ✅
20. [Vendored libraries](#20-vendored-libraries)
21. [Summary matrix](#21-summary-matrix)

---

## 1. QtScript to QJSEngine

**Status: DONE**

All `QScriptEngine`, `QScriptValue`, and `QScriptValueList` references have been
replaced with `QJSEngine`, `QJSValue`, and `QJSValueList`.  CMakeLists.txt uses
`Qt5::Qml` / `Qt6::Qml` instead of the removed `Qt5::Script`.

Error handling was updated: `engine.uncaughtExceptionBacktrace()` and
`engine.uncaughtExceptionLineNumber()` (which do not exist in QJSEngine) are
replaced with helpers that read `errorValue.property("stack")` and
`errorValue.property("lineNumber")` from the returned `QJSValue`.

---

## 2. QDesktopWidget to QScreen

**Status: DONE**

`QDesktopWidget` is removed in Qt6.

### Files affected

| File | Lines | Usage |
|------|-------|-------|
| `gui/uiview.h` | 50 | `QDesktopWidget *fullscreenDisplays` member |
| `gui/uiview.cpp` | 147, 206–222, 267 | `QApplication::desktop()`, `screenCount()`, `screenGeometry()`, `screenNumber()` |

### Migration path

```cpp
// Before
QDesktopWidget *d = QApplication::desktop();
d->screenCount()
d->screenGeometry(index)
d->screenNumber(point)

// After
QGuiApplication::screens().size()
QGuiApplication::screens().at(index)->geometry()
QGuiApplication::screenAt(point)
```

- Remove the `fullscreenDisplays` member; call `QGuiApplication::screens()` directly.
- Replace `connect(..., SIGNAL(screenCountChanged(int)), ...)` with
  `connect(qApp, &QGuiApplication::screenAdded, ...)` and `screenRemoved`.
- Add `#include <QScreen>` and `#include <QGuiApplication>`; remove `#include <QDesktopWidget>`.

---

## 3. QString::SplitBehavior to Qt::SplitBehavior

**Status: DONE**

All `QString::SkipEmptyParts` and `QString::KeepEmptyParts` occurrences (51 across
18 files) have been replaced with the Qt 5.14+ namespace form `Qt::SkipEmptyParts`
/ `Qt::KeepEmptyParts`.  Verified by a warning-free full rebuild.

---

## 4. QWheelEvent::delta to angleDelta

**Status: DONE**

All `event->delta()` calls have been replaced with `event->angleDelta().y()`.
The last three were in `UiRender::wheelEvent` (`render/uirender.cpp`).

---

## 5. QMouseEvent::pos to position

**Status: DONE**

`QMouseEvent::pos()` is deprecated in Qt 5.15 and removed in Qt6.

### Files affected

| File | Lines |
|------|-------|
| `render/uirender.cpp` | 649, 652, 756, 794, 796 |
| `items/uitreeviewwidget.cpp` | 44 |
| `gui/qjsedit/jsedit.cpp` | 484, 485 |

### Migration path

```cpp
// Before
event->pos()       // returns QPoint
// After
event->position().toPoint()  // position() returns QPointF (Qt 5.15+)
```

For Qt5 compatibility, this migration uses local helpers that fall back to
`localPos()` / `posF()` and switches to `position()` on Qt6.

---

## 6. QTime as timer to QElapsedTimer

**Status: DONE**

Using `QTime` as a stopwatch (`start()` / `elapsed()`) is deprecated. `QTime`
itself is not removed in Qt6, but this misuse triggers deprecation warnings.

### Files affected

| File | Symbol | Usage |
|------|--------|-------|
| `render/uirender.h` | `QTime renderMeasure` | member |
| `render/uirender.cpp` | `renderMeasure` | `.start()` / `.elapsed()` |
| `transport/transport.h` | `static QTime renderMeasureAbsolute` | member |
| `app/iannix.h` | `#include <QTime>` | pulled in for timer use |
| `app/iannix.cpp` | local `QTime` | `.start()` / `.elapsed()` |

### Migration path

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

Replace `#include <QTime>` with `#include <QElapsedTimer>` in affected headers.

---

## 7. qrand/qsrand to QRandomGenerator

**Status: DONE**

No occurrences of `qrand` or `qsrand` remain.  The anonymous-id generation in
`app/iannix.cpp` now uses `QRandomGenerator::global()->generate()`; the adjacent
deprecated `QDateTime(QDate)` constructor was replaced with `QDate::startOfDay()`.

---

## 8. QPainter deprecated hints

**Status: DONE**

`QPainter::HighQualityAntialiasing` was removed from the render-hint flags in
`render/abstractionsgl.cpp`.  `QPainter::Antialiasing` alone has provided the
same behavior since Qt 5.0.

---

## 9. QTextOption::setTabStop and setTabStopWidth

**Status: DONE**

All call sites migrated to the Qt 5.10+ `setTabStopDistance` API:

- `render/abstractionsgl.cpp` — `QTextOption::setTabStop(40)` → `setTabStopDistance(40)`
- `gui/codeeditor/codeeditor.cpp` — `CodeEditor::setTabStopWidth` wrapper now calls
  `QPlainTextEdit::setTabStopDistance` internally (public API unchanged)
- `messages/messagemanagerlog.ui` — `tabStopWidth` properties changed to
  `tabStopDistance` so the uic-generated header is warning-free

---

## 10. QFontMetrics::width to horizontalAdvance

**Status: DONE**

`gui/qjsedit/jsedit.cpp` (the only affected file) has been removed; its replacement
`gui/codeeditor/codeeditor.cpp` uses `fontMetrics().horizontalAdvance()` throughout.

---

## 11. QTextCodec (removed in Qt6)

**Status: DONE**

The vendored `qwebsockets/` code that used `QTextCodec` has been removed, and
the stale unused `#include <QTextCodec>` in `app/iannixapp.cpp` has been deleted.
No `QTextCodec` references remain.

---

## 12. QRegExp to QRegularExpression

**Status: DONE**

The vendored `qwebsockets/` and `qextserialport/` code that contained all
`QRegExp` usage has been removed.  No `QRegExp` references remain.

---

## 13. Qt4 preprocessor branches

**Status: DONE**

All `#ifdef QT4`, `#ifndef QT4`, `#ifdef QT5`, and `#ifndef QT5` blocks have
been removed.  Qt5 code paths are now unconditional.

---

## 14. OpenGL immediate mode and QGLWidget

**Status: TODO — major rendering rewrite**

The `QGLWidget` dead-code paths have been removed.  The active `QOpenGLWidget`
path still uses legacy OpenGL 1.x fixed-function pipeline throughout.

### Files with glBegin/glEnd

| File | Occurrences |
|------|-------------|
| `render/abstractionsgl.cpp` | 4 blocks (lines 634, 652, 661, 675) |
| `objects/nxcursor.cpp` | 13+ blocks |
| `objects/nxcurve.cpp` | multiple |
| `objects/nxtrigger.cpp` | multiple |
| `render/uirender.cpp` | ~8 blocks |
| `render/uirenderpreview.cpp` | 1 block |
| `interfaces/extkinectmanager.cpp` | 2 blocks |

OpenGL display lists (`glGenLists`, `glNewList`, `glCallList`) are also used in
`nxcursor.cpp`, `nxcurve.cpp`, and `nxtrigger.cpp`.

### Migration path

Replace with `QOpenGLBuffer` + `QOpenGLShaderProgram` + `QOpenGLVertexArrayObject`.
Suggested order: render layer first (`uirender`, `abstractionsgl`), then objects
(`nxcurve`, `nxcursor`, `nxtrigger`).

---

## 15. std::auto_ptr to std::unique_ptr

**Status: DONE**

The vendored muParser and RtMidi copies that contained all `std::auto_ptr` usage
have been replaced with system packages.  No `std::auto_ptr` references remain
in the project source tree.

---

## 16. register storage class

**Status: DONE**

The `register` keyword has been removed from `geometry/nxpolygon.cpp`.  No
occurrences remain in project source (remaining grep hits are English prose in
vendored zeroconf headers).

---

## 17. SIGNAL/SLOT macros to function pointers

**Status: TODO — low priority, widespread**

The entire codebase uses string-based `connect(obj, SIGNAL(...), obj, SLOT(...))`
syntax.  This works in both Qt5 and Qt6, so it is not a blocker, but the modern
pointer-based form catches errors at compile time and is faster at runtime.

Approximately **200+ occurrences** across 22 files, with `app/iannix.cpp` (~37) and
`gui/uiview.cpp` (~68) being the densest.

### Migration path

```cpp
// Before
connect(obj, SIGNAL(valueChanged(int)), other, SLOT(update(int)));
// After
connect(obj, &ClassName::valueChanged, other, &OtherClass::update);
```

Convert file by file.

---

## 18. qSort to std::sort

**Status: DONE**

The single `qSort` call in `interfaces/interfaceosc.cpp` has been replaced with
`std::sort` (and `<algorithm>` is now included).

---

## 19. QVariant::Type enum

**Status: DONE**

`items/uitreedelegate.cpp` now uses the template form `canConvert<QColor>()`
instead of the deprecated `canConvert(QVariant::Color)`.

---

## 20. Vendored libraries

### qextserialport — DONE, removed

Replaced with `QSerialPort` + `QSerialPortInfo` (`Qt5::SerialPort`).

### qwebsockets — DONE, removed

Replaced with `QWebSocket` + `QWebSocketServer` (`Qt5::WebSockets`).

### qrtmidi — DONE, removed

Replaced with the system `RtMidi` package (6.0.0).  The only breaking API change
between the vendored 2.0.1 and 6.x was the rename of `RtError` to `RtMidiError`;
all call sites in `interfaces/interfacemidi.cpp` have been updated.  The
`interfaces/qrtmidi/` directory has been deleted.

### qmuparser — DONE, removed

Replaced with the system `muParser` package (2.3.5).  The `geometry/qmuparser/`
directory has been deleted; `objects/nxcurve.h` now includes `<muParser.h>`.

### jsedit (`gui/qjsedit/`) — DONE, removed

Replaced by `gui/codeeditor/CodeEditor`, a `QPlainTextEdit` subclass backed by the system
`KF5SyntaxHighlighting` library (openSUSE package: `syntax-highlighting-devel`).
The widget is language-agnostic: call `setLanguage("JavaScript")` (or any other
KSyntaxHighlighting definition name) at construction time.
`QFontMetrics::width` (§10) and `QMouseEvent::pos` (§5) issues from jsedit are gone.

### artnet (`interfaces/artnet/`)

- **Age:** libartnet, 2004–2007
- **Used by:** Nothing — compiled on macOS but not referenced by app code.
- **Action:** Remove entirely, or wire up if Art-Net support is ever desired.

### qffmpeg (`gui/qffmpeg/`) — optional, `USE_FFMPEG`

- **Age:** QTFFmpegWrapper (2009–2010); vendored headers are FFmpeg ~0.10 era
- **Used by:** `render/uirender.h` (when `FFMPEG_INSTALLED` defined)
- **Replacement:** Link against system FFmpeg with updated wrapper, or use Qt Multimedia.
- **Effort:** High — vendored API is ancient vs modern FFmpeg 6.x/7.x.

---

## 21. Summary matrix

| # | Issue | Severity | Blocks Qt6? | Status |
|---|-------|----------|-------------|--------|
| 1 | QtScript → QJSEngine | Critical | **Yes** | ✅ DONE |
| 2 | QDesktopWidget → QScreen | High | **Yes** | ✅ DONE |
| 3 | QString::SplitBehavior | Medium | No | ✅ DONE |
| 4 | QWheelEvent::delta | Low | **Yes** | ✅ DONE |
| 5 | QMouseEvent::pos | Low | **Yes** | ✅ DONE |
| 6 | QTime → QElapsedTimer | Low | **Yes** | ✅ DONE |
| 7 | qrand/qsrand | Low | **Yes** | ✅ DONE |
| 8 | QPainter hints | Low | No | ✅ DONE |
| 9 | setTabStop/Width | Low | No | ✅ DONE |
| 10 | QFontMetrics::width | Low | No | ✅ DONE |
| 11 | QTextCodec | Low | **Yes** | ✅ DONE |
| 12 | QRegExp | Medium | **Yes** | ✅ DONE |
| 13 | Qt4 branches | Medium | No | ✅ DONE |
| 14 | OpenGL legacy | High | Partial | TODO |
| 15 | std::auto_ptr | Low | Compiler err | ✅ DONE |
| 16 | register keyword | Low | Compiler err | ✅ DONE |
| 17 | SIGNAL/SLOT macros | Low | No | TODO |
| 18 | qSort | Low | **Yes** | ✅ DONE |
| 19 | QVariant::Type | Low | No | ✅ DONE |
| 20 | Vendored libs | Varies | Indirect | Partial — qextserialport/qwebsockets/qmuparser/qrtmidi/jsedit removed; artnet and qffmpeg remain |

### Additional fixes found by compiler audit (not in original catalog)

All fixed; the full rebuild is now warning-free:

- `gui/uiview.cpp` — mis-migrated `QApplication::trUtf8("UiView", "Delete")`
  (deprecated, and wrong: "UiView" was passed as source text) replaced with
  `QKeySequence::Delete`.
- `messages/message.cpp` — deprecated `QHostAddress = QString` assignments now
  use explicit `QHostAddress(QString)` construction; deprecated
  `QByteArray += QString` appends now use explicit `.toUtf8()`.
- `interfaces/interfacetcp.cpp` — deprecated `QByteArray::append(QString)` now
  uses `.toUtf8()`.
- `render/abstractionsgl.cpp` — `delete` on `void*` in `~OpenGlTexture()`
  (undefined behavior when built without VLC) is now guarded by
  `#ifdef VLC_INSTALLED`.
- `items/uifileitem.cpp` — added braces to silence `-Wdangling-else`.

### Remaining work

| Item | File(s) | Effort |
|------|---------|--------|
| OpenGL legacy (§14) | 7 files, ~35 `glBegin` blocks + display lists | Large — only remaining Qt6 blocker |
| SIGNAL/SLOT macros (§17) | ~165 occurrences, 22 files | Low priority, not a blocker |
| Vendored `interfaces/artnet/` (§20) | unused — delete or wire up | Small |
| Vendored `gui/qffmpeg/` (§20) | optional `USE_FFMPEG` feature | High if kept |

### Recommended phases

**Phase 1 — DONE.** All small Qt6 blockers and deprecation warnings are fixed;
the Qt5 build compiles with zero warnings.

**Phase 2 — OpenGL modernization (§14):**
Replace immediate-mode rendering with `QOpenGLBuffer` + `QOpenGLShaderProgram`
+ `QOpenGLVertexArrayObject`.  Render layer first (`uirender`,
`abstractionsgl`), then objects (`nxcurve`, `nxcursor`, `nxtrigger`).  This is
the only large item still blocking Qt6.

**Phase 3 — polish:**
Convert SIGNAL/SLOT connections to function-pointer form (§17); decide the fate
of `artnet/` and `qffmpeg/` (§20).
