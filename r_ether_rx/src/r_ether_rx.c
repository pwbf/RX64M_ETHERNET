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
* File Name    : r_ether_rx.c
* Version      : 1.02
* Description  : Ethernet module device driver
***********************************************************************************************************************/
/***********************************************************************************************************************
* History : DD.MM.YYYY Version  Description
*         : 22.07.2014 1.00     First Release
*         : 16.12.2014 1.01     Made changes related to header file include.
*         : 27.03.2015 1.02     Changed the R_ETHER_LinkProcess() Function
***********************************************************************************************************************/

/***********************************************************************************************************************
Includes   <System Includes> , "Project Includes"
***********************************************************************************************************************/
#include <string.h>
#include <machine.h>

/* Access to peripherals and board defines. */
#include "platform.h"

#include "r_ether_rx_if.h"
#include "src/r_ether_rx_private.h"



/***********************************************************************************************************************
Macro definitions
***********************************************************************************************************************/
#define MINIMUM_SIZES_OF_ETHER_FRAME_EXCEPT_CRC     (60)

/***********************************************************************************************************************
Typedef definitions
***********************************************************************************************************************/

/***********************************************************************************************************************
Exported global variables (to be accessed by other files)
***********************************************************************************************************************/

/***********************************************************************************************************************
Private global variables and functions
***********************************************************************************************************************/

/*
 * Private global function prototypes
 */
static void             ether_reset_mac(uint32_t channel);
static void             ether_init_descriptors(uint32_t channel);
static void             ether_config_ethernet(uint32_t channel, const uint8_t mode);
static void             ether_pause_resolution(uint16_t local_ability, uint16_t partner_ability,
                                               uint16_t *ptx_pause, uint16_t *prx_pause);
static void             ether_configure_mac(uint32_t channel, const uint8_t mac_addr[], const uint8_t mode);
static ether_return_t   ether_do_link(uint32_t channel, const uint8_t mode);
static ether_return_t   ether_set_callback(ether_param_t const control);
static ether_return_t   ether_set_promiscuous_mode(ether_param_t const control);
static ether_return_t   ether_set_int_handler(ether_param_t const control);
static ether_return_t   ether_power_on(ether_param_t const control);
static ether_return_t   ether_power_off(ether_param_t const control);
static ether_return_t   power_on(uint32_t channel);
static void             power_off(uint32_t channel);
static void             ether_eint0(void * pparam);
static void             ether_eint1(void * pparam);
static void             ether_int_common(uint32_t channel);
static ether_return_t   power_on_control(uint32_t channel);
static void             power_off_control(uint32_t channel);
static uint8_t          check_mpde_bit(void);

/*
 * Private global variables
 */

/* Pointer to the receive descriptors */
static descriptor_t   * papp_rx_desc[ETHER_CHANNEL_MAX] = {NULL, NULL};

/* Pointer to the transmit descriptors */
static descriptor_t   * papp_tx_desc[ETHER_CHANNEL_MAX] = {NULL, NULL};

/* Pointer to the callback function */
static ether_cb_t       cb_func;

static bool             is_entry = false;

/* 
 * The flag which control the pause frame.
 *
 * The value of flag and the situation which is indicatived of by the value.
 * ETHER_FLAG_OFF (0): Don't use the pause frame (default). 
 * ETHER_FLAG_ON  (1): Use the pause frame. 
 */
static uint8_t          pause_frame_enable[ETHER_CHANNEL_MAX] = { ETHER_FLAG_OFF, ETHER_FLAG_OFF };

/*
 * The flag indicatives of the state that the interrupt of Link Up/Down occur.
 * 
 * Value and state of flag
 * ETHER_FLAG_OFF (0): It is not possible to communicate. 
 * ETHER_FLAG_ON  (1): It is possible to communicate. 
 */
static uint8_t  transfer_enable_flag[ETHER_CHANNEL_MAX];

/*
 * The flag indicatives of the state that the interrupt of magic packet detection occur.
 *
 * Value and state of flag
 * ETHER_FLAG_OFF (0): The interrupt of the magic packet detection has not been generated. 
 * ETHER_FLAG_ON  (1): The interrupt of the magic packet detection was generated. 
 *
 * If the R_ETHER_LinkProcess function is called, and the interrupt processing of the magic packet detection is done,
 * this flag becomes ETHER_FLAG_OFF(0). 
 */
static uint8_t  mpd_flag[ETHER_CHANNEL_MAX];

static uint8_t  mac_addr_buf[ETHER_CHANNEL_MAX][6];

/*
 * The flag indicatives of the state that the interrupt of Link Up/Down occur.
 * 
 * Value and state of flag
 * ETHER_FLAG_OFF         (0) : The Link up/down interrupt has not been generated. 
 * ETHER_FLAG_ON_LINK_OFF (2) : The Link down interrupt was generated. 
 * ETHER_FLAG_ON_LINK_ON  (3) : The Link up interrupt was generated. 
 * 
 * If the R_ETHER_LinkProcess function is called, and the interrupt processing of Link Up/Down is done,
 * this flag becomes ETHER_FLAG_OFF(0). 
 */
static uint8_t  lchng_flag[ETHER_CHANNEL_MAX];

/*
 * PAUSE Resolution as documented in IEEE 802.3-2008_section2 Annex
 * 28B, Table 28B-3. The following table codify logic that
 * determines how the PAUSE is configured for local transmitter
 * and receiver and partner transmitter and receiver.
 */
static const pauseresolution_t pause_resolution[PAUSE_TABLE_ENTRIES] =
{
    { PAUSE_MASKC, PAUSE_VAL0, XMIT_PAUSE_OFF, RECV_PAUSE_OFF },
    { PAUSE_MASKE, PAUSE_VAL4, XMIT_PAUSE_OFF, RECV_PAUSE_OFF },
    { PAUSE_MASKF, PAUSE_VAL6, XMIT_PAUSE_OFF, RECV_PAUSE_OFF },
    { PAUSE_MASKF, PAUSE_VAL7, XMIT_PAUSE_ON,  RECV_PAUSE_OFF },
    { PAUSE_MASKE, PAUSE_VAL8, XMIT_PAUSE_OFF, RECV_PAUSE_OFF },
    { PAUSE_MASKA, PAUSE_VALA, XMIT_PAUSE_ON,  RECV_PAUSE_ON  },
    { PAUSE_MASKF, PAUSE_VALC, XMIT_PAUSE_OFF, RECV_PAUSE_OFF },
    { PAUSE_MASKF, PAUSE_VALD, XMIT_PAUSE_OFF, RECV_PAUSE_ON  }
};

/*
 * Receive, transmit descriptors and their buffer.  They are
 * defined with section pragma directives to easily locate them
 * on the memory map.
 */
#pragma section _RX_DESC
static descriptor_t    rx_descriptors[ETHER_CHANNEL_MAX][ETHER_CFG_EMAC_RX_DESCRIPTORS];
#pragma section _TX_DESC
static descriptor_t    tx_descriptors[ETHER_CHANNEL_MAX][ETHER_CFG_EMAC_TX_DESCRIPTORS];

/* 
 * As for Ethernet buffer, the size of total buffer which are use for transmission and the reception is secured.
 * The total buffer's size which the value is integrated from  EMAC_NUM_BUFFERS (buffer number) and 
 * ETHER_CFG_BUFSIZE (the size of one buffer).
 * The ETHER_CFG_BUFSIZE and EMAC_NUM_BUFFERS are defined by macro in the file "r_ether_private.h".
 * It is sequentially used from the head of the buffer as a receive buffer or a transmission buffer.
 */
#pragma section _ETHERNET_BUFFERS
static etherbuffer_t   ether_buffers[ETHER_CHANNEL_MAX];

#pragma section

static  uint8_t         promiscuous_mode[ETHER_CHANNEL_MAX];


static const ether_control_t ether_ch_0[]=
{
    /* Ether = ch0, Phy access = ch0 */
    {   &ETHERC0,    &EDMAC0,    &ETHERC0.PIR.LONG,  ETHER_CFG_CH0_PHY_ADDRESS, PORT_CONNECT_ET0        },

    /* Ether = ch0, Phy access = ch1 */
    {   &ETHERC0,    &EDMAC0,    &ETHERC1.PIR.LONG,  ETHER_CFG_CH0_PHY_ADDRESS, PORT_CONNECT_ET0_ET1    }
};

static const ether_control_t ether_ch_1[]=
{
    /* Ether = ch1, Phy access = ch0 */
    {   &ETHERC1,    &EDMAC1,    &ETHERC0.PIR.LONG,  ETHER_CFG_CH1_PHY_ADDRESS, PORT_CONNECT_ET0_ET1    },

    /* Ether = ch1, Phy access = ch1 */
    {   &ETHERC1,    &EDMAC1,    &ETHERC1.PIR.LONG,  ETHER_CFG_CH1_PHY_ADDRESS, PORT_CONNECT_ET1        }
};

#if ETHER_CFG_CH0_PHY_ACCESS > 1
    #error "ERROR - ETHER_CFG_CH0_PHY_ACCESS - Unknown register bus is defined in r_ether_rx_config.h"
#endif
#if ETHER_CFG_CH1_PHY_ACCESS > 1
    #error "ERROR - ETHER_CFG_CH1_PHY_ACCESS - Unknown register bus is defined in r_ether_rx_config.h"
#endif

/* This table is used to convert it from the channel number for the communication
   into the channel number for the PHY register access. */
   
static const uint32_t  ether_phy_access[] = 
{
    ETHER_CFG_CH0_PHY_ACCESS,
    ETHER_CFG_CH1_PHY_ACCESS
};


static uint8_t etherc_edmac_power_cont[2][2];

