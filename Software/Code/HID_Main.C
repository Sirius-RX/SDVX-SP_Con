/********************************** (C) COPYRIGHT *******************************
* File Name          : CompositeKM.C
* Author             : Sirius_P
* Version            : V1.1
* Date               : 2022/02/05
* Description        : CH557模拟USB键鼠复合设备,支持类命令,支持唤醒
                       演示键盘鼠标简单操作。其他键值，参考 HID USAGE TABLE协议文档
							  任意字符：主机睡眠状态下,设备远程唤醒主机（注意设备一般需自供电,因为主机休眠可能USB口也会掉电）
*******************************************************************************/
#include "DEBUG.H"
#include "MouseKey.H"

#define Fullspeed//全速USB模式选择，如使用半速则注释

#ifdef  Fullspeed
#define THIS_ENDP0_SIZE         64
#else
#define THIS_ENDP0_SIZE         8          //低速USB，中断传输、控制传输最大包长度为8
#endif

#define ENDP1_IN_SIZE           2          //键盘端点数据包大小
#define ENDP2_IN_SIZE           2          //鼠标端点数据包大小
#define Sense                   1          //旋钮灵敏度

UINT8X  Ep0Buffer[MIN(64, THIS_ENDP0_SIZE + 2)] _at_ 0x0000;                  //端点0 OUT&IN缓冲区，必须是偶地址
UINT8X  Ep1Buffer[MIN(64, ENDP1_IN_SIZE)]     _at_ MIN(64, THIS_ENDP0_SIZE + 2); //端点1 IN缓冲区,必须是偶地址
UINT8X  Ep2Buffer[MIN(64, ENDP2_IN_SIZE)]     _at_ (MIN(64, THIS_ENDP0_SIZE + 2) + MIN(64, ENDP1_IN_SIZE + 2)); //端点2 IN缓冲区,必须是偶地址

UINT8   SetupReq, Ready, UsbConfig;

#define UsbSetupBuf     ((PUSB_SETUP_REQ)Ep0Buffer)

#pragma NOAREGS

