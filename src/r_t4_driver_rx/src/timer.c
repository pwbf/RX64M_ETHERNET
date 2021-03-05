/************************************************************************************
* DISCLAIMER

* This software is supplied by Renesas Electronics Corporation and is only
* intended for use with Renesas products. No other uses are authorized.

* This software is owned by Renesas Electronics Corporation and is protected under
* all applicable laws, including copyright laws.

* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES
* REGARDING THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY,
* INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
* PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY
* DISCLAIMED.


* TO THE MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS
* ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES SHALL BE LIABLE
* FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES
* FOR ANY REASON RELATED TO THIS SOFTWARE, EVEN IF RENESAS OR ITS
* AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.

* Renesas reserves the right, without notice, to make changes to this
* software and to discontinue the availability of this software.
* By using this software, you agree to the additional terms and
* conditions found by accessing the following link:
* http://www.renesas.com/disclaimer
************************************************************************************/

/*  Copyright (C) 2011(2015) Renesas Electronics Corporation All rights reserved.  */

/*""FILE COMMENT""******************************************************************************
 * System Name  : ether driver
 * File Name    : timer.c
 * Version      : 1.02
 * Contents     : timer driver
 * Customer     : Standard
 * Model        : None
 * CPU          : RX64M , RX71M
 * Compiler     :
 * OS           : ---
 * Programmer   :
 * Note         :
 ***********************************************************************************************
 ***********************************************************************************************
* History       : DD.MM.YYYY Version Description
*               : 21.06.2010 1.00 First Release
*               : 25.01.2011 1.01 clean up source code.
*               : 05.01.2015 1.02 Fixed include file.
*""FILE COMMENT END""**************************************************************************/

/******************************************************************************
Includes <System Includes> , "Project Includes"
******************************************************************************/
#include <stdbool.h>
#include "r_t4_driver_rx_config.h"
#include "timer.h"
#include "r_cmt_rx_if.h"

/******************************************************************************
Macro definitions
******************************************************************************/
#define T4_DRV_TIMER_CNT_FREQ 100 // hz(10ms)
static uint32_t cmt_channel = 0xff;

/******************************************************************************
Exported global variables and functions (to be accessed by other files)
******************************************************************************/
void OpenTimer(void);
void CloseTimer(void);

/******************************************************************************
Imported global variables and functions (from other files)
******************************************************************************/
extern void timer_interrupt(void *pdata);

/******************************************************************************
* Function Name : OpenTimer
* Declaration   : void OpenTimer(void)
* Description   : This function starts with the setting of the Timer interrupt
* Arguments     : none
* Return value  : none
******************************************************************************/
void OpenTimer(void)
{
    if (R_CMT_CreatePeriodic(T4_DRV_TIMER_CNT_FREQ, timer_interrupt, &cmt_channel) != true)
    {
        cmt_channel = 0xff;
        while (1)
        {
            /* infinite loop(setting error) */
        }
    }
}

/******************************************************************************
* Function Name : CloseTimer
* Declaration   : void CloseTimer(void)
* Description   : This function finishes a Timer interrupt
* Arguments     : none
* Return value  : none
******************************************************************************/
void CloseTimer(void)
{
    if (0xff == cmt_channel)
    {
        return;
    }

    /* frees the CMT channel and disabling interrupt. */
    if (R_CMT_Stop(cmt_channel) != true)
    {
        /* The CMT channel could not be closed or it was not open */
    }
}

/* FILE END*/