const ether_ch_control_t g_eth_control_ch[] =
{
    {   ether_ch_0, ETHER_CFG_CH0_PHY_ACCESS    },
    {   ether_ch_1, ETHER_CFG_CH1_PHY_ACCESS    }
}; 

/*
 * Renesas Ethernet API functions
 */
/***********************************************************************************************************************
* Function Name: R_ETHER_Initial (Implementation to file hwsetup.c)
* Description  : Initialization of Ether driver.
* Arguments    : none
* Return Value : none
***********************************************************************************************************************/
void R_ETHER_Initial(void)
{
    /* Initialize the transmit and receive descriptor */
    memset(&rx_descriptors, 0x00, sizeof(rx_descriptors));
    memset(&tx_descriptors, 0x00, sizeof(tx_descriptors));

    /* Initialize the Ether buffer */
    memset(&ether_buffers,  0x00, sizeof(ether_buffers));
    
    memset(etherc_edmac_power_cont,  0x00, sizeof(etherc_edmac_power_cont));

    /* Initialize the callback function pointer */
    cb_func.pcb_func = NULL;

    /* Initialize the interrupt handler pointer */
    cb_func.pcb_int_hnd = NULL;

    /* Initialize */
    promiscuous_mode[ETHER_CHANNEL_0] = ETHER_PROMISCUOUS_OFF;
    promiscuous_mode[ETHER_CHANNEL_1] = ETHER_PROMISCUOUS_OFF;

} /* End of function R_ETHER_Initial() */

/***********************************************************************************************************************
* Function Name: R_ETHER_Open_ZC2
* Description  : After ETHERC, EDMAC,0x00000002; and PHY-LSI are reset in software, an auto negotiation of PHY-LSI is
*                 begun.
*                Afterwards, the link signal change interrupt is permitted. 
* Arguments    : channel -
*                    ETHERC channel number
*                mac_addr -
*                    The MAC address of ETHERC
*                pause -
*                    Specifies whether flow control functionality is enabled or disabled.
* Return Value : ETHER_SUCCESS -
*                    Processing completed successfully
*                ETHER_ERR_INVALID_CHAN - 
*                    Nonexistent channel number
*                ETHER_ERR_INVALID_PTR -
*                    Value of the pointer is NULL or FIT_NO_PTR
*                ETHER_ERR_INVALID_DATA -
*                    Value of the argument is out of range
*                ETHER_ERR_OTHER -
*                    
***********************************************************************************************************************/
ether_return_t R_ETHER_Open_ZC2(uint32_t channel, const uint8_t mac_addr[], uint8_t pause)
{
    ether_return_t          ret;
    int16_t                 phy_ret;
    volatile bsp_int_err_t  bsp_int_err;
    const ether_control_t * pether_ch;
    uint32_t                phy_access;

    volatile struct st_etherc __evenaccess    * etherc_adr;
    volatile struct st_edmac __evenaccess     * edmac_adr;

    /* Check argument */
    if (ETHER_CHANNEL_MAX <= channel)
    {
        return ETHER_ERR_INVALID_CHAN;
    }
    if ((NULL == mac_addr) || (FIT_NO_PTR == mac_addr))
    {
        return ETHER_ERR_INVALID_PTR;
    }
    if ((ETHER_FLAG_OFF != pause) && (ETHER_FLAG_ON != pause))
    {
        return ETHER_ERR_INVALID_DATA;
    }

    pether_ch  = g_eth_control_ch[channel].pether_control;
    phy_access = g_eth_control_ch[channel].phy_access;
    etherc_adr = pether_ch[phy_access].petherc;
    edmac_adr  = pether_ch[phy_access].pedmac;

    /* Initialize the flags */
    transfer_enable_flag[channel] = ETHER_FLAG_OFF;
    mpd_flag[channel]             = ETHER_FLAG_OFF;
    lchng_flag[channel]           = ETHER_FLAG_OFF;

    pause_frame_enable[channel] = pause;

    mac_addr_buf[channel][0] = mac_addr[0];
    mac_addr_buf[channel][1] = mac_addr[1];
    mac_addr_buf[channel][2] = mac_addr[2];
    mac_addr_buf[channel][3] = mac_addr[3];
    mac_addr_buf[channel][4] = mac_addr[4];
    mac_addr_buf[channel][5] = mac_addr[5];

    /* Subscribe to r_bsp an interrupt function */
    if(false == is_entry)
    {
        bsp_int_err = R_BSP_InterruptWrite(BSP_INT_SRC_AL1_EDMAC0_EINT0, ether_eint0);  /* EINT0 */
        bsp_int_err = R_BSP_InterruptWrite(BSP_INT_SRC_AL1_EDMAC1_EINT1, ether_eint1);  /* EINT1 */
        is_entry    = true;
    }
    
    /* Software reset */
    ether_reset_mac(channel);

    /* Software reset the PHY */
    phy_ret = phy_init(channel);
    if (R_PHY_OK == phy_ret)
    {
        phy_start_autonegotiate(channel, pause_frame_enable[channel]);
        
        /* Clear all ETHERC status BFR, PSRTO, LCHNG, MPD, ICD */
        etherc_adr->ECSR.LONG = 0x00000037;

        /* Clear all EDMAC status bits */
        edmac_adr->EESR.LONG  = 0x47FF0F9F;

        /* Enable interrupts of interest only. */
        etherc_adr->ECSIPR.BIT.LCHNGIP = 1;

        edmac_adr->EESIPR.BIT.ECIIP    = 1;

        /* Set Ethernet interrupt level and enable */
        ether_enable_icu(channel);

        ret = ETHER_SUCCESS;
    }
    else
    {
        ret = ETHER_ERR_OTHER;
    }

    return ret;
} /* End of function R_ETHER_Open_ZC2() */

/***********************************************************************************************************************
* Function Name: R_ETHER_Close_ZC2
* Description  : Disables Ethernet peripheral
* Arguments    : channel -
*                    ETHERC channel number
* Return Value : ETHER_SUCCESS -
*                    Processing completed successfully
*                ETHER_ERR_INVALID_CHAN - 
*                    Nonexistent channel number
***********************************************************************************************************************/
ether_return_t R_ETHER_Close_ZC2(uint32_t channel)
{
    volatile struct st_etherc __evenaccess    * etherc_adr;
    volatile struct st_edmac __evenaccess     * edmac_adr;
    const ether_control_t                     * pether_ch;
    uint32_t                                    phy_access;

    /* Check argument */
    if (ETHER_CHANNEL_MAX <= channel)
    {
        return ETHER_ERR_INVALID_CHAN;
    }

    pether_ch  = g_eth_control_ch[channel].pether_control;
    phy_access = g_eth_control_ch[channel].phy_access;
    etherc_adr = pether_ch[phy_access].petherc;
    edmac_adr  = pether_ch[phy_access].pedmac;

    /* Disable Ethernet interrupt. */
    ether_disable_icu(channel);
    
    etherc_adr->ECSIPR.BIT.LCHNGIP = 0;
    edmac_adr->EESIPR.BIT.ECIIP    = 0;
    
    ether_clear_icu_source(channel);
    
    /* Disable TE and RE  */
    etherc_adr->ECMR.LONG  = 0x00000000;
    
    /* Initialize the flags */
    transfer_enable_flag[channel] = ETHER_FLAG_OFF;
    mpd_flag[channel]             = ETHER_FLAG_OFF;
    lchng_flag[channel]           = ETHER_FLAG_OFF;

    return ETHER_SUCCESS;
} /* End of function R_ETHER_Close_ZC2() */

/***********************************************************************************************************************
* Function Name: R_ETHER_Read_ZC2
* Description  : Receives an Ethernet frame.  Sets the passed
*                buffer pointer to the Ethernet frame buffer
*                from the driver. This makes the data available to
*                the caller as a zero-copy operation.
* Arguments    : channel -
*                    ETHERC channel number
*                pbuf -
*                    Pointer to buffer that holds the receive data
* Return Value : Returns the number of bytes received.
*                ETHER_NO_DATA -
*                    A zero value indicates no data is received.
*                ETHER_ERR_INVALID_CHAN - 
*                    Nonexistent channel number
*                ETHER_ERR_INVALID_PTR -
*                    Value of the pointer is NULL or FIT_NO_PTR
*                ETHER_ERR_LINK - 
*                    Auto-negotiation is not completed, and reception is not enabled.
*                ETHER_ERR_MPDE -
*                    As a Magic Packet is being detected, transmission and reception is not enabled.
***********************************************************************************************************************/
int32_t R_ETHER_Read_ZC2(uint32_t channel, void **pbuf)
{
    int32_t     num_recvd;
    int32_t     ret;
    int32_t     complete_flag;

    /* Check argument */
    if (ETHER_CHANNEL_MAX <= channel)
    {
        return ETHER_ERR_INVALID_CHAN;
    }
    if ((NULL == pbuf) || (FIT_NO_PTR == pbuf))
    {
        return ETHER_ERR_INVALID_PTR;
    }

    /* When the Link up processing is not completed, return error */
    if (ETHER_FLAG_OFF == transfer_enable_flag[channel])
    {
        ret = ETHER_ERR_LINK;
    }

    /* In case of detection mode of magic packet, return error. */
    else if (check_mpde_bit())
    
    {
        ret = ETHER_ERR_MPDE;
    }

    /* When the Link up processing is completed */
    else
    {
        complete_flag = ETHER_ERR_OTHER;
        while (ETHER_SUCCESS != complete_flag)
        {
            /* When receive data exists. */
            if (RACT != (papp_rx_desc[channel]->status & RACT))
            {
                if (papp_rx_desc[channel]->status & RFE)
                {
                    /* The buffer is released at the error.  */
                    ret = R_ETHER_Read_ZC2_BufRelease(channel);
                }
                else
                {
                    /**
                     * Pass the pointer to received data to application.  This is
                     * zero-copy operation.
                     */
                    (*pbuf) = (void *)papp_rx_desc[channel]->buf_p;

                    /* Get bytes received */
                    num_recvd     = papp_rx_desc[channel]->size;
                    ret           = num_recvd;
                    complete_flag = ETHER_SUCCESS;
                }
            }
            else
            {
                ret           = ETHER_NO_DATA;
                complete_flag = ETHER_SUCCESS;
            }
        }
    }
    return ret;
} /* End of function R_ETHER_Read_ZC2() */

