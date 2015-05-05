#ifndef PIC_UPDATE_H__
#define PIC_UPDATE_H__

typedef unsigned long	UINT32;
typedef unsigned short	UINT16;
typedef unsigned char	UINT8;
typedef unsigned int	UINT;

typedef UINT8	BYTE;
typedef UINT16	WORD;
typedef UINT32	DWORD;

typedef signed int          INT;
typedef signed char         INT8;
typedef signed short int    INT16;
typedef signed long int     INT32;
typedef signed long long    INT64;

typedef enum _BOOL { FALSE = 0, TRUE } BOOL;    /* Undefined size */

#define SOH 01
#define EOT 04
#define DLE 16


#define FRAMEWORK_BUFF_SIZE 1024

typedef enum
{
	READ_BOOT_INFO = 1,
	ERASE_FLASH,
	PROGRAM_FLASH,
	READ_CRC,
	JMP_TO_APP

}T_COMMANDS;

typedef union
{
    WORD Val;
    BYTE v[2]__attribute__((packed));
    struct
    {
        BYTE LB;
        BYTE HB;
    }__attribute__((packed)) byte;
    struct
    {
        BYTE b0:1;
        BYTE b1:1;
        BYTE b2:1;
        BYTE b3:1;
        BYTE b4:1;
        BYTE b5:1;
        BYTE b6:1;
        BYTE b7:1;
        BYTE b8:1;
        BYTE b9:1;
        BYTE b10:1;
        BYTE b11:1;
        BYTE b12:1;
        BYTE b13:1;
        BYTE b14:1;
        BYTE b15:1;
    }__attribute__((packed)) bits;
} WORD_VAL, WORD_BITS;

typedef struct
{
	UINT Len;
	UINT8 Data[FRAMEWORK_BUFF_SIZE];

}T_FRAME;

#endif