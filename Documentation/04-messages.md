IanniX operation is based on the reception of commands for the creation and the management of score data as well as on the transmission of score-related messages for controlling a third-party device.

In the following chapters, IanniX variables and commands with their proper syntax will be listed with the aim of constituting a user reference on the type of data that can be extracted from scores and the actions that IanniX can carry out.

4.1 Sent messages
-----------------

As stated in the previous chapters, IanniX cursors and triggers are responsible for sending messages to other devices. The amount and the type of transmitted messages should be customized in each case by the user according to needs, for avoiding the transmission of useless data. In addition to this, general messages related to *Transport* (cf. Chap. 2.1.3) are sent directly by the sequencer.

In IanniX scripting language, every message is defined by a protocol and a possible address, followed by a series of separate variables or custom values (cf. Fig. 6). In general:
```
<protocol>://<address (if any)> <variable or value 1> <variable or value 2> <variable or value n>
```
For example:
```
osc://ip_out:port_out/cursor cursor_id cursor_xPos cursor_yPos cursor_zPos 0.1 abc
```
Seven communication protocols are supported for data transmission. In a message, they are declared as follows:

* `osc` - OpenSoundControl message (cf. Chap. 5.1.1);
* `direct` - IanniX recursive/loopback message (cf. Chap. 5.4.1);
* `midi` - MIDI message (cf. Chap. 5.2);
* `serial` - ASCII string through serial port connectivity (cf. Chap. 5.3);
* `http` - HTTP request to a web page or service (cf. Chap. 5.1.4);
* `udp` - Raw UDP message (cf. Chap. 5.1.2);
* `tcp` - XML over TCP message (cf. Chap. 5.1.3).

In addition, JavaScript code can be added to messages inside braces, in order to perform a script during sending (cf. Chap. 5.4.1).

### 4.1.1 Cursor-related variables

Cursor-related variables can be combined in a functional way to return the temporal evolution of values and continuous magnitudes (e.g. glissando, speed, and spatial trajectory). Overall, they are: 

* `cursor_id` - ID of the running cursor;
* `cursor_group_id` - group name of the running cursor;
* `cursor_label` - label of the running cursor;
* `cursor_xPos` - X coordinate of the running cursor;
* `cursor_yPos` - Y coordinate of the running cursor;
* `cursor_zPos` - Z coordinate of the running cursor;
* `cursor_value_x` - mapped X coordinate of the running cursor;
* `cursor_value_y` - mapped Y coordinate of the running cursor;
* `cursor_value_z` - mapped Z coordinate of the running cursor;
* `cursor_time` - progression of the cursor on support curve [s];
* `cursor_time_percent` - progression of the cursor from 0. to 1.;
* `cursor_angle` - incidence angle of the running cursor;
* `cursor_xPos_delta` - X coordinate variation;
* `cursor_yPos_delta` - Y coordinate variation;
* `cursor_zPos_delta` - Z coordinate variation;
* `cursor_value_x_delta` - mapped X coordinate variation;
* `cursor_value_y_delta`  - mapped Y coordinate variation;
* `cursor_value_z_delta` - mapped Z coordinate variation;
* `cursor_time_delta` - cursor time variation [s];
* `cursor_time_percent_delta` - cursor time variation from 0. to 1.;
* `cursor_angle_delta` - incidence angle variation;
* `cursor_nb_loop` - loop counter on support curve;
* `cursor_message_ID`  - message counter for cursors;
* `curve_ID` - ID of the support curve;
* `curve_group_id` - group name of the support curve;
* `curve_label` - label of the support curve;
* `curve_xPos` - X coordinate of the support curve;
* `curve_yPos` - Y coordinate of the support curve;
* `curve_zPos` - Z coordinate of the support curve;
* `collision_curve_ID` - ID of the collided curve;
* `collision_curve_group_id` - group name of the collided curve;
* `collision_curve_label` - label of the collided curve;
* `collision_curve_xPos` - X coordinate of the collided curve;
* `collision_curve_yPos` - Y coordinate of the collided curve;
* `collision_curve_zPos` - Z coordinate of the collided curve;
* `collision_xPos` - X coordinate of the collision with a curve;
* `collision_yPos` - Y coordinate of the collision with a curve;
* `collision_zPos` - Z coordinate of the collision with a curve;
* `collision_value_x` - mapped X coordinate of the collision;
* `collision_value_y` - mapped Y coordinate of the collision;
* `collision_value_z` - mapped Z coordinate of the collision;
* `collision_distance` - distance between cursor and collision.

