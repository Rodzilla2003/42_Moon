#ifndef MOON_CORE_H
#define MOON_CORE_H
#define MOON_MU_E 3.986004e14
#define MOON_MU_L 4.902801e12
#define MOON_RAD_E 6378145.0
#define MOON_RAD_L 1738000.0
void MoonEphem(double jd0,double t,double r[3],double v[3]);
void MoonStep(double jd0,double t,double dt,double y[6],double lunar_scale);
void MoonPropagate(double jd0,double t0,double t1,double maxdt,double y[6],double lunar_scale);
#endif