/***********************************************************************************************************************
* Function Name: R_ETHER_Read_ZC2_BufRelease
* Description  : Release the receive buffer.
* Arguments    : channel -
*                    Specifies the ETHERC channel number.
* Return Value : ETHER_SUCCESS -
*                    Processing completed successfully
*                ETHER_ERR_INVALID_CHAN - 
*                    Nonexistent channel number
*                ETHER_ERR_LINK - 
*                    Auto-negotiation is not completed, and reception is not enabled.
*                ETHER_ERR_MPDE -
*                    As a Magic Packet is being detected, transmission and reception is not enabled.
***********************************************************************************************************************/
int32_t R_ETHER_Read_ZC2_BufRelease(uint32_t channel)
{
    int32_t     ret;
    volatile struct st_edmac __evenaccess * edmac_adr;
    const ether_control_t                 * pether_ch;
    uint32_t                                phy_access;
    
    /* Check argument */
    if (ETHER_CHANNEL_MAX <= channel)
    {
        return ETHER_ERR_INVALID_CHAN;
    }

    /* When the Link up processing is not completed, return error */
    if (ETHER_FLAG_OFF == transfer_enable_flag[channel])
    {
        ret = ETHER_ERR_LINK;
    }

    /* In case of detection mode of magic packet, return error. */
    else if (check_mpde_bit())

    {
        ret = ETHER_ERR_MPDE;
    }

    /* When the Link up processing is completed */
    else
    {
        /* When receive data exists */
        if (RACT != (papp_rx_desc[channel]->status & RACT))
        {
            /* Move to next descriptor */
            papp_rx_desc[channel]->status |= RACT;
            {
                uint32_t   status;
                
                status  = RFP1;
                status |= RFP0;
                status |= RFE;
                status |= RFS9_RFOVER;
                status |= RFS8_RAD;
                status |= RFS7_RMAF;
                status |= RFS4_RRF;
                status |= RFS3_RTLF;
                status |= RFS2_RTSF;
                status |= RFS1_PRE;
                status |= RFS0_CERF;
                
                papp_rx_desc[channel]->status &= (~status);
            }
            papp_rx_desc[channel] = papp_rx_desc[channel]->next;
        }
        pether_ch  = g_eth_control_ch[channel].pether_control;
        phy_access = g_eth_control_ch[channel].phy_access;
        edmac_adr  = pether_ch[phy_access].pedmac;
        
        if (0x00000000L == edmac_adr->EDRRR.LONG)
        {
            /* Restart if stopped */
            edmac_adr->EDRRR.LONG = 0x00000001L;
        }
        
        ret = ETHER_SUCCESS;
    }
    return ret;
} /* End of function R_ETHER_Read_ZC2_BufRelease() */

/***********************************************************************************************************************
* Function Name: R_ETHER_Write_ZC2_GetBuf
* Description  : Get Points to the buffer pointer used by the stack.
* Arguments    : channel -
*                    ETHERC channel number
*                pbuf -
*                    Pointer to the starting address of the transmit data destination
*                pbuf_size -
*                    The Maximum size to write to the buffer
* Return Value : ETHER_SUCCESS -
*                    Processing completed successfully
*                ETHER_ERR_INVALID_CHAN - 
*                    Nonexistent channel number
*                ETHER_ERR_INVALID_PTR -
*                    Value of the pointer is NULL or FIT_NO_PTR
*                ETHER_ERR_LINK - 
*                    Auto-negotiation is not completed, and reception is not enabled.
*                ETHER_ERR_MPDE - 
*                    As a Magic Packet is being detected, transmission and reception is not enabled.
*                ETHER_ERR_TACT -
*                    Transmit buffer is not empty.
***********************************************************************************************************************/
ether_return_t R_ETHER_Write_ZC2_GetBuf(uint32_t channel, void **pbuf, uint16_t *pbuf_size)
{
    ether_return_t  ret;

    /* Check argument */
    if (ETHER_CHANNEL_MAX <= channel)
    {
        return ETHER_ERR_INVALID_CHAN;
    }
    if ((NULL == pbuf) || (FIT_NO_PTR == pbuf))
    {
        return ETHER_ERR_INVALID_PTR;
    }
    if ((NULL == pbuf_size) || (FIT_NO_PTR == pbuf_size))
    {
        return ETHER_ERR_INVALID_PTR;
    }

    /* When the Link up processing is not completed, return error */
    if (ETHER_FLAG_OFF == transfer_enable_flag[channel])
    {
        ret = ETHER_ERR_LINK;
    }

    /* In case of detection mode of magic packet, return error. */
    else if (check_mpde_bit())
    {
        ret = ETHER_ERR_MPDE;
    }

    /* When the Link up processing is completed */
    else
    {
        /* All transmit buffers are full */
        if (TACT == (papp_tx_desc[channel]->status & TACT))
        {
            ret = ETHER_ERR_TACT;
        }
        else
        {
            /* Give application another buffer to work with */
            (*pbuf)      = papp_tx_desc[channel]->buf_p;
            (*pbuf_size) = ETHER_CFG_BUFSIZE;
            ret        = ETHER_SUCCESS;
        }
    }
    return ret;
} /* End of function R_ETHER_Write_ZC2_GetBuf() */

/***********************************************************************************************************************
* Function Name: R_ETHER_Write_ZC2_SetBuf
* Description  : Transmits an Ethernet frame. 
*                The transmit descriptor points to the data to transmit. 
*                Data is sent directly from memory as a "zero copy" operation.
* Arguments    : channel -
*                    ETHERC channel number
*                len -
*                    The size (60 to 1,514 bytes) which is the Ethernet frame length minus 4 bytes of CRC
* Return Value : ETHER_SUCCESS -
*                    Processing completed successfully
*                ETHER_ERR_INVALID_CHAN - 
*                    Nonexistent channel number
*                ETHER_ERR_INVALID_DATA -
*                    Value of the argument is out of range
*                ETHER_ERR_LINK -
*                    Auto-negotiation is not completed, and reception is not enabled.
*                ETHER_ERR_MPDE -
*                    As a Magic Packet is being detected, transmission and reception is not enabled.
***********************************************************************************************************************/
ether_return_t R_ETHER_Write_ZC2_SetBuf(uint32_t channel, const uint32_t len)
{
    ether_return_t  ret;
    volatile struct st_edmac __evenaccess * edmac_adr;
    const ether_control_t                 * pether_ch;
    uint32_t                                phy_access;

    /* Check argument */
    if (ETHER_CHANNEL_MAX <= channel)
    {
        return ETHER_ERR_INVALID_CHAN;
    }
    if ((ETHER_BUFSIZE_MIN > len) || (ETHER_BUFSIZE_MAX < len))
    {
        return ETHER_ERR_INVALID_DATA;
    }

    /* When the Link up processing is not completed, return error */
    if (ETHER_FLAG_OFF == transfer_enable_flag[channel])
    {
        ret = ETHER_ERR_LINK;
    }

    /* In case of detection mode of magic packet, return error. */
    else if (check_mpde_bit())
    
    {
        ret = ETHER_ERR_MPDE;
    }

    /* When the Link up processing is completed */
    else
    {
        /* The data of the buffer is made active.  */
        papp_tx_desc[channel]->bufsize = len;
        papp_tx_desc[channel]->status &= (~(TFP1 | TFP0));
        papp_tx_desc[channel]->status |= ((TFP1 | TFP0) | TACT);
        papp_tx_desc[channel]          = papp_tx_desc[channel]->next;
        
        pether_ch  = g_eth_control_ch[channel].pether_control;
        phy_access = g_eth_control_ch[channel].phy_access;
        edmac_adr  = pether_ch[phy_access].pedmac;
        
        if (0x00000000L == edmac_adr->EDTRR.LONG)
        {
            /* Restart if stopped */
            edmac_adr->EDTRR.LONG = 0x00000001L;
        }
        
        ret = ETHER_SUCCESS;
    }
    return ret;
} /* End of function R_ETHER_Write_ZC2_SetBuf() */

/***********************************************************************************************************************
* Function Name: R_ETHER_CheckLink_ZC
* Description  : Verifies the Etherent link is up or not.
* Arguments    : channel -
*                    ETHERC channel number
* Return Value : ETHER_SUCCESS -
*                    Link is up
*                ETHER_ERR_OTHER -
*                    Link is down
*                ETHER_ERR_INVALID_CHAN - 
*                    Nonexistent channel number
***********************************************************************************************************************/
ether_return_t R_ETHER_CheckLink_ZC(uint32_t channel)
{
    int16_t     status;

    /* Check argument */
    if (ETHER_CHANNEL_MAX <= channel)
    {
        return ETHER_ERR_INVALID_CHAN;
    }

    status = phy_get_link_status(channel);

    if (R_PHY_ERROR == status)
    {
        /* Link is down */
        return ETHER_ERR_OTHER;
    }
    else
    {
        /* Link is up */
        return ETHER_SUCCESS;
    }
} /* End of function R_ETHER_CheckLink_ZC() */

