#ifndef TASK_BT_SEND_H_
#define TASK_BT_SEND_H_

#include "cy_pdl.h"
#include "cyhal.h"

/* Start a periodic timer (~200ms) whose ISR pushes a CarDataIn control byte
 * to the server using the current Controller_Events accelerate/gear_sel
 * values. All other bits are sent as 0. */
cy_rslt_t task_bt_send_init(void);

#endif /* TASK_BT_SEND_H_ */
