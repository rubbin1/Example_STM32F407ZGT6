//
// MCU 间通信协议（SOF + LEN + MSG_TYPE + FUNC_CODE + DATA + XOR）
//
#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <stdint.h>
#include "serial.h"

/* ── 帧结构 ─────────────────────────────
   | SOF_H | SOF_L | LEN | MSG_TYPE | FUNC_CODE | DATA[0..N] | XOR |
   | 0xAA  | 0x55  |     |          |           |            |     |
   LEN = 2 + N（MSG_TYPE + FUNC_CODE + DATA 总长度）
   XOR = LEN ^ MSG_TYPE ^ FUNC_CODE ^ DATA[0] ^ ... ^ DATA[N-1]
   SOF 不参与 XOR
────────────────────────────────────────── */

/* Data数据说明：
 *
 * 针对主 -> 从控制外设动作
 * |————LED：————LED0：控制LED0的亮灭，LED0灭，LED1亮
 *            |——LED1：同上
 *            |——LED2：同上
 *            |——LED_FLOW：流水灯，LED_FLOW0流水灯关，LED_FLOW1500流水灯开，500ms
 * |————SMG：————SMG：控制数码管参数
 *                  |————SMG：SMG显示SMGx1x2，x1为位数0-7，x2为当前位显示的数字
 *                          |————SMG_ALL：全图显示SMG，可以显示小数点
 */

#define PROTO_SOF_H  0xAA
#define PROTO_SOF_L  0x55

#define PROTO_DATA_MAX  128

/* ── 消息类型 ──────────────────────────── */
typedef enum {
    MSG_SYSTEM  = 0x00,   // 系统：心跳、应答、复位
    MSG_CONTROL = 0x01,   // 主 → 从：控制外设动作
    MSG_QUERY   = 0x02,   // 主 → 从：查询/读取
    MSG_REPORT  = 0x03,   // 从 → 主：数据上报
    MSG_DATA    = 0x04,   // 双向：数据块透传
    MSG_CUSTOM  = 0x0F,   // 自定义/调试
} ProtoMsgType;

/* ── 功能码 ────────────────────────────── */
/* MSG_SYSTEM */
typedef enum {
    FUNC_SYS_PING  = 0x00,
    FUNC_SYS_ACK   = 0x01,
    FUNC_SYS_ERROR = 0x02,
    FUNC_SYS_RESET = 0x03,
} FuncSystem;

/* MSG_CONTROL */
typedef enum {
    FUNC_CTRL_LED  = 0x00,   // LED 开关
    FUNC_CTRL_SMG  = 0x01,   // 数码管
} FuncControl;

/* MSG_QUERY */
typedef enum {
    FUNC_QUERY_SERVO  = 0x00,
    FUNC_QUERY_ADC    = 0x01,
    FUNC_QUERY_SENSOR = 0x02,
    FUNC_QUERY_STATE  = 0x03,
} FuncQuery;

/* MSG_REPORT */
typedef enum {
    FUNC_RPT_ADC    = 0x00,
    FUNC_RPT_KEY    = 0x01,
    FUNC_RPT_STATE  = 0x02,
    FUNC_RPT_SENSOR = 0x03,
} FuncReport;

/* MSG_DATA */
typedef enum {
    FUNC_DATA_SEND  = 0x00,
    FUNC_DATA_ACK   = 0x01,
    FUNC_DATA_RETRY = 0x02,
} FuncData;

/* ── Control 数据包体 ──────────────────── */

/* LED：MSG_CONTROL / FUNC_CTRL_LED */
typedef struct __attribute__((packed)) {
    uint8_t  led0;      // 0=灭, 1=亮
    uint8_t  led1;
    uint8_t  led2;
    uint16_t flow_ms;   // 0=关流水灯, >0=流水灯间隔(ms)
} ProtoCtrlLed;

/* 数码管模式 */
typedef enum {
    SMG_SINGLE = 0,     // 更新单个数码管位
    SMG_ALL    = 1,     // 全图刷新（含小数点）
} ProtoSmgMode;

/* 单个数码管位更新 */
typedef struct {
    uint8_t pos;        // 位 0~7
    uint8_t digit;      // 显示的数字 0~9 / 0~F
} ProtoSmgSingle;

/* 全图数码管更新 */
typedef struct {
    uint8_t seg[8];     // 每位显示的数字/段码
    uint8_t dp;         // 小数点 bit0~bit7
} ProtoSmgAll;

/* SMG：MSG_CONTROL / FUNC_CTRL_SMG */
typedef struct __attribute__((packed)) {
    uint8_t mode;       // SMG_SINGLE 或 SMG_ALL
    union {
        ProtoSmgSingle single;
        ProtoSmgAll    all;
    };
} ProtoCtrlSmg;

/* ── 解析状态机 ────────────────────────── */
typedef enum {
    WAIT_SOF_H = 0,
    WAIT_SOF_L,
    WAIT_LEN,
    WAIT_PAYLOAD,
    WAIT_XOR,
} ProtoState;

/* ── 帧回调（收完整帧后调用）───────────── */
typedef void (*ProtoCallback)(uint8_t msg_type, uint8_t func_code,
                              const uint8_t *data, uint8_t len);

/* ── 协议实例 ──────────────────────────── */
typedef struct {
    ProtoState    state;
    uint8_t       len;         // MSG_TYPE + FUNC_CODE + DATA 的总长
    uint8_t       idx;         // payload 已收字节数
    uint8_t       msg_type;
    uint8_t       func_code;
    uint8_t       data_buf[PROTO_DATA_MAX];
    uint8_t       xor_calc;
    uint8_t       xor_rcvd;
    ProtoCallback on_frame;
} Protocol;

/* ── API ──────────────────────────────── */
void Proto_Init(Protocol *p, ProtoCallback cb);
void Proto_Feed(Protocol *p, uint8_t byte);
void Proto_SendFrame(Serial_t *s, uint8_t msg_type, uint8_t func_code,
                     const uint8_t *data, uint8_t data_len);

#endif // PROTOCOL_H
