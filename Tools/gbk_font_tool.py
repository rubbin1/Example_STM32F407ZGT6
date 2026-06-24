"""
GBK 字库生成 & 串口烧录工具
用法:
    python gbk_font_tool.py gen 16        # 生成 16x16 字库 + Unicode 映射表
    python gbk_font_tool.py gen 24        # 生成 24x24 字库 + Unicode 映射表
    python gbk_font_tool.py send COM3 16  # 通过串口发送字库和映射表到单片机
    python gbk_font_tool.py gen 16 --send COM3  # 生成并直接发送
"""

import sys
import os
import struct
import serial
import time
from functools import reduce
from operator import xor
from PIL import Image, ImageDraw, ImageFont

# ---------------------------------------------------------------------------
# 配置
# ---------------------------------------------------------------------------
FONT_PATH_16 = "C:/Windows/Fonts/simsun.ttc"   # 宋体 (16x16)
FONT_PATH_24 = "C:/Windows/Fonts/simhei.ttf"   # 黑体 (24x24)

OUTPUT_DIR = os.path.join(os.path.dirname(__file__), "output")

# 串口协议帧
FRAME_HEADER = b'\xAA\x55'
FRAME_TAIL   = b'\x55\xAA'

# Flash 地址分配
FLASH_FONT16_ADDR = 0x000000        # 16x16 字库基址 (占 ~656KB)
FLASH_FONT24_ADDR = 0x100000        # 24x24 字库基址 (1MB 后)
FLASH_MAP16_ADDR  = 0x600000        # 16x16 映射表 (6MB 处)
FLASH_MAP24_ADDR  = 0x700000        # 24x24 映射表

# ---------------------------------------------------------------------------
# GBK 编码遍历
# ---------------------------------------------------------------------------
def gbk_to_unicode(gbk_bytes):
    try:
        return gbk_bytes.decode('gbk')
    except (UnicodeDecodeError, LookupError):
        return None

# ---------------------------------------------------------------------------
# 字模生成
# ---------------------------------------------------------------------------
def render_char(char, font, size):
    """渲染单个字符，返回 bytes（逐行 MSB 在前）"""
    img = Image.new('1', (size, size), 0)
    draw = ImageDraw.Draw(img)
    draw.text((1, 1), char, font=font, fill=1)
    pixels = img.load()
    byte_rows = []
    bytes_per_row = (size + 7) // 8
    for row in range(size):
        for col_byte in range(bytes_per_row):
            byte_val = 0
            for bit in range(8):
                col = col_byte * 8 + bit
                if col < size and pixels[col, row]:
                    byte_val |= (0x80 >> bit)
            byte_rows.append(byte_val)
    return bytes(byte_rows)

def generate_font_bin(size):
    """生成字库 bin 和 排序后的 Unicode→GBK 映射表"""
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    out_path = os.path.join(OUTPUT_DIR, f"gbk_{size}x{size}.bin")
    map_path = os.path.join(OUTPUT_DIR, f"gbk_{size}_uni_map.bin")

    font_path = FONT_PATH_16 if size == 16 else FONT_PATH_24
    font = ImageFont.truetype(font_path, size - 1)

    chars_per_row = 190
    total_chars = 0
    byte_per_char = (size * size) // 8
    records = []  # [(unicode, gbk_index), ...] 只存有效字符

    print(f"生成 {size}x{size} GBK 字库...")
    print(f"  字体: {font_path}")
    print(f"  字库: {out_path}")
    print(f"  映射: {map_path}")

    gbk_idx = 0
    with open(out_path, 'wb') as f:
        for hi in range(0x81, 0xFF):
            for lo_idx in range(chars_per_row):
                if lo_idx < 63:
                    lo = lo_idx + 0x40
                else:
                    lo = lo_idx + 0x41

                gbk_bytes = bytes([hi, lo])
                char = gbk_to_unicode(gbk_bytes)

                if char:
                    bitmap = render_char(char, font, size)
                    records.append((ord(char), gbk_idx))
                else:
                    bitmap = b'\x00' * byte_per_char

                f.write(bitmap)
                gbk_idx += 1
                total_chars += 1

            if hi % 32 == 0:
                sys.stdout.write(f"\r  进度: {total_chars} / {126 * 190} 字")
                sys.stdout.flush()

    # 按 Unicode 码点排序
    records.sort(key=lambda r: r[0])

    # 写入映射表: [count: uint32 LE] + 每条 [unicode: uint16 LE, gbk_idx: uint16 LE]
    with open(map_path, 'wb') as mf:
        mf.write(struct.pack('<I', len(records)))
        for uni, idx in records:
            mf.write(struct.pack('<HH', uni, idx))

    file_size = os.path.getsize(out_path)
    map_size  = os.path.getsize(map_path)
    print(f"\r  完成: {len(records)} 有效字 / {gbk_idx} 位置, 字库 {file_size / 1024:.1f} KB, 映射 {map_size / 1024:.1f} KB\n")
    return out_path, map_path

