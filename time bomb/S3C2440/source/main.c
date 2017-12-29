
#include "config.h"
#include <stdlib.h>

#include <stdio.h> 

//=======================================
#include "app_cfg.h"
#include "Printf.h"
#include "ucos_ii.h"

#include "gui.h"
#include "math.h"
#include "GUI_Protected.h"
#include "GUIDEMO.H"
#include "WM.h"
#include "Dialog.h"
#include "LISTBOX.h"
#include "EDIT.h"
#include "SLIDER.h"
#include "FRAMEWIN.h"

//=========================================
OS_STK  MainTaskStk[MainTaskStkLengh];
int number=1;
int passwordarray[2]={0,0};
int tt;//记录了创建任务的时间，感觉有点多余，不知道为什么要进行添加，难道是模拟了每个wait是150？
int INIT_TIMEOUT=0;

/*******************************************************************
*
*      Structure containing information for drawing routine
*
*******************************************************************
*/

typedef struct
{
	I16 *aY;
}PARAM;

/*******************************************************************
*
*                    Defines
*
********************************************************************
*/

//#define YSIZE (LCD_YSIZE - 100)
//#define DEG2RAD (3.1415926f / 180)

#if LCD_BITSPERPIXEL == 1
  #define COLOR_GRAPH0 GUI_WHITE
  #define COLOR_GRAPH1 GUI_WHITE
#else
  #define COLOR_GRAPH0 GUI_GREEN
  #define COLOR_GRAPH1 GUI_YELLOW
#endif


///////////////////////////////////////////////////////////////////
void (*FunP[3][3])(void);
void reset(void);

int defuse=11,state,timeout;
int dismantled=0;

void addcode(void)//用于输入密码时实现0-9随按键循环出现
{
	if(passwordarray[number]>=9)
		passwordarray[number]=0;
	else
		passwordarray[number]++;
	GUI_DispCharAt(passwordarray[0]+48,20,140);//本方法用于在指定位置（横坐标x＝20,纵坐标y＝140处）显示一个字符
	GUI_DispCharAt(passwordarray[1]+48,40,140);
}

void show(int t)//用于显示剩余时间
{
	int fir=t/10;
	int sec=t%10;
	GUI_SetColor(GUI_BLACK);
	GUI_SetBkColor(GUI_WHITE);
	if(fir!=0)
		GUI_DispCharAt('0'+fir,20,60);
	else
    	GUI_DispStringAt("     ",20,60);
    GUI_DispCharAt('0'+sec,40,60);
}

//Setting
void s_up(void)//增加时间
{
	timeout++;
	if(timeout>99)
		timeout=99;
	show(timeout);
}

void s_down(void)//减少时间
{
	timeout--;
	if(timeout<0)
		timeout=0;
	show(timeout);
}

void s_arm(void)//确定提交时间，转为状态1，用户输入密码，定时器开始倒计时
{
	GUI_SetColor(GUI_BLACK);
	GUI_SetBkColor(GUI_WHITE);
	GUI_SetFont(&GUI_Font32_ASCII); 
	GUI_DispStringAt("Bomb Counting down...",20,200); 
	GUI_DispStringAt("Please enter the password",20,100);	
	state=1;
}

//Timing
void t_up(void)//输入密码时，进行数字加
{
	addcode();	
}

void t_down()//输入密码时进行数字减
{
	number=number?0:1;
	GUI_SetColor(GUI_BLACK);
	GUI_SetBkColor(GUI_WHITE);
	GUI_SetFont(&GUI_Font32_ASCII); 
	GUI_DispCharAt(' * ',20*(number+1),140);
	GUI_DispCharAt(passwordarray[1-number]+'0',(2-number)*20,140); 
	Uart_Printf("password:%d\n",number);
}

void t_arm(void)//确定提交密码
{
    int temp=passwordarray[0]*10+passwordarray[1];
    
	if(temp==defuse)//密码是对的，会回到setting状态，即本实现中的状态2
	{
		Uart_Printf("Correct Password\n"); 
		GUI_ClearRect(0,0,LCD_XSIZE,LCD_YSIZE);
		GUI_SetColor(GUI_BLACK);
		GUI_SetBkColor(GUI_WHITE);
		GUI_SetFont(&GUI_Font32_ASCII); 
		GUI_DispStringAt("Bomb cleared!",20,10);
		GUI_DispStringAt("Press any button to retry",20,60);
		state=2;
	}
	else
		Uart_Printf("Incorrect Password\n"); 
}  

//Get input，会判断哪个键被按下
//实验中用到的四个键返回的值是1，3，5，7
U8 Key_Scan( void )
{
	Delay( 80 ) ;
	rGPBDAT &=~(1<<6);
	rGPBDAT |=1<<7;

	if(      (rGPFDAT&(1<< 0)) == 0 )		return 1;
	else if( (rGPFDAT&(1<< 2)) == 0 )		return 3;
	else if( (rGPGDAT&(1<< 3)) == 0 )		return 5;//确认键
	else if( (rGPGDAT&(1<<11)) == 0 )		return 7;//reset

	rGPBDAT &=~(1<<7);
	rGPBDAT |=1<<6;
	if(      (rGPFDAT&(1<< 0)) == 0 )		return 2;
	else if( (rGPFDAT&(1<< 2)) == 0 )		return 4;
	else if( (rGPGDAT&(1<< 3)) == 0 )		return 6;
	else if( (rGPGDAT&(1<<11)) == 0 )		return 8;
	else return 0xff;	
}



void explode(void)
{
	GUI_SetColor(GUI_WHITE);
	GUI_SetBkColor(GUI_BLACK);
	GUI_ClearRect(0,0,LCD_XSIZE,LCD_YSIZE);
	GUI_SetFont(&GUI_Font32_ASCII); 
	GUI_DispStringAt("The bomb has exploded...",20,10);
	GUI_DispStringAt("Press any button to retry",20,60);
	state=2;
}

