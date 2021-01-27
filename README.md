# CAD Mouse Project

## Introduction

I took on this project because the middle mouse button on my mouse doesn't work well and if you're familiar with Solidworks and many other CAD software, the middle mouse button is often used to rotate, pan, and roll the model you are viewing. Manipulating the view of a CAD model without a functioning middle mouse button can be quite frustrating so I took on the task of developing a separate controller I could use specifically for performing these functions.

![project showcase](https://github.com/BrandonBNguyen/CADMouse/blob/master/Images/showcase.gif)

## Part Selection

### Sensor Selection

I initially started by thinking about what sensors I would want to use to interpret commands to rotate, pan, roll, and zoom the view model. 

For the actions of rotating and panning the view model, I selected a simple joystick controller I had leftover from an Arduino kit because rotating and panning are actions that require both horizontal and vertical inputs from the mouse.

![Arduino Joystick Module](https://github.com/BrandonBNguyen/CADMouse/blob/master/Images/ArduinoJoystickModule.jpg)

For the actions of rolling and zooming the view model, I selected the KY-040 rotary encoder as these actions only require mouse movement along a single axis.

![Arduino Joystick Module](https://github.com/BrandonBNguyen/CADMouse/blob/master/Images/KY-040Encoder.jpg)

### Board Selection

From preliminary testing with an Arduino Uno (which was unable to stimulate keyboard and mouse inputs to a computer), I knew I needed to select a microcontroller that would be able to appear as a native mouse and keyboard by implementing the `Mouse.h` and `Keyboard.h` libraries. As a result, I selected the Arduino Micro, which is a small Arduino board capable of implementing these libraries.

![Arduino Micro Microcontroller](https://github.com/BrandonBNguyen/CADMouse/blob/master/Images/ArduinoMicro.jpg)

## CAD Design

I started the design by importing existing CAD models of the electronic components into Solidworks and drawing  a rudimentary sketch of their placements relative to each other, being sure to take into account the clearances required of the Dupont wire connections to the sensor and board pins.

![Preliminary placement sketch](https://github.com/BrandonBNguyen/CADMouse/blob/master/Images/PreliminarySketch.PNG)

After fully defining and determining their placements, I modeled a case that would hold all the components in place via fasteners and provide an opening at the end for the micro USB port on the Arduino Micro. 

![Case Model](https://github.com/BrandonBNguyen/CADMouse/blob/master/Images/BottomCase.PNG)

After the case was designed, I modeled the lids that would be fastened to the case and enclose the wiring.

![Enclosed Case Model](https://github.com/BrandonBNguyen/CADMouse/blob/master/Images/CaseWithLid.PNG)

## Manufacturing

### 3D Printing

In order to prevent wasted plastic, I started by printing specifically the sections  of the case that interfaced with the electronic components to ensure proper fitting. By performing this step, I was able to find several instances where the components did not fit due to minor deviations between the CAD models of the components and their actual dimensions. Taking into account these differences, I was able to modify the CAD such that the components fit properly.

Below is a screenshot of the various iterations of the test prints used to ensure proper fitting of components to the case.

![Test Prints](https://github.com/BrandonBNguyen/CADMouse/blob/master/Images/TestPrints.jpeg)

### Wiring

The wiring between the sensors and the microcontroller was done using Dupont connectors. The connectors between the joystick and the Arduino Micro were shortened by cutting out segments of the connector, stripping the ends, soldering them together, and insulating them with electrical tape. 

![Internal Wiring](https://github.com/BrandonBNguyen/CADMouse/blob/master/Images/InternalWiring.jpeg)

## User Guide

When the mouse is intially connected and booted up, the joystick controller is set such that moving it will cause the model to be rotated and the knob controller is set such that rotating it will cause the model to be rolled. 

Pressing down on the joystick and activating its switch will cause it to enter a panning mode such that inputs from the joystick cause the model to be panned instead of rotated. Pressing the joystick again will put it back in rotating mode.

Pressing down on the knob and activating its switch will cause it to enter a zooming mode such that inputs from the knob will cause the view window to zoom in and out of the model. Pressing the knob again will return the knob to rolling mode.

## Code Implementation

### Technical Challenges

The code run by the CAD Mouse is written and organized in `CAD_Mouse.ino`. The major technical problem I encountered while getting the mouse to manipulate the model smoothly was the mouse cursor colliding with the edge of the screen, preventing the view model from continuing to be moved in any way. The solution I came up with for this problem was to have the controller record the movements of the mouse from its initial position when the mouse was being used. During use, if the mouse cursor strayed too far from its initial position, the cursor would be quickly moved back to its initial position before continuing. This allowed the rotating and panning of view model to be fairly smooth when moving continuously.

### Organization

#### setup()

The initial `setup()` function is used to set up the input pins to read from the sensors and record the initial position of the rotary encoder.

#### loop()

The main `loop()`function reads data from the sensors and stimulates mouse and keyboard inputs to manipulate the CAD view. It performs the following tasks in sequence.

 1. Read the position of the joystick.
 2. Determine if the joystick switch has been pressed and switch between rotating and panning mode, accordingly.
 3. Determine if the knob switch has been pressed and switch between rolling and zooming mode, accordingly.
 4. If the joystick leaves the center position, begin stimulating mouse and keyboard inputs to move the CAD view model. If the joystick was already outside the center position, continue stimulating mouse movements to move the model. If the joystick reenters the center position, stop stimulating mouse and keyboard inputs.
 5. If the joystick isn't being used, read input from the rotary encoder. If the rotary encoder starts moving, begin stimulating mouse and keyboard inputs to move the model. Continue moving the model while rotary encoder  is still recording movement. If the rotary encoder has not recorded movement for a specified time, stop stimulating mouse and keyboard inputs.

Additionally, the code is factored in to several functions to help facilitate the tasks carried out by `loop()` and prevent repetitive code.

## Skills Demonstrated

 - Demonstrated knowledge of C++ and applied it to use a microcontroller to read sensor data and control a system based on sensor inputs.
 - Demonstrated ability to write organized, well-commented code and write project documentation.
 - Demonstrated strong computer-aided design skills and good workflow by constraining existing components and developing custom parts around those in an assembly.
 - Demonstrated ability to perform rapid prototyping with the use of FDM 3D printing to test component fitting.
