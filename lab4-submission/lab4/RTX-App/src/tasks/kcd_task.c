/** 
 * @brief The KCD Task Template File 
 * @note  The file name and the function name can be changed
 * @see   k_tasks.h
 */
#include "../../../include/rtx.h"
#include "../../../include/common.h"
#include "../../../include/printf.h"

#define cmd_not_found "\r\nCommand not found\r\n"
#define invalid_cmd "\r\nInvalid command\r\n"

U8 *copy(U8* dest, const U8* source, size_t len)
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

void _init_message(U8 *buf, int tid, const U8 *message, U8 type, int length)
{
	size_t msg_hdr_size = sizeof(RTX_MSG_HDR);
	RTX_MSG_HDR *ptr = (void *)buf;
	ptr->length = MSG_HDR_SIZE + length; // set the message length
	ptr->type = type;			// set message type
	ptr->sender_tid = tid;			// set sender id

	U8 *temp_buf = buf + msg_hdr_size;
	copy(temp_buf, message, length);
	return;
}

U8 string[KCD_CMD_BUF_SIZE];
int count = 0;

task_t cmds[62] = {0};

int strlen(const char *str) {
    unsigned int count = 0;
    while(*str !='\0')
    {
        count++;
        str++;
    }
    return count;
}

int get_index(U8 ascii_val){
    
    if (ascii_val >= 48 && ascii_val <= 57) // Numeric
    {
        return ascii_val - 48;
    }
    else if (ascii_val >= 65 && ascii_val <= 90) // Uppercase
    {
        return ascii_val - 55;
    }
    else if (ascii_val >= 97 && ascii_val <= 122) // Lowercase
    {
        return ascii_val - 61;
    }
    else
    {
        return RTX_ERR;
    }
}

void clear_string() {
    for (size_t i = 0; i < KCD_CMD_BUF_SIZE; i++)
    {
        string[i] = NULL;
    }
    count = 0;
}

void error_message(const char *err_msg){
    int length = strlen(err_msg) + 1; 
    U8 buf[MSG_HDR_SIZE + KCD_CMD_BUF_SIZE];
    _init_message(buf, TID_KCD, (const U8 *)err_msg, DISPLAY, length);
    send_msg(TID_CON, buf);
}

const U8 *get_state(U8 state){
    switch (state)
    {
    case READY:
        return "READY";
    case RUNNING:
        return "RUNNING";
    case BLK_SEND:
        return "BLK_SEND";
    case BLK_RECV:
        return "BLK_RECV";
    case SUSPENDED:
        return "SUSPENDED";

    default:
        return NULL;
    }
}


void KCD_handler(){
    if (string[2] == 'T' || string[2] == 'M')
    {
        RTX_TASK_INFO task;
        for (size_t i = 1; i < MAX_TASKS; i++)
        {
            tsk_get(i, &task);
            if (task.state != DORMANT && task.state != NULL)
            {
                const U8 *state = get_state(task.state);
                char str[50];
                int free = RTX_ERR;
                if (string[2] == 'M')
                {
                    free = mbx_get(task.tid);
                    if (free != RTX_ERR)
                    {
                        sprintf(str, "\r\nTID: %d State: %s Free: %d\r\n", task.tid, state, free);
                    }
                } else {
                    sprintf(str, "\r\nTID: %d State: %s\r\n", task.tid, state);
                }

                if(string[2] == 'T' || (string[2] == 'M' && free != RTX_ERR)){
                    U8 buf[MSG_HDR_SIZE + 50];
                    int length = strlen(str) + 1;
                    _init_message(buf, TID_KCD, (U8 *)str, DISPLAY, length);
                    send_msg(TID_CON, (const U8 *)buf);
                }
            }
        }
    } else {
        error_message(cmd_not_found);
    }
}

void task_kcd(void)
{
    if(mbx_create(KCD_MBX_SIZE) == RTX_ERR){
        return;
    }
    U8 str[KCD_CMD_BUF_SIZE];

    while (1)
    {
        if(recv_msg(&str, KCD_CMD_BUF_SIZE) != RTX_ERR){
            RTX_MSG_HDR *hdr = (RTX_MSG_HDR *) str;
            if (hdr->type == KCD_REG){
                int index = get_index(str[MSG_HDR_SIZE]);

                if (index != RTX_ERR){
                    cmds[index] = hdr->sender_tid;
                }
            } else if (hdr->type == KEY_IN){
                hdr->type = DISPLAY;
                hdr->sender_tid = TID_KCD;
                send_msg(TID_CON, str);

                //Received an enter
                if (str[MSG_HDR_SIZE] == '\r'){

                    //Is a command
                    if (string[0] == '%') {
                        int index = get_index(string[1]);

                        if (index == RTX_ERR) {
                            error_message(cmd_not_found);
                        } else {
                            task_t tid = cmds[index];
                            RTX_TASK_INFO info;

                            if (string[1] == 'L') {
                                KCD_handler();
                            } else {
                                int ret = tsk_get(tid, &info);
                                if (ret == RTX_OK && info.state != DORMANT && tid != TID_NULL) {
                                    U8 buf[MSG_HDR_SIZE + KCD_CMD_BUF_SIZE];
                                    _init_message(buf, TID_KCD, string, KCD_CMD, count);
                                    send_msg(tid, (const U8 *)buf);
                                } else {
                                    error_message(cmd_not_found);
                                }
                            }
                        }
                    } else {
                        error_message(invalid_cmd);
                    }
                    clear_string();
                } else {
                    if (count == KCD_CMD_BUF_SIZE){
                        clear_string();
                    } else {
                        string[count] = str[MSG_HDR_SIZE];
                        count++;
                    }
                }
            }
        }
    }  
}

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */
