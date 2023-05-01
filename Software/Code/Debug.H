/********************************** (C) COPYRIGHT *******************************
* File Name          : Debug.H
* Author             : WCH
* Version            : V1.0
* Date               : 2020/07/03
* Description        : This file contains all the functions prototypes for UART
*                      Printf , Delay functions.									 
*******************************************************************************/
#ifndef	__DEBUG_H__
#define __DEBUG_H__

#include <absacc.h>
#include <stdio.h>
#include <string.h>
#include <intrins.h>
#include "CH557X.H"

//���庯������ֵ
#ifndef  SUCCESS
#define  SUCCESS    0
#endif
#ifndef  FAIL
#define  FAIL       0xFF
#endif


#define	FREQ_SYS	                       48000000    //ϵͳ��Ƶ48MHz
#define  OSC_EN_XT                       0           //�ⲿ����ʹ�ܣ�Ĭ�Ͽ����ڲ�����
#ifndef  UART0_BUAD                                    
#define  UART0_BUAD                      115200        
#define  UART1_BUAD                      57600         
#endif                                                 
#ifndef  DE_PRINTF                                   //����Pirntf�������
#define  DE_PRINTF                       0
#endif

#ifndef MIN
#define MIN(a,b)                (((a) <= (b)) ? (a) : (b))
#endif

void CfgFsys(void);                                  //CH557ʱ��ѡ�������
void mDelayuS(UINT16 n);                             //��uSΪ��λ��ʱ
void mDelaymS(UINT16 n);                             //��mSΪ��λ��ʱ
void mInitSTDIO(void);                               //T1��Ϊ�����ʷ�����
  
#endif