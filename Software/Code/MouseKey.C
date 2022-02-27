/********************************** (C) COPYRIGHT *******************************
* File Name          : Mouse_Key.C
* Author             : Sirius_P
* Version            : V1.0
* Date               : 2022/02/011
* Description        : Ch557按键检测以及编码器检测
*******************************************************************************/
#include "MouseKey.H"

#pragma  NOAREGS

/*******************************************************************************
* Function Name  : GPIO_Init
* Description    : GPIO端口初始化函数
* Input          : PORTx:0~4
*                  PINx:位域,每个位对应该Port的一个引脚
*                  MODEx:
*                        0:高阻输入模式，引脚没有上拉电阻
*                        1:推挽输出模式，具有对称驱动能力，可以输出或者吸收较大电流
*                        2:开漏输出，支持高阻输入，引脚没有上拉电阻
*                        3:准双向模式(标准 8051)，开漏输出，支持输入，引脚有上拉电阻(默认模式)
* Return         : None
*******************************************************************************/
void GPIO_Init(UINT8 PORTx, UINT8 PINx, UINT8 MODEx)
{
    UINT8 Px_DIR_PU, Px_MOD_OC;

    switch(PORTx)                                       //读出初始值
    {
    case PORT0:
        Px_MOD_OC = P0_MOD_OC;
        Px_DIR_PU = P0_DIR_PU;
        break;
    case PORT1:
        Px_MOD_OC = P1_MOD_OC;
        Px_DIR_PU = P1_DIR_PU;
        break;
    case PORT2:
        Px_MOD_OC = P2_MOD_OC;
        Px_DIR_PU = P2_DIR_PU;
        break;
    case PORT3:
        Px_MOD_OC = P3_MOD_OC;
        Px_DIR_PU = P3_DIR_PU;
        break;
    case PORT4:
        Px_MOD_OC = P4_MOD_OC;
        Px_DIR_PU = P4_DIR_PU;
        break;
    default :
        break;
    }

    switch(MODEx)
    {
    case MODE0:                                           //高阻输入模式，引脚没有上拉电阻
        Px_MOD_OC &= ~PINx;
        Px_DIR_PU &= ~PINx;
        break;
    case MODE1:                                           //推挽输出模式，具有对称驱动能力，可以输出或者吸收较大电流
        Px_MOD_OC &= ~PINx;
        Px_DIR_PU |= PINx;
        break;
    case MODE2:                                           //开漏输出，支持高阻输入，引脚没有上拉电阻
        Px_MOD_OC |= PINx;
        Px_DIR_PU &= ~PINx;
        break;
    case MODE3:                                           //准双向模式(标准 8051)，开漏输出，支持输入，引脚有上拉电阻
        Px_MOD_OC |= PINx;
        Px_DIR_PU |= PINx;
        break;
    default :
        break;
    }
    switch(PORTx)                                         //回写
    {
    case PORT0:
        P0_MOD_OC = Px_MOD_OC;
        P0_DIR_PU = Px_DIR_PU;
        break;
    case PORT1:
        P1_MOD_OC = Px_MOD_OC;
        P1_DIR_PU = Px_DIR_PU;
        break;
    case PORT2:
        P2_MOD_OC = Px_MOD_OC;
        P2_DIR_PU = Px_DIR_PU;
        break;
    case PORT3:
        P3_MOD_OC = Px_MOD_OC;
        P3_DIR_PU = Px_DIR_PU;
        break;
    case PORT4:
        P4_MOD_OC = Px_MOD_OC;
        P4_DIR_PU = Px_DIR_PU;
        break;
    default :
        break;
    }
}

/*******************************************************************************
* Function Name  : GPIO_INT_Init
* Description    : 可设置 RXD1_L、P15_L、P14_L、P03_L、RXD0_L、P0.0～P0.7、P1.0～P1.3、P2.0～P2.3、P4.0～P4.7 扩展引脚的外部中断
*                  同时还包含兼容C51的 INT1_L、INT0_L 的外部中断触发
*                  (RXD1_L、RXD0_L具体是哪个引脚取决于引脚是否映射)
* Input          : IntSrc:共9个外部中断，按位域表示，具体定义见GPIO.H
*                  Mode：0：电平模式   1：边沿模式 (注意扩展引脚的中断模式是统一配置的)
*                  NewState：0：关闭对应外部中断使能  1：开启对应外部中断
* Return         : None
*******************************************************************************/
void GPIO_INT_Init( UINT16 IntSrc, UINT8 Mode, UINT8 NewState )
{
    /* 中断触发模式设置 */
    if(IntSrc & 0x7F || IntSrc & 0xF000)            //存在扩展中断
    {
        SAFE_MOD = 0x55;                                    //进入安全模式
        SAFE_MOD = 0xAA;
        if(Mode == INT_EDGE)                                //边沿触发模式
        {
            GPIO_IE |= bIE_IO_EDGE;
        }
        else                                                //电平触发模式
        {
            GPIO_IE &= ~bIE_IO_EDGE;
        }

    }

    if(IntSrc & INT_INT0_L)                         //存在外部中断0
    {
        IT0 = 1;
    }
    if(IntSrc & INT_INT1_L)                         //存在外部中断1
    {
        IT1 = 1;
    }

    SAFE_MOD = 0x55;                                    //进入安全模式
    SAFE_MOD = 0xAA;
    /* 中断使能状态 */
    if(NewState == Enable)                              //开启外部中断
    {
        GPIO_IE |= ((UINT8)IntSrc & 0x7F);
        if(IntSrc & INT_INT0_L)                         //存在外部中断0
        {
            EX0 = 1;
        }
        if(IntSrc & INT_INT1_L)                         //存在外部中断1
        {
            EX1 = 1;
        }
        if(IntSrc & 0xF000)
        {
            IntSrc = IntSrc / 256;
            PORT_CFG |= (UINT8)IntSrc & 0xFF;
        }
        IE_GPIO = 1;                                    //开启扩展GPIO中断
        EA = 1;                                         //开启总中断
    }
    else                                               //关闭对应外部中断
    {
        GPIO_IE &= ~((UINT8)IntSrc & 0x7F);
        if(IntSrc & INT_INT0_L)                         //存在外部中断0
        {
            EX0 = 0;
        }
        if(IntSrc & INT_INT1_L)                         //存在外部中断1
        {
            EX1 = 0;
        }
        if(IntSrc & 0xF000)
        {
            IntSrc = IntSrc / 256;
            PORT_CFG |= ~(UINT8)IntSrc & 0xFF;
        }
        IE_GPIO = 0;                                   //关闭扩展GPIO中断
    }
}