/*设备描述符*/
UINT8C DevDesc[] =
{
    0x12,                                  //bLength
    0x01,                                  //bDescriptorType
    0x10, 0x01,                            //bcdUSB
    0x00,                                  //bDeviceClass
    0x00,                                  //bDeviceSubClass
    0x00,                                  //bDeviceProtocol
    THIS_ENDP0_SIZE,                       //bMAXPacketSize0
    0x86, 0x1a,                            //idVendor
    0xe1, 0xe6,                            //idProduct
    0x00, 0x01,                            //bcdDevice
    0x01,                                  //iManufacturer
    0x02,                                  //iProduct
    0x00,                                  //iSerialNumber
    0x01                                   //bNumConfigurations
};
/*字符串描述符*/
UINT8C  MyLangDescr[] = { 0x04, 0x03, 0x09, 0x04 };                                     // 语言描述符
UINT8C  MyManuInfo[] = { 0x0E, 0x03, 'S', 0, 'i', 0, 'r', 0, 'i', 0, 'u', 0, 's', 0 };  // 厂家信息
UINT8C  MyProdInfo[] = { 0x0C, 0x03, 'S', 0, 'P', 0, 'C', 0, 'o', 0, 'n', 0 };          // 产品信息
/*HID类报表描述符*/
UINT8C KeyRepDesc[] =
{
    0x05, 0x01,                                        //Usage Page (Generic Desktop)
    0x09, 0x06,                                        //Usage (Keyboard)
    0xA1, 0x01,                                        //Collection (Application)
    /*上传数据*/
    //第1-2字节前3比特
    0x05, 0x07,                                        //Usage Page (Keyboard/Keypad)
    0x19, 0x04,                                        //Usage Minimum
    0x29, 0x0E,                                        //Usage Maximum
    0x15, 0x00,                                        //Logical Minimum
    0x25, 0x01,                                        //Logical Maximum
    0x95, 0x0B,                                        //Report Count
    0x75, 0x01,                                        //Report Size
    0x81, 0x02,                                        //Input (Data,Variable,Absolute)
    //第2字节后5比特
    0x95, 0x05,                                        //Report Count
    0x75, 0x01,                                        //Report Size
    0x81, 0x01,                                        //Input (Constant)
    0xC0                                               //End Collection
};
UINT8C MouseRepDesc[] =
{
    0x05, 0x01,                                        //Usage Page (Generic Desktop)
    0x09, 0x02,                                        //Usage (Mouse)
    0xA1, 0x01,                                        //Collection (Application)
    /*上传数据*/
    //第1-2字节
    0x05, 0x01,                                      	 //Usage Page (Generic Desktop)
    0x09, 0x30,                                        //Local X
    0x09, 0x31,                                        //Local Y
    0x15, 0x81,                                        //Logical Minimum
    0x25, 0x7f,                                        //Logical Maximum
    0x95, 0x02,                                        //Report Count
    0x75, 0x08,                                        //Report Size
    0x81, 0x06,                                        //Input (Data,Variable,Relative)
    0xC0                                               //End Collection
};
/*配置描述符*/
UINT8C CfgDesc[] =
{
    /*配置描述符*/
    0x09,                                              //bLength
    0x02,                                              //bDescroptorType
    0x3b, 0x00,                                        //wTotalLength
    0x02,                                              //bNumInterfaces
    0x01,                                              //bConfigurationValue
    0x00,                                              //iConfiguration
    0xA0,                                              //bmAttributes
    0x96,                                              //MaxPower

    /*接口描述符，键盘功能*/
    0x09,                                              //bLength
    0x04,                                              //bDescriptorType
    0x00,                                              //bInterfaceNumber
    0x00,                                              //bAlternateSetting
    0x01,                                              //bNumEndpoints
    0x03,                                              //bInterfaceClass
    0x01,                                              //bInterfaceSubClass
    0x01,                                              //bInterfaceProtocol
    0x00,                                              //iInterface
    /*HID类描述符*/
    0x09,                                              //bLength
    0x21,                                              //bDescriptorType
    0x11, 0x01,                                        //bcdHID
    0x00,                                              //bCountryCode
    0x01,                                              //bNumDescriptors
    0x22,                                              //DescriptorType
    sizeof(KeyRepDesc) & 0xFF, sizeof(KeyRepDesc) >> 8, //wDescriptorLength
    /*端点描述符*/
    0x07,                                              //bLength
    0x05,                                              //bDescriptorType
    0x81,                                              //bEndpointAddress
    0x03,                                              //bmAttributes
    ENDP1_IN_SIZE, 0x00,                               //wMaxPacketSize
    0x0a,                                              //bInterval,ms
	 
    /*接口描述符，鼠标功能*/
    0x09,                                              //bLength
    0x04,                                              //bDescriptorType
    0x01,                                              //bInterfaceNumber
    0x00,                                              //bAlternateSetting
    0x01,                                              //bNumEndpoints
    0x03,                                              //bInterfaceClass
    0x01,                                              //bInterfaceSubClass
    0x02,                                              //bInterfaceProtocol
    0x00,                                              //iInterface
    /*HID类描述符*/
    0x09,                                              //bLength
    0x21,                                              //bDescriptorType
    0x10, 0x01,                                        //bcdHID
    0x00,                                              //bCountryCode
    0x01,                                              //bNumDescriptors
    0x22,                                              //DescriptorType
    sizeof(MouseRepDesc) & 0xFF, sizeof(MouseRepDesc) >> 8, //wDescriptorLength
    /*端点描述符*/
    0x07,                                              //bLength
    0x05,                                              //bDescriptorType
    0x82,                                              //bEndpointAddress
    0x03,                                              //bmAttributes
    ENDP2_IN_SIZE, 0x00,                               //wMaxPacketSize
    0x0a                                               //bInterval,ms
};
/*键盘数据*/
UINT8 HIDKey[ENDP1_IN_SIZE];
/*鼠标数据*/
UINT8 HIDMouse[2] = { 0x0, 0x0 };
UINT8 Endp1Busy = 0;                                  //传输完成控制标志位
UINT8 Endp2Busy = 0;
UINT8 WakeUpEnFlag = 0;                               //远程唤醒使能标志