### 4.1.2 Trigger-related variables

Trigger-related variables are commonly used in a message to start events on third-party devices and to control discrete data (e.g. loading a preset, or sending MIDI note and velocity). These are:

* `trigger_id` - ID of the trigger;
* `trigger_group_id` - group name of the trigger;
* `trigger_label` - label of the trigger;
* `trigger_xPos` - X coordinate of the trigger;
* `trigger_yPos` - Y coordinate of the trigger;
* `trigger_zPos` - Z coordinate of the trigger;
* `trigger_value_x` - cursor-mapped X coordinate of the trigger;
* `trigger_value_y` - cursor-mapped Y coordinate of the trigger;
* `trigger_value_z` - cursor-mapped Z coordinate of the trigger;
* `trigger_value` - trigger status (0 or 127) according to duration;
* `trigger_duration` - trigger duration [s];
* `trigger_distance` - distance between trigger and cursor;
* `trigger_side` - direction in which the trigger is hit (0 or 1);
* `trigger_message_ID` - message counter for triggers.

Triggers can also report a variable related to cursors and vice versa. But while the former send only one message when hit, running cursors keep sending messages over time and might return unwanted trigger values.

### 4.1.3 *Transport* variables

*Transport* variables are useful for synchronization purposes during the communication with other software (e.g. programming environments and sequencers):

* `timetag` - OSC time tag (cf. Chap. 5.1.1);
* `status` - global playback status (play, stop, or fast rewind);
* `global_time` - elapsed time [s];
* `global_time_verbose` - timecode (mmm:ss:fff).

4.2 Received commands
---------------------

IanniX command messages are received from compatible network protocols as well as from recursive interface to control the whole operation of the sequencer and perform actions on IanniX scores. Likewise, they contribute to the scripting language for the creation of score files editable in the *Script editor* (cf. Chap. 2.2.1).

### 4.2.1 Score instances and viewport

* `load <filename>` - load a score file
* `clear` - erase the content of the current score 
* `registerTexture <name> <position> <filename>` - import an external image into the score
* `registerColor <name> <red> <green> <blue> <alpha>` - initialize a color name from RGBA code
* `registerColorHue <name> <hue> <saturation> <value> <alpha>` - initialize a color name from HSVA code
* `sendMessage <message>` - send a message in IanniX format (cf. Chap. 4.1)
* `log <text>` - log information to IanniX message log
* `mouse <x> <y>` - set mouse cursor position in the viewport
* `center <x> <y> 0` - move the camera position in the viewport
* `rotate <angle x> <angle y> <angle z>` - rotate the camera position in the viewport
* `zoom <value>` - change the current zoom in the viewport

### 4.2.2 Object instances and identification

* `add <trigger/curve/cursor> <ID/auto>` - add a IanniX object to the score and sets its ID
* `remove <ID>` - remove an object from the score
* `setId <old ID> <new ID>` - change the object ID
* `setGroup <target> <name>` - set the group for one or more objects (cf. Chap. 2.2.1)
* `setLabel <target> <name>` - set the label for one or more objects
* `setActive <target> <0/1>` - set the activation status for one or more objects
* `setSolo <target> <0/1>` - set the solo mode for one or more objects
* `setMute <target> <0/1>` - set the mute mode for one or more objects

### 4.2.3  Object positioning