/***********************************************************************************************************************
* Function Name: R_ETHER_LinkProcess
* Description  : The Link up processing, the Link down processing, and the magic packet detection processing are
*                 executed. 
* Arguments    : channel -
*                    ETHERC channel number
* Return Value : none
***********************************************************************************************************************/
void R_ETHER_LinkProcess(uint32_t channel)
{
    volatile struct st_etherc __evenaccess    * etherc_adr;
    const ether_control_t                     * pether_ch;
    uint32_t                                    phy_access;

    int32_t         ret;
    ether_cb_arg_t  cb_arg;

    if (ETHER_CHANNEL_MAX <= channel)
    {
        return;
    }

    /* When the magic packet is detected. */
    if (ETHER_FLAG_ON == mpd_flag[channel])
    {
        mpd_flag[channel] = ETHER_FLAG_OFF;
        
        if ((NULL != cb_func.pcb_func) && (FIT_NO_FUNC != cb_func.pcb_func))
        {
            cb_arg.channel  = channel;
            cb_arg.event_id = ETHER_CB_EVENT_ID_WAKEON_LAN;
            (*cb_func.pcb_func)((void *)&cb_arg);

            /*
             * After the close function is called, the open function is called 
             * to have to set ETHERC to a usual operational mode
             * to usually communicate after magic packet is detected. 
             */
            R_ETHER_Close_ZC2(channel);
            R_ETHER_Open_ZC2(channel, mac_addr_buf[channel], pause_frame_enable[channel]);
            
        }
    }

    /* When the link is up */
    if (ETHER_FLAG_ON_LINK_ON == lchng_flag[channel])
    {  
        /* 
         * The Link Up/Down is confirmed by the Link Status bit of PHY register1, 
         * because the LINK signal of PHY-LSI is used for LED indicator, and 
         * isn't used for notifing the Link Up/Down to external device.
         */
        ret = R_ETHER_CheckLink_ZC(channel);
        if (ETHER_SUCCESS == ret)
        {
            /*
             * The status of the LINK signal became "link-up" even if PHY-LSI did not detect "link-up"
             * after a reset. To avoid this wrong detection, processing in R_ETHER_LinkProcess has been modified to
             * clear the flag after link-up is confirmed in R_ETHER_LinkCheck_ZC.
             */
            lchng_flag[channel] = ETHER_FLAG_OFF;

            /*
             * ETHERC and EDMAC are set after ETHERC and EDMAC are reset in software
             * and sending and receiving is permitted. 
             */
            ether_configure_mac(channel, mac_addr_buf[channel], NO_USE_MAGIC_PACKET_DETECT);
            ether_do_link(channel, NO_USE_MAGIC_PACKET_DETECT);
            
            transfer_enable_flag[channel] = ETHER_FLAG_ON;
            
            if ((NULL != cb_func.pcb_func) && (FIT_NO_FUNC != cb_func.pcb_func))
            {
                cb_arg.channel  = channel;
                cb_arg.event_id = ETHER_CB_EVENT_ID_LINK_ON;
                (*cb_func.pcb_func)((void *)&cb_arg);
            }
        }
        else
        {
            /* no process */
        }
    }

    /* When the link is down */
    else if (ETHER_FLAG_ON_LINK_OFF == lchng_flag[channel])
    {
        lchng_flag[channel] = ETHER_FLAG_OFF;

        /* 
         * The Link Up/Down is confirmed by the Link Status bit of PHY register1, 
         * because the LINK signal of PHY-LSI is used for LED indicator, and 
         * isn't used for notifying the Link Up/Down to external device.
         */
        ret = R_ETHER_CheckLink_ZC(channel);
        if (ETHER_ERR_OTHER == ret)
        {
            pether_ch  = g_eth_control_ch[channel].pether_control;
            phy_access = g_eth_control_ch[channel].phy_access;
            etherc_adr = pether_ch[phy_access].petherc;
            
            /* Disable receive and transmit. */
            etherc_adr->ECMR.BIT.RE = 0;
            etherc_adr->ECMR.BIT.TE = 0;
            
            transfer_enable_flag[channel] = ETHER_FLAG_OFF;
            
            if ((NULL != cb_func.pcb_func) && (FIT_NO_FUNC != cb_func.pcb_func))
            {
                cb_arg.channel  = channel;
                cb_arg.event_id = ETHER_CB_EVENT_ID_LINK_OFF;
                (*cb_func.pcb_func)((void *)&cb_arg);
            }
        }
        else
        {
            ; /* no operation */
        }
    }
    else
    {
        ; /* no operation */
    }
} /* End of function R_ETHER_LinkProcess() */

/***********************************************************************************************************************
* Function Name: R_ETHER_WakeOnLAN
* Description  : The setting of ETHERC is changed from a usual sending and
*                receiving mode to the magic packet detection mode. 
* Arguments    : channel -
*                    ETHERC channel number
* Return Value : ETHER_SUCCESS -
*                    Processing completed successfully
*                ETHER_ERR_INVALID_CHAN - 
*                    Nonexistent channel number
*                ETHER_ERR_LINK - 
*                    Auto-negotiation is not completed, and reception is not enabled.
*                ETHER_ERR_OTHER -
*                    A switch to magic packet detection was performed when the link state was link is down.
***********************************************************************************************************************/
ether_return_t R_ETHER_WakeOnLAN(uint32_t channel)
{
    volatile struct st_etherc __evenaccess    * etherc_adr;
    const ether_control_t                     * pether_ch;
    uint32_t                                    phy_access;
    ether_return_t                              ret;

    /* Check argument */
    if (ETHER_CHANNEL_MAX <= channel)
    {
        return ETHER_ERR_INVALID_CHAN;
    }

    /* When the Link up processing is not completed, return error */
    if (ETHER_FLAG_OFF == transfer_enable_flag[channel])
    {
        ret = ETHER_ERR_LINK;
    }

    /* When the Link up processing is completed */
    else
    {
        /* Change to the magic packet detection mode.  */
        ether_configure_mac(channel, mac_addr_buf[channel], USE_MAGIC_PACKET_DETECT);
        ret = ether_do_link(channel, USE_MAGIC_PACKET_DETECT);
        if (ETHER_SUCCESS == ret)
        {

            pether_ch  = g_eth_control_ch[channel].pether_control;
            phy_access = g_eth_control_ch[channel].phy_access;
            etherc_adr = pether_ch[phy_access].petherc;

            /* It is confirmed not to become Link down while changing the setting. */
            if (ETHER_CFG_LINK_PRESENT == etherc_adr->PSR.BIT.LMON)
            {
                ret = ETHER_SUCCESS;
            }
            else
            {
                ret = ETHER_ERR_OTHER;
            }
        }
        else
        {
            ret = ETHER_ERR_OTHER;
        }
    }
    return ret;
} /* End of function R_ETHER_WakeOnLAN() */

/***********************************************************************************************************************
* Function Name: R_ETHER_Read
* Description  : Receive Ethernet frame. Receives data to the location specified by the pointer to the receive buffer,
*                using non-zero-copy communication.
* Arguments    : channel -
*                    ETHERC channel number
*                pbuf - 
*                    The receive buffer (to store the receive data)
* Return Value : Returns the number of bytes received.
*                ETHER_NO_DATA -
*                    A zero value indicates no data is received.
*                ETHER_ERR_INVALID_CHAN - 
*                    Nonexistent channel number
*                ETHER_ERR_INVALID_PTR -
*                    Value of the pointer is NULL or FIT_NO_PTR
*                ETHER_ERR_LINK - 
*                    Auto-negotiation is not completed, and reception is not enabled.
*                ETHER_ERR_MPDE -
*                    As a Magic Packet is being detected, transmission and reception is not enabled.
***********************************************************************************************************************/
int32_t R_ETHER_Read(uint32_t channel, void *pbuf)
{
    int32_t     ret;
    int32_t     ret2;
    uint8_t   * pread_buffer_address;  /* Buffer location controlled by the Ethernet driver */

    /* Check argument */
    if (ETHER_CHANNEL_MAX <= channel)
    {
        return ETHER_ERR_INVALID_CHAN;
    }
    if ((NULL == pbuf) || (FIT_NO_PTR == pbuf))
    {
        return ETHER_ERR_INVALID_PTR;
    }

    /* (1) Retrieve the receive buffer location controlled by the  descriptor. */
    ret = R_ETHER_Read_ZC2(channel, (void **)&pread_buffer_address);
    
    /* When there is data to receive */
    if (ret > ETHER_NO_DATA)
    {
        /* (2) Copy the data read from the receive buffer which is controlled by the descriptor to
               the buffer which is specified by the user (up to 1024 bytes). */
        memcpy(pbuf, pread_buffer_address, (uint32_t)ret);
        
        /* (3) Read the receive data from the receive buffer controlled by the descriptor,
               and then release the receive buffer. */
        ret2 = R_ETHER_Read_ZC2_BufRelease(channel);

        if (ETHER_SUCCESS == ret2)  /* When this function is completed successfully */
        {
            /* Do Nothing */
        }

        /* Overwrite the error information */
        else
        {
            ret = ret2;
        }
    }

    /* When there is no data to receive */
    else
    {
        /* Do Nothing */
    }
    return ret;
} /* End of function R_ETHER_Read() */

