#ifndef MPI_POINTER
#define MPI_POINTER     *
#endif

#define MPI_IOCSTATUS_MASK                      (0x7FFF)

/****************************************************************************/
/*  Common IOCStatus values for all replies                                 */
/****************************************************************************/

#define MPI_IOCSTATUS_SUCCESS                   (0x0000)
#define MPI_IOCSTATUS_INVALID_FUNCTION          (0x0001)
#define MPI_IOCSTATUS_BUSY                      (0x0002)
#define MPI_IOCSTATUS_INVALID_SGL               (0x0003)
#define MPI_IOCSTATUS_INTERNAL_ERROR            (0x0004)
#define MPI_IOCSTATUS_RESERVED                  (0x0005)
#define MPI_IOCSTATUS_INSUFFICIENT_RESOURCES    (0x0006)
#define MPI_IOCSTATUS_INVALID_FIELD             (0x0007)
#define MPI_IOCSTATUS_INVALID_STATE             (0x0008)
#define MPI_IOCSTATUS_OP_STATE_NOT_SUPPORTED    (0x0009)

/*
 * Values for SASStatus.
 */
#define MPI_SASSTATUS_SUCCESS                           (0x00)
#define MPI_SASSTATUS_UNKNOWN_ERROR                     (0x01)
#define MPI_SASSTATUS_INVALID_FRAME                     (0x02)
#define MPI_SASSTATUS_UTC_BAD_DEST                      (0x03)
#define MPI_SASSTATUS_UTC_BREAK_RECEIVED                (0x04)
#define MPI_SASSTATUS_UTC_CONNECT_RATE_NOT_SUPPORTED    (0x05)
#define MPI_SASSTATUS_UTC_PORT_LAYER_REQUEST            (0x06)
#define MPI_SASSTATUS_UTC_PROTOCOL_NOT_SUPPORTED        (0x07)
#define MPI_SASSTATUS_UTC_STP_RESOURCES_BUSY            (0x08)
#define MPI_SASSTATUS_UTC_WRONG_DESTINATION             (0x09)
#define MPI_SASSTATUS_SHORT_INFORMATION_UNIT            (0x0A)
#define MPI_SASSTATUS_LONG_INFORMATION_UNIT             (0x0B)
#define MPI_SASSTATUS_XFER_RDY_INCORRECT_WRITE_DATA     (0x0C)
#define MPI_SASSTATUS_XFER_RDY_REQUEST_OFFSET_ERROR     (0x0D)
#define MPI_SASSTATUS_XFER_RDY_NOT_EXPECTED             (0x0E)
#define MPI_SASSTATUS_DATA_INCORRECT_DATA_LENGTH        (0x0F)
#define MPI_SASSTATUS_DATA_TOO_MUCH_READ_DATA           (0x10)
#define MPI_SASSTATUS_DATA_OFFSET_ERROR                 (0x11)
#define MPI_SASSTATUS_SDSF_NAK_RECEIVED                 (0x12)
#define MPI_SASSTATUS_SDSF_CONNECTION_FAILED            (0x13)
#define MPI_SASSTATUS_INITIATOR_RESPONSE_TIMEOUT        (0x14)

typedef signed   char   S8;
typedef unsigned char   U8;
typedef signed   short  S16;
typedef unsigned short  U16;

typedef int32_t   S32;
typedef u_int32_t U32;

typedef struct _S64
{
    U32          Low;
    S32          High;
} S64;

typedef struct _U64
{
    U32          Low;
    U32          High;
} U64;

/* Serial Management Protocol Passthrough Reply */
typedef struct _MSG_SMP_PASSTHROUGH_REPLY
{
    U8                      PassthroughFlags;   /* 00h */
    U8                      PhysicalPort;       /* 01h */
    U8                      MsgLength;          /* 02h */
    U8                      Function;           /* 03h */
    U16                     ResponseDataLength; /* 04h */
    U8                      Reserved1;          /* 06h */
    U8                      MsgFlags;           /* 07h */
    U32                     MsgContext;         /* 08h */
    U8                      Reserved2;          /* 0Ch */
    U8                      SASStatus;          /* 0Dh */
    U16                     IOCStatus;          /* 0Eh */
    U32                     IOCLogInfo;         /* 10h */
    U32                     Reserved3;          /* 14h */
    U8                      ResponseData[4];    /* 18h */
} MSG_SMP_PASSTHROUGH_REPLY, MPI_POINTER PTR_MSG_SMP_PASSTHROUGH_REPLY,
  SmpPassthroughReply_t, MPI_POINTER pSmpPassthroughReply_t;
