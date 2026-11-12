"""Run real 42 executables in isolated cases; verify capture, flyby and step convergence."""
import argparse,json,shutil,subprocess,tempfile,os
from pathlib import Path
import numpy as np
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--binary',required=True);a=p.parse_args();binary=str(Path(a.binary).resolve())
out=root/'lab/results';out.mkdir(exist_ok=True)
def run(name,step=10,capture=True):
 with tempfile.TemporaryDirectory(prefix='moon-case-') as t:
  d=Path(t);shutil.copytree(root/'examples/LunarCapture',d,dirs_exist_ok=True)
  s=(d/'Inp_Sim.txt').read_text().replace('440000.0  10.0',f'440000.0  {step}').replace('TRUE                            !  Graphics Front End?', 'FALSE                           !  Graphics Front End?')
  (d/'Inp_Sim.txt').write_text(s)
  if not capture:
   lines=(d/'MoonMission.txt').read_text().splitlines();x=lines[0].split();x[2]='0';lines[0]=' '.join(x);(d/'MoonMission.txt').write_text('\n'.join(lines)+'\n')
  for f in d.glob('lunar_*.csv'):f.unlink()
  result=subprocess.run([binary,os.path.relpath(d,root/"42")],cwd=root/'42',stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,timeout=120)
  target=out/name;target.mkdir(exist_ok=True);(target/'run.log').write_text(result.stdout)
  assert result.returncode==0,result.stdout
  for f in d.glob('lunar_*.csv'):shutil.copy2(f,target/f.name)
  return np.genfromtxt(target/'lunar_telemetry.csv',delimiter=',',names=True)
x=run('capture');off=run('no_capture',capture=False);fine=run('step_2_5',step=2.5)
for z in [x,off,fine]:
 for n in z.dtype.names:assert np.isfinite(z[n]).all(),n
 assert (np.diff(z['time_s'])>=0).all()
 assert z['height_m'].min()>0
 assert np.sqrt(z['x_ecl_m']**2+z['y_ecl_m']**2+z['z_ecl_m']**2).min()>6378145
 assert z['time_s'][-1]>=440000
post=x[x['phase']==1];assert len(post)>700
assert post['energy_j_kg'].max()<0
assert post['eccentricity'].max()<.01
assert post['height_m'].min()>290e3 and post['height_m'].max()<310e3
assert (off['phase']==0).all() and off['height_m'][-1]>40e6 and off['energy_j_kg'][-1]>0
# Compare sampled positions at common physical times, excluding the intentional duplicate burn row.
coarse=x[(x['phase']==1)&(x['time_s']<440000)];f=fine[fine['phase']==1]
common={round(t,6):i for i,t in enumerate(f['time_s'])};err=[]
for z in coarse:
 if round(z['time_s'],6) in common:
  b=f[common[round(z['time_s'],6)]];err.append(np.linalg.norm([z[k]-b[k] for k in ['rx_m','ry_m','rz_m']]))
assert len(err)>700 and max(err)<500
rr=np.array([post[k] for k in ['rx_m','ry_m','rz_m']]).T
angles=np.arccos(np.clip(np.sum(rr[:-1]*rr[1:],axis=1)/np.linalg.norm(rr[:-1],axis=1)/np.linalg.norm(rr[1:],axis=1),-1,1))
turns=float(angles.sum()/(2*np.pi));assert turns>6
summary=dict(passed=True,post_capture_height_km=[float(post['height_m'].min()/1000),float(post['height_m'].max()/1000)],max_eccentricity=float(post['eccentricity'].max()),energy_j_kg=[float(post['energy_j_kg'].min()),float(post['energy_j_kg'].max())],orbits_observed=turns,step_convergence_max_position_error_m=float(max(err)),no_capture_final_height_km=float(off['height_m'][-1]/1000),no_capture_final_energy_j_kg=float(off['energy_j_kg'][-1]),capture_time_s=388800)
(out/'checks.json').write_text(json.dumps(summary,indent=2));print(json.dumps(summary,indent=2))
plt.rcParams.update({'font.family':'DejaVu Sans','font.size':11,'axes.spines.top':False,'axes.spines.right':False})
fig,ax=plt.subplots(2,2,figsize=(11,7.6),layout='constrained')
for z,label,c in [(x,'С торможением','#2463a6'),(off,'Без торможения','#ae6240')]:
 t=z['time_s']/86400
 ax[0,0].semilogy(t,z['height_m']/1000,label=label,color=c)
 ax[0,1].plot(t,z['speed_m_s']/1000,color=c)
 ax[1,0].plot(t,z['energy_j_kg']/1e6,color=c)
ax[0,0].set(ylabel='Высота над Луной, км',title='Подлёт и захват');ax[0,0].legend(fontsize=9)
ax[0,1].set(ylabel='Скорость, км/с',title='Скорость относительно Луны')
ax[1,0].set(ylabel='Энергия, МДж/кг',title='Энергия относительно Луны');ax[1,0].axhline(0,c='black',lw=.7)
ax[1,1].plot(post['time_s']/86400,post['height_m']/1000,color='#29905a');ax[1,1].set(ylabel='Высота над Луной, км',title='После торможения')
for ar in ax.flat:ar.set_xlabel('Время, сут');ar.grid(alpha=.2);ar.axvline(4.5,c='#888888',ls='--',lw=.8)
fig.savefig(out/'capture_diagnostics.png',dpi=170);plt.close(fig)
fig,axs=plt.subplots(1,2,figsize=(11,5.2),layout='constrained')
axs[0].plot(x['mx_ecl_m']/1e6,x['my_ecl_m']/1e6,'--',color='#aaaaaa',label='Путь Луны')
axs[0].plot(x['x_ecl_m']/1e6,x['y_ecl_m']/1e6,color='#2463a6',label='Аппарат');axs[0].scatter([0],[0],s=65,c='#29905a',label='Земля');axs[0].set(xlabel='X, тыс. км',ylabel='Y, тыс. км',title='Перелёт: проекция на эклиптику');axs[0].legend(fontsize=9);axs[0].axis('equal')
u=rr[0]/np.linalg.norm(rr[0]);normal=np.cross(rr[0],rr[1]);normal/=np.linalg.norm(normal);v=np.cross(normal,u)
axs[1].plot(rr@u/1000,rr@v/1000,color='#814da6');axs[1].add_patch(plt.Circle((0,0),1738,color='#c6c8cc'));axs[1].set(xlabel='X, км',ylabel='Y, км',title='Витки в плоскости лунной орбиты');axs[1].set_aspect('equal');axs[1].grid(alpha=.2)
fig.savefig(out/'trajectory.png',dpi=170);plt.close(fig)