* `setPos <target> <x> <y> <z>` - set the absolute position of an object
* `setPosX <target> <x>` - set the X coordinate of an object
* `setPosY <target> <y>` - set the Y coordinate of an object
* `setPosZ <target> <z>` - set the Z coordinate of an object
* `setTranslate <target> <∆x> <∆y> <∆z>` - shift the position of an object
* `setTime <target> <time [s]>` - set the cursor position in relation to its support curve
* `setTimePercent <target> <time from 0. to 1.>` - set the cursor position in relation to its support curve
* `setCurve <target> <curve ID/lastCurve>` - link a cursor with a support curve
* `setPointAt <target> <index> <x> <y> <z>` - set the point position on a straight curve (straight)
* `setSmoothPointAt <target> <index> <x> <y> <z>` - set the point position on a curve (smooth)
* `setPointXAt <target> <index> <x>` - set the X coordinate of a point on the curve
* `setPointYAt <target> <index> <y>` - set the Y coordinate of a point on the curve
* `setPointZAt <target> <index> <z>` - set the Y coordinate of a point on the curve
* `removePointAt <target> <index>` - remove a point from the curve
* `translatePoint <target> <index> <∆x> <∆y> <∆z>` - shift the position of a point on the curve
* `translatePoints <target> <∆x> <∆y> <∆z>` - shift the position of all points on the curve
* `shiftPoints <target> <index> <-1/1>` - shift points of a curve in a direction
* `setResize <target> <width> <height>` - resize a curve according to width and height
* `setResizeF <target> <scale factor>` - resize a curve according to a scale factor
* `displayCurveEditor <target> 1` - show the point editor for a curve
* `displayCurveResample <target> 1` - show the resampling tool for a curve
* `setPointsEllipse <target> <width> <height>` - set a circular curve according to its size
* `setEquation <target> cartesian <x eq.>,<y eq.>,<z eq.> ` - set a parametric curve according to cartesian coordinates
* `setEquation <target> polar <r eq.>,<φ eq.>,<θ eq.>` - set a parametric curve according to polar coordinates
* `setEquationParam <target> <parameter name> <value>` - set a parameter value in the curve equation
* `setEquationNbPoints <target> <number of points>` - set the number of points to calculate the curve equation
* `setPointsTxt <target> <scale factor> <font> <text>` - set a curve from text characters
* `setPointsPath <target> <scale factor> <SVG path>` - set a curve from [SVG path](https://www.w3schools.com/graphics/svg_path.asp) data
* `setPointsLines <target> <scale factor> <points (x,y)>` - set a straight curve from [SVG polyline](https://www.w3schools.com/graphics/svg_polyline.asp)

### 4.2.4  Object appearance

* `setSize <target> <thickness/size>` - set the thickness or size of an object
* `setWidth <target> <value>` - set the cursor width
* `setDepth <target> <value>` - set the cursor depth
* `setColor <target> <name/RGBA code>` - set the color of an object using RGB color space
* `setColorActive <target> <name/RGBA code>` - set the color of an active object using RGB color space
* `setColorInactive <target> <name/RGBA code>` - set the color of an inactive object using RGB color space
* `setColorMultiply <target> <name/RGBA code>` - set the color multiplication for an object using RGB color space
* `setColorHue <target> <name/HSVA code>` - set the color of an object using HSV color space
* `setColorActiveHue <target> <name/HSVA code>` - set the color of an active object using HSV color space
* `setColorInactiveHue <target> <name/HSVA code>` - set the color of an inactive object using HSV color space
* `setColorMultiplyHue <target> <name/HSVA code>` - set the color multiplication for an object using HSV color space
* `setTexture <target> <name>` - set a preloaded texture for an object
* `setTextureActive <target> <name>` - set a preloaded texture for an active object
* `setTextureInactive <target> <name>` - set a preloaded texture for an inactive object

### 4.2.5  Object behavior

* `setSpeed <target> <factor>` - set the cursor speed as a factor related to Transport
* `setSpeed <target> auto <path end [s]>` - set the duration of entire cursor path on support curve
* `setSpeedF <target> <factor>` - apply a multiplication factor on cursor speed (master cursor speed)
* `setOffset <target> <initial time [s]> <start [s]> <end [s]/end>` - set the offset parameters for a cursor
* `setPattern <target> <easing> 0 <loop pattern>` - set loop pattern and cursor acceleration
* `setBoundsSource <target> <range x> <range y> <range z>` - set a custom mapping area for a cursor
* `setBoundsSourceMode <target> <0/1/2/3>` - set the coordinate mapping mode for a cursor
* `setBoundsTarget <target> <range x> <range y> <range z>` - set the range of output values for a cursor
* `setFire <target> <none/group/all>` - set the trigger firing mode for a cursor
* `setMessage <target> <interval [ms]>, <msg_1>, <msg_2>` - set one or more messages to be transmitted by an object
* `setMessageInterval <target> <interval [ms]>` - set the message transmission rate for an object
* `trig <target>` - force an object to send its preset messages
* `setTriggerOff <target> <duration [s]>` - set the trigger duration
* `setElasticity <target> <factor>` - set the elasticity factor of a curve

### 4.2.6 Sequencing

* `play <speed factor (optional)>` - start the score playback
* `stop` - stop the score playback
* `fastrewind` - reset the score to initial position
* `goto <time [s]>` - go to a specific timecode
* `speed <factor>` - change the global speed factor in relation to grid
---

Copyright (C) 2016-2017 - Julian Scordato (original author)  
Adapted and maintained (C) 2026 - Zhengchao Ding  
Licensed under [Creative Commons Attribution-ShareAlike 4.0 International](https://creativecommons.org/licenses/by-sa/4.0/)
