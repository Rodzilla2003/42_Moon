#ifndef MOON_MISSION_H
#define MOON_MISSION_H
#include "moon_core.h"
struct SCType;
extern int MoonEnabled;
extern double MoonTelemetry[8];
void MoonInit(void);
void MoonDynamics(struct SCType *s);
void MoonPublish(void);
void MoonReport(void);
void MoonClose(void);
#ifdef _USE_QTPLOT_
#ifdef _ENABLE_GUI_
extern long PauseFlag;
#else
extern long MoonPauseFlag;
#define PauseFlag MoonPauseFlag
#endif
void ToPlot(double t);
void PumpQt(void);
#endif
#endif
