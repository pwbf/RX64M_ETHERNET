/***********************************************************************************************************************
* DISCLAIMER
* This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No 
* other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all 
* applicable laws, including copyright laws. 
* THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
* THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, 
* FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM 
* EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES 
* SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS 
* SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
* Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of 
* this software. By using this software, you agree to the additional terms and conditions found by accessing the 
* following link:
* http://www.renesas.com/disclaimer 
*
* Copyright (C) 2014 Renesas Electronics Corporation. All rights reserved.    
***********************************************************************************************************************/
/***********************************************************************************************************************
* File Name    : r_ether_setting_rx.c
* Version      : 1.01
* Description  : Ethernet module device driver
***********************************************************************************************************************/
/**********************************************************************************************************************
* History : DD.MM.YYYY Version  Description
*         : 22.07.2014 1.00     First Release
*         : 16.12.2014 1.01     Deleted header file include. Renamed file.
***********************************************************************************************************************/

/***********************************************************************************************************************
Includes   <System Includes> , "Project Includes"
***********************************************************************************************************************/
#include "platform.h"

#include "r_ether_rx_if.h"
#include "src/r_ether_rx_private.h"

/***********************************************************************************************************************
Macro definitions
***********************************************************************************************************************/

/***********************************************************************************************************************
Typedef definitions
***********************************************************************************************************************/

/***********************************************************************************************************************
Imported global variables and functions (from other files)
***********************************************************************************************************************/

/***********************************************************************************************************************
Exported global variables (to be accessed by other files)
***********************************************************************************************************************/

/***********************************************************************************************************************
Private global variables and functions
***********************************************************************************************************************/
#if ETHER_CFG_MODE_SEL == 0
    /* MII */
    static void ether_port_enable_mii(uint8_t connect);
    static void ether_port_disable_mii(uint8_t connect);
#elif ETHER_CFG_MODE_SEL == 1
    /* RMII */
    static void ether_port_enable_rmii(uint8_t connect);
    static void ether_port_disable_rmii(uint8_t connect);
#else
    #error "ERROR - ETHER_CFG_MODE_SEL - Unknown Ethernet interface is selected in r_ether_rx_config.h"
#endif

/***********************************************************************************************************************
* Function Name: ether_enable_icu
* Description  :
* Arguments    : channel -
*                    Ethernet channel number
* Return Value : none
***********************************************************************************************************************/
void ether_enable_icu(uint32_t channel)
{
#if (ETHER_CFG_AL1_INT_PRIORTY >= 1) && (ETHER_CFG_AL1_INT_PRIORTY <= 15)
    ICU.IPR[IPR_ICU_GROUPAL1].BIT.IPR = ETHER_CFG_AL1_INT_PRIORTY;
#else
    #error "ERROR- ETHER_CFG_AL1_INT_PRIORTY - Interrupt priority level is out of range defined in r_ether_rx_config.h."
#endif
    
    if (ETHER_CHANNEL_0 == channel)
    {
        ICU.GENAL1.BIT.EN4 = 1;
    }
    else
    {
        ICU.GENAL1.BIT.EN5 = 1;
    }
    
    ICU.IER[IER_ICU_GROUPAL1].BIT.IEN1 = 1;
} /* End of function ether_enable_icu() */

/***********************************************************************************************************************
* Function Name: ether_disable_icu
* Description  :
* Arguments    : channel -
*                    Ethernet channel number
* Return Value : none
***********************************************************************************************************************/
void ether_disable_icu(uint32_t channel)
{
    ICU.IER[IER_ICU_GROUPAL1].BIT.IEN1 = 0;

    if (ETHER_CHANNEL_0 == channel)
    {
        ICU.GENAL1.BIT.EN4 = 0;
    }
    else
    {
        ICU.GENAL1.BIT.EN5 = 0;
    }
} /* End of function ether_disable_icu() */

/***********************************************************************************************************************
* Function Name: ether_clear_icu_source
* Description  :
* Arguments    : channel -
*                    Ethernet channel number
* Return Value : none
***********************************************************************************************************************/
void ether_clear_icu_source(uint32_t channel)
{
    ICU.IR[IR_ICU_GROUPAL1].BIT.IR = 0;
    
    if (ETHER_CHANNEL_0 == channel)
    {
        ICU.GRPAL1.BIT.IS4 = 0;
    }
    else
    {
        ICU.GRPAL1.BIT.IS5 = 0;
    }
} /* End of function ether_clear_icu_source() */

/***********************************************************************************************************************
* Function Name: ether_port_enable
* Description  :
* Arguments    : channel -
*                    Ethernet channel number
* Return Value : none
***********************************************************************************************************************/
void ether_port_enable(uint8_t connect)
{
/* Port pins default to inputs. To ensure safe initialization set the pin states
   before changing the data direction registers. This will avoid any unintentional
   state changes on the external ports.
   Many peripheral modules will override the setting of the port registers. Ensure
   that the state is safe for external devices if the internal peripheral module is
   disabled or powered down. */

#if ETHER_CFG_MODE_SEL == 0 /* MII */
    ether_port_enable_mii(connect);
#endif

#if ETHER_CFG_MODE_SEL == 1 /* RMII */
    ether_port_enable_rmii(connect);
#endif

} /* End of function ether_port_enable() */

