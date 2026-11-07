#ifndef __NOCTURNAL_PATHS_H
#define __NOCTURNAL_PATHS_H

#define SDCARD_DIRECTORY      "/media/mmcblk0p2"
#define ROMS_DIRECTORY        SDCARD_DIRECTORY "/ROMS"
#define CFW_FOLDER_DIRECTORY  SDCARD_DIRECTORY "/cfw"
#define SYSTEM_DIRECTORY      CFW_FOLDER_DIRECTORY "/bios"
#define ASSETS_DIRECTORY      CFW_FOLDER_DIRECTORY "/assets"
#define LOGS_DIRECTORY        CFW_FOLDER_DIRECTORY "/logs"
#define SAVE_DIRECTORY        CFW_FOLDER_DIRECTORY "/saves"
#define CONFIG_DIRECTORY      CFW_FOLDER_DIRECTORY "/configs"
#define CORES_DIRECTORY       CFW_FOLDER_DIRECTORY "/Phobos/cores"
#define SAVESTATE_DIRECTORY   CFW_FOLDER_DIRECTORY "/Phobos/savestates"
#define BORDERS_DIRECTORY     CFW_FOLDER_DIRECTORY "/Phobos/borders"

#define LOG_PATH              LOGS_DIRECTORY   "/Phobos.log"
#define RTC_PATH              SAVE_DIRECTORY   "/rtc.txt"
#define MAXPATH 512

#endif //__NOCTURNAL_PATHS_H