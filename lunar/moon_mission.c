#include "42.h"
#include "moon_mission.h"
void KaneNBodyRK4(struct SCType *);
void ChangeNFrame(struct OrbitType *,long,long);
int MoonEnabled=0;
long MoonPauseFlag=0;
double MoonTelemetry[8]; /* h_m, speed, energy, ecc, cumulative dv, phase, Earth range, time */
static double Y[6],Jd0,CaptureTime,DvTotal;
static int Captured=0,CaptureAllowed=1;
static FILE *Csv=0,*Events=0;
static double LastOut=-1e9;
static void relative(double t,double r[3],double v[3]) {
    double m[3],mv[3];int i;MoonEphem(Jd0,t,m,mv);
    for(i=0;i<3;i++){r[i]=Y[i]-m[i];v[i]=Y[i+3]-mv[i];}
}
static void metrics(double t) {
    double r[3],v[3],h[3],ev[3],R,V;int i;
    relative(t,r,v);R=MAGV(r);V=MAGV(v);VxV(r,v,h);VxV(v,h,ev);
    for(i=0;i<3;i++)ev[i]=ev[i]/MOON_MU_L-r[i]/R;
    MoonTelemetry[0]=R-MOON_RAD_L;MoonTelemetry[1]=V;
    MoonTelemetry[2]=.5*V*V-MOON_MU_L/R;MoonTelemetry[3]=MAGV(ev);
    MoonTelemetry[4]=DvTotal;MoonTelemetry[5]=Captured;
    MoonTelemetry[6]=MAGV(Y);MoonTelemetry[7]=t;
    if(!isfinite(R)||!isfinite(V)||MAGV(Y)<MOON_RAD_E||R<MOON_RAD_L) {
        fprintf(stderr,"Invalid trajectory or surface impact at %.3f s\n",t);exit(2);
    }
}
static void capture(double t) {
    double r[3],v[3],m[3],mv[3],vt[3],dv[3],R,vr,vc;int i;
    relative(t,r,v);R=MAGV(r);vr=VoV(r,v)/R;
    for(i=0;i<3;i++)vt[i]=v[i]-vr*r[i]/R;
    if(UNITV(vt)<1e-6){fprintf(stderr,"No tangential capture direction\n");exit(2);}
    vc=sqrt(MOON_MU_L/R);MoonEphem(Jd0,t,m,mv);
    metrics(t);
    LastOut=-1e9; MoonReport(); /* preserve the pre-burn state at the same event time */
    for(i=0;i<3;i++){dv[i]=vc*vt[i]-v[i];Y[i+3]=mv[i]+vc*vt[i];}
    DvTotal+=MAGV(dv);Captured=1;
    fprintf(Events,"capture,%.9f,%.12g,%.12g,%.12g,%.12g\n",t,MAGV(dv),R-MOON_RAD_L,MAGV(v),vc);fflush(Events);
    printf("LUNAR CAPTURE: t=%.1f s h=%.3f km dV=%.3f m/s\n",t,(R-MOON_RAD_L)/1000,MAGV(dv));
}
void MoonInit(void) {
    char path[1024];int i;
    snprintf(path,sizeof(path),"%sMoonMission.txt",InOutPath);
    FILE *f=fopen(path,"r");if(!f)return;
    if(Nsc!=1||Norb!=1||EphemOption!=EPH_MEAN||SC[0].Nb!=1) {
        fprintf(stderr,"Lunar mission requires one rigid spacecraft, one orbit, MEAN ephemeris\n");exit(2);
    }
    double epoch;
    if(fscanf(f,"%lf %lf %d",&epoch,&CaptureTime,&CaptureAllowed)!=3){fclose(f);exit(2);}
    for(i=0;i<6;i++)if(fscanf(f,"%lf",&Y[i])!=1){fclose(f);exit(2);}
    fclose(f);Jd0=TT.JulDay;
    if(fabs(Jd0-epoch)>1e-7){fprintf(stderr,"Mission epoch differs from launch solution; recompute trajectory\n");exit(2);}
    if(CaptureTime<0||STOPTIME<=CaptureTime||DTSIM<=0){fprintf(stderr,"Invalid mission duration\n");exit(2);}
    MoonEnabled=1;
    /* Equivalent departure impulse from a 400 km circular orbit at the same point. */
    double vr=VoV(Y,Y+3)/MAGV(Y),vc=sqrt(MOON_MU_E/MAGV(Y)),vt[3],dv[3];
    for(i=0;i<3;i++)vt[i]=Y[i+3]-vr*Y[i]/MAGV(Y);
    UNITV(vt);for(i=0;i<3;i++)dv[i]=Y[i+3]-vc*vt[i];DvTotal=MAGV(dv);
    snprintf(path,sizeof(path),"%slunar_telemetry.csv",InOutPath);Csv=fopen(path,"w");
    snprintf(path,sizeof(path),"%slunar_events.csv",InOutPath);Events=fopen(path,"w");
    if(!Csv||!Events){fprintf(stderr,"Cannot write mission output\n");exit(2);}
    fprintf(Csv,"time_s,phase,x_ecl_m,y_ecl_m,z_ecl_m,vx_ecl_m_s,vy_ecl_m_s,vz_ecl_m_s,mx_ecl_m,my_ecl_m,mz_ecl_m,rx_m,ry_m,rz_m,height_m,speed_m_s,energy_j_kg,eccentricity,total_dv_m_s\n");
    fprintf(Events,"event,time_s,dv_m_s,height_m,speed_before_m_s,speed_after_m_s\n");
    fprintf(Events,"departure,0,%.12g,%.12g,%.12g,%.12g\n",DvTotal,MAGV(Y)-MOON_RAD_E,vc,MAGV(Y+3));
    MoonPublish();metrics(0);MoonReport();
}
void MoonDynamics(struct SCType *s) {
    /* Translation is integrated separately; native 42 retains rigid-body attitude. */
    KaneNBodyRK4(s);
    double t=SimTime,end=SimTime+DTSIM;
    if(!Captured&&CaptureAllowed&&t<CaptureTime&&end>=CaptureTime) {
        MoonPropagate(Jd0,t,CaptureTime,5.0,Y,1.0);t=CaptureTime;capture(t);
    }
    MoonPropagate(Jd0,t,end,5.0,Y,1.0);
}
void MoonPublish(void) {
    struct SCType *s=&SC[0];struct OrbitType *o=&Orb[0];int i;
    double p[3],v[3],r[3],w[3];long old=o->World;
    long world=Captured?LUNA:EARTH;
    if(Captured){relative(SimTime,r,w);MxV(World[LUNA].CNH,r,p);MxV(World[LUNA].CNH,w,v);
    } else {MxV(World[EARTH].CNH,Y,p);MxV(World[EARTH].CNH,Y+3,v);}
    o->World=world;o->Regime=ORB_CENTRAL;o->mu=World[world].mu;
    for(i=0;i<3;i++){o->PosN[i]=p[i];o->VelN[i]=v[i];s->PosR[i]=s->VelR[i]=0;}
    RV2Eph(DynTime,o->mu,p,v,&o->SMA,&o->ecc,&o->inc,&o->RAAN,&o->ArgP,&o->anom,&o->tp,&o->SLR,&o->alpha,&o->rmin,&o->MeanMotion,&o->Period);
    if(old!=world)ChangeNFrame(o,old,world);
    for(i=0;i<3;i++){s->PosN[i]=p[i];s->VelN[i]=v[i];}
    FindCLN(p,v,o->CLN,o->wln);
    MxM(Frm[0].CL,o->CLN,Frm[0].CN);
    metrics(SimTime);
}
void MoonReport(void) {
    if(!MoonEnabled||!Csv)return;
    double t=MoonTelemetry[7];
    if(t-LastOut<DTOUT-1e-7 && !(t>=STOPTIME) && !(t==CaptureTime))return;
    double m[3],mv[3],r[3],v[3];int i;
    MoonEphem(Jd0,t,m,mv);relative(t,r,v);
    fprintf(Csv,"%.9f,%d",t,Captured);
    for(i=0;i<6;i++)fprintf(Csv,",%.15g",Y[i]);
    for(i=0;i<3;i++)fprintf(Csv,",%.15g",m[i]);
    for(i=0;i<3;i++)fprintf(Csv,",%.15g",r[i]);
    for(i=0;i<5;i++)fprintf(Csv,",%.15g",MoonTelemetry[i]);
    fputc('\n',Csv);LastOut=t;
}
void MoonClose(void){if(Csv){fclose(Csv);Csv=0;}if(Events){fclose(Events);Events=0;}}
