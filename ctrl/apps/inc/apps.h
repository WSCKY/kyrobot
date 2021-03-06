/**
  ******************************************************************************
  * @file    apps/apps.c
  * @author  kyChu<kychu@qq.com>
  * @brief   Header file for apps.c file.
  ******************************************************************************
  */

#ifndef __APPS_H
#define __APPS_H

#include "log.h"

#include "drivers.h"

#include "mesg.h"
#include "compass.h"
#include "att_est_q.h"
#include "filesystem.h"
#include "filetransfer.h"

#include "stm32f7xx_hal.h"

void gui_task(void const *argument);
void evt_proc_task(void const *argument);

void test_case_task(void const *argument);

#endif /* __APPS_H */

/******************** kyChu<kyChu@qq.com> **** END OF FILE ********************/
