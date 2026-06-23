//
// MCU 间通信协议实现
//
#include "protocol.h"

void Proto_Init(Protocol *p, ProtoCallback cb)
{
    p->state    = WAIT_SOF_H;
    p->len      = 0;
    p->idx      = 0;
    p->xor_calc = 0;
    p->on_frame = cb;
}

/* ── 发送 ──────────────────────────────── */
void Proto_SendFrame(Serial_t *s, uint8_t msg_type, uint8_t func_code,
                     const uint8_t *data, uint8_t data_len)
{
    uint8_t xor_val = 0;
    uint8_t len = 2 + data_len;   // MSG_TYPE + FUNC_CODE + DATA

    // SOF
    Serial_SendByte(s, PROTO_SOF_H);
    Serial_SendByte(s, PROTO_SOF_L);

    // LEN
    Serial_SendByte(s, len);
    xor_val ^= len;

    // MSG_TYPE
    Serial_SendByte(s, msg_type);
    xor_val ^= msg_type;

    // FUNC_CODE
    Serial_SendByte(s, func_code);
    xor_val ^= func_code;

    // DATA
    for (uint8_t i = 0; i < data_len; i++) {
        Serial_SendByte(s, data[i]);
        xor_val ^= data[i];
    }

    // XOR
    Serial_SendByte(s, xor_val);
}

/* ── 接收状态机 ────────────────────────── */
void Proto_Feed(Protocol *p, uint8_t byte)
{
    switch (p->state) {

    case WAIT_SOF_H:
        if (byte == PROTO_SOF_H) {
            p->state = WAIT_SOF_L;
        }
        break;

    case WAIT_SOF_L:
        if (byte == PROTO_SOF_L) {
            p->state    = WAIT_LEN;
            p->xor_calc = 0;
        } else {
            p->state = WAIT_SOF_H;
        }
        break;

    case WAIT_LEN:
        p->len      = byte;
        p->idx      = 0;
        p->xor_calc = byte;
        if (p->len >= 2 && p->len <= PROTO_DATA_MAX) {   // 至少含 MSG_TYPE + FUNC_CODE
            p->state = WAIT_PAYLOAD;
        } else {
            p->state = WAIT_SOF_H;
        }
        break;

    case WAIT_PAYLOAD:
        if (p->idx == 0) {
            p->msg_type = byte;          // 第 1 字节：消息类型
        } else if (p->idx == 1) {
            p->func_code = byte;         // 第 2 字节：功能码
        } else {
            p->data_buf[p->idx - 2] = byte;  // 后续：数据
        }
        p->xor_calc ^= byte;
        p->idx++;
        if (p->idx >= p->len) {
            p->state = WAIT_XOR;
        }
        break;

    case WAIT_XOR:
        p->xor_rcvd = byte;
        if (p->xor_calc == p->xor_rcvd && p->on_frame) {
            p->on_frame(p->msg_type, p->func_code,
                        p->data_buf, p->len - 2);   // len - 2 = DATA 长度
        }
        p->state = WAIT_SOF_H;
        break;
    }
}
