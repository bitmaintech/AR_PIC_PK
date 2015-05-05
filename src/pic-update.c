/*********************************
pic-update.c
* The most simplistic C program ever written.
* An epileptic monkey on crack could write this code.
*****************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include<time.h>

#include "pic-update.h"

static T_FRAME RxBuff;
static BOOL RxFrameValid;

/**
 * Static table used for the table_driven implementation.
 *****************************************************************************/
static const UINT16 crc_table[16] =
{
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef
};

/********************************************************************
* Function: 	CalculateCrc()
*
* Precondition:
*
* Input: 		Data pointer and data length
*
* Output:		CRC.
*
* Side Effects:	None.
*
* Overview:     Calculates CRC for the given data and len
*
*
* Note:		 	None.
********************************************************************/
UINT16 CalculateCrc(UINT8 *data, UINT32 len)
{
    UINT i;
    UINT16 crc = 0;

    while(len--)
    {
        i = (crc >> 12) ^ (*data >> 4);
	    crc = crc_table[i & 0x0F] ^ (crc << 4);
	    i = (crc >> 12) ^ (*data >> 0);
	    crc = crc_table[i & 0x0F] ^ (crc << 4);
	    data++;
	}

    return (crc & 0xFFFF);
}

uint16_t structare_cmd(uint8_t cmd, uint8_t *cmd_buf, uint8_t *data, uint8_t data_len)
{
	uint16_t crc;
	uint16_t cmd_len;
	uint16_t i;
	uint8_t u8data;
	cmd_buf[0] = SOH;
	cmd_len = 1;
	cmd_buf[cmd_len++] = cmd;
	if((data_len !=0) && (data != NULL))
	{
		memcpy(&cmd_buf[2], data, data_len);
		cmd_len += data_len;
	}
	crc = CalculateCrc(&cmd_buf[1], cmd_len - 1);
	//process DLE
	cmd_len = 1;
	if((cmd == SOH) || (cmd == EOT)|| (cmd == DLE))
	{
		cmd_buf[cmd_len++] = DLE;
	}
	cmd_buf[cmd_len++] = cmd;
	if((data_len !=0) && (data != NULL))
	{
		for(i = 0; i < data_len; i++)
		{
			if((data[i] == SOH) || (data[i] == EOT)|| (data[i] == DLE))
			{
				cmd_buf[cmd_len++] = DLE;
			}
			cmd_buf[cmd_len++] = data[i];
		}
	}
	u8data = crc & 0xff;
	if((u8data == SOH) || (u8data == EOT)|| (u8data == DLE))
	{
		cmd_buf[cmd_len++] = DLE;
	}
	cmd_buf[cmd_len++] = u8data;
	u8data = crc >> 8;
	if((u8data == SOH) || (u8data == EOT)|| (u8data == DLE))
	{
		cmd_buf[cmd_len++] = DLE;
	}
	cmd_buf[cmd_len++] = u8data;
	cmd_buf[cmd_len] = EOT;
	cmd_len += 1;
	return cmd_len;
}

void BuildRxFrame(UINT8 *RxData, INT16 RxLen)
{
	static BOOL Escape = FALSE;
	WORD_VAL crc;


	while((RxLen > 0) && (!RxFrameValid)) // Loop till len = 0 or till frame is valid
	{
		RxLen--;

		if(RxBuff.Len >= sizeof(RxBuff.Data))
		{
			RxBuff.Len = 0;
		}

		switch(*RxData)
		{

			case SOH: //Start of header
				if(Escape)
				{
					// Received byte is not SOH, but data.
					RxBuff.Data[RxBuff.Len++] = *RxData;
					// Reset Escape Flag.
					Escape = FALSE;
				}
				else
				{
					// Received byte is indeed a SOH which indicates start of new frame.
					RxBuff.Len = 0;
				}
				break;

			case EOT: // End of transmission
				if(Escape)
				{
					// Received byte is not EOT, but data.
					RxBuff.Data[RxBuff.Len++] = *RxData;
					// Reset Escape Flag.
					Escape = FALSE;
				}
				else
				{
					// Received byte is indeed a EOT which indicates end of frame.
					// Calculate CRC to check the validity of the frame.
					if(RxBuff.Len > 1)
					{
						crc.byte.LB = RxBuff.Data[RxBuff.Len-2];
						crc.byte.HB = RxBuff.Data[RxBuff.Len-1];
						if((CalculateCrc(RxBuff.Data, (UINT32)(RxBuff.Len-2)) == crc.Val) && (RxBuff.Len > 2))
						{
							// CRC matches and frame received is valid.
							RxFrameValid = TRUE;

						}
					}

				}
				break;


		    case DLE: // Escape character received.
				if(Escape)
				{
					// Received byte is not ESC but data.
					RxBuff.Data[RxBuff.Len++] = *RxData;
					// Reset Escape Flag.
					Escape = FALSE;
				}
				else
				{
					// Received byte is an escape character. Set Escape flag to escape next byte.
					Escape = TRUE;
				}
				break;

			default: // Data field.
			    RxBuff.Data[RxBuff.Len++] = *RxData;
			    // Reset Escape Flag.
			    Escape = FALSE;
				break;

		}

		//Increment the pointer.
		RxData++;

	}

}

