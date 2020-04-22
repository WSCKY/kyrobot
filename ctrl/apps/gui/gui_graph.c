/*
 * gui_graph.c
 *
 *  Created on: Apr 21, 2020
 *      Author: kychu
 */

#include "gui_demo.h"

#include "FRAMEWIN.h"
#include "GRAPH.h"

#define BK_COLOR_0        0xFF5555
#define BK_COLOR_1        0x880000

#define DIST_TO_BORDER   0
#define DIST_TO_WIN      5

#define BORDER_TOP       0
#define BORDER_BOTTOM    9
#define BORDER_LEFT      19
#define BORDER_RIGHT     0

#define COLOR_BK         GUI_DARKGRAY
#define COLOR_BORDER     BK_COLOR_1
#define COLOR_FRAME      GUI_BLACK
#define COLOR_GRID       GUI_GRAY

#define SCALE_H_HEIGHT   4

#define TICK_DIST_H      25
#define TICK_DIST_V      20

#define TIME_RUN         20000
#define TIME_STEP        15

#define MAX_NUM_DATA_OBJ 5

#define GRAPH_DIV        9   // (2^9 = 512) If this value is changed _aWaves[] need to be changed too!
#define GRID_DIST_X      25
#define GRID_DIST_Y      10
#define GRID_OFF_Y       25

/*********************************************************************
*
*       Typedef / Data
*
**********************************************************************
*/
typedef struct {
  int     ScaleVOff;
  int     DataVOff;
  int     GridVOff;
  void (* pfAddData)(GRAPH_DATA_Handle hData, int DataID);
  int     NumWaves;
} GRAPH_WAVE;

static int _HeartBeat[] = {
    2,   4,   6,   8,  10,   6,   2,   0,   0,   0,
   -8,  16,  40,  64,  88,  58,  28,  -2, -32, -30,
  -20, -10,   0,   2,   2,   4,   4,   6,   6,   8,
    8,  10,  12,  14,  16,  18,  20,  16,  12,   8,
    4,   2,   2,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0
};

static GUI_COLOR _aColorData[MAX_NUM_DATA_OBJ] = {
  0x50C0FF,
  0xFFC050,
  0x50FFC0,
  0x800000,
  0x000080
};

GRAPH_SCALE_Handle _hScaleH, _hScaleV;
static int         _DataAdjust;

/*********************************************************************
*
*       Static functions
*
**********************************************************************
*/
/*********************************************************************
*
*       _AddData_Sine
*/
static void _AddData_Sine(GRAPH_DATA_Handle hData, int DataID) {
  static int x1000[MAX_NUM_DATA_OBJ];
  I32        SinHQ;
  int        Multi, Step;

  switch (DataID) {
  case 0:
    Multi = 70;
    Step  = 3;
    break;
  case 1:
    Multi = 50;
    Step  = 1;
    break;
  case 2:
    Multi = 30;
    Step  = 7;
    break;
  default:
    return;
  }
  SinHQ          = GUI__SinHQ(x1000[DataID]);
  x1000[DataID] += 1000 * Step;
  GRAPH_DATA_YT_AddValue(hData, ((SinHQ * Multi) >> 16) + _DataAdjust);
}

/*********************************************************************
*
*       _AddData_Heartbeat
*/
static void _AddData_Heartbeat(GRAPH_DATA_Handle hData, int DataID) {
  static int Index;

  GUI_USE_PARA(DataID);
  GRAPH_DATA_YT_AddValue(hData, (_HeartBeat[Index]) + _DataAdjust);
  if (++Index == GUI_COUNTOF(_HeartBeat)) {
    Index = 0;
  }
}

/*********************************************************************
*
*       DATA _aWave - Keep below _AddData-functions
*/
GRAPH_WAVE _aWave[] = {
  {
    157,                // Vertical scale offset in relation to GRAPH_DIV
    152,                // Vertical data  offset in relatio n to GRAPH_DIV
    21,                 // Vertical grid  offset in relation to GRAPH_DIV
    _AddData_Heartbeat, // Pointer to specific AddData function
    1                   // Number of waves
  },
  {
    265,
    253,
    23,
    _AddData_Sine,
    3
  },
  {0}
};

/*********************************************************************
*
*       _ShowGraph
*/
static void _ShowGraph(GRAPH_Handle hGraph, GRAPH_DATA_Handle hData[], int DataCount, void (* pfAddData)(GRAPH_DATA_Handle hData, int DataID)) {
  int Count, Data_xSize, xSize;
  int TimeStart, TimeDiff, TimeStep;
  int i, Flag;

  xSize      = LCD_GetXSize();
  Data_xSize = xSize - (DIST_TO_BORDER << 1) - (BORDER_LEFT + BORDER_RIGHT);
  Count      = 0;
  //
  // Attach data objects
  //
  for (i = 0; i < DataCount; i++) {
    GRAPH_AttachData(hGraph, hData[i]);
  }
  //
  // Add values before GRAPH is displayed
  //
  while (Count < Data_xSize) {
    for (i = 0; i < DataCount; i++) {
      pfAddData(hData[i], i);
    }
    Count++;
  }
  //
  // Add values depending on time
  //

  TimeStart = GUI_GetTime();
  Flag = 1;
  do {
    TimeDiff = GUI_GetTime() - TimeStart;
    for (i = 0; i < DataCount; i++) {
      pfAddData(hData[i], i);
    }
    if (Flag) {
      Flag = 0;
      GUI_Exec();
      GRAPH_DetachScale(hGraph, _hScaleH);
      GRAPH_DetachScale(hGraph, _hScaleV);
      WM_ValidateWindow(hGraph);
    }

    TimeStep  = GUI_GetTime() - TimeStart;
    if ((TimeStep - TimeDiff) < TIME_STEP) {
      GUI_Delay(TIME_STEP - (TimeStep - TimeDiff));
    }
  } while ((TimeDiff < TIME_RUN));
  for (i = 0; i < DataCount; i++) {
    GRAPH_DetachData(hGraph, hData[i]);
  }
}

