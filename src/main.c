/*******************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only
* intended for use with Renesas products. No other uses are authorized. This
* software is owned by Renesas Electronics Corporation and is protected under
* all applicable laws, including copyright laws.
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
* THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT
* LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
* AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED.
* TO THE MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS
* ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES SHALL BE LIABLE
* FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR
* ANY REASON RELATED TO THIS SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE
* BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software
* and to discontinue the availability of this software. By using this software,
* you agree to the additional terms and conditions found by accessing the
* following link:
* http://www.renesas.com/disclaimer *
* Copyright (C) 2014 Renesas Electronics Corporation
* and Renesas Solutions Corp. All rights reserved.
*******************************************************************************/
/*******************************************************************************
* File Name    : main.c
* Version      : 1.00
* Device(s)    : Renesas R5F564MxxDxx
* Tool-Chain   : Renesas e2 studio v3.0.1.09 or later
*              : C/C++ Compiler Package for RX Family V2.02.00 or later
* OS           : None
* H/W Platform : Renesas Starter Kit+ RX64M
* Description  : This is the main tutorial code.
******************************************************************************/
/*****************************************************************************
* History : DD.MM.YYYY Version Description
*         : 04.08.2014 1.00    First Release
*         : 31.10.2014 1.01    All network modules supported
******************************************************************************/

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/
#include <string.h>
#include "platform.h"
#include "r_usb_basic_config.h"
#include "r_usb_basic_if.h"
#include "r_usb_hmsc_if.h"
#include "r_t4_itcpip.h"
#include "r_ether_rx_if.h"
#include "r_t4_dhcp_client_rx_if.h"
#include "r_t4_dns_client_rx_if.h"
#include "r_t4_http_server_rx_if.h"
#include "r_t4_ftp_server_rx_if.h"

#include "r_sys_time.h"
#include "timer.h"

/******************************************************************************
Section    <Section Definition> , "Project Sections"
******************************************************************************/

/******************************************************************************
Exported global variables and functions (to be accessed by other files)
******************************************************************************/
extern void usb_cstd_IdleTaskStart(void);
extern void usb_hmsc_task_start(void);
extern void usb_apl_task_switch(void);

extern uint8_t _myethaddr[][6];
extern TCPUDP_ENV tcpudp_env[];
extern uint8_t dnsaddr1[];
extern uint8_t dnsaddr2[];

/******************************************************************************
Private global variables and functions
******************************************************************************/
static UW tcpudp_work[ 21504 / sizeof(UW)]; // calculated by W tcpudp_get_ramsize( void )

void   set_tcpudp_env(DHCP *dhcp);
void   usb_cpu_FunctionUSB0IP(void);
void   usb_cpu_FunctionUSB1IP(void);

/******************************************************************************
Imported global variables and functions (from other files)
******************************************************************************/
extern volatile uint8_t  link_detect[ETHER_CHANNEL_MAX];

/******************************************************************************
Function Name   : main
Description     : sample main
Arguments       : none
Return value    : none
******************************************************************************/

void main(void)
{
    usb_err_t   usb_err = USB_SUCCESS;
    ER          ercd;
    W           ramsize;
    volatile DHCP       dhcp;

    /* start system time */
    start_system_time();

    /* Initialize the USB driver scheduler for Non-OS */
    usb_cstd_ScheInit();

    /* Initialize the USB driver hardware */
    usb_cpu_target_init();

    /* USB driver open */
    usb_err = R_USB_Open(USB_IP1);
    if(usb_err != USB_SUCCESS)
    {
        while(1);
    }

/* Condition compilation by the difference of USB function */
#if USB_FUNCSEL_USBIP0_PP != USB_NOUSE_PP
    usb_cpu_FunctionUSB0IP();     /* USB0 pin function and port mode setting. */
#endif  /* USB_FUNCSEL_USBIP0_PP != USB_NOUSE_PP */

#if USB_FUNCSEL_USBIP1_PP != USB_NOUSE_PP
    usb_cpu_FunctionUSB1IP();   /* USB1 port mode and Switch mode Initialize */
#endif  /* USB_FUNCSEL_USBIP1_PP != USB_NOUSE_PP */


    /* HMSC sample code - Start the Idle task. */
    usb_cstd_IdleTaskStart();

    /* HMSC sample code - Start host-related USB drivers. */
    usb_hmsc_task_start();

    /* Initialize the Ethernet driver */
    R_ETHER_Initial();

    /* start LAN controller */
    ercd = lan_open();
    if (ercd != E_OK)
    {
        for( ;; );
    }

    /* DHCP client */
    OpenTimer();
    while (ETHER_FLAG_ON_LINK_ON != link_detect[0] && ETHER_FLAG_ON_LINK_ON != link_detect[1])
    {
        R_ETHER_LinkProcess(0);
        R_ETHER_LinkProcess(1);
    }

    if (!r_dhcp_open(&dhcp, (unsigned char*)tcpudp_work, &_myethaddr[0][0]))
    {
        set_tcpudp_env(&dhcp);
    }
    CloseTimer();

    if (NULL != &dhcp)
    {
        nop();
    }

    /* Get the size of the work area used by the T4 (RAM size). */
    ramsize = tcpudp_get_ramsize();
    if (ramsize > (sizeof(tcpudp_work)))
    {
        /* Then reserve as much memory array for the work area as the size 
           indicated by the returned value. */
        for( ;; );
    }

    /* Initialize the TCP/IP */
    ercd = tcpudp_open(tcpudp_work);
    if (ercd != E_OK)
    {
        for( ;; );
    }

    /* Init FTP server */
    R_ftp_srv_open();

    /* Init DNS clienr */
    R_dns_init();

    /* Sample code main loop */
    while( 1 )
    {
        /* Ethernet driver link up processing */
        R_ETHER_LinkProcess(0);
        R_ETHER_LinkProcess(1);

        /* HTTPD wake-up */
        R_httpd();
        /* FTP server wake-up*/
        R_ftpd();
        /* DNS client wake-up*/
        R_dns_process();

        /* USB task processing */
        usb_apl_task_switch();
    }
}
/******************************************************************************
End of function main
******************************************************************************/

