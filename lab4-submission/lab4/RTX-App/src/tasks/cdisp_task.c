/**
 * @brief The Console Display Task Template File 
 * @note  The file name and the function name can be changed
 * @see   k_tasks.h
 */
#include "../kernel/k_inc.h"
#include "../kernel/k_msg.h"
#include "../../../include/rtx.h"
#include "../../../include/common.h"
#include "../../../include/printf.h"
#include "../../../include/common_ext.h"

void task_cdisp(void)
{
    if(k_mbx_create(CON_MBX_SIZE) == RTX_ERR){
        // return RTX_ERR;
    }
    U8 buf[CON_MBX_SIZE];
    LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef *)LPC_UART0;
    while (1)
    {
        while(!(pUart->LSR & LSR_TEMT));
        if(recv_msg(buf, CON_MBX_SIZE) != RTX_ERR){
            RTX_MSG_HDR *hdr = (RTX_MSG_HDR *) buf;
            if (hdr->type == DISPLAY)
            {
                send_msg(TID_UART, buf);
                //enable UART0 transmit interrupt
                pUart->IER ^= IER_THRE;
            }
            
        }
    }
    
}

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