/***********************************************************************************************************************
* Function Name: ether_port_disable
* Description  :
* Arguments    : channel -
*                    Ethernet channel number
* Return Value : none
***********************************************************************************************************************/
void ether_port_disable(uint8_t connect)
{
#if ETHER_CFG_MODE_SEL == 0 /* MII */
    ether_port_disable_mii(connect);
#endif

#if ETHER_CFG_MODE_SEL == 1 /* RMII */
    ether_port_disable_rmii(connect);
#endif

} /* End of function ether_port_disable() */

#if ETHER_CFG_MODE_SEL == 0 /* MII */
/***********************************************************************************************************************
* Function Name: ether_port_enable_mii
* Description  : Configures port pins.
* Arguments    : channel -
*                    Ethernet channel number
* Return Value : none
***********************************************************************************************************************/
static void ether_port_enable_mii(uint8_t connect)
{
    /* Clear PDR and PMR */
    if (PORT_CONNECT_ET0 == (connect & PORT_CONNECT_ET0))
    {
    /*
    Pin Functions : Port
    --------------------
    ET0_TX_CLK     : PC4
    ET0_RX_CLK     : P76
    ET0_TX_EN      : P80
    ET0_ETXD3      : PC6
    ET0_ETXD2      : PC5
    ET0_ETXD1      : P82
    ET0_ETXD0      : P81
    ET0_TX_ER      : PC3
    ET0_RX_DV      : PC2
    ET0_ERXD3      : PC0
    ET0_ERXD2      : PC1
    ET0_ERXD1      : P74
    ET0_ERXD0      : P75
    ET0_RX_ER      : P77
    ET0_CRS        : P83
    ET0_COL        : PC7
    ET0_MDC        : P72
    ET0_MDIO       : P71
    ET0_LINKSTA    : P34
    ET_EXOUT       : NC
    ET_WOL         : NC
    */
        /* ET0 */
        PORT3.PDR.BIT.B4 = 0;
        PORT7.PDR.BIT.B1 = 0;
        PORT7.PDR.BIT.B2 = 0;
        PORT7.PDR.BIT.B4 = 0;
        PORT7.PDR.BIT.B5 = 0;
        PORT7.PDR.BIT.B6 = 0;
        PORT7.PDR.BIT.B7 = 0;
        PORT8.PDR.BIT.B0 = 0;
        PORT8.PDR.BIT.B1 = 0;
        PORT8.PDR.BIT.B2 = 0;
        PORT8.PDR.BIT.B3 = 0;
        PORTC.PDR.BYTE   = 0x00;
        
        PORT3.PMR.BIT.B4 = 0;
        PORT7.PMR.BIT.B1 = 0;
        PORT7.PMR.BIT.B2 = 0;
        PORT7.PMR.BIT.B4 = 0;
        PORT7.PMR.BIT.B5 = 0;
        PORT7.PMR.BIT.B6 = 0;
        PORT7.PMR.BIT.B7 = 0;
        PORT8.PMR.BIT.B0 = 0;
        PORT8.PMR.BIT.B1 = 0;
        PORT8.PMR.BIT.B2 = 0;
        PORT8.PMR.BIT.B3 = 0;
        PORTC.PMR.BYTE   = 0x00;
    }

    if (PORT_CONNECT_ET1 == (connect & PORT_CONNECT_ET1))
    {
    /*
    Pin Functions : Port
    --------------------
    ET1_TX_CLK     : PG2
    ET1_RX_CLK     : PG0
    ET1_TX_EN      : P60
    ET1_ETXD3      : PG6
    ET1_ETXD2      : PG5
    ET1_ETXD1      : PG4
    ET1_ETXD0      : PG3
    ET1_TX_ER      : PG7
    ET1_RX_DV      : P90
    ET1_ERXD3      : P97
    ET1_ERXD2      : P96
    ET1_ERXD1      : P95
    ET1_ERXD0      : P94
    ET1_RX_ER      : PG1
    ET1_CRS        : P92
    ET1_COL        : P91
    ET1_MDC        : P31
    ET1_MDIO       : P30
    ET1_LINKSTA    : P93
    */
        /* ET1 */
        PORT3.PDR.BIT.B0 = 0;
        PORT3.PDR.BIT.B1 = 0;
        PORT6.PDR.BIT.B0 = 0;
        PORT9.PDR.BIT.B0 = 0;
        PORT9.PDR.BIT.B1 = 0;
        PORT9.PDR.BIT.B2 = 0;
        PORT9.PDR.BIT.B3 = 0;       /* LINKSTA */
        PORT9.PDR.BIT.B4 = 0;
        PORT9.PDR.BIT.B5 = 0;
        PORT9.PDR.BIT.B6 = 0;
        PORT9.PDR.BIT.B7 = 0;
        PORTG.PDR.BYTE   = 0x00;
        
        PORT3.PMR.BIT.B0 = 0;
        PORT3.PMR.BIT.B1 = 0;
        PORT6.PMR.BIT.B0 = 0;
        PORT9.PMR.BIT.B0 = 0;
        PORT9.PMR.BIT.B1 = 0;
        PORT9.PMR.BIT.B2 = 0;
        PORT9.PMR.BIT.B3 = 0;       /* LINKSTA */
        PORT9.PMR.BIT.B4 = 0;
        PORT9.PMR.BIT.B5 = 0;
        PORT9.PMR.BIT.B6 = 0;
        PORT9.PMR.BIT.B7 = 0;
        PORTG.PMR.BYTE   = 0x00;
    }

    /* PFS register write protect off */
    R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_MPC);
   
    /* Select pin function */
    if (PORT_CONNECT_ET0 == (connect & PORT_CONNECT_ET0))
    {
        /* ET0 */
        MPC.P34PFS.BYTE = 0x11;
        MPC.P71PFS.BYTE = 0x11;
        MPC.P72PFS.BYTE = 0x11;
        MPC.P74PFS.BYTE = 0x11;
        MPC.P75PFS.BYTE = 0x11;
        MPC.P76PFS.BYTE = 0x11;
        MPC.P77PFS.BYTE = 0x11;
        MPC.P80PFS.BYTE = 0x11;
        MPC.P81PFS.BYTE = 0x11;
        MPC.P82PFS.BYTE = 0x11;
        MPC.P83PFS.BYTE = 0x11;
        MPC.PC0PFS.BYTE = 0x11;
        MPC.PC1PFS.BYTE = 0x11;
        MPC.PC2PFS.BYTE = 0x11;
        MPC.PC3PFS.BYTE = 0x11;
        MPC.PC4PFS.BYTE = 0x11;
        MPC.PC5PFS.BYTE = 0x11;
        MPC.PC6PFS.BYTE = 0x11;
        MPC.PC7PFS.BYTE = 0x11;
    }


    if (PORT_CONNECT_ET1 == (connect & PORT_CONNECT_ET1))
    {
        /* ET1 */
        MPC.P30PFS.BYTE = 0x14;
        MPC.P31PFS.BYTE = 0x14;
        MPC.P60PFS.BYTE = 0x14;
        MPC.P90PFS.BYTE = 0x14;
        MPC.P91PFS.BYTE = 0x14;
        MPC.P92PFS.BYTE = 0x14;
        MPC.P93PFS.BYTE = 0x14;      /* LINKSTA */

        MPC.P94PFS.BYTE = 0x14;
        MPC.P95PFS.BYTE = 0x14;
        MPC.P96PFS.BYTE = 0x14;
        MPC.P97PFS.BYTE = 0x14;
        MPC.PG0PFS.BYTE = 0x14;
        MPC.PG1PFS.BYTE = 0x14;
        MPC.PG2PFS.BYTE = 0x14;
        MPC.PG3PFS.BYTE = 0x14;
        MPC.PG4PFS.BYTE = 0x14;
        MPC.PG5PFS.BYTE = 0x14;
        MPC.PG6PFS.BYTE = 0x14;
        MPC.PG7PFS.BYTE = 0x14;
    }

    /* PFS register write protect on */
    R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_MPC);
    
    /* Switch to the selected input/output function */
    if (PORT_CONNECT_ET0 == (connect & PORT_CONNECT_ET0))
    {
        /* ET0 */
        MPC.PFENET.BIT.PHYMODE0 = 1; /* Select Ethernet mode    MII mode for ch0 */

        PORT3.PMR.BIT.B4 = 1;
        PORT7.PMR.BIT.B1 = 1;
        PORT7.PMR.BIT.B2 = 1;
        PORT7.PMR.BIT.B4 = 1;
        PORT7.PMR.BIT.B5 = 1;
        PORT7.PMR.BIT.B6 = 1;
        PORT7.PMR.BIT.B7 = 1;
        PORT8.PMR.BIT.B0 = 1;
        PORT8.PMR.BIT.B1 = 1;
        PORT8.PMR.BIT.B2 = 1;
        PORT8.PMR.BIT.B3 = 1;
        PORTC.PMR.BYTE   = 0xFF;
    }
    
    if (PORT_CONNECT_ET1 == (connect & PORT_CONNECT_ET1))
    {
        /* ET1 */
        MPC.PFENET.BIT.PHYMODE1 = 1; /* Select Ethernet mode    MII mode for ch1 */

        PORT3.PMR.BIT.B0 = 1;
        PORT3.PMR.BIT.B1 = 1;
        PORT6.PMR.BIT.B0 = 1;
        PORT9.PMR.BIT.B0 = 1;
        PORT9.PMR.BIT.B1 = 1;
        PORT9.PMR.BIT.B2 = 1;
        PORT9.PMR.BIT.B3 = 1;       /* LINKSTA */
        PORT9.PMR.BIT.B4 = 1;
        PORT9.PMR.BIT.B5 = 1;
        PORT9.PMR.BIT.B6 = 1;
        PORT9.PMR.BIT.B7 = 1;
        PORTG.PMR.BYTE   = 0xFF;
    }
} /* End of function ether_port_enable_mii() */