/******************************************************************************
Function Name   : set_tcpudp_env
Description     : Set DHCP's error logs to TCP event buffer.
Arguments       : none
Return value    : none
******************************************************************************/
void    set_tcpudp_env(DHCP *dhcp)
{
    if (NULL != dhcp)
    {
        memcpy(tcpudp_env[0].ipaddr, dhcp->ipaddr, 4);
        memcpy(tcpudp_env[0].maskaddr, dhcp->maskaddr, 4);
        memcpy(tcpudp_env[0].gwaddr, dhcp->gwaddr, 4);

        memcpy((char *)dnsaddr1, (char *)dhcp->dnsaddr, 4);
        memcpy((char *)dnsaddr2, (char *)dhcp->dnsaddr2, 4);
    }
}

/******************************************************************************
Function Name   : usb_cpu_FunctionUSB0IP
Description     : USB0 port mode and Switch mode Initialize
Arguments       : none
Return value    : none
******************************************************************************/
void usb_cpu_FunctionUSB0IP(void)
{
#if (USB_FUNCSEL_USBIP0_PP != USB_NOUSE_PP)
    R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_LPC_CGC_SWR);

  #if USB_FUNCSEL_USBIP0_PP == USB_PERI_PP
    PORT1.PMR.BIT.B6    = 1u;

    R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_MPC);
    MPC.P16PFS.BYTE = 0x11; /* USB0_VBUS */
    R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_MPC);

  #elif USB_FUNCSEL_USBIP0_PP == USB_HOST_PP

    PORT1.PMR.BIT.B4 = 1u;  /* P14 set USB0_OVRCURA */
    PORT1.PMR.BIT.B6 = 1u;  /* P16 set VBUS_USB */

    R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_MPC);
    MPC.P14PFS.BYTE = 0x12; /* USB0_OVRCURA */
    R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_MPC);

    R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_MPC);
    MPC.P16PFS.BYTE = 0x12; /* USB0_VBUSEN */
    R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_MPC);

  #endif  /* USB_FUNCSEL_USBIP0_PP == USB_PERI_PP */

    R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_LPC_CGC_SWR);
#endif /*  (USB_FUNCSEL_USBIP0_PP != USB_NOUSE_PP) */ 
}   /* eof usb_cpu_FunctionUSB0IP() */

/******************************************************************************
Function Name   : usb_cpu_FunctionUSB1IP
Description     : USB1 port mode and Switch mode Initialize
Arguments       : none
Return value    : none
******************************************************************************/
void usb_cpu_FunctionUSB1IP(void)
{
#if (USB_FUNCSEL_USBIP1_PP != USB_NOUSE_PP)
    R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_LPC_CGC_SWR);

  #if USB_FUNCSEL_USBIP1_PP == USB_PERI_PP
    PORT1.PMR.BIT.B1    = 1u;

    R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_MPC);
    MPC.P11PFS.BYTE = 0x14; /* USBHS_VBUS */
    R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_MPC);


  #elif USB_FUNCSEL_USBIP1_PP == USB_HOST_PP
    PORT1.PMR.BIT.B0    = 1u;
    PORT1.PMR.BIT.B1    = 1u;

    R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_MPC);
    MPC.P10PFS.BYTE = 0x15; /* USBHS_OVRCURA */
    R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_MPC);

    R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_MPC);
    MPC.P11PFS.BYTE = 0x15; /* USBHS_VBUSEN */
    R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_MPC);
  #endif  /* USB_FUNCSEL_USBIP1_PP == USB_PERI_PP */

    R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_LPC_CGC_SWR);
#endif /*  (USB_FUNCSEL_USBIP1_PP != USB_NOUSE_PP) */
}   /* eof usb_cpu_FunctionUSB1IP() */



/******************************************************************************
End of file
******************************************************************************/