/***********************************************************************************************************************
* Function Name: R_ETHER_Write
* Description  : Transmit Ethernet frame. Transmits data from the location specified by the pointer to the transmit 
*                buffer, with the data size equal to the specified frame length, using non-zero-copy communication.
* Arguments    : channel -
*                    ETHERC channel number
*              : *pbuf -
*                    Transmit buffer pointer
*              : len -
*                    The size (60 to 1,514 bytes) which is the Ethernet frame length minus 4 bytes of CRC
* Return Value : ETHER_SUCCESS -
*                    Processing completed successfully
*                ETHER_ERR_INVALID_CHAN - 
*                    Nonexistent channel number
*                ETHER_ERR_INVALID_DATA -
*                    Value of the argument is out of range
*                ETHER_ERR_INVALID_PTR -
*                    Value of the pointer is NULL or FIT_NO_PTR
*                ETHER_ERR_LINK - 
*                    Auto-negotiation is not completed, and reception is not enabled.
*                ETHER_ERR_MPDE -
*                    As a Magic Packet is being detected, transmission and reception is not enabled.
*                ETHER_ERR_TACT -
*                    Transmit buffer is not empty.
***********************************************************************************************************************/
ether_return_t R_ETHER_Write(uint32_t channel, void *pbuf, const uint32_t len)
{
    ether_return_t  ret;
    uint8_t       * pwrite_buffer_address;
    uint16_t        write_buf_size;

    if (ETHER_CHANNEL_MAX <= channel)
    {
        return ETHER_ERR_INVALID_CHAN;
    }
    if ((NULL == pbuf) || (FIT_NO_PTR == pbuf))
    {
        return ETHER_ERR_INVALID_PTR;
    }
    if ((ETHER_BUFSIZE_MIN > len) || (ETHER_BUFSIZE_MAX < len))
    {
        return ETHER_ERR_INVALID_DATA;
    }
    
    /* (1) Retrieve the transmit buffer location controlled by the  descriptor. */
    ret = R_ETHER_Write_ZC2_GetBuf(channel, (void **)&pwrite_buffer_address, &write_buf_size);
    
    /* Writing to the transmit buffer (buf) is enabled. */
    if (ETHER_SUCCESS == ret)
    {
        if (write_buf_size < len)
        {
            ret = ETHER_ERR_TACT;   /* Transmit buffer overflow */
        }
        else
        {
            /* Write the transmit data to the transmit buffer. */
            /* 
             * Write 60 to 1514 bytes of data in the transmit buffer (Ethernet frame minus 4 bytes of CRC).
             * To transmit data less than 60 bytes, make sure to pad the data with zero to be 60 bytes.
             */
            if (MINIMUM_SIZES_OF_ETHER_FRAME_EXCEPT_CRC > len)
            {
                memset(pwrite_buffer_address, 0x00, (uint32_t)MINIMUM_SIZES_OF_ETHER_FRAME_EXCEPT_CRC);
            }
            
            /* (2) Write the data to the transmit buffer controlled by the  descriptor. */
            memcpy(pwrite_buffer_address, pbuf, len);
            
            /* (3) Enable the EDMAC to transmit data in the transmit buffer. */
            ret = R_ETHER_Write_ZC2_SetBuf(channel, len);
            
            /* 
             * Confirm that the transmission is completed.
             * Data written in the transmit buffer is transmitted by the EDMAC. Make sure that the
             * transmission is completed after writing data to the transmit buffer.
             * If the R_ETHER_Close_ZC2 function is called to stop the Ethernet communication before
             * verifying that the transmission is completed, the written data written may not be transmitted.
             */
            ret = R_ETHER_CheckWrite(channel);
        }
    }
    return ret;
} /* End of function R_ETHER_Write() */

/***********************************************************************************************************************
* Function Name: R_ETHER_CheckWrite
* Description  : Checking that the data has been sent. 
* Arguments    : channel -
*                    ETHERC channel number
* Return Value : ETHER_SUCCESS -
*                    Processing completed successfully
*                ETHER_ERR_INVALID_CHAN - 
*                    Nonexistent channel number
***********************************************************************************************************************/
ether_return_t R_ETHER_CheckWrite(uint32_t channel)
{
    ether_return_t                          ret;
    volatile struct st_edmac __evenaccess * edmac_adr;
    const ether_control_t                 * pether_ch;
    uint32_t                                phy_access;

    /* Check argument */
    if (ETHER_CHANNEL_MAX <= channel)
    {
        ret = ETHER_ERR_INVALID_CHAN;
    }
    else
    {
        pether_ch  = g_eth_control_ch[channel].pether_control;
        phy_access = g_eth_control_ch[channel].phy_access;
        edmac_adr  = pether_ch[phy_access].pedmac;
        while (0 != edmac_adr->EDTRR.BIT.TR)
        {
            /* Do Nothing */
        }
        ret = ETHER_SUCCESS;
    }
    return ret;
} /* End of function R_ETHER_CheckWrite() */

/***********************************************************************************************************************
* Function Name: R_ETHER_Control
* Description  : By command argument is a function to change the settings of Ether driver.
* Arguments    : cmd - 
*                    Control code
*                control -
*                    Parameters according to the control code
* Return Value : ETHER_SUCCESS -
*                    Processing completed successfully
*                ETHER_ERR_INVALID_CHAN - 
*                    Nonexistent channel number
*                ETHER_ERR_CHAN_OPEN -
*                    Indicates the Ethernet cannot be opened because it is being used by another application
*                ETHER_ERR_INVALID_ARG -
*                    Invalid argument
***********************************************************************************************************************/
ether_return_t R_ETHER_Control(ether_cmd_t const cmd, ether_param_t const control)
{
    ether_return_t      ret;

    switch (cmd)
    {
        /* Set the callback function pointer */
        case CONTROL_SET_CALLBACK:
            ret = ether_set_callback(control);
        break;
        
        case CONTROL_SET_PROMISCUOUS_MODE:
            ret = ether_set_promiscuous_mode(control);
        break;
        
        case CONTROL_SET_INT_HANDLER:
            ret = ether_set_int_handler(control);
        break;
        
        case CONTROL_POWER_ON:
            ret = ether_power_on(control);
        break;
        
        case CONTROL_POWER_OFF:
            ret = ether_power_off(control);
        break;
        
        /* Commands not supported */
        default:
            ret = ETHER_ERR_INVALID_ARG;
        break;
    }
    return ret;
} /* End of function R_ETHER_Control() */

/***********************************************************************************************************************
* Function Name: R_ETHER_GetVersion
* Description  : Returns this module's version information.
*                The version number is encoded where the top 2 bytes are the major version number and the bottom 2 bytes
*                are the minor version number. For example, Version 4.25 would be returned as 0x00040019.
* Arguments    : none
* Return Value : Version number
***********************************************************************************************************************/
#pragma inline(R_ETHER_GetVersion)
uint32_t R_ETHER_GetVersion(void)
{
    return ((((uint32_t)ETHER_RX_VERSION_MAJOR) << 16) | ((uint32_t)ETHER_RX_VERSION_MINOR));
} /* End of function R_ETHER_GetVersion() */

/*
 * Private functions
 */

/***********************************************************************************************************************
* Function Name: ether_reset_mac
* Description  : The EDMAC and EtherC are reset through the software reset.
* Arguments    : channel -
*                    ETHERC channel number
* Return Value : none
***********************************************************************************************************************/
static void ether_reset_mac(uint32_t channel)
{
    volatile uint32_t   i;

    /* Software reset */
    if (ETHER_CHANNEL_0 == channel)
    {
        EDMAC0.EDMR.BIT.SWR    = 1;
    }
    else
    {
        EDMAC1.EDMR.BIT.SWR    = 1;
    }
    

    /*
     * Waiting time until the initialization of ETHERC and EDMAC is completed is 64 cycles
     * in the clock conversion of an internal bus of EDMAC. 
     */
    for (i = 0; i < 0x00000100; i++)
    {
        ;
    }

} /* End of function ether_reset_mac() */

/***********************************************************************************************************************
* Function Name: ether_init_descriptors
* Description  : The EDMAC descriptors and the driver buffers are initialized.
* Arguments    : channel -
*                    ETHERC channel number
* Return Value : none
***********************************************************************************************************************/
static void ether_init_descriptors(uint32_t channel)
{
    descriptor_t  * pdescriptor;
    uint32_t        i;

    /* Initialize the receive descriptors */
    for (i = 0; i < ETHER_CFG_EMAC_RX_DESCRIPTORS; i++)
    {
        pdescriptor          = (descriptor_t *)&rx_descriptors[channel][i];
        pdescriptor->buf_p   = (uint8_t *)&(ether_buffers[channel].buffer[i][0]);
        pdescriptor->bufsize = ETHER_CFG_BUFSIZE;
        pdescriptor->size    = 0;
        pdescriptor->status  = RACT;
        pdescriptor->next    = (descriptor_t *)&rx_descriptors[channel][(i + 1)];
    }

    /* The last descriptor points back to the start */
    pdescriptor->status |= RDLE;
    pdescriptor->next    = (descriptor_t *)&rx_descriptors[channel][0];

    /* Initialize application receive descriptor pointer */
    papp_rx_desc[channel] = (descriptor_t *)&rx_descriptors[channel][0];


    /* Initialize the transmit descriptors */
    for (i = 0; i < ETHER_CFG_EMAC_TX_DESCRIPTORS; i++)
    {
        pdescriptor          = (descriptor_t *)&tx_descriptors[channel][i];
        pdescriptor->buf_p   = (uint8_t *)&(ether_buffers[channel].buffer[(ETHER_CFG_EMAC_RX_DESCRIPTORS + i)][0]);
        pdescriptor->bufsize = 1;       /* Set a value equal to or greater than 1. (reference to UMH)
                                           When transmitting data, the value of size is set to the function argument
                                           R_ETHER_Write_ZC2_SetBuf. */
        pdescriptor->size    = 0;       /* Reserved : The write value should be 0. (reference to UMH) */
        pdescriptor->status  = 0;
        pdescriptor->next    = (descriptor_t *)&(tx_descriptors[channel][(i + 1)]);
    }

    /* The last descriptor points back to the start */
    pdescriptor->status |= TDLE;
    pdescriptor->next    = (descriptor_t *)&tx_descriptors[channel][0];

    /* Initialize application transmit descriptor pointer */
    papp_tx_desc[channel] = (descriptor_t *)&tx_descriptors[channel][0];
} /* End of function ether_init_descriptors() */