/***********************************************************************************************************************
* Function Name: ether_port_disable_mii
* Description  : Configures port pins.
* Arguments    : channel -
*                    Ethernet channel number
* Return Value : none
***********************************************************************************************************************/
static void ether_port_disable_mii(uint8_t connect)
{
    /* Clear PDR and PMR */
    if (PORT_CONNECT_ET0 == (connect & PORT_CONNECT_ET0))
    {
    /*
    Pin Functions : Port
    --------------------
    ET0_TX_CLK     : PC4
    ET0_RX_CLK     : P76
    ET0_TX_EN      : P80
    ET0_ETXD3      : PC6
    ET0_ETXD2      : PC5
    ET0_ETXD1      : P82
    ET0_ETXD0      : P81
    ET0_TX_ER      : PC3
    ET0_RX_DV      : PC2
    ET0_ERXD3      : PC0
    ET0_ERXD2      : PC1
    ET0_ERXD1      : P74
    ET0_ERXD0      : P75
    ET0_RX_ER      : P77
    ET0_CRS        : P83
    ET0_COL        : PC7
    ET0_MDC        : P72
    ET0_MDIO       : P71
    ET0_LINKSTA    : P34
    ET_EXOUT       : NC
    ET_WOL         : NC
    */
        /* ET0 */
        PORT3.PODR.BIT.B4 = 0;      /* ET0_LINKSTA -> P34(don't care) */
        PORT7.PODR.BIT.B1 = 0;      /* ET0_MDIO    -> P71(don't care) */
        PORT7.PODR.BIT.B2 = 0;      /* ET0_MDC     -> P72(Low Output) 
                                       So as not to take data when the state of MDC is irregularity, Low is output from
                                        MDC. */
        PORT7.PODR.BIT.B4 = 0;      /* ET0_ERXD1   -> P74(don't care) */
        PORT7.PODR.BIT.B5 = 0;      /* ET0_ERXD0   -> P75(don't care) */
        PORT7.PODR.BIT.B6 = 0;      /* ET0_RX_CLK  -> P76(don't care) */
        PORT7.PODR.BIT.B7 = 0;      /* ET0_RX_ER   -> P77(don't care) */
        PORT8.PODR.BIT.B0 = 0;      /* ET0_TX_EN   -> P80(Low Output:deassert) */
        PORT8.PODR.BIT.B1 = 0;      /* ET0_ETXD0   -> P81(Low Output) */
        PORT8.PODR.BIT.B2 = 0;      /* ET0_ETXD1   -> P82(Low Output) */
        PORT8.PODR.BIT.B3 = 0;      /* ET0_CRS     -> P83(don't care) */
        PORTC.PODR.BYTE = 0x00;     /* ET0_COL     -> PC7(don't care)
                                       ET0_ETXD3   -> PC6(Low Output)
                                       ET0_ETXD2   -> PC5(Low Output)
                                       ET0_TX_CLK  -> PC4(don't care)
                                       ET0_TX_ER   -> PC3(Low Output:deassert)
                                       ET0_RX_DV   -> PC2(don't care)
                                       ET0_ERXD2   -> PC1(don't care)
                                       ET0_ERXD3   -> PC0(don't care) */

        PORT3.PDR.BIT.B4 = 0;       /* ET0_LINKSTA -> P34(I/O:Input) */
        PORT7.PDR.BIT.B1 = 0;       /* ET0_MDIO    -> P71(I/O:Input) */
        PORT7.PDR.BIT.B2 = 1;       /* ET0_MDC     -> P72(I/O:Output)*/
        PORT7.PDR.BIT.B4 = 0;       /* ET0_ERXD1   -> P74(I/O:Input) */
        PORT7.PDR.BIT.B5 = 0;       /* ET0_ERXD0   -> P75(I/O:Input) */
        PORT7.PDR.BIT.B6 = 0;       /* ET0_RX_CLK  -> P76(I/O:Input) */
        PORT7.PDR.BIT.B7 = 0;       /* ET0_RX_ER   -> P77(I/O:Input) */
        PORT8.PDR.BIT.B0 = 1;       /* ET0_TX_EN   -> P80(I/O:Output)*/
        PORT8.PDR.BIT.B1 = 1;       /* ET0_ETXD0   -> P81(I/O:Output)*/
        PORT8.PDR.BIT.B2 = 1;       /* ET0_ETXD1   -> P82(I/O:Output)*/
        PORT8.PDR.BIT.B3 = 0;       /* ET0_CRS     -> P83(I/O:Input) */
        
         PORTC.PDR.BYTE = 0x78;     /* ET0_COL    -> PC7(I/O:Input)
                                       ET0_ETXD3  -> PC6(I/O:Output)
                                       ET0_ETXD2  -> PC5(I/O:Output)
                                       ET0_TX_CLK -> PC4(I/O:Output)
                                       ET0_TX_ER  -> PC3(I/O:Output)
                                       ET0_RX_DV  -> PC2(I/O:Input)
                                       ET0_ERXD2  -> PC1(I/O:Input)
                                       ET0_ERXD3  -> PC0(I/O:Input)  */

        PORT3.PMR.BIT.B4 = 0;
        PORT7.PMR.BIT.B1 = 0;
        PORT7.PMR.BIT.B2 = 0;
        PORT7.PMR.BIT.B4 = 0;
        PORT7.PMR.BIT.B5 = 0;
        PORT7.PMR.BIT.B6 = 0;
        PORT7.PMR.BIT.B7 = 0;
        PORT8.PMR.BIT.B0 = 0;
        PORT8.PMR.BIT.B1 = 0;
        PORT8.PMR.BIT.B2 = 0;
        PORT8.PMR.BIT.B3 = 0;
        PORTC.PMR.BYTE   = 0xFF;
    }

    if (PORT_CONNECT_ET1 == (connect & PORT_CONNECT_ET1))
    {
    /*
    Pin Functions : Port
    --------------------
    ET1_TX_CLK     : PG2
    ET1_RX_CLK     : PG0
    ET1_TX_EN      : P60
    ET1_ETXD3      : PG6
    ET1_ETXD2      : PG5
    ET1_ETXD1      : PG4
    ET1_ETXD0      : PG3
    ET1_TX_ER      : PG7
    ET1_RX_DV      : P90
    ET1_ERXD3      : P97
    ET1_ERXD2      : P96
    ET1_ERXD1      : P95
    ET1_ERXD0      : P94
    ET1_RX_ER      : PG1
    ET1_CRS        : P92
    ET1_COL        : P91
    ET1_MDC        : P31
    ET1_MDIO       : P30
    ET1_LINKSTA    : P93
    */
        /* ET1 */
        PORT3.PODR.BIT.B0 = 0;      /* ET1_MDIO   -> P30(don't care) */        
        PORT3.PODR.BIT.B1 = 0;      /* ET1_MDC    -> P31(Low Output) 
                                       So as not to take data when the state of MDC is irregularity, Low is output from
                                        MDC. */
        PORT6.PODR.BIT.B0 = 0;      /* ET1_TX_EN  -> P60(Low Output:deassert) */
        PORT9.PODR.BIT.B0 = 0;      /* ET1_RX_DV  -> P90(don't care) */
        PORT9.PODR.BIT.B1 = 0;      /* ET1_COL    -> P91(don't care) */
        PORT9.PODR.BIT.B2 = 0;      /* ET1_CRS    -> P92(don't care) */
        PORT9.PODR.BIT.B3 = 0;      /* LINKSTA    -> P93(don't care) */
        PORT9.PODR.BIT.B4 = 0;      /* ET1_ERXD0  -> P94(don't care) */
        PORT9.PODR.BIT.B5 = 0;      /* ET1_ERXD1  -> P95(don't care) */
        PORT9.PODR.BIT.B6 = 0;      /* ET1_ERXD2  -> P96(don't care) */
        PORT9.PODR.BIT.B7 = 0;      /* ET1_ERXD3  -> P97(don't care) */
        PORTG.PODR.BIT.B0 = 0;      /* ET1_RX_CLK -> PG0(don't care) */
        PORTG.PODR.BIT.B1 = 0;      /* ET1_RX_ER  -> PG1(don't care) */
        PORTG.PODR.BIT.B2 = 0;      /* ET1_TX_CLK -> PG2(don't care) */
        PORTG.PODR.BIT.B3 = 0;      /* ET1_ETXD0  -> PG3(Low Output) */
        PORTG.PODR.BIT.B4 = 0;      /* ET1_ETXD1  -> PG4(Low Output) */
        PORTG.PODR.BIT.B5 = 0;      /* ET1_ETXD2  -> PG5(Low Output) */
        PORTG.PODR.BIT.B6 = 0;      /* ET1_ETXD3  -> PG6(Low Output) */
        PORTG.PODR.BIT.B7 = 0;      /* ET1_TX_ER  -> PG7(Low Output:deassert) */

        PORT3.PDR.BIT.B0 = 0;       /* ET1_MDIO   -> P30(I/O:Input)  */
        PORT3.PDR.BIT.B1 = 1;       /* ET1_MDC    -> P31(I/O:Output) */
        PORT6.PDR.BIT.B0 = 1;       /* ET1_TX_EN  -> P60(I/O:Output) */
        PORT9.PDR.BIT.B0 = 0;       /* ET1_RX_DV  -> P90(I/O:Input)  */
        PORT9.PDR.BIT.B1 = 0;       /* ET1_COL    -> P91(I/O:Input)  */
        PORT9.PDR.BIT.B2 = 0;       /* ET1_CRS    -> P92(I/O:Input)  */
        PORT9.PDR.BIT.B3 = 0;       /* LINKSTA    -> P93(I/O:Input)  */
        PORT9.PDR.BIT.B4 = 0;       /* ET1_ERXD0  -> P94(I/O:Input)  */
        PORT9.PDR.BIT.B5 = 0;       /* ET1_ERXD1  -> P95(I/O:Input)  */
        PORT9.PDR.BIT.B6 = 0;       /* ET1_ERXD2  -> P96(I/O:Input)  */
        PORT9.PDR.BIT.B7 = 0;       /* ET1_ERXD3  -> P97(I/O:Input)  */
        PORTG.PDR.BIT.B0 = 0;       /* ET1_RX_CLK -> PG0(I/O:Input)  */
        PORTG.PDR.BIT.B1 = 0;       /* ET1_RX_ER  -> PG1(I/O:Input)  */
        PORTG.PDR.BIT.B2 = 1;       /* ET1_TX_CLK -> PG2(I/O:Output) */
        PORTG.PDR.BIT.B3 = 1;       /* ET1_ETXD0  -> PG3(I/O:Output) */
        PORTG.PDR.BIT.B4 = 1;       /* ET1_ETXD1  -> PG4(I/O:Output) */
        PORTG.PDR.BIT.B5 = 1;       /* ET1_ETXD2  -> PG5(I/O:Output) */
        PORTG.PDR.BIT.B6 = 1;       /* ET1_ETXD3  -> PG6(I/O:Output) */
        PORTG.PDR.BIT.B7 = 1;       /* ET1_TX_ER  -> PG7(I/O:Output) */

        PORT3.PMR.BIT.B0 = 0;
        PORT3.PMR.BIT.B1 = 0;
        PORT6.PMR.BIT.B0 = 0;
        PORT9.PMR.BIT.B0 = 0;
        PORT9.PMR.BIT.B1 = 0;
        PORT9.PMR.BIT.B2 = 0;
        PORT9.PMR.BIT.B3 = 0;
        PORT9.PMR.BIT.B4 = 0;
        PORT9.PMR.BIT.B5 = 0;
        PORT9.PMR.BIT.B6 = 0;
        PORT9.PMR.BIT.B7 = 0;
        PORTG.PMR.BIT.B0 = 0;
        PORTG.PMR.BIT.B1 = 0;
        PORTG.PMR.BIT.B2 = 0;
        PORTG.PMR.BIT.B3 = 0;
        PORTG.PMR.BIT.B4 = 0;
        PORTG.PMR.BIT.B5 = 0;
        PORTG.PMR.BIT.B6 = 0;
        PORTG.PMR.BIT.B7 = 0;
    }

} /* End of function ether_port_disable_mii() */

