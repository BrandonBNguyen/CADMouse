#include <Mouse.h>
#include <Keyboard.h>
#include <math.h>
#include <StackArray.h>

// Pin Definitions
#define joy_VRy A1
#define joy_VRx A2
#define joy_switch 8

#define rotary_switch 3
#define rotary_DT 4
#define rotary_CLK 5

// Defines borders of region where mouse is allowed to be within
const int mouse_x_range = 3000; // Once mouse exits range of -1500 to 1500 in x, return the mouse to original position.
const int mouse_y_range = 900;  // Once mouse exits range of -450 to 450 in y, return the mouse to original position.

// Constants and variables regarding joystick control
const int   activationRadius = 25;           // How far from the center does the switch need to be to actually activate MMB.
const float rotation_step_multiplier = 0.04; // Governs mouse sensitivity for rotation. The higher, the faster the rotation.
const float panning_step_multiplier = 0.04;  // Governs mouse sensitivity for panning. The higher, the faster the pan.
int         stick_x;                         // Variables for input from joystick sensor.
int         stick_y;
const int   switch_threshold = 8;
int         joy_switch_off_counter = 0;      // Counts the iterations since last HIGH signal from joy_switch pin. If this is greater than
#define     ROTATING true                    // switch_threshold, then switch the joystick mode between rotating and panning.
#define     PANNING false
bool        joystick_mode = ROTATING;        // Joystick should initially be rotating the model.

// Constants and variables regarding rotary encoder control
const int   rolling_step_multiplier = 60;   // Governs mouse sensitivity for rolling. The higher, the faster the roll.
const int   pulse_threshold = 4;            // Governs the net number of pulses in the same direction required before initating mouse movement.
int         pulse_counter = 0;
int         prev_DT;                        // prev_DT and prev_CLK pins are used to determine the direction of rotation
int         prev_CLK;                       // when changes in the states of the DT and CLK pins are detected.
int         rotary_switch_off_counter = 0;  // Counts the iterations since the last HIGH signal from the rotary_switch pin. If this is
#define     ROLLING true                    // greater than switch_threshold, then switch the rotary encoder mode between rolling and
#define     ZOOMING false                   // zooming.
#define     CLOCKWISE 1
#define     COUNTERCLOCKWISE -1
bool        rotary_mode = ROLLING;          // Rotary encoder should initially be rolling the model.

// Mouse variables
int  mouse_pos_x = 0;     // Variables for tracking position of cursor relative to original position (measured in pixels).
int  mouse_pos_y = 0;
bool MMB_pressed = false; // Used to track if MMB is being held down during a movement (such as in rotating, panning, or rolling).

// Refresh Rate
const int refresh_rate = 144;

void setup() {
  pinMode(joy_switch, INPUT);
  
  pinMode(rotary_switch, INPUT);
  pinMode(rotary_DT, INPUT);
  pinMode(rotary_CLK, INPUT);

  prev_DT = digitalRead(rotary_DT);
  prev_CLK = digitalRead(rotary_CLK);

  Serial.begin(9600);
}

bool outside_activationRadius(int stick_x, int stick_y) {
  // Returns whether the stick has been moved far enough to trigger mouse control.
  int center_x = stick_x - 511; // Coordinates with range of -511 to 512 with 0 being center.
  int center_y = stick_y - 511;
  if (sqrt(square(center_x) + square(center_y)) > activationRadius) {
    return true;
  } else {
    return false;
  }
}

void joystick_move_mouse(int stick_x, int stick_y) {
  // Move the mouse a certain amount depending on input from the joystick.
  float multiplier;
  if (joystick_mode == ROTATING) {
    multiplier = rotation_step_multiplier;
  } else {
    multiplier = panning_step_multiplier;
  }
  int move_x = floor(stick_x - 512) * 256 / 1024 * multiplier;  // Map the stick analog input (ranging from 0 to 1023) to signed
  int move_y = floor(stick_y - 512) * -256 / 1024 * multiplier; // char (ranging from -128 to 127) for use with Mouse.move().
  Mouse.move(move_x, move_y, 0);
  mouse_pos_x += move_x;
  mouse_pos_y += move_y;
}

void return_mouse() {
  // Returns mouse to original position before usage.
  if (Mouse.isPressed(MOUSE_MIDDLE)) {
    Mouse.release(MOUSE_MIDDLE);
  }

  int move_x; // Break movement into largest chunks possible to achieve
  int move_y; // return movement in least amount of movements as possible.
  while (mouse_pos_x != 0 || mouse_pos_y != 0) {
    if (abs(mouse_pos_x) > 127) {
      if (mouse_pos_x > 0) {move_x = -127;}
      else {move_x = 127;}
    } else {
      move_x = -mouse_pos_x;
    }
    if (abs(mouse_pos_y) > 127) {
      if (mouse_pos_y > 0) {move_y = -127;}
      else {move_y = 127;}
    } else {
      move_y = -mouse_pos_y;
    }
    // Update mouse position while executing mouse movement.
    mouse_pos_x += move_x;
    mouse_pos_y += move_y;

    Mouse.move(move_x, move_y, 0);
  }
}