void Key_ISR() //设置中断寄存器,本方法执行结束即中断返回，就回到原本正在执行的任务
{
	int key;
	key = Key_Scan();
	while(Key_Scan()==key);
	if(state<=2) //因为本系统只有0、1、2这3个状态，所以只在这种情况下做处理，否则clear
	{
		if(key % 2 == 1 && key<7)
			FunP[state][(key-1)/2]();
		if(key==7)
			reset();    //在2状态下，任意键都会使其reset()
		if(rEINTPEND & 1<<11)
			rEINTPEND |= 1<<11;
		if(rEINTPEND & 1<<19)
			rEINTPEND |= 1<<19;
	}
	//Clear Rs, SRCPND INTPND
	ClearPending(BIT_EINT8_23);
	ClearPending(BIT_EINT0);
	ClearPending(BIT_EINT2);
}

void KeyInit()
{
	MMU_Init();//初始化内存管理单元
	
	//设置IO口为外部中断模式
	rGPGCON |= (1<<1 | 1<<7 | 1<<11 | 1<<13 );
    //因为外部中断引脚和通用IO引脚F和G复用，在使用中断之前要把相应的引脚配置成中断模式
	rEXTINT1 = 0;//设置中断寄存器，设定EINT0-23的触发方式。
	
	//中断服务程序的入口地址,当中断发生时会自动进行跳转
	pISR_EINT0 = pISR_EINT2 = pISR_EINT8_23 = (U32)Key_ISR;
	
	//外部中断悬挂寄存器
	rEINTPEND |= (1<<11 | 1<<19);
	ClearPending(BIT_EINT0 | BIT_EINT2 | BIT_EINT8_23);
    //pending寄存器清零。清零表示无外部中断请求，1表示有中断请求
	
	//INTMASK寄存器用语外部中断屏蔽，被置1的位不能响应中断。打开按键的中断,enable eint 11,19
	rEINTMASK &= ~(1<<11 | 1<<19);
	
    //打开中断
	EnableIrq(BIT_EINT0 | BIT_EINT2 | BIT_EINT8_23);
}

static void Wait(void)//类似于小系统的时钟（周期为150个tick），不停的对状态进行监控，负责触发explode()
{
	int gct=GUI_GetTime();
    //GUI_GetTime()方法体在GUITime.c中，调用了GUI_X_GetTime()，而GUI_X_GetTime()调用了os_time.c中的OSTimeGet().发现最终获得的时间是tick，即系统节拍。
	if(gct-tt>150)
	{
		tt=gct;
		
		if(state==1)
            
		{
			if(timeout==0)
				explode();
			else
			{
				timeout--;
				show(timeout);
			}
		}
	}
}

void reset()
{
    GUI_SetColor(GUI_BLACK);
 	GUI_SetBkColor(GUI_WHITE);
    GUI_SetFont(&GUI_Font32_ASCII); 
 
  	GUI_ClearRect(0,0,LCD_XSIZE,LCD_YSIZE);
	state=0;
	timeout=INIT_TIMEOUT;
	passwordarray[0]=passwordarray[1]=0;
	GUI_DispStringAt("Set time:",20,10);
	show(timeout);
}

int num=0;

int Main(void)
{
	//设定密码
	defuse=11;
	
	//函数指针表初始化，分别表示不同状态下，按下不用按键调用的函数
	FunP[0][0]=s_down;//在0状态下按下0或1键会改数字
	FunP[0][1]=s_up;
	FunP[0][2]=s_arm;//在0状态下按2键会调用s_arm()进入状态1
    
    FunP[1][0]=t_down;
	FunP[1][1]=t_up;
	FunP[1][2]=t_arm;//在1状态下按2键会调用t_arm()进入状态1
	
	FunP[2][0]=FunP[2][1]=FunP[2][2]=reset;
	
    /*初始化2440目标板
     1. 设定系统运行时钟频率
     2. 端口初始化
     3. MMU初始化
     4. 串口初始化
     5. LED指示灯初始化
     */
	TargetInit(); 
	
	//初始化uC/OS-II内核
   	OSInit();	 
   	
   	//初始化系统时间
   	OSTimeSet(0);
   
    tt=GUI_GetTime();//tt记录了创建任务的时间
   	
   	//创建系统初始任务
    //OSTaskCreate()方法定义见uCOS_II/source/ucos_ii.h
    //INT8U OSTaskCreate(void (*task)(void *p_arg),void *p_arg, OS_STK *ptos, INT8U  prio);
    //具体实现见os_task.c文件中
  	OSTaskCreate (timeBomb,(void *)0, &MainTaskStk[MainTaskStkLengh - 1], MainTaskPrio);
  	
	OSStart();	
	return 0;
}

void timeBomb(void *pdata) //Main Task create taks0 and task1
{
#if OS_CRITICAL_METHOD == 3// Allocate storage for CPU status register 
	OS_CPU_SR  cpu_sr;
#endif

	OS_ENTER_CRITICAL();//屏蔽中断操作,定义见uCOS_II/arm/OS_CPU.H
		Timer0Init();//初始化定时器设置,见Timer.c。Initialize Timer0 use for ucos time tick
		ISRInit();   //设置中断控制器,initial interrupt prio or enable or disable
		KeyInit();   //本文件内方法
	OS_EXIT_CRITICAL();
	
	//print massage to Uart
	OSPrintfInit(); 
	OSStatInit();
	
	GUI_Init();   
  
	reset();
	
	while(1)
	{  
		Wait();
	}	
}
