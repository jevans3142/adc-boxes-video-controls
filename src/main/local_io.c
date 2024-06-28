// Local IO: Buttons, LEDs
//-----------------------------------

#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "esp_log.h"

#include "main.h"
#include "local_io.h"
#include "pindefs.h"
#include "ethernet.h"

// Logging tag
static const char *TAG = "local_io";

// Input message queue handle pointer - passed in from main module
static QueueHandle_t *input_event_queue_ptr;

// Define internal buffers that hold the 'current' state of the IO expanders

SemaphoreHandle_t output_state_buffer_mutex = NULL; // protects:
uint8_t output_state_buffer_changed_flag = 0;       // 0 unchanged, 1 changed since last output run therefore outputs need refreshed
struct Output_Buffer_Struct output_state_buffer;    // Raw state of outputs

SemaphoreHandle_t input_state_buffer_mutex = NULL; // protects:
struct Input_Buffer_Struct input_state_buffer;     // Raw state of inputs
struct Input_Buffer_Struct input_state_counts;     // Counters for debouncing
struct Input_Buffer_Struct input_debounced_buffer; // Debounced state

// Button array for loop
uint8_t button_pin_array[] = {PIN_BUTTON_1, PIN_BUTTON_2, PIN_BUTTON_3, PIN_BUTTON_4, PIN_BUTTON_5, PIN_BUTTON_6};
        

// Main tasks: output refresh and input debouncing
// =============================================================================

static void refresh_outputs(void)
{
    // Update button LEDs when required

    if (output_state_buffer_mutex == NULL)
    {
        ESP_LOGE(TAG, "Output buffer mutex NULL at at refresh_outputs");
        return;
    }

    if (xSemaphoreTake(output_state_buffer_mutex, (TickType_t)10) == pdTRUE)
    {
        if (output_state_buffer_changed_flag == 0)
        {
            xSemaphoreGive(output_state_buffer_mutex);
            ESP_LOGD(TAG, "refresh outputs not required");
            return;
        }

        output_state_buffer_changed_flag = 0;

        gpio_set_level(PIN_LED_A, (output_state_buffer.led_panel & 1) != 0);
        gpio_set_level(PIN_LED_B, (output_state_buffer.led_panel & 2) != 0);
        gpio_set_level(PIN_LED_C, (output_state_buffer.led_panel & 4) != 0);

        xSemaphoreGive(output_state_buffer_mutex);
        ESP_LOGD(TAG, "Output at refresh outputs:%d", output_state_buffer.led_panel); // TODO - print output
    }
    else
    {
        ESP_LOGW(TAG, "Output buffer mutex timeout at refresh_outputs");
        return;
    }
}

static void button_debounce(uint8_t *raw_input, uint8_t *state, uint8_t *counter, uint8_t message_type)
{
    if (*raw_input == 0)
    {
        *counter = 0;
        if (*state != 0)
        {
            // Button has been pressed and released, send message to main logic
            struct Queued_Input_Message_Struct new_message;
            new_message.type = message_type;
            if (message_type == IN_MSG_TYP_ROUTING)
            {
                new_message.panel_button = *state - 1; // Note change from physical button 1-6 to array index 0-5 for reference to settings struct in main logic
            }

            if (xQueueSend(*input_event_queue_ptr, (void *)&new_message, 0) == pdTRUE)
            {
                ESP_LOGI(TAG, "Sending message from button debounce: %i,%i", new_message.type, new_message.panel_button);
            }
            else
            {
                ESP_LOGW(TAG, "Sending message from button debounce failed due to queue full? - %i,%i", new_message.type, new_message.panel_button);
            }
            *state = 0;
        }
    }
    else
    {
        *counter = *counter + 1;
    }

    if (*counter >= INPUT_DEBOUNCE_LOOP_COUNT)
    {
        *state = *raw_input;
    }
}

