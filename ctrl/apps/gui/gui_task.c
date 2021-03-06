/*
 * gui_task.c
 *
 *  Created on: Apr 10, 2020
 *      Author: kychu
 */

#include "apps.h"

#include "log.h"

#include "vscn.h"
#include "disp.h"

#include "WM.h"
#include "GUI.h"
#include "DIALOG.h"
#if CONFIG_STEMWIN_DEMO_ENABLE
#include "GUIDEMO.h"
#endif /* CONFIG_STEMWIN_DEMO_ENABLE */

#include "gui_demo.h"
#include "evt_proc.h"

#include "boot_logo.c"
#include "background.c"

static const char *TAG = "GUI";

vscn_handle_t *ips_ram;

static osThreadId emwin_task_handle;
static uint32_t displayer_off = 0;

void emwin_task(void const *argument);

static status_t ips_write_cmd(uint8_t cmd);
static status_t ips_write_dat(uint8_t dat);
static status_t ips_write_buf(uint8_t* buf, uint16_t size);

static void gui_home_btn_evt_cb(int id, btn_evt_type_t evt);

static struct btn_cb_t home_btn_evt = {
  gui_home_btn_evt_cb,
  NULL
};

void gui_task(void const *argument)
{
  uint8_t *fb;
  disp_dev_t *ips_drv;
  uint32_t disp_switch_flag = 0;
  uint32_t disp_backlight_save = 0;

  ky_info(TAG, "GUI task started");

  fb = kmm_alloc(160 * 80 * 2);
  ips_drv = kmm_alloc(sizeof(disp_dev_t));
  ips_ram = kmm_alloc(sizeof(vscn_handle_t));
  if(fb == NULL || ips_ram == NULL || ips_drv == NULL) {
    ky_err(TAG, "no enough memory");
    goto exit;
  }

  ips_drv->frame_width = 160;
  ips_drv->frame_height = 80;
  ips_drv->direction = DIRECT_LEFT;
  ips_drv->ram_w_off = 26;
  ips_drv->ram_h_off = 1;

  ips_drv->hw_init = dispif_init;
  ips_drv->wr_reg = ips_write_cmd;
  ips_drv->wr_dat = ips_write_dat;
  ips_drv->wr_buf = ips_write_buf;

  ips_ram->ui_x = 160;
  ips_ram->ui_y = 80;
  ips_ram->ui_buf = fb;
  ips_ram->back_color = UI_COLOR_BLACK;
  ips_ram->text_color = 0xFFFF;

  output_port_clear(IO_DISP_RST);
  delay(100);
  output_port_set(IO_DISP_RST);
  delay(100);

  if(disp_init(ips_drv) != status_ok) {
    ky_err(TAG, "disp init failed");
    goto exit;
  }

  if(vscn_init(ips_ram) != status_ok) {
    ky_err(TAG, "gui init failed");
    goto exit;
  }
  vscn_clear(ips_ram);

  osThreadDef(EMWIN, emwin_task, osPriorityNormal, 0, 2048);
  emwin_task_handle = osThreadCreate(osThread(EMWIN), NULL);
  if(emwin_task_handle == NULL) {
    ky_err(TAG, "emwin task create failed.");
    goto exit;
  }

  btn_evt_register_callback(&home_btn_evt);
  dispif_set_backlight(500); // display on

  for(;;) {
    delay(30);
    if(displayer_off != 0) {
      if(disp_switch_flag != displayer_off) {
        // turn off the displayer
        disp_backlight_save = dispif_get_backlight();
        dispif_set_backlight(0);
        disp_display_off(ips_drv);
        vTaskSuspend(emwin_task_handle); // suspend GUI task.
        disp_switch_flag = displayer_off;
      }
    } else {
      if(disp_switch_flag != displayer_off) {
        // turn on the displayer
        disp_display_on(ips_drv);
        vTaskResume(emwin_task_handle); // resume GUI task.
        dispif_set_backlight(disp_backlight_save);
        disp_switch_flag = displayer_off;
      } else {
        disp_refresh(ips_drv, ips_ram->ui_buf);
      }
    }
  }

exit:
  kmm_free(fb);
  kmm_free(ips_ram);
  kmm_free(ips_drv);
  vTaskDelete(NULL);
}

void emwin_task(void const *argument)
{
  __HAL_RCC_CRC_CLK_ENABLE(); /* Enable the CRC Module */

  /* STemwin Init */
  GUI_Init();

  /* Activate the use of memory device feature */
  WM_SetCreateFlags(WM_CF_MEMDEV);
  GUI_DrawBitmap(&bmboot_logo, 0, 15);
  delay(2000);
  for(;;) {
#if CONFIG_STEMWIN_DEMO_ENABLE
    /* Start Demo */
    GUIDEMO_Main();
#else
    switch(gui_iconview_start()) {
    case menu_code_waves:
      gui_graph_start();
    break;
    case menu_code_system:
      switch(gui_setting_menu_start()) {
      case 0:
        gui_ctrl_steer_start();
      break;
      case 1:
        gui_ctrl_motor_start();
      break;
      case 2:
        gui_config_backlight_start();
      break;
      }
    break;
    case menu_code_poweroff:
      output_port_clear(IO_PWR_CTRL);
    break;
    case menu_code_nothing:
    default: break;
    }
#endif /* CONFIG_STEMWIN_DEMO_ENABLE */
  }
}

static status_t ips_write_cmd(uint8_t cmd)
{
  uint8_t val = cmd;
  output_port_clear(IO_DISP_DC);
  return dispif_tx_bytes(&val, 1);
}

static status_t ips_write_dat(uint8_t dat)
{
  uint8_t val = dat;
  output_port_set(IO_DISP_DC);
  return dispif_tx_bytes(&val, 1);
}

static status_t ips_write_buf(uint8_t* buf, uint16_t size)
{
  output_port_set(IO_DISP_DC);
  return dispif_tx_bytes_dma(buf, size);
}

static void gui_home_btn_evt_cb(int id, btn_evt_type_t evt)
{
  if(evt == btn_evt_pressed) {
    if(id == BTN_PWR) {
      displayer_off ^= 1;
    }
  }
}
