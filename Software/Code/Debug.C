/********************************** (C) COPYRIGHT *******************************
* File Name          : Debug.C
* Author             : WCH
* Version            : V1.4
* Date               : 2021/12/15
* Description        : This file contains all the functions prototypes for UART
*                      Printf , Delay functions.
*******************************************************************************/
#include "Debug.H"

/*******************************************************************************
* Function Name  : CfgFsys
* Description    : CH557时钟选择和配置函数,复位默认使用内部晶振，Fsys=12MHz，
*                   FREQ_SYS可以通过CLOCK_CFG配置得到，公式如下：
*                   Fsys = (Fosc * 4/(CLOCK_CFG & MASK_SYS_CK_SEL);具体时钟需要自己配置
* Input          : None
* Return         : None
*******************************************************************************/
void CfgFsys(void)
{
#if OSC_EN_XT
    P7 = P7 & 0xF0 | 0x06;							  //启动外部晶振前,P7.0处于低电平,P7.1处于上拉状态
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;
    CLOCK_CFG |= bOSC_EN_XT;                          //使能外部晶振
    mDelaymS(10);
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;
    CLOCK_CFG &= ~bOSC_EN_INT;                        //关闭内部晶振
    SAFE_MOD = 0x00;
#endif

    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;
#if FREQ_SYS == 48000000
    CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x07;  // 48MHz

#endif

#if FREQ_SYS == 32000000
    CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x06;  // 32MHz

#endif

#if FREQ_SYS == 24000000
    CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x05;  // 24MHz

#endif

#if FREQ_SYS == 16000000
    CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x04;  // 16MHz

#endif

#if FREQ_SYS == 12000000
    CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x03;  // 12MHz

#endif

#if FREQ_SYS == 3000000
    CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x02;  // 3MHz

#endif

#if FREQ_SYS == 750000
    CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x01;  // 750KHz

#endif

#if FREQ_SYS == 187500
    CLOCK_CFG = CLOCK_CFG & ~ MASK_SYS_CK_SEL | 0x00;  // 187.5KHz

#endif

    SAFE_MOD = 0x00;
}

/*******************************************************************************
* Function Name  : mDelayus
* Description    : us延时函数，以uS为单位延时
* Input          : n
* Return         : None
*******************************************************************************/
void mDelayuS(UINT16 n)
{
#ifdef	FREQ_SYS
#if		FREQ_SYS <= 6000000
    n >>= 2;
#endif
#if		FREQ_SYS <= 3000000
    n >>= 2;
#endif
#if		FREQ_SYS <= 750000
    n >>= 4;
#endif
#endif
    while(n)   // total = 12~13 Fsys cycles, 1uS @Fsys=12MHz
    {
        ++ SAFE_MOD;  // 2 Fsys cycles, for higher Fsys, add operation here
#ifdef	FREQ_SYS
#if		FREQ_SYS >= 14000000
        ++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 16000000
        ++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 18000000
        ++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 20000000
        ++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 22000000
        ++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 24000000
        ++ SAFE_MOD;
#endif
#endif
        -- n;
    }
}

/*******************************************************************************
* Function Name  : mDelayms
* Description    : ms延时函数，以mS为单位延时
* Input          : n
* Return         : None
*******************************************************************************/
void mDelaymS(UINT16 n)
{
    while(n)
    {
        mDelayuS(1000);
        -- n;
    }
}
