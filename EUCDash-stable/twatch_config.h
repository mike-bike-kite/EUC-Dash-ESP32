// The remote service we wish to connect to.
// #define LILYGO_WATCH_2019_WITH_TOUCH      // To use T-Watch2019 with touchscreen, please uncomment this line
// #define LILYGO_WATCH_2019_NO_TOUCH       // To use T-Watch2019 Not touchscreen , please uncomment this line
#define LILYGO_WATCH_2020_V1                //T-watch_2020

#define LILYGO_WATCH_LVGL
#define TWATCH_LVGL_DOUBLE_BUFFER
#define TWATCH_USE_PSRAM_ALLOC_LVGL
#define LVGL_BUFFER_SIZE        (240*240)
#include <LilyGoWatch.h>
