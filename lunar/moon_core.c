/* Geocentric ecliptic frame. SI units. Earth + moving Moon point masses.
   LunaPosition is the same Meeus ephemeris used by 42's MEAN option. */
#include "moon_core.h"
#include "orbkit.h"
#include <math.h>
void MoonEphem(double jd0,double t,double r[3],double v[3]) {
    double a[3],b[3];int i;
    LunaPosition(jd0+t/86400.0,r);
    if(v) {
        LunaPosition(jd0+(t-1.0)/86400.0,a);
        LunaPosition(jd0+(t+1.0)/86400.0,b);
        for(i=0;i<3;i++) v[i]=(b[i]-a[i])/2.0;
    }
}
static void rhs(double jd0,double t,const double y[6],double f[6],double scale) {
    double m[3],d[3],r2=0,m2=0,d2=0;int i;
    MoonEphem(jd0,t,m,0);
    for(i=0;i<3;i++){d[i]=m[i]-y[i];r2+=y[i]*y[i];m2+=m[i]*m[i];d2+=d[i]*d[i];}
    double r3=r2*sqrt(r2),m3=m2*sqrt(m2),d3=d2*sqrt(d2);
    for(i=0;i<3;i++) {
        f[i]=y[i+3];
        f[i+3]=-MOON_MU_E*y[i]/r3 + scale*MOON_MU_L*(d[i]/d3-m[i]/m3);
    }
}
void MoonStep(double jd0,double t,double dt,double y[6],double scale) {
    double k1[6],k2[6],k3[6],k4[6],u[6];int i;
    rhs(jd0,t,y,k1,scale);
    for(i=0;i<6;i++)u[i]=y[i]+dt*k1[i]/2;
    rhs(jd0,t+dt/2,u,k2,scale);
    for(i=0;i<6;i++)u[i]=y[i]+dt*k2[i]/2;
    rhs(jd0,t+dt/2,u,k3,scale);
    for(i=0;i<6;i++)u[i]=y[i]+dt*k3[i];
    rhs(jd0,t+dt,u,k4,scale);
    for(i=0;i<6;i++)y[i]+=dt*(k1[i]+2*k2[i]+2*k3[i]+k4[i])/6;
}
void MoonPropagate(double jd0,double t0,double t1,double maxdt,double y[6],double scale) {
    double t=t0,dt;
    while(t<t1-1e-8){dt=fmin(maxdt,t1-t);MoonStep(jd0,t,dt,y,scale);t+=dt;}
}
