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
/******************************************************************************
* File Name    : r_usb_hmsc_apl.h
* Version      : 1.00
* Device(s)    : Renesas R5F564MxxDxx
* Tool-Chain   : Renesas e2 studio v3.0.1.09 or later
*              : C/C++ Compiler Package for RX Family V2.02.00 or later
* OS           : None
* H/W Platform : Renesas Starter Kit+ RX64M
* Description  : USB HMSC Sample Code Header
******************************************************************************/
/*****************************************************************************
* History : DD.MM.YYYY Version Description
*         : 04.08.2014 1.00    First Release
******************************************************************************/
#ifndef R_USB_HMSC_APL_H
#define R_USB_HMSC_APL_H

/******************************************************************************
Includes   <System Includes> , "Project Includes"
******************************************************************************/
#include "platform.h"

/*****************************************************************************
Macro definitions
******************************************************************************/
/* Host Sample Task */
#define USB_HMSCSMP_TSK     USB_TID_6           /* Task ID */
#define USB_HMSCSMP_MBX     USB_HMSCSMP_TSK     /* Mailbox ID */
#define USB_HMSCSMP_MPL     USB_HMSCSMP_TSK     /* Memorypool ID */

/******************************************************************************
Typedef definitions
******************************************************************************/

/******************************************************************************
Exported global variables
******************************************************************************/

/******************************************************************************
Exported global functions (to be accessed by other files)
******************************************************************************/

#endif /* R_USB_HMSC_APL_H */
/******************************************************************************
End  Of File
******************************************************************************/