/*******************************************************************************
* Function Name  : Enp1IntIn
* Description    : USB设备模式端点1的中断上传
* Input          : *buf: 上传数据
*                  len:上传数据长度
* Return         : None
*******************************************************************************/
void Enp1IntIn( UINT8 *buf, UINT8 len )
{
    memcpy( Ep1Buffer, buf, len );                                            //加载上传数据
    UEP1_T_LEN = len;                                                         //上传数据长度
    UEP1_CTRL = UEP1_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_ACK;                 //有数据时上传数据并应答ACK
}

/*******************************************************************************
* Function Name  : Enp2IntIn1
* Description    : USB设备模式端点2的中断上传函数
* Input          : *buf: 上传数据
*                  len:上传数据长度
* Return         : None
*******************************************************************************/
void Enp2IntIn( UINT8 *buf, UINT8 len )
{
    memcpy( Ep2Buffer, buf, len);                                             //加载上传数据
    UEP2_T_LEN = len;                                                         //上传数据长度
    UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_ACK;                 //有数据时上传数据并应答ACK
}

/*******************************************************************************
* Function Name  : USB_DeviceInterrupt
* Description    : USB中断服务程序
* Input          : None
* Return         : None
*******************************************************************************/
void	USB_DeviceInterrupt(void) interrupt INT_NO_USB using 1
{
    UINT8	len;
    static	UINT8	SetupReqCode;
    static  UINT16 SetupLen;
    static	PUINT8	pDescr;
    if(UIF_TRANSFER)  // USB传输完成
    {
USB_DevIntNext:
        switch(USB_INT_ST & ( bUIS_SETUP_ACT | MASK_UIS_TOKEN | MASK_UIS_ENDP ))  //分析操作令牌和端点号
        {
        case UIS_TOKEN_IN | 2:  // endpoint 2# 批量端点上传
        case bUIS_SETUP_ACT | UIS_TOKEN_IN | 2:
            UEP2_T_LEN = 0;                                                       //预使用发送长度一定要清空
            UEP2_CTRL ^= bUEP_T_TOG;                                              //手动翻转
            Endp2Busy = 0;                                                        
            UEP2_CTRL = UEP2_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_NAK;             //默认应答NAK
            break;                                                                
        case UIS_TOKEN_IN | 1:  // endpoint 1# 中断端点上传                       
        case bUIS_SETUP_ACT | UIS_TOKEN_IN | 1:                                   
            UEP1_T_LEN = 0;                                                       //预使用发送长度一定要清空
            UEP1_CTRL ^= bUEP_T_TOG;                                              //手动翻转
            Endp1Busy = 0;                                                        
            UEP1_CTRL = UEP1_CTRL & ~ MASK_UEP_T_RES | UEP_T_RES_NAK;             //默认应答NAK
            break;                                                                
        case UIS_TOKEN_IN | 0:  // endpoint 0# IN
        case bUIS_SETUP_ACT | UIS_TOKEN_IN | 0:
            switch(SetupReqCode)
            {
            case USB_GET_DESCRIPTOR:
                len = SetupLen >= THIS_ENDP0_SIZE ? THIS_ENDP0_SIZE : SetupLen;   //本次传输长度
                memcpy( Ep0Buffer, pDescr, len );  /* 加载上传数据 */
                SetupLen -= len;
                pDescr += len;
                UEP0_T_LEN = len;
                UEP0_CTRL ^= bUEP_T_TOG;  // 翻转
                break;
            case USB_SET_ADDRESS:
                USB_DEV_AD = USB_DEV_AD & bUDA_GP_BIT | SetupLen;
                UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                break;
            default:
                UEP0_T_LEN = 0;  // 状态阶段完成中断或者是强制上传0长度数据包结束控制传输
                UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
                break;
            }
            break;
        case UIS_TOKEN_OUT | 0:  // endpoint 0# OUT
        case bUIS_SETUP_ACT | UIS_TOKEN_OUT | 0:
            UEP0_CTRL ^= bUEP_R_TOG;                                      //同步标志位翻转
            break;
        default:
            if ((USB_INT_ST & (bUIS_SETUP_ACT | MASK_UIS_TOKEN)) == (bUIS_SETUP_ACT | UIS_TOKEN_FREE))  // endpoint 0# SETUP
            {
                UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;
                SetupLen = ((UINT16)UsbSetupBuf->wLengthH << 8) + UsbSetupBuf->wLengthL;
                len = 0;
                SetupReqCode = UsbSetupBuf->bRequest;
                if( UsbSetupBuf->wLengthH || SetupLen > 0x7F) SetupLen = 0x7F;  // 限制总长度
                len = 0;  // 默认为成功并且上传0长度
                if ((UsbSetupBuf->bRequestType & USB_REQ_TYP_MASK) != USB_REQ_TYP_STANDARD)
                {
                    switch(SetupReqCode)
                    {
                    case 0x01://GetReport
                        break;
                    case 0x02://GetIdle
                        break;
                    case 0x03://GetProtocol
                        break;
                    case 0x09://SetReport
                        break;
                    case 0x0A://SetIdle
                        break;
                    case 0x0B://SetProtocol
                        break;
                    default:
                        len = 0xFFFF;                                  /*命令不支持*/
                        break;
                    }
                }
                else  // 标准请求
                {
                    switch(SetupReqCode)  // 请求码
                    {
                    case USB_GET_DESCRIPTOR:
                        switch(UsbSetupBuf->wValueH )
                        {
                        case 1:  // 设备描述符
                            pDescr = (PUINT8)( &DevDesc[0] );
                            len = sizeof( DevDesc );
                            break;
                        case 2:  // 配置描述符
                            pDescr = (PUINT8)( &CfgDesc[0] );
                            len = sizeof( CfgDesc );
                            break;
                        case 3:  // 字符串描述符
                            switch(UsbSetupBuf->wValueL)
                            {
                            case 1:
                                pDescr = (PUINT8)( &MyManuInfo[0] );
                                len = sizeof( MyManuInfo );
                                break;
                            case 2:
                                pDescr = (PUINT8)( &MyProdInfo[0] );
                                len = sizeof( MyProdInfo );
                                break;
                            case 0:
                                pDescr = (PUINT8)( &MyLangDescr[0] );
                                len = sizeof( MyLangDescr );
                                break;
                            default:
                                len = 0xFF;  // 不支持的字符串描述符
                                break;
                            }
                            break;
                        case USB_DESCR_TYP_REPORT:
                            if(UsbSetupBuf->wIndexL == 0)                   //接口0报表描述符
                            {
                                pDescr = KeyRepDesc;                        //数据准备上传
                                len = sizeof(KeyRepDesc);
                            }
                            else if(UsbSetupBuf->wIndexL == 1)              //接口1报表描述符
                            {
                                pDescr = MouseRepDesc;                      //数据准备上传
                                len = sizeof(MouseRepDesc);
                            }
                            else
                            {
                                len = 0xFFFF;                                 //本程序只有2个接口，这句话正常不可能执行
                            }
                            break;
                        default:
                            len = 0xFF;  // 不支持的描述符类型
                            break;
                        }
                        if(SetupLen > len) SetupLen = len;  // 限制总长度
                        len = SetupLen >= THIS_ENDP0_SIZE ? THIS_ENDP0_SIZE : SetupLen;  // 本次传输长度
                        memcpy(Ep0Buffer, pDescr, len);  /* 加载上传数据 */
                        SetupLen -= len;
                        pDescr += len;
                        break;
                    case USB_SET_ADDRESS:
                        SetupLen = UsbSetupBuf->wValueL;  // 暂存USB设备地址
                        break;
                    case USB_GET_CONFIGURATION:
                        Ep0Buffer[0] = UsbConfig;
                        if ( SetupLen >= 1 ) len = 1;
                        break;
                    case USB_SET_CONFIGURATION:
                        UsbConfig = UsbSetupBuf->wValueL;
                        if(UsbConfig)
                        {
                            Ready = 1;                                                   //set config命令一般代表usb枚举完成的标志
                        }
                        break;
                    case USB_CLEAR_FEATURE:
                        if ((UsbSetupBuf->bRequestType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_ENDP)  // 端点
                        {
                            switch(UsbSetupBuf->wIndexL)
                            {
                            case 0x82:
                                UEP2_CTRL = UEP2_CTRL & ~ ( bUEP_T_TOG | MASK_UEP_T_RES ) | UEP_T_RES_NAK;
                                break;
                            case 0x02:
                                UEP2_CTRL = UEP2_CTRL & ~ ( bUEP_R_TOG | MASK_UEP_R_RES ) | UEP_R_RES_ACK;
                                break;
                            case 0x81:
                                UEP1_CTRL = UEP1_CTRL & ~ ( bUEP_T_TOG | MASK_UEP_T_RES ) | UEP_T_RES_NAK;
                                break;
                            case 0x01:
                                UEP1_CTRL = UEP1_CTRL & ~ ( bUEP_R_TOG | MASK_UEP_R_RES ) | UEP_R_RES_ACK;
                                break;
                            default:
                                len = 0xFF;  // 不支持的端点
                                break;
                            }
                        }
                        else if((UsbSetupBuf->bRequestType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_DEVICE)// 设备
                        {
                            WakeUpEnFlag = 0;
                            break;
                        }
                        else
                        {
                            len = 0xFFFF;                                                // 不是端点不支持
                        }
                        break;
                    case USB_SET_FEATURE:                                               /* Set Feature */
                        if((UsbSetupBuf->bRequestType & 0x1F) == 0x00)                    /* 设置设备 */
                        {
                            if(((( UINT16 )UsbSetupBuf->wValueH << 8 ) | UsbSetupBuf->wValueL) == 0x01)
                            {
                                if(CfgDesc[ 7 ] & 0x20)
                                {
                                    WakeUpEnFlag = 1;                                   /* 设置唤醒使能标志 */
                                }
                                else
                                {
                                    len = 0xFFFF;                                        /* 操作失败 */
                                }
                            }
                            else
                            {
                                len = 0xFFFF;                                            /* 操作失败 */
                            }
                        }
                        else if((UsbSetupBuf->bRequestType & 0x1F) == 0x02)        /* 设置端点 */
                        {
                            if((((UINT16)UsbSetupBuf->wValueH << 8 ) | UsbSetupBuf->wValueL) == 0x00)
                            {
                                switch(((UINT16)UsbSetupBuf->wIndexH << 8) | UsbSetupBuf->wIndexL)
                                {
                                case 0x82:
                                    UEP2_CTRL = UEP2_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;/* 设置端点2 IN STALL */
                                    break;
                                case 0x02:
                                    UEP2_CTRL = UEP2_CTRL & (~bUEP_R_TOG) | UEP_R_RES_STALL;/* 设置端点2 OUT Stall */
                                    break;
                                case 0x81:
                                    UEP1_CTRL = UEP1_CTRL & (~bUEP_T_TOG) | UEP_T_RES_STALL;/* 设置端点1 IN STALL */
                                    break;
                                default:
                                    len = 0xFFFF;                               //操作失败
                                    break;
                                }
                            }
                            else
                            {
                                len = 0xFFFF;                                   //操作失败
                            }
                        }
                        else
                        {
                            len = 0xFFFF;                                      //操作失败
                        }
                        break;
                    case USB_GET_INTERFACE:
                        Ep0Buffer[0] = 0x00;
                        if(SetupLen >= 1) len = 1;
                        break;
                    case USB_GET_STATUS:
                        Ep0Buffer[0] = 0x00;
                        Ep0Buffer[1] = 0x00;
                        if ( SetupLen >= 2 ) len = 2;
                        else len = SetupLen;
                        break;
                    default:
                        len = 0xFF;  // 操作失败
                        break;
                    }
                }
                if(len == 0xFF)  // 操作失败
                {
                    SetupReqCode = 0xFF;
                    UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL;  // STALL
                }
                else if(len <= THIS_ENDP0_SIZE)  // 上传数据或者状态阶段返回0长度包
                {
                    UEP0_T_LEN = len;
                    UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;  // 默认数据包是DATA1
                }
                else  // 下传数据或其它
                {
                    UEP0_T_LEN = 0;  // 虽然尚未到状态阶段，但是提前预置上传0长度数据包以防主机提前进入状态阶段
                    UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;  // 默认数据包是DATA1
                }
                break;
            }
            else
            {
                break;
            }
        }

        UIF_TRANSFER = 0;  // 清中断标志
    }
    else if(UIF_BUS_RST)  // USB总线复位
    {
        UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        UEP1_CTRL = UEP_T_RES_NAK;
        UEP2_CTRL = UEP_T_RES_NAK;
        USB_DEV_AD = 0x00;
        UIF_SUSPEND = 0;
        UIF_TRANSFER = 0;
        Ready = 0;
        UIF_BUS_RST = 0;                                                 //清中断标志
    }
    else if(UIF_SUSPEND)  // USB总线挂起/唤醒完成
    {
        UIF_SUSPEND = 0;
        if (USB_MIS_ST & bUMS_SUSPEND)  // 挂起
        {

        }
        else  // 唤醒
        {

        }
    }
    else  // 意外的中断,不可能发生的情况
    {
        USB_INT_FG = 0xFF;  // 清中断标志
    }
    if(UIF_TRANSFER) goto USB_DevIntNext;

}

/*******************************************************************************
* Function Name  : InitUSB_Device
* Description    : USB设备初始化
* Input          : None
* Return         : None
*******************************************************************************/
void	InitUSB_Device(void)  // 初始化USB设备
{
    IE_USB = 0;
    USB_CTRL = 0x00;  // 先设定模式
    UEP1_T_LEN = 0;                                                            //预使用发送长度一定要清空
    UEP2_T_LEN = 0;                                                            //预使用发送长度一定要清空
    UEP4_1_MOD &= ~(bUEP4_RX_EN | bUEP4_TX_EN);                                //端点0单64字节收发缓冲区
    UEP4_1_MOD = UEP4_1_MOD & ~bUEP1_BUF_MOD | bUEP1_TX_EN;                    // 端点1上传IN
    UEP2_3_MOD = UEP2_3_MOD & ~bUEP2_BUF_MOD | bUEP2_TX_EN;                    // 端点2上传IN
    UEP0_DMA = Ep0Buffer;
    UEP1_DMA = Ep1Buffer;
    UEP2_DMA = Ep2Buffer;
    USB_DEV_AD = 0x00;
    UDEV_CTRL &= ~ bUD_PD_EN;  // 禁止DP/DM下拉电阻

#ifndef Fullspeed
    UDEV_CTRL |= bUD_LOW_SPEED;                                                //选择低速1.5M模式
    USB_CTRL |= bUC_LOW_SPEED;
#else
    UDEV_CTRL &= ~bUD_LOW_SPEED;                                               //选择全速12M模式，默认方式
    USB_CTRL &= ~bUC_LOW_SPEED;
#endif

    USB_CTRL |= bUC_DEV_PU_EN | bUC_INT_BUSY;  // 启动USB设备及DMA，在中断期间中断标志未清除前自动返回NAK
    UDEV_CTRL |= bUD_PORT_EN;  // 允许USB端口
    USB_INT_FG = 0xFF;  // 清中断标志
    USB_INT_EN = bUIE_SUSPEND | bUIE_TRANSFER | bUIE_BUS_RST;
    IE_USB = 1;
}

/*******************************************************************************
* Function Name  : HIDValueHandle
* Description    : HID键值处理
* Input          : None
* Return         : None
*******************************************************************************/
void HIDValueHandle()
{
    HIDKey[0] = P0;
    HIDKey[1] = P1;
    while( Endp1Busy )
    {
        ;    //如果忙（上一包数据没有传上去），则等待。
    }
    Endp1Busy = 1;                                               //设置为忙状态
    Enp1IntIn(HIDKey, sizeof(HIDKey));
    if(HIDKey[0] != 0 || HIDKey[1] != 0)
    {
        while( Endp2Busy )
        {
            ;    //如果忙（上一包数据没有传上去），则等待。
        }
        Endp2Busy = 1;
        Enp2IntIn(HIDMouse, sizeof(HIDMouse));
        HIDMouse[0] = 0;
        HIDMouse[1] = 0;
    }
}

/*******************************************************************************
* Function Name  : GPIO_STD0_ISR
* Description    : INT0(P32) 引脚外部中断服务函数
* Input          : None
* Return         : None
*******************************************************************************/
void GPIO_STD0_ISR(void) interrupt INT_NO_INT0
{
    if(P3_4)
    {
        HIDMouse[0] = HIDMouse[0] + Sense;
    }
    if(!P3_4)
    {
        HIDMouse[0] = HIDMouse[0] - Sense;
    }
}

/*******************************************************************************
* Function Name  : GPIO_STD1_ISR
* Description    : INT1(P33) 引脚外部中断服务函数
* Input          : None
* Return         : None
*******************************************************************************/
void GPIO_STD1_ISR(void) interrupt INT_NO_INT1
{
    if(P3_5)
    {
        HIDMouse[1] = HIDMouse[1] + Sense;
    }
    if(!P3_5)
    {
        HIDMouse[1] = HIDMouse[1] - Sense;
    }
}

/*******************************************************************************
* Function Name  : main
* Description    : Main program
* Input          : None
* Return         : None
*******************************************************************************/
void main(void)
{
    CfgFsys( );                                                           //CH557时钟选择配置
    mDelaymS(20);                                                         //修改主频等待内部晶振稳定,必加
    GPIO_Init(PORT0, PIN0 | PIN1 | PIN2 | PIN3 | PIN4 | PIN5 | PIN6 | PIN7, MODE0);
    GPIO_Init(PORT1, PIN0 | PIN1 | PIN2, MODE0);
    GPIO_Init(PORT3, PIN2, MODE0);
    GPIO_Init(PORT3, PIN3, MODE0);
    GPIO_Init(PORT3, PIN4, MODE0);
    GPIO_Init(PORT3, PIN5, MODE0);
    GPIO_INT_Init((INT_INT0_L | INT_INT1_L), INT_EDGE, Enable);           //外部中断配置INT0\INT1
    InitUSB_Device();                                                     //USB设备模式初始化
    IP_EX = bIP_USB;                                                      //调整USB中断优先级
    memset(HIDKey, 0, sizeof(HIDKey));                                    //清空缓冲区
    memset(HIDMouse, 0, sizeof(HIDMouse));

    while(1)
    {
        if(Ready)
        {
            HIDValueHandle();
        }
    }
}