/***********************************************************************************************************************
* Function Name: ether_config_ethernet
* Description  : Configure the Ethernet Controller (EtherC) and the Ethernet
*                Direct Memory Access controller (EDMAC).
* Arguments    : channel -
*                    ETHERC channel number
*                mode - 
*                   The operational mode is specified. 
*                   NO_USE_MAGIC_PACKET_DETECT (0) - Communicate mode usually
*                   USE_MAGIC_PACKET_DETECT    (1) - Magic packet detection mode
* Return Value : none
***********************************************************************************************************************/
static void ether_config_ethernet(uint32_t channel, const uint8_t mode)
{
    volatile struct st_etherc __evenaccess    * etherc_adr;
    volatile struct st_edmac __evenaccess     * edmac_adr;
    const ether_control_t                     * pether_ch;
    uint32_t                                    phy_access;

    if (ETHER_CHANNEL_MAX <= channel)
    {
        return;
    }

    pether_ch  = g_eth_control_ch[channel].pether_control;
    phy_access = g_eth_control_ch[channel].phy_access;
    etherc_adr = pether_ch[phy_access].petherc;
    edmac_adr  = pether_ch[phy_access].pedmac;

    /* Magic packet detection mode */
    if (USE_MAGIC_PACKET_DETECT == mode)
    {
        etherc_adr->ECSIPR.LONG = 0x00000006;
        edmac_adr->EESIPR.LONG  = 0x00400000;
    }

    /* Normal mode */
    else
    {
        /* LINK Signal Change Interrupt Enable */
        etherc_adr->ECSIPR.BIT.LCHNGIP = 1;
        edmac_adr->EESIPR.BIT.ECIIP    = 1;

        /*
         * This is sample code.
         * Sets up interrupt when you use interrupts.
         * Enable interrupts of interest only.
         */
        edmac_adr->EESIPR.BIT.FRIP = 1;     /* Enable the frame receive interrupt. */
        edmac_adr->EESIPR.BIT.TCIP = 1;     /* Enable the frame transmit end interrupt. */
    }
    
    /* Ethernet length 1514bytes + CRC and intergap is 96-bit time */
    etherc_adr->RFLR.LONG = 1518;
    etherc_adr->IPGR.LONG = 0x00000014;



#ifdef __LIT
    /* Set little endian mode */
    /* Ethernet length 1514bytes + CRC and intergap is 96-bit time */
    edmac_adr->EDMR.BIT.DE = 1;
#endif /* __LIT */

    /* Initialize Rx descriptor list address */
    /* Casting the pointer to a uint32_t type is valid because the Renesas Compiler uses 4 bytes per pointer. */
    edmac_adr->RDLAR = (void*) papp_rx_desc[channel];
    
    /* Initialize Tx descriptor list address */
    /* Casting the pointer to a uint32_t type is valid because the Renesas Compiler uses 4 bytes per pointer. */
    edmac_adr->TDLAR = (void*) papp_tx_desc[channel];
    
    /* Don't reflect the EESR.RMAF bit status in the RD0.RFS bit in the receive descriptor */
    edmac_adr->TRSCER.LONG = 0x00000080;
    
    /* Threshold of Tx_FIFO */
    /* To prevent a transmit underflow, setting the initial value (store and forward modes) is recommended. */
    edmac_adr->TFTR.LONG = 0x00000000;
    
    /* transmit fifo & receive fifo is 2048 bytes */
    edmac_adr->FDR.LONG  = 0x0000070F;
    
    /*  Configure receiving method
    b0      RNR - Receive Request Bit Reset - Continuous reception of multiple frames is possible.
    b1      RNC - Receive Request Bit Non-Reset Mode - The RR bit is automatically reset.
    b31:b2  Reserved set to 0
    */
    edmac_adr->RMCR.LONG  = 0x00000001;

} /* End of function ether_config_ethernet() */

/***********************************************************************************************************************
* Function Name: ether_pause_resolution
* Description  : Determines PAUSE frame generation and handling. Uses
*                the resolution Table 28B-3 of IEEE 802.3-2008.
* Arguments    : local_ability -
*                    local PAUSE capability (2 least significant bits)
*                partner_ability -
*                    link partner PAUSE capability (2 least significant bits)
*                *ptx_pause -
*                    pointer to location to store the result of the table lookup for transmit
*                    PAUSE. 1 is enable, 0 is disable.
*                *prx_pause -
*                    pointer to location to store the result of the table lookup for receive
*                    PAUSE. 1 is enable, 0 is disable.
* Return Value : none
***********************************************************************************************************************/
static void ether_pause_resolution(uint16_t local_ability, uint16_t partner_ability,
                              uint16_t *ptx_pause, uint16_t *prx_pause)
{
    uint32_t    i;
    uint32_t    ability_compare;

    /*
     * Arrange the bits so that they correspond to the Table 28B-3
     * of the IEEE 802.3 values.
     */
    ability_compare = (uint32_t)(((local_ability & LINK_RES_ABILITY_MASK) << LINK_RES_LOCAL_ABILITY_BITSHIFT) |
                       (partner_ability & LINK_RES_ABILITY_MASK));

    /* Walk through the look up table */
    for (i = 0; i < PAUSE_TABLE_ENTRIES; i++)
    {
        if ((ability_compare & pause_resolution[i].mask) == pause_resolution[i].value)
        {
            (*ptx_pause) = pause_resolution[i].transmit;
            (*prx_pause) = pause_resolution[i].receive;
            return;
        }
    }
} /* End of function ether_pause_resolution() */

/***********************************************************************************************************************
* Function Name: ether_configure_mac
* Description  : Software reset is executed, and ETHERC and EDMAC are configured. 
* Arguments    : channel -
*                    ETHERC channel number
*                mac_addr -
*                    The MAC address of ETHERC
*                mode -
*                    The operational mode is specified. 
*                    NO_USE_MAGIC_PACKET_DETECT (0) - Communicate mode usually
*                    USE_MAGIC_PACKET_DETECT    (1) - Magic packet detection mode
* Return Value : none
***********************************************************************************************************************/
static void ether_configure_mac(uint32_t channel, const uint8_t mac_addr[], const uint8_t mode)
{
    uint32_t    mac_h;
    uint32_t    mac_l;
    volatile struct st_etherc __evenaccess    * etherc_adr;
    const ether_control_t                     * pether_ch;
    uint32_t                                    phy_access;

    if (ETHER_CHANNEL_MAX <= channel)
    {
        return;
    }
    
    pether_ch  = g_eth_control_ch[channel].pether_control;
    phy_access = g_eth_control_ch[channel].phy_access;
    etherc_adr = pether_ch[phy_access].petherc;

    /* Software reset */
    ether_reset_mac(channel);

    /* Set MAC address */ 
    mac_h = (((((uint32_t)mac_addr[0] << 24)  | \
               ((uint32_t)mac_addr[1] << 16)) | \
               ((uint32_t)mac_addr[2] << 8 )) | \
                (uint32_t)mac_addr[3]);

    mac_l = (((uint32_t)mac_addr[4] << 8 ) | \
              (uint32_t)mac_addr[5]);
             
    etherc_adr->MAHR      = mac_h;
    etherc_adr->MALR.LONG = mac_l;

    /* Initialize receive and transmit descriptors */
    ether_init_descriptors(channel);

    /* Perform rest of hardware interface configuration */
    ether_config_ethernet(channel, mode);

} /* End of function ether_configure_mac() */

