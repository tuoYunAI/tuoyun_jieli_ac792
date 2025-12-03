#说明：遍历当前文件夹下所有.zip文件生成名为zip_info.bin的信息文件
#调用方法：python gen_info_file.py

import os
import struct

def get_last_16_chars(filename):
    base = os.path.basename(filename)
    if len(base) > 16:
        return base[-16:].encode('ascii')
    else:
        return base.encode('ascii').ljust(16, b'\x00')

def main():
    files = [f for f in os.listdir('.') if f.endswith('.zip')]
    file_num = len(files)

    with open('zip_info.bin', 'wb') as fout:
        # 写入文件数量，4字节小端
        fout.write(struct.pack('<I', file_num))

        for f in files:
            with open(f, 'rb') as fin:
                jl_bin = fin.read(20)
                img_dsc = fin.read(12)

            filename_bytes = get_last_16_chars(f)
            fout.write(filename_bytes)
            fout.write(jl_bin)
            fout.write(img_dsc)

if __name__ == '__main__':
    main()
