#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "freertos/queue.h" 

#include "main.h"
#include "local_io.h"
#include "ethernet.h"
#include "storage.h"

// Queue handles input to logic from button panels, messages received on ethernet
// Avoids having to poll inputs from main logic (polling, denbouncing, buffering of buttons etc handled in local_io module)
QueueHandle_t input_event_queue; 

// Holds the various settings
// Note that route info is held in 'physical' 1-40 numbering not the 0-39 form - decrements are applied below when commands are sent
struct Settings_Struct settings;

static const char *TAG = "main";

static void input_logic_task(void)
{
    // Task which responds to button presses on the front panel, ethernet messages
    // Uses a queue to respond when required

    while(1)
    {   
        struct Queued_Input_Message_Struct incoming_msg;
        if (xQueueReceive(input_event_queue, &incoming_msg, (TickType_t) portMAX_DELAY) == pdTRUE)
        {
            // Message recieved from queue
            ESP_LOGI(TAG,"Processing message in input logic, type:%i",incoming_msg.type);

            switch (incoming_msg.type)
            {
            case IN_MSG_TYP_ROUTING:
                // Routing input from button panel - send command to switcher
                ESP_LOGI(TAG,"Sending video routing message");
                uint8_t input = settings.routing_sources[incoming_msg.panel_button];
                uint8_t output = settings.routing_destination;

                // Decrement in/outs by 1 to go from physical 1-40 numbering to zero index 
                send_video_route(input - 1, output - 1);

                break;

            case IN_MSG_TYP_ETHERNET:
                // Incoming text message on ethenet
                ESP_LOGI(TAG,"Processing routing confirm message");
                // Work out if the incoming routing confirm applies to any of our screens
                u_int8_t found_button = 0;

                if ((incoming_msg.output + 1) == settings.routing_destination)
                {
                    // It is our screen
                    for (uint8_t button = 0; button<6; button++)
                    {
                        if ((incoming_msg.input + 1) == settings.routing_sources[button])
                        {
                            // and it's one of our outputs 
                            found_button = button + 1; // Got to convert back from zero index to physical button, because 0 = no LED lit
                            break;
                        } 
                    }
                    set_button_led_state(found_button);  
                }

                break;
            default:
                ESP_LOGW(TAG,"Input message unknown:%i",incoming_msg.type);
                break;
            }
            
        }
    }

}

static void local_test_mode(void)
{
    // Vegas mode - LED and button test only
    while (1)
    {
        // Do some dumb polling of the buttons to light any up that are selected

        set_button_led_state(get_button_panel_state());
        vTaskDelay(5);
    
    }
        
}

void app_main(void)
{   
    // Set up input event queue
    input_event_queue = xQueueCreate (32, sizeof(struct Queued_Input_Message_Struct)); 
    if (input_event_queue == NULL)
    {
        ESP_LOGE(TAG,"Unable to create input event queue, rebooting");
        esp_restart();
    }

    // Retrive settings from SD card 
    settings = get_settings();

    //Set up local buttons, LEDs, relay outputs and warning lights
    setup_local_io(&input_event_queue);

    vTaskDelay(10); // Wait to see if buttons are being held down
    //Check to see if we're heading into 'vegas mode' for testing rather than the proper application
    if (get_button_panel_state() == 1)
    {
        local_test_mode();
        return; 
    }

    // Set up ethernet stack and communication with video router
    setup_ethernet(settings.router_ip, settings.router_port, &input_event_queue);

    xTaskCreate( (TaskFunction_t) input_logic_task, "input_logic_task", 2048, NULL, 5, NULL);
}