static void refresh_inputs(void)
{
    if (input_state_buffer_mutex == NULL)
    {
        ESP_LOGE(TAG, "Input buffer mutex NULL at at refresh_inputs");
        return;
    }

    if (xSemaphoreTake(input_state_buffer_mutex, (TickType_t)10) == pdTRUE)
    {
        // Get which button is pressed - note that for simplicity if multiple buttons are pressed
        // then we just get the higer numbered one 
        
        uint8_t temp_button_state = 0;

        for (uint8_t button = 0; button<6; button++)
        {
            if (gpio_get_level(button_pin_array[button]) == 0) // Buttons pulled low when pressed
            {
                temp_button_state = button + 1;
            }
        }
        
        input_state_buffer.button_panel = temp_button_state;

        // Debounce raw inputs into debounced state for buttons, trigger events if required
        button_debounce(&input_state_buffer.button_panel, &input_debounced_buffer.button_panel, &input_state_counts.button_panel, IN_MSG_TYP_ROUTING);

        xSemaphoreGive(input_state_buffer_mutex);
        ESP_LOGD(TAG, "Input button state at refresh_inputs:%d",input_debounced_buffer.button_panel);
    }
    else
    {
        ESP_LOGW(TAG, "Input buffer mutex timeout at refresh_inputs");
        return;
    }
}

static void input_poll_task(void)
{
    while (1)
    {
        refresh_inputs();
        refresh_outputs();
        vTaskDelay(REFRESH_LOOP_TICKS / portTICK_PERIOD_MS);
    }
}

static uint8_t buffer_single_read(uint8_t *buffer, char *label)
{
    // Generic function for returning a value from the buffer
    if (input_state_buffer_mutex == NULL)
    {
        ESP_LOGW(TAG, "Input buffer read mutex NULL at %s", label);
        return NULL;
    }

    if (xSemaphoreTake(input_state_buffer_mutex, (TickType_t)10) == pdTRUE)
    {
        uint8_t value = *buffer;
        xSemaphoreGive(input_state_buffer_mutex);
        ESP_LOGD(TAG, "Input buffer read at %s:%i", label, value);
        return value;
    }
    else
    {
        ESP_LOGW(TAG, "Input buffer read mutex timeout at %s", label);
        return NULL;
    }
}

static void buffer_single_write(uint8_t *buffer, uint8_t value, char *label)
{
    // Generic function for writing a value to the buffer
    if (output_state_buffer_mutex == NULL)
    {
        ESP_LOGW(TAG, "Output buffer write mutex NULL at %s", label);
        return;
    }

    if (xSemaphoreTake(output_state_buffer_mutex, (TickType_t)10) == pdTRUE)
    {
        if (*buffer != value)
        {
            *buffer = value;
            output_state_buffer_changed_flag = 1;
        }
        xSemaphoreGive(output_state_buffer_mutex);
        ESP_LOGD(TAG, "Output buffer write at %s:%i", label, value);
        return;
    }
    else
    {
        ESP_LOGW(TAG, "Output buffer write mutex timeout at %s", label);
        return;
    }
}

// Setup and zero outputs at poweron
// =============================================================================

void setup_local_io(QueueHandle_t *input_queue)
{
    // Set up mutexes for local buffer of IO state
    output_state_buffer_mutex = xSemaphoreCreateMutex();
    input_state_buffer_mutex = xSemaphoreCreateMutex();

    // Set up input pins
    gpio_config_t i_conf;
    i_conf.intr_type = GPIO_INTR_DISABLE;
    i_conf.mode = GPIO_MODE_INPUT;
    i_conf.pin_bit_mask = (PIN_BUTTON_MASK);
    i_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    i_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    gpio_config(&i_conf);

    // Set up output pins
    gpio_config_t o_conf;
    o_conf.intr_type = GPIO_INTR_DISABLE;
    o_conf.mode = GPIO_MODE_OUTPUT;
    o_conf.pin_bit_mask = (PIN_LED_MASK);
    o_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    o_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&o_conf);

    // Set up local pointers to the event queue in the main logic
    input_event_queue_ptr = input_queue;

    xTaskCreate((TaskFunction_t)input_poll_task, "input_poll_task", 2048, NULL, 5, NULL);
}

// Main button panels (routing buttons)
// =============================================================================

uint8_t get_button_panel_state()
{
    // Returns button panel state
    char s[17];
    snprintf(s, 17, "Button panel");
    return buffer_single_read(&input_debounced_buffer.button_panel, s);
}

void set_button_led_state(uint8_t value)
{
    // Sets the state of the button panel LEDs
    char s[22];
    snprintf(s, 22, "Button panel LEDs");
    buffer_single_write(&output_state_buffer.led_panel, value, s);
}
