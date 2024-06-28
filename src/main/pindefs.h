// Pin definitions for IO
//-----------------------------------

#ifndef PINDEFS_H_INCLUDED
#define PINDEFS_H_INCLUDED

// Pin defs

#define PIN_SDCARD_CMD 15
#define PIN_SDCARD_CLK 14
#define PIN_SDCARD_DTA 2

#define PIN_PHY_MDC 23
#define PIN_PHY_MDIO 18
#define PIN_PHY_RST -1
#define PIN_PHY_POWER 12

#define PIN_LED_A 3 
#define PIN_LED_B 4 
#define PIN_LED_C 5 

#define PIN_BUTTON_1 13
#define PIN_BUTTON_2 16
#define PIN_BUTTON_3 32
#define PIN_BUTTON_4 33
#define PIN_BUTTON_5 34
#define PIN_BUTTON_6 35

#define PIN_BUTTON_MASK (1ULL << PIN_BUTTON_1) | (1ULL << PIN_BUTTON_2) | (1ULL << PIN_BUTTON_3) | (1ULL << PIN_BUTTON_4) | (1ULL << PIN_BUTTON_5) | (1ULL << PIN_BUTTON_6)

#define PIN_LED_MASK (1ULL << PIN_LED_A) | (1ULL << PIN_LED_B) | (1ULL << PIN_LED_C)  

#endif