/********************************************************************
* Function: 	HandleCommand()
*
* Precondition:
*
* Input: 		None.
*
* Output:		None.
*
* Side Effects:	None.
*
* Overview: 	Process the received frame and take action depending on
				the command received.
*
*
* Note:		 	None.
********************************************************************/
void HandleCommand(void)
{
	UINT8 Cmd;
	UINT8 i;
	// First byte of the data field is command.
	Cmd = RxBuff.Data[0];
	// Process the command.
	switch(Cmd)
	{
		case READ_BOOT_INFO: // Read boot loader version info.
			printf("Version %d.%d\n", RxBuff.Data[1], RxBuff.Data[2]);
			break;

		case ERASE_FLASH:
			printf("Pic haved erase app flash\n");
			break;

		case PROGRAM_FLASH:
		   	//printf("Pic programe flash return\n");
		   	printf(".");
		   	break;


		case READ_CRC:
			printf("Read Crc{%#x} from pic\n", RxBuff.Data[1]| (RxBuff.Data[2]<<8));
			break;

	    case JMP_TO_APP:
	    	// Exit firmware upgrade mode.
	    	//RunApplication = TRUE;
	    	break;

	    default:
	    	// Nothing to do.
	    	break;
	}



}

void ProcessReturn(UINT8 *RxData, INT16 RxLen)
{
	BuildRxFrame(RxData, RxLen);
	HandleCommand();
}

