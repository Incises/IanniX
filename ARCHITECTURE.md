# IanniX Architecture

This document is a human-readable guide to how IanniX is put together today.
It is intended for contributors who need a working mental model before making
changes.

It describes the code as it exists in the current tree, not the historical
design or the ideal end state.

## 1. What IanniX Is

IanniX is a desktop application for building and playing graphical scores.
Users edit curves, cursors, triggers, textures, and messages in a Qt UI, then
run the score as a real-time scheduler that emits events over protocols such as
OSC, MIDI, HTTP, TCP, UDP, Serial, Direct, and Syphon.

At a high level, the application combines:

- a document and object model
- a scriptable command layer
- a real-time playback scheduler
- an OpenGL-based renderer
- a protocol/message dispatch layer
- editor and inspector UI around the score

## 2. Top-Level Runtime Model

The main runtime object is `IanniX`.

The startup path is:

1. `main()` in `app/iannixapp.cpp` creates `IanniXApp`
2. `IanniXApp::launch()` discovers application paths, splash screen, fonts, and the project to load
3. `IanniX` is constructed
4. `IanniX` creates the main window (`UiView`), renderer, inspector, transport UI, interfaces, and message infrastructure
5. `readyToStart()` loads the project, starts periodic timers, and begins rendering

From there, the application runs as an event-driven Qt app with an additional
real-time scheduler loop driven by a `QTimer`.

## 3. The Main Architectural Units

### 3.1 Application Shell

The application shell lives in:

- `iannixapp.*`
- `iannix.*`
- `misc/application.*`

Responsibilities:

- bootstrap the Qt app
- discover runtime asset directories (`Examples`, `Tools`, `Patches`)
- create and wire the main window
- own the currently loaded document(s)
- own the scheduler timer
- act as the main command dispatcher
- bridge between UI, documents, and network/message interfaces

`misc/application.*` contains shared global state used across the app:

- current application instance
- current renderer
- path locations
- default message templates
- global render settings and options

This is not a small dependency-free utility layer. It is effectively shared
application context.

### 3.2 Document Layer

The score is represented by `NxDocument` in `objects/nxdocument.*`.

Responsibilities:

- own the objects and groups of a score
- load and save score/script files
- maintain undo/redo snapshots
- host a per-document `QJSEngine`
- evaluate score scripts and expose document functions to them
- serialize score state back into script format
- receive scripted incoming-message callbacks

Important detail:

- `IanniX` owns the active document relationship
- `NxDocument` owns the score content
- script files are not just passive assets; they are executable score logic

The code supports one current document plus additional opened documents in a
hash, but the user-facing workflow is still centered on a single active working
document.

### 3.3 Object Model

The core score model lives in `objects/`.

Main types:

- `NxObject`: abstract base for score objects
- `NxCurve`: geometric path definition and equation/path logic
- `NxCursor`: moving playhead that traverses a curve and emits messages
- `NxTrigger`: event point that fires when hit by a cursor
- `NxGroup`: logical grouping and activity/mute/solo container

Key design choices:

- objects are both data objects and UI tree items (`QTreeWidgetItem`)
- property mutation is exposed through Qt properties and command dispatch
- grouping is important at runtime, not just for organization
- objects are indexed by both group and activity/type buckets for scheduler use

`NxGroup` stores objects as:

- active/inactive
- by object type
- keyed by object id

That indexing is important because the scheduler walks those buckets directly.

### 3.4 Rendering Layer

Rendering is centered on:

- `render/uirender.*`
- `render/uirenderpreview.*`
- `abstractionsgl.*`

Responsibilities:

- draw the score and selection state
- manage camera/zoom/rotation/center state
- handle mouse and keyboard editing gestures
- preview performance output
- load and manage textures
- capture snapshots / video frames

The renderer is still an older-style OpenGL subsystem with custom drawing
abstractions and some legacy immediate-mode assumptions. It is functional, but
it is one of the more modernization-sensitive parts of the codebase.

`UiRender` is not just a view. It also contains editing logic, selection logic,
drag behavior, import interactions, and render timing.

### 3.5 UI Layer

Most UI code lives in:

- `gui/`
- `transport/`
- `items/`

Important pieces:

- `UiView`: the main window and top-level action wiring
- `UiInspector`: object/file/config inspector
- `Transport`: transport controls, performance panel, timer display, script editor launcher
- `UiEditor`: score/script editor
- `UiHelp`: contextual help system
- `items/*`: tree widgets, delegates, file/color/texture/path editors

The UI is not isolated from the model. It is tightly coupled to commands,
selection, transport, and object mutation.

### 3.6 Messaging and Interfaces

The message system lives in:

- `messages/`
- `interfaces/`

There are two related concepts:

1. inbound control messages
2. outbound event messages

`MessageManager` is the central coordinator. It:

- stores the registered network interfaces
- logs traffic
- forwards inbound commands to `IanniX`
- formats outbound object-triggered messages using `Message`

`Message` parses message templates such as OSC/MIDI/TCP URLs and expands runtime
variables from triggers, cursors, curves, collision state, and transport state.

Each interface class handles a specific transport:

- `InterfaceOsc`
- `InterfaceMidi`
- `InterfaceHttp`
- `InterfaceSerial`
- `InterfaceTcp`
- `InterfaceUdp`
- `InterfaceDirect`
- `InterfaceSyphon` on macOS

The interfaces are concrete adapters. The core application stays protocol-aware
through `MessagesType`, but most transport-specific details live in those
interface classes.

## 4. Core Runtime Flows

### 4.1 Load a Project

The typical load flow is:

1. `IanniX::loadProject()` identifies a score file or folder
2. file UI syncs the project tree
3. a current `NxDocument` is created or reused
4. `NxDocument::open()` loads the file
5. if the file is script-based, it is evaluated in `QJSEngine`
6. score-building callbacks create objects via the command layer
7. the renderer and inspector now reflect the document state

This is important: score files are not merely deserialized into structs. The
system replays script/command content to reconstruct state.

### 4.2 Execute a Command

The central command interpreter is `IanniX::execute()`.

Almost everything converges there:

- UI actions
- script calls
- inbound network commands
- some internal system commands

That function:

- determines command source (`GUI`, `network`, `script`, etc.)
- parses the textual command
- creates/removes objects
- dispatches property changes
- manipulates transport state
- loads/saves projects
- routes object-oriented commands to the selected target object/group/document

This command layer is one of the most important architectural seams in the app.
It is the shared language between the UI, scripting system, and remote control.

### 4.3 Scheduler Tick

Playback is driven from `IanniX::timerTick()`.

Per tick, the app:

1. parses pending OSC and other manually polled interfaces
2. computes elapsed wall-clock delta
3. advances global transport time
4. opens message bundling if needed
5. iterates all documents, groups, and active cursors
6. updates curves as needed
7. advances each cursor in time
8. emits cursor messages
9. checks trigger and curve collisions
10. closes message bundling

The scheduler is cursor-centric. Cursors are the active agents that move through
score geometry and cause output.

### 4.4 Outbound Message Flow

When a cursor or trigger emits an event:

1. object runtime builds a `MessageManagerDestination`
2. `MessageManager::outgoingMessage()` iterates message patterns on the object
3. `Message` expands runtime variables and encodes the result
4. the corresponding network interface sends the message
5. log views receive a human-readable trace

This makes message templates a major part of the behavior model. Objects do not
hardcode protocol payloads; they describe them via message pattern strings.

### 4.5 Inbound Control Flow

When data arrives from OSC/HTTP/MIDI/TCP/etc.:

1. interface parses the transport payload into a `MessageIncomming`
2. interface calls `MessageManager::incomingMessage()`
3. `MessageManager` logs it and forwards it to `IanniX`
4. `IanniX::incomingMessage()` runs command execution and optional document script hooks

This is how IanniX supports remote control and script-reactive behavior using
the same underlying command model.

## 5. Data Ownership and Coupling

This project is not built around strict layer isolation.

A more accurate mental model is:

- `IanniX` is the orchestrator
- `Application` is global shared state
- `NxDocument` is the score container plus scripting host
- objects are model objects with UI and serialization responsibilities
- `UiRender` is view plus editing controller
- `MessageManager` is protocol hub and formatter/dispatcher