# ---------------------------------------------------------------------------
# 串口传输协议
# ---------------------------------------------------------------------------
def send_file_via_serial(port, file_path, flash_addr, baudrate=115200):
    file_size = os.path.getsize(file_path)
    label = os.path.basename(file_path)
    print(f"打开串口 {port} @ {baudrate}...")
    ser = serial.Serial(port, baudrate, timeout=2)
    print(f"发送: {label} -> Flash 0x{flash_addr:06X} ({file_size} bytes)")

    with open(file_path, 'rb') as f:
        offset = 0
        while offset < file_size:
            chunk = f.read(256)
            if not chunk:
                break

            frame = FRAME_HEADER
            frame += struct.pack('<I', flash_addr + offset)
            frame += struct.pack('<H', len(chunk))
            frame += chunk
            frame += bytes([reduce(xor, chunk, 0)])
            frame += FRAME_TAIL
            ser.write(frame)

            ack = ser.read(1)
            if ack == b'\x06':
                offset += len(chunk)
            elif ack == b'\x15':
                print(f"  NAK @ offset {offset}, 重发...")
                f.seek(offset)
            else:
                print(f"  超时 @ offset {offset}, 重发...")
                f.seek(offset)

            if offset % (256 * 50) == 0:
                pct = offset * 100 // file_size
                print(f"\r  进度: {pct}% ({offset}/{file_size})", end='')

    ser.close()
    print(f"\r  发送完成: {offset} / {file_size} bytes\n")

# ---------------------------------------------------------------------------
# main
# ---------------------------------------------------------------------------
def parse_args(args):
    """解析: 命令(gen/send) 数字(size, 0=all) --send <port>"""
    cmd   = None
    sizes = [16]
    port  = None
    i = 1
    while i < len(args):
        a = args[i]
        if a in ('gen', 'send'):
            cmd = a
        elif a.isdigit():
            s = int(a)
            sizes = [16, 24] if s == 0 else [s]
        elif a == 'all':
            sizes = [16, 24]
        elif a == '--send':
            i += 1
            port = args[i] if i < len(args) else 'COM3'
        i += 1
    return cmd, sizes, port

if __name__ == '__main__':
    cmd, sizes, port = parse_args(sys.argv)

    if cmd == 'gen':
        for size in sizes:
            font_path, map_path = generate_font_bin(size)
            if port:
                faddr = FLASH_FONT16_ADDR if size <= 16 else FLASH_FONT24_ADDR
                maddr = FLASH_MAP16_ADDR if size <= 16 else FLASH_MAP24_ADDR
                send_file_via_serial(port, font_path, faddr)
                send_file_via_serial(port, map_path,  maddr)

    elif cmd == 'send':
        for size in sizes:
            font_path = os.path.join(OUTPUT_DIR, f"gbk_{size}x{size}.bin")
            map_path  = os.path.join(OUTPUT_DIR, f"gbk_{size}_uni_map.bin")
            if not os.path.exists(font_path):
                print(f"文件不存在: {font_path}, 请先 gen {size}")
                continue
            if not os.path.exists(map_path):
                print(f"映射表不存在: {map_path}, 请重新 gen {size}")
                continue
            faddr = FLASH_FONT16_ADDR if size <= 16 else FLASH_FONT24_ADDR
            maddr = FLASH_MAP16_ADDR if size <= 16 else FLASH_MAP24_ADDR
            send_file_via_serial(port or 'COM3', font_path, faddr)
            send_file_via_serial(port or 'COM3', map_path,  maddr)

    else:
        print(__doc__)
        print("示例:")
        print("  python gbk_font_tool.py gen all --send COM9   # 生成并发送 16+24")
        print("  python gbk_font_tool.py gen 16 --send COM9    # 只 16×16")
        print("  python gbk_font_tool.py send all COM9          # 发送已有的 16+24")