/*********************************************************************
*
*       _cbBk
*/
static void _cbBk(WM_MESSAGE * pMsg) {
  switch (pMsg->MsgId) {
  case WM_PAINT:
    GUI_SetBkColor(BK_COLOR_1);
    GUI_Clear();
  break;
  default:
    WM_DefaultProc(pMsg);
  }
}

void gui_graph_start(void)
{
  const WIDGET_EFFECT * pEffectOld;
  GRAPH_Handle          hGraph;
  GRAPH_DATA_Handle     hData[MAX_NUM_DATA_OBJ];
  int                   xSize, ySize, i;
  int                   Data_ySize;

  xSize      = LCD_GetXSize();
  ySize      = LCD_GetYSize();
  pEffectOld = WIDGET_SetDefaultEffect(&WIDGET_Effect_Simple);
  //
  // Set Callback function for background window
  //
  WM_SetCallback(WM_HBKWIN, _cbBk);
  hGraph = GRAPH_CreateEx(0, 0, xSize, ySize, WM_HBKWIN, WM_CF_SHOW | WM_CF_CONST_OUTLINE, 0, 0);
  GRAPH_SetBorder(hGraph, BORDER_LEFT, BORDER_TOP, BORDER_RIGHT, BORDER_BOTTOM);
  WM_SetHasTrans (hGraph);
  GRAPH_SetColor (hGraph, COLOR_BK,     GRAPH_CI_BK);
  GRAPH_SetColor (hGraph, COLOR_BORDER, GRAPH_CI_BORDER);
  GRAPH_SetColor (hGraph, COLOR_FRAME,  GRAPH_CI_FRAME);
  GRAPH_SetColor (hGraph, COLOR_GRID,   GRAPH_CI_GRID);
  //
  // Adjust grid
  //
  GRAPH_SetGridVis  (hGraph, 1);
  GRAPH_SetGridDistX(hGraph, GRID_DIST_X);
  GRAPH_SetGridDistY(hGraph, GRID_DIST_Y);
  WM_BringToBottom  (hGraph);
  //
  // Create and configure GRAPH_DATA_YT object
  //
  for (i = 0; i < MAX_NUM_DATA_OBJ; i++) {
    hData[i] = GRAPH_DATA_YT_Create(_aColorData[i], xSize - (DIST_TO_BORDER << 1) - BORDER_LEFT, 0, 0);
  }
  Data_ySize = ySize - BORDER_BOTTOM;
  //
  // Create and configure GRAPH_SCALE objects
  //
  _hScaleH = GRAPH_SCALE_Create(BORDER_BOTTOM >> 1, GUI_TA_VCENTER, GRAPH_SCALE_CF_HORIZONTAL, TICK_DIST_H);
  _hScaleV = GRAPH_SCALE_Create(BORDER_LEFT   >> 1, GUI_TA_HCENTER, GRAPH_SCALE_CF_VERTICAL,   TICK_DIST_V);
  GRAPH_SCALE_SetPos(_hScaleH, ySize - SCALE_H_HEIGHT);
  GRAPH_SCALE_SetOff(_hScaleH, -5);
  //
  // Show some graphs
  //
  i = 0;
  while (_aWave[i].pfAddData != 0) {
    GRAPH_AttachScale(hGraph, _hScaleH);
    GRAPH_AttachScale(hGraph, _hScaleV);
    _DataAdjust = (Data_ySize * _aWave[i].DataVOff) >> GRAPH_DIV;
    GRAPH_SetGridOffY (hGraph, (Data_ySize * _aWave[i].GridVOff) >> GRAPH_DIV);
    GRAPH_SCALE_SetOff(_hScaleV, (((Data_ySize - BORDER_BOTTOM) * _aWave[i].ScaleVOff) >> GRAPH_DIV));
    _ShowGraph(hGraph, hData, _aWave[i].NumWaves, _aWave[i].pfAddData);
    i++;
  }
  //
  // Clean up
  //
  GRAPH_DetachScale(hGraph, _hScaleH);
  GRAPH_DetachScale(hGraph, _hScaleV);
  GRAPH_SCALE_Delete(_hScaleH);
  GRAPH_SCALE_Delete(_hScaleV);
  for (i = 0; i < MAX_NUM_DATA_OBJ; i++) {
    GRAPH_DATA_YT_Delete(hData[i]);
  }
  WM_DeleteWindow(hGraph);
  WIDGET_SetDefaultEffect(pEffectOld);
}