#endif

#if ETHER_CFG_MODE_SEL == 1 /* RMII */
/***********************************************************************************************************************
* Function Name: ether_port_enable_rmii
* Description  : Configures port pins.
* Arguments    : channel -
*                    Ethernet channel number
* Return Value : none
***********************************************************************************************************************/
static void ether_port_enable_rmii(uint8_t connect)
{
    if (PORT_CONNECT_ET0 == (connect & PORT_CONNECT_ET0))
    {
    /*
    Pin Functions : Port
    --------------------
    ET0_MDC        : P72
    ET0_MDIO       : P71
    ET0_WOL        : NC
    ET0_LINKSTA    : P34
    ET0_EXOUT      : NC
    REF50CK0       : P76
    RMII0_TXD1     : P82
    RMII0_TXD0     : P81
    RMII0_TXD_EN   : P80
    RMII0_RXD1     : P74
    RMII0_RXD0     : P75
    RMII0_RX_ER    : P77
    RMII0_CRS_DV   : P83
    */
        PORT3.PDR.BIT.B4 = 0;
        PORT7.PDR.BIT.B1 = 0;
        PORT7.PDR.BIT.B2 = 0;
        PORT7.PDR.BIT.B4 = 0;
        PORT7.PDR.BIT.B5 = 0;
        PORT7.PDR.BIT.B6 = 0;
        PORT7.PDR.BIT.B7 = 0;
        PORT8.PDR.BIT.B0 = 0;
        PORT8.PDR.BIT.B1 = 0;
        PORT8.PDR.BIT.B2 = 0;
        PORT8.PDR.BIT.B3 = 0;

        PORT3.PMR.BIT.B4 = 0;
        PORT7.PMR.BIT.B1 = 0;
        PORT7.PMR.BIT.B2 = 0;
        PORT7.PMR.BIT.B4 = 0;
        PORT7.PMR.BIT.B5 = 0;
        PORT7.PMR.BIT.B6 = 0;
        PORT7.PMR.BIT.B7 = 0;
        PORT8.PMR.BIT.B0 = 0;
        PORT8.PMR.BIT.B1 = 0;
        PORT8.PMR.BIT.B2 = 0;
        PORT8.PMR.BIT.B3 = 0;
    }

    if (PORT_CONNECT_ET1 == (connect & PORT_CONNECT_ET1))
    {
    /*
    Pin Functions : Port
    --------------------
    ET1_MDC        : P31
    ET1_MDIO       : P30
    ET1_LINKSTA    : P93
    ET1_EXOUT      : NC
    REF50CK1       : PG0
    RMII1_TXD1     : PG4
    RMII1_TXD0     : PG3
    RMII1_TXD_EN   : P60
    RMII1_RXD1     : P95
    RMII1_RXD0     : P94
    RMII1_RX_ER    : PG1
    RMII1_CRS_DV   : P92
    */

    /* Clear PDR and PMR */
        PORT3.PDR.BIT.B0 = 0;
        PORT3.PDR.BIT.B1 = 0;
        PORT6.PDR.BIT.B0 = 0;
        PORT9.PDR.BIT.B2 = 0;
        PORT9.PDR.BIT.B3 = 0;
        PORT9.PDR.BIT.B4 = 0;
        PORT9.PDR.BIT.B5 = 0;
        PORTG.PDR.BIT.B0 = 0;
        PORTG.PDR.BIT.B1 = 0;
        PORTG.PDR.BIT.B3 = 0;
        PORTG.PDR.BIT.B4 = 0;

        PORT3.PMR.BIT.B0 = 0;
        PORT3.PMR.BIT.B1 = 0;
        PORT6.PMR.BIT.B0 = 0;
        PORT9.PMR.BIT.B2 = 0;
        PORT9.PMR.BIT.B3 = 0;
        PORT9.PMR.BIT.B4 = 0;
        PORT9.PMR.BIT.B5 = 0;
        PORTG.PMR.BIT.B0 = 0;
        PORTG.PMR.BIT.B1 = 0;
        PORTG.PMR.BIT.B3 = 0;
        PORTG.PMR.BIT.B4 = 0;
    }

    /* PFS register write protect off */
    R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_MPC);

    /* Select pin function */
    if (PORT_CONNECT_ET0 == (connect & PORT_CONNECT_ET0))
    {
        /* ET0 */
        MPC.P34PFS.BYTE = 0x11;
        MPC.P71PFS.BYTE = 0x11;
        MPC.P72PFS.BYTE = 0x11;
        MPC.P74PFS.BYTE = 0x12;
        MPC.P75PFS.BYTE = 0x12;
        MPC.P76PFS.BYTE = 0x12;
        MPC.P77PFS.BYTE = 0x12;
        MPC.P80PFS.BYTE = 0x12;
        MPC.P81PFS.BYTE = 0x12;
        MPC.P82PFS.BYTE = 0x12;
        MPC.P83PFS.BYTE = 0x12;
    }

    if (PORT_CONNECT_ET1 == (connect & PORT_CONNECT_ET1))
    {
        /* ET1 */
        MPC.P30PFS.BYTE = 0x14;
        MPC.P31PFS.BYTE = 0x14;
        MPC.P60PFS.BYTE = 0x15;
        MPC.P92PFS.BYTE = 0x15;
        MPC.P93PFS.BYTE = 0x14;
        MPC.P94PFS.BYTE = 0x15;
        MPC.P95PFS.BYTE = 0x15;
        MPC.PG0PFS.BYTE = 0x15;
        MPC.PG1PFS.BYTE = 0x15;
        MPC.PG3PFS.BYTE = 0x15;
        MPC.PG4PFS.BYTE = 0x15;
    }

    /* PFS register write protect on */
    R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_MPC);

    /* Switch to the selected input/output function */
    if (PORT_CONNECT_ET0 == (connect & PORT_CONNECT_ET0))
    {
        /* ET0 */
        MPC.PFENET.BIT.PHYMODE0 = 0; /* Select Ethernet mode    RMII mode for ch0 */

        PORT3.PMR.BIT.B4 = 1;
        PORT7.PMR.BIT.B1 = 1;
        PORT7.PMR.BIT.B2 = 1;
        PORT7.PMR.BIT.B4 = 1;
        PORT7.PMR.BIT.B5 = 1;
        PORT7.PMR.BIT.B6 = 1;
        PORT7.PMR.BIT.B7 = 1;
        PORT8.PMR.BIT.B0 = 1;
        PORT8.PMR.BIT.B1 = 1;
        PORT8.PMR.BIT.B2 = 1;
        PORT8.PMR.BIT.B3 = 1;
    }

    if (PORT_CONNECT_ET1 == (connect & PORT_CONNECT_ET1))
    {
        /* ET1 */
        MPC.PFENET.BIT.PHYMODE1 = 0; /* Select Ethernet mode    RMII mode for ch1 */
        
        PORT3.PMR.BIT.B0 = 1;
        PORT3.PMR.BIT.B1 = 1;
        PORT6.PMR.BIT.B0 = 1;
        PORT9.PMR.BIT.B2 = 1;
        PORT9.PMR.BIT.B3 = 1;
        PORT9.PMR.BIT.B4 = 1;
        PORT9.PMR.BIT.B5 = 1;
        PORTG.PMR.BIT.B0 = 1;
        PORTG.PMR.BIT.B1 = 1;
        PORTG.PMR.BIT.B3 = 1;
        PORTG.PMR.BIT.B4 = 1;
    }
} /* End of function ether_port_enable_rmii() */