This leads to strong coupling in several places:

- objects inherit Qt UI classes and domain interfaces at the same time
- document loading depends on executable script content
- the renderer knows about selection, editing, and object manipulation
- transport state is accessed globally from message formatting and scheduling
- command parsing is centralized instead of being split into isolated services

That coupling is workable, but contributors should expect behavior changes to
ripple across UI, scripting, transport, and messaging.

## 6. Repository Map by Responsibility

Use this as a practical “where should I look?” guide.

- `iannixapp.*`: process startup and path discovery
- `iannix.*`: application shell, command interpreter, scheduler, project management
- `misc/application.*`: shared app context, global render/options state
- `objects/`: score model and runtime behavior
- `geometry/`: math primitives and curve support
- `render/`: OpenGL widgets and performance preview
- `abstractionsgl.*`: shared OpenGL drawing and texture helpers
- `gui/`: main window, help, inspector, splash, message box
- `transport/`: transport panel, timer, about box, script editor
- `items/`: tree and editor widgets for score editing
- `messages/`: protocol-independent message formatting, logging, dispatch
- `interfaces/`: protocol-specific network/device adapters
- `Tools/`: runtime script libraries, templates, bundled helper assets
- `Examples/`: sample scores
- `Patches/`: integration examples for external tools

## 7. Build-Time Architecture

The current CMake build treats these as major external dependencies:

- Qt 5 or experimental Qt 6
- `muparser`
- `RtMidi`

Optional features add:

- FFmpeg
- Kinect
- Wacom
- Syphon on macOS

The project is now 64-bit-only. The active CMake configuration explicitly
recognizes:

- x86_64 / AMD64
- ARM64 / aarch64
- macOS universal `x86_64;arm64`

## 8. Platform-Specific Structure

- macOS:
  uses Syphon and Objective-C++ sources, bundles runtime assets in the app bundle
- Linux:
  uses ALSA-related code paths and links OpenGL/ALSA system libraries
- Windows:
  uses WinMM-related code paths and platform libraries such as `winmm`

The interface layer carries most of the protocol/platform-specific behavior, but
build configuration and a few UI/rendering details are also platform-sensitive.

## 9. Architectural Pressure Points

These are the areas most likely to affect multiple subsystems when changed.

### Command and Property System

The text command interpreter is a central integration point. It is powerful, but
also broad: UI, scripting, remote control, document loading, and object mutation
all depend on it.

### Scripting

Each document has its own JS engine, and score files can create objects,
configure behavior, and respond to incoming messages. This makes scripts part of
the runtime architecture, not just automation.

### Rendering

The renderer mixes drawing, interaction, selection, and capture. Render changes
often affect input behavior and editing behavior too.

### Messaging

Message templates are dynamic and can depend on object state, cursor state,
collision state, transport state, and script evaluation. A small change in
message generation can affect many protocols at once.

## 10. Recommended Mental Model for Contributors

When making a change, think in this order:

1. Is this a document/model concern, a command concern, a UI concern, a render concern, or a protocol concern?
2. Does the change affect both local editing and remote/script-driven behavior?
3. Does the change need to be serialized back into score files?
4. Does playback timing or message emission depend on it?
5. Does it touch one of the migration-sensitive areas listed in `MIGRATION.md`?

If you keep those five questions in mind, most of the project’s coupling becomes
easier to navigate.

## 11. Suggested Reading Order

For someone new to the codebase, this is a good order:

1. `BUILD.md`
2. `MIGRATION.md`
3. `app/iannixapp.cpp`
4. `app/iannix.h` / `app/iannix.cpp`
5. `objects/nxdocument.*`
6. `objects/nxobject.*`, `nxcurve.*`, `nxcursor.*`, `nxtrigger.*`, `nxgroup.*`
7. `render/uirender.*`
8. `messages/message.*` and `messages/messagemanager.*`
9. one representative interface such as `interfaces/interfaceosc.*`

That path mirrors the real runtime shape of the application and gets you to the
important architectural seams quickly.
