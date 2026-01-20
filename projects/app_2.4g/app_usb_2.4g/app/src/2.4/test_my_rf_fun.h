#include "Types.h"
#include "hal_drv_rf.h"

extern RF_HandleTypeDef hrf;

void rf_simple_init(void);
void rf_simple_init_old(void);

void bk24_send_data(void);
void bk24_send_data_intc(void);
void rf_simple_receive();
void rf_intc_receive();
