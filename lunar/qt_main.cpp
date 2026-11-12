#ifdef _WIN32
#include <winsock2.h>
#endif
#include <QApplication>
#include <QMainWindow>
#include <QPushButton>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QElapsedTimer>
#include <QCloseEvent>
#include <QMessageBox>
#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QVector>
#include <cmath>
#include <cstdlib>
extern "C" {
#include "42.h"
#include "moon_mission.h"
int exec(int,char **);
}
class Plot:public QWidget {
public:
 QString title,xlabel,ylabel;QVector<QPointF> pts;QColor color;bool equal=false,zero=false;
 Plot(QString t,QString x,QString y,QColor c):title(t),xlabel(x),ylabel(y),color(c){setMinimumSize(410,280);}
 void paintEvent(QPaintEvent*)override {
  QPainter p(this);p.setRenderHint(QPainter::Antialiasing);p.fillRect(rect(),Qt::white);
  p.setPen(Qt::black);p.setFont(QFont("Arial",12));p.drawText(QRect(0,8,width(),26),Qt::AlignCenter,title);
  QRectF box(78,45,width()-105,height()-108);
  double xmin=0,xmax=1,ymin=0,ymax=1;
  if(!pts.isEmpty()){xmin=xmax=pts[0].x();ymin=ymax=pts[0].y();for(auto a:pts){xmin=qMin(xmin,a.x());xmax=qMax(xmax,a.x());ymin=qMin(ymin,a.y());ymax=qMax(ymax,a.y());}
   if(zero){ymin=qMin(ymin,0.0);ymax=qMax(ymax,0.0);}if(xmax-xmin<1e-9)xmax=xmin+1;if(ymax-ymin<1e-9){ymin-=.5;ymax+=.5;}
   double dy=(ymax-ymin)*.05;ymin-=dy;ymax+=dy;
   if(equal){double scale=qMax((xmax-xmin)/box.width(),(ymax-ymin)/box.height());double xm=(xmin+xmax)/2,ym=(ymin+ymax)/2;xmin=xm-scale*box.width()/2;xmax=xm+scale*box.width()/2;ymin=ym-scale*box.height()/2;ymax=ym+scale*box.height()/2;}}
  p.setFont(QFont("Arial",9));
  for(int i=0;i<=4;i++){double f=i/4.;double x=box.left()+f*box.width(),y=box.bottom()-f*box.height();p.setPen(QColor(225,228,232));p.drawLine(QPointF(x,box.top()),QPointF(x,box.bottom()));p.drawLine(QPointF(box.left(),y),QPointF(box.right(),y));p.setPen(Qt::black);p.drawText(QRectF(x-35,box.bottom()+5,70,20),Qt::AlignCenter,QString::number(xmin+f*(xmax-xmin),'g',4));p.drawText(QRectF(4,y-10,68,20),Qt::AlignRight|Qt::AlignVCenter,QString::number(ymin+f*(ymax-ymin),'g',4));}
  p.drawRect(box);p.drawText(QRectF(box.left(),height()-27,box.width(),22),Qt::AlignCenter,xlabel);p.save();p.translate(16,box.center().y());p.rotate(-90);p.drawText(QRectF(-box.height()/2,-12,box.height(),24),Qt::AlignCenter,ylabel);p.restore();
  QPainterPath path;bool first=true;for(auto a:pts){QPointF q(box.left()+(a.x()-xmin)/(xmax-xmin)*box.width(),box.bottom()-(a.y()-ymin)/(ymax-ymin)*box.height());if(first){path.moveTo(q);first=false;}else path.lineTo(q);}p.save();p.setClipRect(box);p.setPen(QPen(color,1.6));p.drawPath(path);p.restore();
 }
};
static Plot *plots[4];static QPushButton *startButton;static bool running=false,finished=false;static int ac;static char **av;static double last=-1e9;
extern "C" void PumpQt(void){static QElapsedTimer timer;if(!timer.isValid()||timer.elapsed()>10){timer.restart();QApplication::processEvents();}}
extern "C" void ToPlot(double t){
 if(!MoonEnabled)return;
 if(t-last>=DTOUT-1e-7||t>=STOPTIME){last=t;
  plots[0]->pts.append(QPointF(t/86400,std::log10(qMax(MoonTelemetry[0]/1000,1e-9))));
  plots[1]->pts.append(QPointF(t/86400,MoonTelemetry[1]/1000));
  plots[2]->pts.append(QPointF(t/86400,MoonTelemetry[2]/1e6));
  if(MoonTelemetry[5]>0){const auto &s=SC[0];plots[3]->pts.append(QPointF(s.PosN[0]/1000,s.PosN[1]/1000));}
  for(auto p:plots)p->update();}
 if(t>=STOPTIME&&!finished){finished=true;startButton->setText(QString::fromUtf8("Готово"));startButton->setEnabled(false);}PumpQt();
}
class Window:public QMainWindow {
 void closeEvent(QCloseEvent *e)override{MoonClose();e->accept();if(running)std::exit(0);}
};
int main(int argc,char **argv){
 QApplication app(argc,argv);ac=argc;av=argv;Window w;w.setWindowTitle(QString::fromUtf8("Перелёт к Луне и выход на орбиту"));w.resize(1120,810);
 auto body=new QWidget;auto layout=new QVBoxLayout(body);auto grid=new QGridLayout;
 plots[0]=new Plot(QString::fromUtf8("Высота над Луной"),QString::fromUtf8("Время, сут"),QString::fromUtf8("log10(h / км)"),QColor("#2463a6"));
 plots[1]=new Plot(QString::fromUtf8("Скорость относительно Луны"),QString::fromUtf8("Время, сут"),QString::fromUtf8("Скорость, км/с"),QColor("#29905a"));
 plots[2]=new Plot(QString::fromUtf8("Удельная энергия относительно Луны"),QString::fromUtf8("Время, сут"),QString::fromUtf8("Энергия, МДж/кг"),QColor("#b85032"));plots[2]->zero=true;
 plots[3]=new Plot(QString::fromUtf8("Орбита после торможения"),QString::fromUtf8("X, км"),QString::fromUtf8("Y, км"),QColor("#814da6"));plots[3]->equal=true;
 for(int i=0;i<4;i++)grid->addWidget(plots[i],i/2,i%2);layout->addLayout(grid);startButton=new QPushButton(QString::fromUtf8("Старт"));layout->addWidget(startButton);w.setCentralWidget(body);
 QObject::connect(startButton,&QPushButton::clicked,[&](){
  if(running){PauseFlag=!PauseFlag;startButton->setText(QString::fromUtf8(PauseFlag?"Продолжить":"Пауза"));return;}
  if(!QDir("Model").exists()||argc<2||!QFileInfo(QString::fromLocal8Bit(argv[1])+"/MoonMission.txt").exists()){
    QMessageBox::critical(&w,QString::fromUtf8("Настройки запуска"),QString::fromUtf8("Рабочий каталог: папка 42. Параметры: ../examples/LunarCapture без кавычек."));return;}
  running=true;startButton->setText(QString::fromUtf8("Пауза"));exec(ac,av);running=false;finished=true;startButton->setText(QString::fromUtf8("Готово"));startButton->setEnabled(false);
 });w.show();
 if(qEnvironmentVariableIsSet("MOON_QT_SMOKE"))QTimer::singleShot(0,[&](){startButton->click();w.grab().save(qEnvironmentVariable("MOON_QT_SMOKE"));app.quit();});
 return app.exec();
}
