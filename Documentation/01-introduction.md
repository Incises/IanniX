IanniX is a graphical sequencer for digital art. Through various communication protocols, it synchronizes single events as well as continuous data to external environments (e.g. Pure Data and Processing) and hardware such as MIDI devices and microcontroller boards (cf. Chaps. 4 and 5).

Its main window shows a representation of a multidimensional and multi-format score which is programmable via GUI, JavaScript and third-party applications that use a compatible protocol; in this way, users are not forced to a specific method for approaching to the score but can benefit of multiple designing strategies according to their expertise. IanniX scores are based on three types of abstract objects to be placed in a 3D space: triggers, curves, and cursors (cf. Chap. 3). Triggers and curves represent single and continuous events, respectively. Cursors are time-based elements (playheads) that can move along the curves in order to read a specific sequence of space-limited events. An unlimited number of cursors can be added to the score. In this sense, IanniX proposes a three-dimensional and poly-temporal sequencer, unlike its predecessor [UPIC](https://en.wikipedia.org/wiki/UPIC) that was based on two-dimensional drawing and allowed for only a single timeline from left to right, as an emulation of the conventional direction of reading. Also, IanniX runs independently from any sound synthesis engine; thus, it is suitable for varied applications.

IanniX is free, open-source, and cross-platform, in order to reach almost the whole community of computer users without significant limitations. The software package is available for download at www.iannix.org.

1.1 Main applications
---------------------

Through the communication with audio environments or MIDI devices, IanniX can be used as a tool for the creation and performance of musical scores with a graphic notation. Many object attributes as well as various mapping modes (cf. Chap. 3.2) allow the user to match the characteristics and behavior of cursors, curves, and triggers to sound and music parameters and several MIDI messages (cf. Chap. 5.2). The capability to import external graphics amplifies the representational possibilities of basic objects; furthermore, sketches and notes can be integrated into the score for the definition of the final result. Scores can also be generated partially or entirely by script, thus adding a further level of complexity. Several examples are contained into IanniX software package, including the score of *Recurrences* (2011) by Thierry Coduys and an excerpt from *Metastaseis* (1953-54) by Iannis Xenakis.

The strong relation between sound and visual content that emerges through the use of IanniX has been often a stimulus to reveal the score as an integral part of the work. In this sense, IanniX has been used in audiovisual works for controlling audiovisual parameters and showing their representation to the public, even to facilitate formal intelligibility (cf. [Showcase | IanniX](https://www.iannix.org/en/projects/)). 

Specific usages of IanniX include the control of sound spatialization, both for the definition of virtual sound trajectories and the routing of audio signals in complex sound projection systems. Moreover, IanniX has been employed in sonification processes.

1.2 Classification of IanniX scores
-----------------------------------

In relation to their functionality and to the interaction mode with third-party software and hardware, different score types have been recognized:

* **control score**; it is autonomous, reproducible, and determinist; similarly to the operation of UPIC, once the score has been set, the sequencer produces a data output for controlling further processes (e.g. sound synthesis, sampling, and spatialization);
* **reactive score**; it reacts to external stimuli without generating any output; primary purposes are visualization and graphical representation of functions or data received from software environments and devices (e.g. the deformation of a curve expressed by a parametric equation or a 3D path detected by a motion capture device);
* **generative score**; it is produced by algorithms written in [JavaScript](https://developer.mozilla.org/en-US/docs/Web/JavaScript) and can evolve either in a predetermined way or not; therefore, the script generates the score as output (cf. Chap. 2.2.1);
* **interactive score**; based on human-computer interaction or software interaction, it involves the cooperation between various entities; in this context, IanniX may act as a mapping device but also introduces compositional and temporal dimensions; a bi-directional data flow is involved;
* **recursive score**; IanniX can control itself, that is to say the output related to an object can be received as input command for controlling either the sequencer or another object; in some cases, this may imply feedback or deformation of the score as a function of time (cf. Chap. 5.4.1).
---

Copyright (C) 2016-2017 - Julian Scordato (original author)  
Adapted and maintained (C) 2026 - Zhengchao Ding  
Licensed under [Creative Commons Attribution-ShareAlike 4.0 International](https://creativecommons.org/licenses/by-sa/4.0/)
