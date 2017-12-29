/*-/////////////////////////////////////////////////////////////
define the Tasks' Stk lengh Prio Function on the project
////////////////////////////////////////////////////////////-*/
#ifndef APP_CFG_H
#define APP_CFG_H


//Task Stk lengh

#define	MainTaskStkLengh	2048   	// Define the MainTask stack length
#define	TaskUartStkLengh	1024    	// Define the TaskUart stack length


//Task function
void    timeBomb(void *pdata);   		//MainTask task
void    TaskUart(void *pdata);          //Task Uart


//Task Prio
#define NormalTaskPrio  5
#define MainTaskPrio 	NormalTaskPrio
#define TaskUartPrio  	NormalTaskPrio+1



#endif