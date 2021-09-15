/**
 * @brief The Wall Clock Display Task Template File 
 * @note  The file name and the function name can be changed
 * @see   k_tasks.h
 */

#include "../../../include/rtx.h"
#include "../../../include/common.h"
#include "../../../include/printf.h"
#include "../../../include/bsp/LPC1768/timer.h"

#define WCLCK_MBX_SIZE 0x80

U8 *_copy(U8* dest, const U8* source, size_t len)
{
    if ( dest == NULL || source == NULL ) {
        return NULL;
    }
    
    U8 *ptr = dest;
	for (size_t i = 0; i < len; i++)
	{
		*ptr++ = *source++;
	}
	
    return dest;   
}

void __init_message(U8 *buf, int tid, const U8 *message, U8 type, int length)
{
	size_t msg_hdr_size = sizeof(RTX_MSG_HDR);
	RTX_MSG_HDR *ptr = (void *)buf;
	ptr->length = MSG_HDR_SIZE + length; // set the message length
	ptr->type = type;			// set message type
	ptr->sender_tid = tid;			// set sender id

	U8 *temp_buf = buf + msg_hdr_size;
	_copy(temp_buf, message, length);
	return;
}

void task_wall_clock(void)
{
    if (mbx_create(WCLCK_MBX_SIZE) == RTX_ERR){
        return;
    }
    U8 buf[MSG_HDR_SIZE + 20];
    __init_message(buf, TID_WCLCK, (U8 *)"W", KCD_REG, 3);
    send_msg(TID_KCD, (const U8 *)buf);
    U32 ref_tick = 0;
    U32 base_time = 0;
    U8 display = TRUE;
    TIMEVAL period;
    period.sec = 1;
    period.usec = 0;
    rt_tsk_set(&period);

    while (1)
    {
        recv_msg_nb(buf, MSG_HDR_SIZE + 20);
        if (buf[MSG_HDR_SIZE + 2] == 'R')
        {
            ref_tick = (g_timer_count / 2000) * 2000;
            base_time = 0;
            display = TRUE;
        } else if (buf[MSG_HDR_SIZE + 2] == 'S') {
					// Check if input is valid
            if (buf[MSG_HDR_SIZE + 6] == ':' && buf[MSG_HDR_SIZE + 9] == ':' && buf[MSG_HDR_SIZE + 12] == NULL)
            {
							if (buf[MSG_HDR_SIZE + 4] >= 48 && buf[MSG_HDR_SIZE + 4] <= 57 &&
									buf[MSG_HDR_SIZE + 5] >= 48 && buf[MSG_HDR_SIZE + 5] <= 57 && 
									buf[MSG_HDR_SIZE + 7] >= 48 && buf[MSG_HDR_SIZE + 7] <= 57 && 
									buf[MSG_HDR_SIZE + 8] >= 48 && buf[MSG_HDR_SIZE + 8] <= 57 &&
									buf[MSG_HDR_SIZE + 10] >= 48 && buf[MSG_HDR_SIZE + 10] <= 57 &&
									buf[MSG_HDR_SIZE + 11] >= 48 && buf[MSG_HDR_SIZE +11] <= 57)
							{
                U32 seconds = ((U8)buf[MSG_HDR_SIZE + 4] - 48) * 10 * 3600;
                seconds += ((U8)buf[MSG_HDR_SIZE + 5] - 48) * 3600;
                seconds += ((U8)buf[MSG_HDR_SIZE + 7] - 48) * 10 * 60;
                seconds += ((U8)buf[MSG_HDR_SIZE + 8] - 48) * 60;
                seconds += ((U8)buf[MSG_HDR_SIZE + 10] - 48) * 10;
                seconds += ((U8)buf[MSG_HDR_SIZE + 11] - 48);
                base_time = seconds;
                ref_tick = g_timer_count;
                display = TRUE;
							}
            }
        } else if (buf[MSG_HDR_SIZE + 2] == 'T') {
            display = FALSE;
        }

        if (display)
        {
            U32 seconds = base_time + (g_timer_count - ref_tick) * 5E-4;
            U8 str[15];
            U8 hours = (seconds/3600);
            U8 minutes = (seconds - hours * 3600)/60;
            seconds = seconds - hours * 3600 - minutes * 60;
            sprintf((char *)str, "\r\n%02d:%02d:%02d\r\n", hours % 24, minutes, seconds);
            __init_message(buf, TID_WCLCK, str, DISPLAY, 13);
            send_msg_nb(TID_CON, buf);
        }
        
        for (size_t i = 0; i < MSG_HDR_SIZE + 20; i++)
        {
            buf[i] = NULL;
        }
        rt_tsk_susp();
    }
    
}

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

