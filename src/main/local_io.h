// Local IO: Buttons, LEDs, IR relay IO, warning lights
//-----------------------------------

#ifndef LOCAL_IO_H_INCLUDED
#define LOCAL_IO_H_INCLUDED

// Define structures that can be used for state buffers and debouncing of IO
struct Input_Buffer_Struct
{
    uint8_t button_panel; // 0 is unpressed 1-6 pressed
};

struct Output_Buffer_Struct
{
    uint8_t led_panel; // 0 is unlit, 1-6 lit
};

// Debounce properties
#define INPUT_DEBOUNCE_LOOP_COUNT 3
#define REFRESH_LOOP_TICKS 10

void setup_local_io(QueueHandle_t *input_queue);

uint8_t get_button_panel_state();
void set_button_led_state(uint8_t value);

#endif