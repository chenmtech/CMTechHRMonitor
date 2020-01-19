/*
 * App_ECGFunc.h : 心电采集功能应用程序头文件
*/


#ifndef APP_ECGFUNC_H
#define APP_ECGFUNC_H

extern void ECGFunc_Init(); // ECG功能初始化
extern void ECGFunc_StartEcg(); // 启动采集ECG信号
extern void ECGFunc_Start1mV(); // 启动采集1mV定标信号
extern void ECGFunc_Stop(); // 停止采集

#endif
