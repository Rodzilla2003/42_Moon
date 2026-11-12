"""Recompute the departure velocity and lunar arrival angle with the same C force model."""
import argparse,ctypes as C,json
from pathlib import Path
import numpy as np
from scipy.optimize import least_squares
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--library',required=True);p.add_argument('--write-case',action='store_true');a=p.parse_args()
lib=C.CDLL(str(Path(a.library).resolve()));ptr=np.ctypeslib.ndpointer(dtype=np.float64,flags='C_CONTIGUOUS')
lib.MoonEphem.argtypes=[C.c_double,C.c_double,ptr,ptr]
lib.MoonPropagate.argtypes=[C.c_double,C.c_double,C.c_double,C.c_double,ptr,C.c_double]
j=json.loads((root/'lab/results/launch.json').read_text());jd=j['jd_tt'];T=j['capture_time_s'];r0=np.array(j['r0_m'],dtype=float)
m=np.zeros(3);mv=np.zeros(3);lib.MoonEphem(jd,T,m,mv);e=m/np.linalg.norm(m);tangent=mv-e*np.dot(mv,e);tangent/=np.linalg.norm(tangent);radius=1738e3+300e3
seed=np.r_[np.array(j['v0_m_s'])/1000,j.get('arrival_angle_rad',-.86785)]
def residual(z):
 y=np.r_[r0,z[:3]*1000];lib.MoonPropagate(jd,0,T,5,y,1)
 u=e*np.cos(z[3])+tangent*np.sin(z[3]);dr=y[:3]-m;dv=y[3:]-mv
 return np.r_[(dr-radius*u)/1e6,np.dot(dr,dv)/max(np.linalg.norm(dr),1)/1000]
fit=least_squares(residual,seed,diff_step=1e-8,max_nfev=120,xtol=1e-12,ftol=1e-12,gtol=1e-12,x_scale=[.01,.01,.01,1])
if np.linalg.norm(fit.fun)>1e-5:raise RuntimeError('Shooting did not converge; case was not changed')
j.update(v0_m_s=(fit.x[:3]*1000).tolist(),arrival_angle_rad=float(fit.x[3]),residual_km=(fit.fun[:3]*1000).tolist())
print(json.dumps(j,indent=2))
if a.write_case:
 (root/'lab/results/launch.json').write_text(json.dumps(j,indent=2))
 (root/'examples/LunarCapture/MoonMission.txt').write_text(' '.join(map(str,[jd,T,1]))+'\n'+' '.join(map(str,r0))+'\n'+' '.join(map(str,j['v0_m_s']))+'\n')
