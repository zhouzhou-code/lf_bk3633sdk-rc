#ifndef __APP_SLEEP_H__
#define __APP_SLEEP_H__

#include <stdint.h>
#include "aon_rtc.h"

void app_deep_sleep_wakeup_by_rtc(uint32_t sleep_ms);



#endif // SLEEP_H