void rotary_move_mouse(int rotation_direction) {
  // Move the mouse a certain amount depending on input from the rotary encoder.
  pulse_counter += rotation_direction;
  if (abs(pulse_counter) == pulse_threshold) {
    pulse_counter = 0;
    Mouse.begin();
    Serial.println("MOUSE BEGIN");
    if (rotary_mode == ROLLING) {
      Keyboard.begin();
      Keyboard.press(KEY_LEFT_ALT);
      if (!Mouse.isPressed(MOUSE_MIDDLE)) {
        Mouse.press(MOUSE_MIDDLE);
      }
      int move_x = -rotation_direction * rolling_step_multiplier;
      mouse_pos_x += move_x;
      Mouse.move(move_x, 0, 0);
      Serial.println("MOUSE MOVE");
      Mouse.release(MOUSE_MIDDLE);
      Keyboard.release(KEY_LEFT_ALT);
      Keyboard.end();
      return_mouse();
    } else {
      Mouse.move(0, 0, -rotation_direction);
    }
    Mouse.end();
    Serial.println("MOUSE END");
  }
  Serial.print("ROTARY PULSE COUNTER: ");
  Serial.println(pulse_counter);
}

void loop() {
  // Orient such that the +y direction is towards the output prongs and the +x direction is to the right of that.
  stick_x = 1023 - analogRead(joy_VRy);
  stick_y = 1023 - analogRead(joy_VRx);

  // Determine if the joystick switch has been triggered and switch mode between ROTATING or ROTATING, accordingly.
  joy_switch_off_counter++;
  int switch_signal = digitalRead(joy_switch);
  if (switch_signal == 1) {
    joy_switch_off_counter = 0;
  }
  if (joy_switch_off_counter == switch_threshold) {
    if (joystick_mode == ROTATING) {
      joystick_mode = PANNING;
    } else {
      joystick_mode = ROTATING;
    }
  }

  // Determine if the rotary encoder switch has been triggered and switch mode between ROLLING and ZOOMING, accordingly.
  rotary_switch_off_counter++;
  int rotary_switch_signal = digitalRead(rotary_switch);
  if (rotary_switch_signal == HIGH) {
    rotary_switch_off_counter = 0;
  }
  if (rotary_switch_off_counter == switch_threshold) {
    pulse_counter = 0;
    if (rotary_mode == ROLLING) {
      rotary_mode = ZOOMING;
    } else {
      rotary_mode = ROLLING;
    }
  }

  // When the stick initially leaves the activation radius, hold down the middle mouse button and move the cursor.
  if (!MMB_pressed && outside_activationRadius(stick_x, stick_y)) {
    if (joystick_mode == PANNING) {
      Keyboard.begin();
      Keyboard.press(KEY_LEFT_CTRL);
    }
    Mouse.begin();
    if (!Mouse.isPressed(MOUSE_MIDDLE)) {
      Mouse.press(MOUSE_MIDDLE);
    }
    joystick_move_mouse(stick_x, stick_y);
    MMB_pressed = true;
  } else 

  // While the mouse continues to move outside the activation radius, continue moving the cursor.
  if (MMB_pressed && outside_activationRadius(stick_x, stick_y)) {
    joystick_move_mouse(stick_x, stick_y);

    // If the mouse has exceeded the specified range, return the mouse.
    if (abs(mouse_pos_x) >= mouse_x_range / 2 || abs(mouse_pos_y) >= mouse_y_range / 2) {
      return_mouse();
    }
    // Since return_mouse() returns the mouse with the MMB unclicked, reclick it.
    if (!Mouse.isPressed(MOUSE_MIDDLE)) {
      Mouse.press(MOUSE_MIDDLE);
    }
  } else 

  // When the stick finally reenters the activation radius, release the middle mouse button and return the cursor to its original position.
  if (MMB_pressed && !outside_activationRadius(stick_x, stick_y)) {
    if (joystick_mode == PANNING) {
      Keyboard.release(KEY_LEFT_CTRL);
      Keyboard.end();
    }
    return_mouse();
    Mouse.end();
    MMB_pressed = false;

    prev_DT = digitalRead(rotary_DT);
    prev_CLK = digitalRead(rotary_CLK);
  } else {
    // If joystick is not being used to control the mouse, check and update rotary encoder stuff.
    int curr_DT = digitalRead(rotary_DT);
    int curr_CLK = digitalRead(rotary_CLK);
  
    if (prev_DT != curr_DT) {
      if (prev_DT == HIGH) {
        if (prev_CLK == LOW) {
          rotary_move_mouse(CLOCKWISE);
        } else {
          rotary_move_mouse(COUNTERCLOCKWISE);
        }
      } else {
        if (prev_CLK == LOW) {
          rotary_move_mouse(COUNTERCLOCKWISE);
        } else {
          rotary_move_mouse(CLOCKWISE);
        }
      }
    } else if (prev_CLK != curr_CLK) {
      if (prev_CLK == HIGH) {
        if (prev_DT == LOW) {
          rotary_move_mouse(COUNTERCLOCKWISE);
        } else {
          rotary_move_mouse(CLOCKWISE);
        }
      } else {
        if (prev_DT == LOW) {
          rotary_move_mouse(CLOCKWISE);
        } else {
          rotary_move_mouse(COUNTERCLOCKWISE);
        }
      }
    } // Do nothing if the previous and current values for both have not changed.
  
    prev_DT = curr_DT;
    prev_CLK = curr_CLK;
  }

  delay((int)(1000 * 1 / refresh_rate)); // Execute loop at frequency specified by refresh_rate
}