#define CMD_RETRY		0x05
int main(int argc, char *argv[])
{
	int fd;
	FILE *up_file;
	UINT8 fl_rd_buf[256] = {'\0'};
	UINT8 pre_buf[256];
	UINT8 hex_buf[256];
	UINT8 buf[256];
	UINT8 buf_len;
	INT8 rd_ret, wr_ret;
	UINT8 i;
	UINT8 retry_flash = 0;
	UINT8 retry_cnt;
	UINT32 rd_line = 0;
	UINT32 u32data;
	int seconds;
	printf("pic update\n");
	if((fd = open("/dev/bitmain0", O_RDWR|O_NONBLOCK)) >= 0)//restart pic
	{
		printf("open /dev/bitmain0 to restart pic\n");
		buf[0] = 0x55;
		buf[1] = 0xaa;
		buf[2] = 0xaa;
		buf[3] = 0xbb;
		buf_len = 4;
		if(write(fd, buf, buf_len) != buf_len)
			printf("send pic restart cmd error\n");
		sleep(10);
		seconds= time((time_t*)NULL);
		while(1)
		{
			if((time((time_t*)NULL) - seconds) > 2)// 2S
			{
				printf("wait pic loader timeout\n");
				break;
			}
			fd = open("/dev/bitmainbl0", O_RDWR|O_NONBLOCK);
			if(fd >= 0)
				break;
			else
				usleep(1000 * 100);
		}
	}
	else
		fd = open("/dev/bitmainbl0", O_RDWR|O_NONBLOCK);
	if (fd < 0) {
		perror("Unable to open /dev/bitmainbl0");
		return 1;
	}
	else
	{
		printf("open /dev/bitmainbl0 ok fd = %#x\n", fd);
	}
	retry_cnt = 0;
	while(retry_cnt++ < CMD_RETRY)
	{
		//get bootloader version
		buf_len = structare_cmd(READ_BOOT_INFO, buf, NULL, 0);
		#if 0
		printf("write data buf_len=%d\n", buf_len);
		for(i = 0; i < buf_len; i++)
		{
			printf("[%d]{%#x}, ", i, buf[i]);
		}
		printf("\n");
		#endif
		write(fd, buf, buf_len);
		seconds= time((time_t*)NULL);
		while((time((time_t*)NULL) - seconds) < 1)// 1S
		{
			if((rd_ret = read(fd, buf, sizeof(buf))) > 0)
				break;
			usleep(1000 * 100); //100ms
		}

		if(rd_ret > 0)
		{
			printf("read data\n");
			for(i = 0; i < rd_ret; i++)
			{
				printf("[%d]{%#x}, ", i, buf[i]);
			}
			printf("\n");
			ProcessReturn(buf, rd_ret);
			break;
		}
		else
		{
			printf("get bootloader version read error %d\n", rd_ret);
		}
	}

	//flash app
	if((up_file = fopen(argv[1], "r")) == NULL)
	{
		printf("open pic update file fail \n");
		goto start_app;
	}
	else
	{
		printf("open pic update file success %#x \n", up_file);
	}

	//erase flash
	retry_cnt = 0;
	while(retry_cnt++ < CMD_RETRY)
	{
		buf_len = structare_cmd(ERASE_FLASH, buf, NULL, 0);
		printf("write data buf_len=%d\n", buf_len);
		for(i = 0; i < buf_len; i++)
		{
			printf("[%d]{%#x}, ", i, buf[i]);
		}
		printf("\n");
		write(fd, buf, buf_len);
		seconds= time((time_t*)NULL);
		while((time((time_t*)NULL) - seconds) < 5)// 5S
		{
			if((rd_ret = read(fd, buf, sizeof(buf))) > 0)
				break;
			usleep(1000 * 100); //100ms
		}

		if(rd_ret > 0)
		{
			printf("read data\n");
			for(i = 0; i < rd_ret; i++)
			{
				printf("[%d]{%#x}, ", i, buf[i]);
			}
			printf("\n");

			ProcessReturn(buf, rd_ret);
			break;
		}
		else
		{
			printf("erase flash read error %d\n", rd_ret);
		}
	}
	if(retry_cnt == CMD_RETRY)
	{
		printf("Erase flash error\n");
		return -1;
	}
	while(fgets(fl_rd_buf, sizeof(fl_rd_buf) - 1, up_file) != NULL)
	{
		UINT32 data_len;
		if( fl_rd_buf[strlen(fl_rd_buf)-1] == '\n' || fl_rd_buf[strlen(fl_rd_buf)-1] == '\r' )
        	fl_rd_buf[strlen(fl_rd_buf)-1] = '\0';
		//printf("fl_rd_buf %s\n", fl_rd_buf);
		rd_line++;
		if(EOF == sscanf(fl_rd_buf, "%s", pre_buf))//空行
			continue;
		sscanf(fl_rd_buf, "%*1c%s", pre_buf);
		memset(fl_rd_buf, '\0', sizeof(fl_rd_buf));
		//printf("%dpre_buf: %s\n", rd_line, pre_buf);
		for(i = 0; i < strlen(pre_buf)/2; i++)
		{
			/*
			if(0 == (i%16))
			{
				printf("\n");
			}
			*/
			sscanf(pre_buf + 2 *i, "%02x", &u32data);
			hex_buf[i] = (INT8)u32data;
			//printf("{%d}[%#x], ", i, hex_buf[i]);
		}
		//printf("\n");
		data_len = i;
		retry_flash  = 0;
retry_flash_lb:
		buf_len = structare_cmd(PROGRAM_FLASH, buf, hex_buf, data_len);
		#if 0
		printf("write data buf_len=%d\n", buf_len);
		for(i = 0; i < buf_len; i++)
		{
			printf("[%d]{%#x}, ", i, buf[i]);
		}
		printf("\n");
		#endif
		write(fd, buf, buf_len);
		seconds= time((time_t*)NULL);
		while((time((time_t*)NULL) - seconds) < 1)// 1S
		{
			if((rd_ret = read(fd, buf, sizeof(buf))) > 0)
				break;
			usleep(1000); //1ms
		}

		if(rd_ret > 0)
		{
			#if 0
			printf("read data\n");
			for(i = 0; i < rd_ret; i++)
			{
				printf("[%d]{%#x}, ", i, buf[i]);
			}
			printf("\n");
			#endif
			ProcessReturn(buf, rd_ret);
		}
		else
		{
			printf("line: %d flash flash read error %d\n", rd_line, rd_ret);
			printf("%dpre_buf: %s  buf_len{%d}\n ", rd_line, pre_buf, buf_len);
			if(retry_flash++ < CMD_RETRY)//几次重试
				goto retry_flash_lb;
			else
				goto start_app;
		}
	}
	//get crc
	if( up_file != NULL)
		fclose(up_file);
	remove(argv[1]); //升级完删除
	//Start to App
start_app:
	printf("Control bootloader to app\n");
	buf_len = structare_cmd(JMP_TO_APP, buf, NULL, 0);
	write(fd, buf, buf_len);
	return 0;
}

