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
	while(n){  // total = 12~13 Fsys cycles, 1uS @Fsys=12MHz
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
	while(n){
		mDelayuS(1000);
		-- n;
	}
}                                         

/*******************************************************************************
* Function Name  : mInitSTDIO
* Description    : CH557串口0初始化,默认使用T1作波特率发生器，也可使用T2
*                   -T1作UART0的波特率发生器：RCLK=0且TCLK=0
*                   -T2作UART0的波特率发生器：RCLK=1或TCLK=1               
* Input          : None
* Return         : None
*******************************************************************************/
void mInitSTDIO(void)
{
	UINT32 x;
	UINT8 x2; 

	/* 串口0使用模式1 */
	SM0 = 0;
	SM1 = 1;
	SM2 = 0;                                                                 
	
	/* 使用Timer1作为波特率发生器 */
	RCLK = 0;                                                                  //UART0接收时钟
	TCLK = 0;                                                                  //UART0发送时钟
	
	PCON |= SMOD;
	x = 10 * FREQ_SYS / UART0_BUAD / 16;                                       //如果更改主频，注意x的值不要溢出                            
	x2 = x % 10;
	x /= 10;
	if(x2 >= 5) x++;                                                       //四舍五入

	TMOD = TMOD & ~ bT1_GATE & ~ bT1_CT & ~ MASK_T1_MOD | bT1_M1;              //0X20，Timer1作为8位自动重载定时器
	T2MOD = T2MOD | bTMR_CLK | bT1_CLK;                                        //Timer1时钟选择
	TH1 = 0-x;                                                                 //12MHz晶振,buad/12为实际需设置波特率
	TR1 = 1;                                                                   //启动定时器1
	TI = 1;
	REN = 1;                                                                   //串口0接收使能
}

/*******************************************************************************
* Function Name  : CH557WDTModeSelect
* Description    : CH557看门狗模式选择
* Input          : mode：
*                   0：timer
*                   1：watchDog
* Return         : None
*******************************************************************************/
void CH557WDTModeSelect(UINT8 mode)
{
	SAFE_MOD = 0x55;
	SAFE_MOD = 0xaa;                                                             //进入安全模式
	if(mode){
	 GLOBAL_CFG |= bWDOG_EN;                                                    //启动看门狗复位
	}
	else GLOBAL_CFG &= ~bWDOG_EN;	                                              //启动看门狗仅仅作为定时器
	SAFE_MOD = 0x00;                                                             //退出安全模式
	WDOG_COUNT = 0;                                                              //看门狗赋初值
}

/*******************************************************************************
* Function Name  : CH557WDTFeed
* Description    : CH557看门狗喂狗
*                  看门狗复位时间(s) = (256-tim)/(Fsys/131072)
*                  00H(Fsys=12MHz)=2.8s
*                  80H(Fsys=12MHz)=1.4s
* Input          : tim：看门狗计数赋值
* Return         : None
*******************************************************************************/
void CH557WDTFeed(UINT8 tim)
{
	WDOG_COUNT = tim;                                                             //看门狗计数器赋值	
}
