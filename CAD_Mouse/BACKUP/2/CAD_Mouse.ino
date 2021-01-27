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

// Determine if the mouse and keyboard are being used.
int mouse_isAlive = false;
int keyboard_isAlive = false;

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
const int   rolling_step_multiplier = 30;   // Governs mouse sensitivity for rolling. The higher, the faster the roll.
const int   pulse_threshold = 4;            // Governs the net number of pulses in the same direction required before initating mouse movement.
int         pulse_counter = 0;
const int   rotary_off_threshold = 35;
int         rotary_spin_off_counter = 0;    // Counts the number of iterations since the last movement detected by the rotary encoder.
int         remaining_roll = 0;             // Counts the amount of roll left to be executed.
int         prev_DT;                        // prev_DT and prev_CLK pins are used to determine the direction of rotation
int         prev_CLK;                       // when changes in the states of the DT and CLK pins are detected.
int         rotary_switch_off_counter = 0;  // Counts the iterations since the last HIGH signal from the rotary_switch pin. If this is
#define     ROLLING true                    // greater than switch_threshold, then switch the rotary encoder mode between rolling and
#define     ZOOMING false                   // zooming.
#define     CLOCKWISE -1
#define     COUNTERCLOCKWISE 1
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

int get_rotary_movement() {
  // Returns the CLOCKWISE, COUNTERCLOCKWISE, or 0 depending on if there is rotation detected by the rotary encoder.
  int curr_DT = digitalRead(rotary_DT);
  int curr_CLK = digitalRead(rotary_CLK);
  
  int return_direction = 0;
  if (prev_DT != curr_DT) {
    if (prev_DT == HIGH) {
      if (prev_CLK == LOW) {
        return_direction = CLOCKWISE;
      } else {
        return_direction = COUNTERCLOCKWISE;
      }
    } else {
      if (prev_CLK == LOW) {
        return_direction = COUNTERCLOCKWISE;
      } else {
        return_direction = CLOCKWISE;
      }
    }
  } else if (prev_CLK != curr_CLK) {
    if (prev_CLK == HIGH) {
      if (prev_DT == LOW) {
        return_direction = COUNTERCLOCKWISE;
      } else {
        return_direction = CLOCKWISE;
      }
    } else {
      if (prev_DT == LOW) {
        return_direction = CLOCKWISE;
      } else {
        return_direction = COUNTERCLOCKWISE;
      }
    }
  }

  prev_DT = curr_DT;
  prev_CLK = curr_CLK;

  return return_direction;
}

void rotary_move_mouse() {
  // Move the mouse a certain amount depending on remaining roll required.
  int move_x;
  if (abs(remaining_roll) > 127) {
    if (remaining_roll > 0) {
      move_x = 127;
    } else {
      move_x = -127;
    }
  } else {
    move_x = remaining_roll;
  }
  mouse_pos_x += move_x;
  remaining_roll -= move_x;

  Mouse.move(move_x, 0, 0);
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
      keyboard_isAlive = true;
      Keyboard.press(KEY_LEFT_CTRL);
    }
    Mouse.begin();
    mouse_isAlive = true;
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
      keyboard_isAlive = false;
    }
    return_mouse();
    Mouse.end();
    mouse_isAlive = false;
    MMB_pressed = false;

    // Rotary Reset
    prev_DT = digitalRead(rotary_DT);
    prev_CLK = digitalRead(rotary_CLK);
    rotary_spin_off_counter = 0;
  } else {
    // If joystick is not being used to control the mouse, check and update rotary encoder stuff.
    int rotary_signal = get_rotary_movement();
    if (rotary_signal == 0 and remaining_roll == 0) {
      if (rotary_spin_off_counter <= rotary_off_threshold) {
        rotary_spin_off_counter++; // If there is no signal from the knob but it hasn't been long enough since the knob hasn't been touched,
      }                       // keep counting.
      if (rotary_spin_off_counter == rotary_off_threshold) {
        // If there is no signal from the knob and it has been long enough since the knob hasn't been touched, return the mouse and end
        // mouse and keyboard control.
        if (mouse_isAlive) {
          return_mouse();
          if (!Mouse.isPressed(MOUSE_MIDDLE)) {
            Mouse.press(MOUSE_MIDDLE);
          }
          delay(100);
          Mouse.click(MOUSE_MIDDLE); // Get rid of sticky mouse middle.
          Mouse.end();
          mouse_isAlive = false;
        }
        if (keyboard_isAlive) {
          Keyboard.release(KEY_LEFT_ALT);
          Keyboard.end();
          keyboard_isAlive = false;
        }
      }
    } else {
      if (rotary_signal != 0) {
        // If input is being received from the knob, reset spin_off_counter and turn on control of mouse and keyboard to roll screen.
        Keyboard.begin();
        keyboard_isAlive = true;
        Keyboard.press(KEY_LEFT_ALT);
        Mouse.begin();
        mouse_isAlive = true;
        if (!Mouse.isPressed(MOUSE_MIDDLE)) {
          Mouse.press(MOUSE_MIDDLE);
        }
        rotary_spin_off_counter = 0;
      }
      remaining_roll += rolling_step_multiplier * rotary_signal;
      rotary_move_mouse();
    }
  }
  Serial.print("KEYBOARD: ");
  Serial.print(keyboard_isAlive);
  Serial.print("\t\tMOUSE: ");
  Serial.println(mouse_isAlive);

  delay((int)(1000 * 1 / refresh_rate)); // Execute loop at frequency specified by refresh_rate
}