/***********************************************************************************************************************
* Function Name: ether_do_link
* Description  : Determines the partner PHY capability through
*                auto-negotiation process. The link abilities
*                are handled to determine duplex, speed and flow
*                control (PAUSE frames).
* Arguments    : channel -
*                    ETHERC channel number
*                mode - 
*                    The operational mode is specified. 
*                    NO_USE_MAGIC_PACKET_DETECT (0) - Communicate mode usually
*                    USE_MAGIC_PACKET_DETECT    (1) - Magic packet detection mode
* Return Value : ETHER_SUCCESS -
*                    Processing completed successfully
*                ETHER_ERR_INVALID_CHAN -
*                    Nonexistent channel number
*                ETHER_ERR_OTHER -
*                    
***********************************************************************************************************************/
static ether_return_t ether_do_link(uint32_t channel, const uint8_t mode)               
{
    ether_return_t  ret;
    uint16_t        link_speed_duplex  = 0;
    uint16_t        local_pause_bits   = 0;
    uint16_t        partner_pause_bits = 0;
    uint16_t        transmit_pause_set = 0;
    uint16_t        receive_pause_set  = 0;
    uint16_t        full_duplex        = 0;
    uint16_t        link_result        = 0;
    volatile struct st_etherc __evenaccess    * etherc_adr;
    volatile struct st_edmac __evenaccess     * edmac_adr;
    const ether_control_t                     * pether_ch;
    uint32_t                                    phy_access;

    if (ETHER_CHANNEL_MAX <= channel)
    {
        return ETHER_ERR_INVALID_CHAN;
    }

    pether_ch  = g_eth_control_ch[channel].pether_control;
    phy_access = g_eth_control_ch[channel].phy_access;
    etherc_adr = pether_ch[phy_access].petherc;
    edmac_adr  = pether_ch[phy_access].pedmac;

    /* Set the link status */
    link_result = phy_set_autonegotiate(channel, &link_speed_duplex, &local_pause_bits, &partner_pause_bits);

    if (R_PHY_OK == link_result)
    {
        switch (link_speed_duplex)
        {
            /* Half duplex link */
            case PHY_LINK_100H:
                etherc_adr->ECMR.BIT.DM  = 0;
                etherc_adr->ECMR.BIT.RTM = 1;
                ret                      = ETHER_SUCCESS;
            break;

            case PHY_LINK_10H:
                etherc_adr->ECMR.BIT.DM  = 0;
                etherc_adr->ECMR.BIT.RTM = 0;
                ret                      = ETHER_SUCCESS;
            break;

            /* Full duplex link */
            case PHY_LINK_100F:
                etherc_adr->ECMR.BIT.DM  = 1;
                etherc_adr->ECMR.BIT.RTM = 1;
                full_duplex              = 1;
                ret                      = ETHER_SUCCESS;
            break;

            case PHY_LINK_10F:
                etherc_adr->ECMR.BIT.DM  = 1;
                etherc_adr->ECMR.BIT.RTM = 0;
                full_duplex              = 1;
                ret                      = ETHER_SUCCESS;
            break;

            default:
                ret = ETHER_ERR_OTHER;
            break;
        }

        /* At the communicate mode usually */
        if (ETHER_SUCCESS == ret)
        {
            if (NO_USE_MAGIC_PACKET_DETECT == mode)
            {
                
                /* When pause frame is used */
                if ((full_duplex) && (ETHER_FLAG_ON == pause_frame_enable[channel]))
                {
                    /* Set automatic PAUSE for 512 bit-time */
                    etherc_adr->APR.LONG = 0x0000FFFF;

                    /* Set unlimited retransmit of PAUSE frames */
                    etherc_adr->TPAUSER.LONG = 0;

                    /* PAUSE flow control FIFO settings. */
                    edmac_adr->FCFTR.LONG = 0x00000000;

                    /* Control of a PAUSE frame whose TIME parameter value is 0 is enabled. */
                    etherc_adr->ECMR.BIT.ZPF = 1;

                    /**
                     * Enable PAUSE for full duplex link depending on
                     * the pause resolution results
                     */
                    ether_pause_resolution(local_pause_bits, partner_pause_bits,
                                             &transmit_pause_set, &receive_pause_set);

                    if (XMIT_PAUSE_ON == transmit_pause_set)
                    {
                        /* Enable automatic PAUSE frame transmission */
                        etherc_adr->ECMR.BIT.TXF = 1;
                    }
                    else
                    {
                        /* Disable automatic PAUSE frame transmission */
                        etherc_adr->ECMR.BIT.TXF = 0;
                    }

                    if (RECV_PAUSE_ON == receive_pause_set)
                    {
                        /* Enable reception of PAUSE frames */
                        etherc_adr->ECMR.BIT.RXF = 1;
                    }
                    else
                    {
                        /* Disable reception of PAUSE frames */
                        etherc_adr->ECMR.BIT.RXF = 0;
                    }
                }

                /* When pause frame is not used */
                else
                {
                    /* Disable PAUSE for half duplex link */
                    etherc_adr->ECMR.BIT.TXF = 0;
                    etherc_adr->ECMR.BIT.RXF = 0;
                }

                /* Set the promiscuous mode bit */
                etherc_adr->ECMR.BIT.PRM = promiscuous_mode[channel];

                /* Enable receive and transmit. */
                etherc_adr->ECMR.BIT.RE = 1;
                etherc_adr->ECMR.BIT.TE = 1;

                /* Enable EDMAC receive */
                edmac_adr->EDRRR.LONG = 0x1;
            }

            /* At the magic packet detection mode */
            else
            {
                /* The magic packet detection is permitted. */
                etherc_adr->ECMR.BIT.MPDE = 1;

                /* Because data is not transmitted for the magic packet detection waiting, 
                   only the reception is permitted. */
                etherc_adr->ECMR.BIT.RE = 1;

                /*
                 * The reception function of EDMAC keep invalidity 
                 * because the receive data don't need to be read when the magic packet detection mode.
                 */
            }
        }
    }
    else
    {
        ret = ETHER_ERR_OTHER;
    }

    return ret;
} /* End of function ether_do_link() */

/***********************************************************************************************************************
* Function Name: ether_set_callback
* Description  : Set the callback function
* Arguments    : control -
*                    Callback function pointer. 
*                    If you would rather poll for finish then please input FIT_NO_FUNC for this argument.
* Return Value : ETHER_SUCCESS
*                    Processing completed successfully
***********************************************************************************************************************/
static ether_return_t ether_set_callback(ether_param_t const control)
{
    void                (*pcb_func)(void *);

    pcb_func = control.ether_callback.pcb_func;

    /* Check callback function pointer, of whether or NULL FIT_NO_FUNC */
    if ((NULL != pcb_func) && (FIT_NO_FUNC != pcb_func))
    {
        cb_func.pcb_func = pcb_func;                        /* Set the callback function */
    }

    return ETHER_SUCCESS;
} /* End of function ether_set_callback() */

/***********************************************************************************************************************
* Function Name: ether_set_promiscuous_mode
* Description  : Set promiscuous mode bit
* Arguments    : control -
*                    Promiscuous mode bit
* Return Value : ETHER_SUCCESS -
*                    Processing completed successfully
*                ETHER_ERR_INVALID_CHAN - 
*                    Nonexistent channel number
***********************************************************************************************************************/
static ether_return_t ether_set_promiscuous_mode(ether_param_t const control)
{
    ether_promiscuous_t   * p_arg;
    uint32_t                channel;

    p_arg   = control.p_ether_promiscuous;
    channel = p_arg->channel;

    if (ETHER_CHANNEL_MAX <= channel)
    {
        return ETHER_ERR_INVALID_CHAN;
    }
    else
    {
        promiscuous_mode[channel] = p_arg->bit;
    }
    return ETHER_SUCCESS;
} /* End of function ether_set_promiscuous_mode() */

/***********************************************************************************************************************
* Function Name: ether_set_int_handler
* Description  : Set the interrupt handler
* Arguments    : control -
*                    Interrupt handler pointer.
*                    If you would rather poll for finish then please input FIT_NO_FUNC for this argument.
* Return Value : ETHER_SUCCESS -
*                    Processing completed successfully
*                ETHER_ERR_INVALID_ARG -
*                    Invalid argument
***********************************************************************************************************************/
static ether_return_t ether_set_int_handler(ether_param_t const control)
{
    void                (*pcb_int_hnd)(void *);
    ether_return_t      ret = ETHER_ERR_INVALID_ARG;

    pcb_int_hnd = control.ether_int_hnd.pcb_int_hnd;
    if (NULL != pcb_int_hnd)
    {
        cb_func.pcb_int_hnd = pcb_int_hnd;
        ret                 = ETHER_SUCCESS;
    }
    return ret;
}  /* End of function ether_set_int_handler() */

/***********************************************************************************************************************
* Function Name: ether_power_on
* Description  : Turns on power to a ETHER channel.
* Arguments    : control -
*                    Ethernet channel number
* Return Value : ETHER_SUCCESS -
*                    Processing completed successfully
*                ETHER_ERR_INVALID_CHAN -
*                    Nonexistent channel number
*                ETHER_ERR_CHAN_OPEN -
*                    Indicates the Ethernet cannot be opened because it is being used by another application
*                ETHER_ERR_INVALID_ARG -
*                    Invalid argument
***********************************************************************************************************************/
static ether_return_t ether_power_on(ether_param_t const control)
{
    uint32_t                                    channel;
    const ether_control_t                     * pether_ch;
    uint32_t                                    phy_access;
    ether_return_t                              ret;
    
    ret     = ETHER_ERR_INVALID_ARG;
    channel = (uint32_t)control.channel;
    
    if (ETHER_CHANNEL_MAX > channel)
    {
        pether_ch  = g_eth_control_ch[channel].pether_control;
        phy_access = g_eth_control_ch[channel].phy_access;

        /* Set port connect */
        ether_port_enable(pether_ch[phy_access].port_connect);

        /* ETHERC/EDMAC Power on */
        ret = power_on_control(channel);
    }
    return ret;
}  /* End of function ether_power_on() */

/***********************************************************************************************************************
* Function Name: ether_power_off
* Description  : Turns on power to a ETHER channel.
* Arguments    : control -
*                    Ethernet channel number
* Return Value : ETHER_SUCCESS -
*                    Processing completed successfully
*                ETHER_ERR_INVALID_ARG -
*                    Invalid argument
***********************************************************************************************************************/
static ether_return_t ether_power_off(ether_param_t const control)
{
    uint32_t            channel;
    ether_return_t      ret;
    
    ret     = ETHER_ERR_INVALID_ARG;
    channel = (uint32_t)control.channel;
    
    if (ETHER_CHANNEL_MAX > channel)
    {
        /* ETHERC/EDMAC Power off */
        power_off_control(channel);
        ret = ETHER_SUCCESS;
    }
    return ret;
}  /* End of function ether_power_off() */