/***********************************************************************************************************************
* Function Name: ether_port_disable_rmii
* Description  : Configures port pins.
* Arguments    : channel -
*                    Ethernet channel number
* Return Value : none
***********************************************************************************************************************/
static void ether_port_disable_rmii(uint8_t connect)
{
    if (PORT_CONNECT_ET0 == (connect & PORT_CONNECT_ET0))
    {
    /*
    Pin Functions : Port
    --------------------
    ET0_MDC        : P72
    ET0_MDIO       : P71
    ET0_WOL        : NC
    ET0_LINKSTA    : P34
    ET0_EXOUT      : NC
    REF50CK0       : P76
    RMII0_TXD1     : P82
    RMII0_TXD0     : P81
    RMII0_TXD_EN   : P80
    RMII0_RXD1     : P74
    RMII0_RXD0     : P75
    RMII0_RX_ER    : P77
    RMII0_CRS_DV   : P83
    */
       /* Output data */
        PORT3.PODR.BIT.B4 = 0;      /* ET0_LINKSTA  -> P34(don't care) */
        PORT7.PODR.BIT.B1 = 0;      /* ET0_MDIO     -> P71(don't care) */
        PORT7.PODR.BIT.B2 = 0;      /* ET0_MDC      -> P72(Low Output) 
                                       So as not to take data when the state of MDC is irregularity, Low is output from
                                        MDC. */
        PORT7.PODR.BIT.B4 = 0;      /* RMII0_RXD1   -> P74(don't care) */
        PORT7.PODR.BIT.B5 = 0;      /* RMII0_RXD0   -> P75(don't care) */
        PORT7.PODR.BIT.B6 = 0;      /* REF50CK0     -> P76(don't care) */
        PORT7.PODR.BIT.B7 = 0;      /* RMII0_RX_ER  -> P77(don't care) */
        PORT8.PODR.BIT.B0 = 0;      /* RMII0_TXD_EN -> P80(Low Output:deassert) */
        PORT8.PODR.BIT.B1 = 0;      /* RMII0_TXD0   -> P81(don't care) */
        PORT8.PODR.BIT.B2 = 0;      /* RMII0_TXD1   -> P82(don't care) */
        PORT8.PODR.BIT.B3 = 0;      /* RMII0_CRS_DV -> P83(don't care) */
        
        /* Direction */
        PORT3.PDR.BIT.B4 = 0;       /* ET0_LINKSTA  -> P34(input)  */
        PORT7.PDR.BIT.B1 = 0;       /* ET0_MDIO     -> P71(input)  */
        PORT7.PDR.BIT.B2 = 1;       /* ET0_MDC      -> P72(output) */
        PORT7.PDR.BIT.B4 = 0;       /* RMII0_RXD1   -> P74(input)  */
        PORT7.PDR.BIT.B5 = 0;       /* RMII0_RXD0   -> P75(input)  */
        PORT7.PDR.BIT.B6 = 0;       /* REF50CK0     -> P76(input)  */
        PORT7.PDR.BIT.B7 = 0;       /* RMII0_RX_ER  -> P77(input)  */
        PORT8.PDR.BIT.B0 = 1;       /* RMII0_TXD_EN -> P80(output) */
        PORT8.PDR.BIT.B1 = 1;       /* RMII0_TXD0   -> P81(output) */
        PORT8.PDR.BIT.B2 = 1;       /* RMII0_TXD1   -> P82(output) */
        PORT8.PDR.BIT.B3 = 0;       /* RMII0_CRS_DV -> P83(input)  */
    
        /* Port mode */
        PORT3.PMR.BIT.B4 = 0;
        PORT7.PMR.BIT.B1 = 0;
        PORT7.PMR.BIT.B2 = 0;
        PORT7.PMR.BIT.B4 = 0;
        PORT7.PMR.BIT.B5 = 0;
        PORT7.PMR.BIT.B6 = 0;
        PORT7.PMR.BIT.B7 = 0;
        PORT8.PMR.BIT.B0 = 0;
        PORT8.PMR.BIT.B1 = 0;
        PORT8.PMR.BIT.B2 = 0;
        PORT8.PMR.BIT.B3 = 0;
    }

    if (PORT_CONNECT_ET1 == (connect & PORT_CONNECT_ET1))
    {
    /*
    Pin Functions : Port
    --------------------
    ET1_MDC        : P31
    ET1_MDIO       : P30
    ET1_LINKSTA    : P93
    ET1_EXOUT      : NC
    REF50CK1       : PG0
    RMII1_TXD1     : PG4
    RMII1_TXD0     : PG3
    RMII1_TXD_EN   : P60
    RMII1_RXD1     : P95
    RMII1_RXD0     : P94
    RMII1_RX_ER    : PG1
    RMII1_CRS_DV   : P92
    */

    /* Clear PDR and PMR */
        /* Output data */
        PORT3.PODR.BIT.B0 = 0;       /* ET1_MDIO     ->P30(don't care) */
        PORT3.PODR.BIT.B1 = 0;       /* ET1_MDC      ->P31(Low Output) 
                                        So as not to take data when the state of MDC is irregularity, Low is output
                                         from MDC. */
        PORT6.PODR.BIT.B0 = 0;       /* RMII1_TXD_EN ->P60(Low Output:deassert) */
        PORT9.PODR.BIT.B2 = 0;       /* RMII1_CRS_DV ->P92(don't care) */
        PORT9.PODR.BIT.B3 = 0;       /* ET1_LINKSTA  ->P93(don't care) */
        PORT9.PODR.BIT.B4 = 0;       /* RMII1_RXD0   ->P94(don't care) */
        PORT9.PODR.BIT.B5 = 0;       /* RMII1_RXD1   ->P95(don't care) */
        PORTG.PODR.BIT.B0 = 0;       /* REF50CK1     ->PG0(don't care) */
        PORTG.PODR.BIT.B1 = 0;       /* RMII1_RX_ER  ->PG1(don't care) */
        PORTG.PODR.BIT.B3 = 0;       /* RMII1_TXD0   ->PG3(don't care) */
        PORTG.PODR.BIT.B4 = 0;       /* RMII1_TXD1   ->PG4(don't care) */

        /* Direction */
        PORT3.PDR.BIT.B0 = 0;       /* ET1_MDIO     ->P30(input)  */
        PORT3.PDR.BIT.B1 = 1;       /* ET1_MDC      ->P31(output) */
        PORT6.PDR.BIT.B0 = 1;       /* RMII1_TXD_EN ->P60(output) */
        PORT9.PDR.BIT.B2 = 0;       /* RMII1_CRS_DV ->P92(input)  */
        PORT9.PDR.BIT.B3 = 0;       /* ET1_LINKSTA  ->P93(input)  */
        PORT9.PDR.BIT.B4 = 0;       /* RMII1_RXD0   ->P94(input)  */
        PORT9.PDR.BIT.B5 = 0;       /* RMII1_RXD1   ->P95(input)  */
        PORTG.PDR.BIT.B0 = 0;       /* REF50CK1     ->PG0(input)  */
        PORTG.PDR.BIT.B1 = 0;       /* RMII1_RX_ER  ->PG1(input)  */
        PORTG.PDR.BIT.B3 = 1;       /* RMII1_TXD0   ->PG3(output) */
        PORTG.PDR.BIT.B4 = 1;       /* RMII1_TXD1   ->PG4(output) */
        
        /* Port mode */
        PORT3.PMR.BIT.B0 = 0;
        PORT3.PMR.BIT.B1 = 0;
        PORT6.PMR.BIT.B0 = 0;
        PORT9.PMR.BIT.B2 = 0;
        PORT9.PMR.BIT.B3 = 0;
        PORT9.PMR.BIT.B4 = 0;
        PORT9.PMR.BIT.B5 = 0;
        PORTG.PMR.BIT.B0 = 0;
        PORTG.PMR.BIT.B1 = 0;
        PORTG.PMR.BIT.B3 = 0;
        PORTG.PMR.BIT.B4 = 0;
    }

} /* End of function ether_port_disable_rmii() */

#endif  /* ETHER_CFG_MODE_SEL == 1 */

/* End of File */
