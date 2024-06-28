// Storage module
//-----------------------------------

#ifndef STORAGE_H_INCLUDED
#define STORAGE_H_INCLUDED

struct Settings_Struct {
    uint8_t routing_sources[6]; // Sources labeled 1-40 for each button
    uint8_t routing_destination; // Destination labeled 1-40
    uint32_t router_ip;
    uint32_t router_port;
};

#define MOUNT_POINT "/sdcard"
#define CFG_FILE "/config.txt"
#define MAX_CHAR_SIZE 256

struct Settings_Struct get_settings(void);

#endif  