/***********************************************************************************************************************
* Function Name: power_on
* Description  : Turns on power to a ETHER channel.
* Arguments    : channel -
*                    ETHERC channel number
* Return Value : ETHER_SUCCESS -
*                    Processing completed successfully
*                ETHER_ERR_INVALID_CHAN -
*                    Nonexistent channel number
*                ETHER_ERR_CHAN_OPEN -
*                    Indicates the Ethernet cannot be opened because it is being used by another application
***********************************************************************************************************************/
static ether_return_t power_on(uint32_t channel)
{
    /* Check argument */
    if (ETHER_CHANNEL_MAX <= channel)
    {
        /* Should never get here. Valid channel number is checked above. */
        return ETHER_ERR_INVALID_CHAN;
    }

    /* Attempt to obtain lock so we know we have exclusive access to ETHER channel. */
    if (false == R_BSP_HardwareLock((mcu_lock_t)(BSP_LOCK_ETHERC0 + channel)))
    {
        return ETHER_ERR_CHAN_OPEN;
    }
    
    /* Attempt to obtain lock so we know we have exclusive access to EDMAC channel. */
    if (false == R_BSP_HardwareLock((mcu_lock_t)(BSP_LOCK_EDMAC0 + channel)))
    {
        /* Release reservation on peripheral. */
        R_BSP_HardwareUnlock((mcu_lock_t)(BSP_LOCK_ETHERC0 + channel));
        return ETHER_ERR_CHAN_OPEN;
    }

    /* Enable writing to MSTP registers. */
    R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_LPC_CGC_SWR);

    /* Enable selected ETHERC/EDMAC Channel. */
    if (ETHER_CHANNEL_0 == channel)
    {
        MSTP(EDMAC0) = 0;
    }
    else
    {
        MSTP(EDMAC1) = 0;
    }
    
    /* Disable writing to MSTP registers. */
    R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_LPC_CGC_SWR);
    
    return ETHER_SUCCESS;
} /* End of function power_on() */

/***********************************************************************************************************************
* Function Name: power_off
* Description : Turns off power to a ETHER channel.
* Arguments    : channel -
*                    ETHERC channel number
* Return Value : none
***********************************************************************************************************************/
static void power_off(uint32_t channel)
{

    /* Check argument */
    if (ETHER_CHANNEL_MAX <= channel)
    {
        /* Should never get here. Valid channel number is checked above. */
        return;
    }

    /* Enable writing to MSTP registers. */
    R_BSP_RegisterProtectDisable(BSP_REG_PROTECT_LPC_CGC_SWR);
    
    /* Enable selected ETHERC/EDMAC Channel. */
    if (ETHER_CHANNEL_0 == channel)
    {
        MSTP(EDMAC0) = 1;
    }
    else
    {
        MSTP(EDMAC1) = 1;
    }
    
    /* Disable writing to MSTP registers. */
    R_BSP_RegisterProtectEnable(BSP_REG_PROTECT_LPC_CGC_SWR);
    
    /* Release reservation on peripheral. */
    R_BSP_HardwareUnlock((mcu_lock_t)(BSP_LOCK_ETHERC0 + channel));
    R_BSP_HardwareUnlock((mcu_lock_t)(BSP_LOCK_EDMAC0 + channel));

} /* End of function power_off() */

/***********************************************************************************************************************
* Function Name: power_on_control
* Description  : Powers on the channel if the ETHEC channel used and the PHY access channel are different, or if the 
*                PHY access channel is powered off.
* Arguments    : channel -
*                    ETHERC channel number
* Return Value : ETHER_SUCCESS -
*                    Processing completed successfully
*                ETHER_ERR_INVALID_CHAN -
*                    Nonexistent channel number
*                ETHER_ERR_CHAN_OPEN -
*                    Indicates the Ethernet cannot be opened because it is being used by another application
*              : ETHER_ERR_OTHER -
*                    
***********************************************************************************************************************/
static ether_return_t power_on_control(uint32_t channel)
{
    ether_return_t  ret = ETHER_ERR_OTHER;

    etherc_edmac_power_cont[ether_phy_access[channel]] [channel] = 1;

    if(channel != ether_phy_access[channel])
    {
        ret = power_on(channel);
        if (ETHER_SUCCESS != ret)
        {
            return ret;
        }
    }
    

    if (!(( 1 == etherc_edmac_power_cont[ether_phy_access[channel]] [0] ) &&
         ( 1 == etherc_edmac_power_cont[ether_phy_access[channel]] [1] )))
    {

        ret = power_on(ether_phy_access[channel]);
    }
    
    return ret;
    
} /* End of function power_on_control() */

/***********************************************************************************************************************
* Function Name: power_off_control
* Description  : Powers off the channel if the ETHEC channel used and the PHY access channel are different, or if the 
*                PHY access channel is powered off.
* Arguments    : channel -
*                    ETHERC channel number
* Return Value : none
***********************************************************************************************************************/
static void power_off_control(uint32_t channel)
{
    etherc_edmac_power_cont[ether_phy_access[channel]] [channel] = 0;

    if(channel != ether_phy_access[channel])
    {
        power_off(channel);
    }
    
    if(( 0 == etherc_edmac_power_cont[ether_phy_access[channel]] [0] ) &&
       ( 0 == etherc_edmac_power_cont[ether_phy_access[channel]] [1] ))
    {

        power_off(ether_phy_access[channel]);
    }
} /* End of function power_off_control() */


/***********************************************************************************************************************
* Function Name: check_mpde_bit
* Description  : 
* Arguments    : none
* Return Value : 1: Magic Packet detection is enabled.
*                0: Magic Packet detection is disabled.
***********************************************************************************************************************/
static uint8_t check_mpde_bit(void)
{
          /* The MPDE bit can be referred to only when ETHERC0 operates. */
    if (  ( ((1 == etherc_edmac_power_cont[0][0]) || (1 == etherc_edmac_power_cont[1][0]))
               && (1 == ETHERC0.ECMR.BIT.MPDE)
               )

          /* The MPDE bit can be referred to only when ETHERC1 operates. */
       || ( ((1 == etherc_edmac_power_cont[0][1]) || (1 == etherc_edmac_power_cont[1][1]))
               && (1 == ETHERC1.ECMR.BIT.MPDE)
               )
        )
    {
        return 1;
    }
    else
    {
        return 0;
    }
} /* End of function check_mpde_bit() */

/***********************************************************************************************************************
* Function Name: ether_eint0
* Description  : EINT0 interrupt processing (A callback function to be called from r_bsp.)
* Arguments    : pparam -
*                    unused
* Return Value : none
***********************************************************************************************************************/
static void ether_eint0(void * pparam)
{
    volatile uint32_t   dummy;

    ether_int_common(ETHER_CHANNEL_0);

    dummy = (uint32_t)pparam;
} /* End of function ether_eint0() */

/***********************************************************************************************************************
* Function Name: ether_eint1
* Description  : EINT1 interrupt processing (A callback function to be called from r_bsp.)
* Arguments    : pparam -
*                    unused
* Return Value : none
***********************************************************************************************************************/
static void ether_eint1(void * pparam)
{
    volatile uint32_t   dummy;

    ether_int_common(ETHER_CHANNEL_1);

    dummy = (uint32_t)pparam;
} /* End of function ether_eint1() */

/***********************************************************************************************************************
* Function Name: ether_int_common
* Description  : Interrupt handler for Ethernet receive and transmit interrupts.
* Arguments    : channel -
*                    ETHERC channel number
* Return Value : none
***********************************************************************************************************************/
static void ether_int_common(uint32_t channel)
{
    uint32_t    status_ecsr;
    uint32_t    status_eesr;
    volatile struct st_etherc __evenaccess    * etherc_adr;
    volatile struct st_edmac __evenaccess     * edmac_adr;
    const ether_control_t                     * pether_ch;
    uint32_t                                    phy_access;
    ether_cb_arg_t                              cb_arg;

    pether_ch  = g_eth_control_ch[channel].pether_control;
    phy_access = g_eth_control_ch[channel].phy_access;
    etherc_adr = pether_ch[phy_access].petherc;
    edmac_adr  = pether_ch[phy_access].pedmac;

    status_ecsr = etherc_adr->ECSR.LONG;
    status_eesr = edmac_adr->EESR.LONG;

    /* Callback : Interrupt handler */
    if ((NULL != cb_func.pcb_int_hnd) && (FIT_NO_FUNC != cb_func.pcb_int_hnd))
    {
        cb_arg.channel     = channel;
        cb_arg.status_ecsr = status_ecsr;
        cb_arg.status_eesr = status_eesr;
        (*cb_func.pcb_int_hnd)((void *)&cb_arg);
    }

    /* When the ETHERC status interrupt is generated */
    if (status_eesr & EMAC_ECI_INT)
    {
        /* When the link signal change interrupt is generated */
        if (status_ecsr & EMAC_LCHNG_INT)
        {
            /* The state of the link signal is confirmed and Link Up/Down is judged. */
            /* When becoming Link up */
            if (ETHER_CFG_LINK_PRESENT == etherc_adr->PSR.BIT.LMON)
            {
                lchng_flag[channel] = ETHER_FLAG_ON_LINK_ON;
            }

            /* When Link becomes down */
            else
            {
                lchng_flag[channel] = ETHER_FLAG_ON_LINK_OFF;
            }
        }

        /* When the Magic Packet detection interrupt is generated */
        if (status_ecsr & EMAC_MPD_INT)
        {
            mpd_flag[channel] = ETHER_FLAG_ON;
        }

        /*
         * Because each bit of the ECSR register is cleared when one is written, 
         * the value read from the register is written and the bit is cleared. 
         */
        /* Clear all ETHERC status BFR, PSRTO, LCHNG, MPD, ICD */
        etherc_adr->ECSR.LONG = status_ecsr;
    }
    
    /* This is sample code */
    /* Whether it is a necessary code is confirmed. */
    if (status_eesr & EMAC_RFCOF_INT)
    {
        /* do nothing */
    }
    if (status_eesr & EMAC_TC_INT)
    {
        /* do nothing */
    }
    if (status_eesr & EMAC_FR_INT)
    {
        /* do nothing */
    }
    
    if (status_eesr & EMAC_RDE_INT)
    {
        /* do nothing */
    }
    
    if (status_eesr & EMAC_RFOF_INT)
    {
        /* do nothing */
    }

    /*
     * Because each bit of the EESR register is cleared when one is written, 
     * the value read from the register is written and the bit is cleared. 
     */
    edmac_adr->EESR.LONG  = status_eesr; /* Clear EDMAC status bits */

     /* Whether it is a necessary code is confirmed. */

} /* End of function ether_int_common() */

