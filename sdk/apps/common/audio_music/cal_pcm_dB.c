#include "system/includes.h"

// 使用定点运算的PCM分贝值计算函数
// samples: PCM数据数组
// length: 数组长度
// 返回值: 分贝值 * 100 (比如返回-2000表示-20dB)
int32_t calculate_db_level(const int16_t *samples, size_t size)
{
    size_t length = size / sizeof(int16_t);

    if (samples == NULL || length == 0) {
        return -32768;  // 返回最小值表示无声
    }


    // 计算平方和，注意防止溢出
    uint64_t sum = 0;  // 使用64位避免溢出
    for (size_t i = 0; i < length; i++) {
        // 每次除以16避免中间结果溢出
        int32_t val = samples[i] / 16;
        sum += (uint64_t)(val * val);
    }

    // 计算RMS值，这里需要除以length
    uint32_t rms = 0;
    if (length > 0) {
        // 先除以length，再乘以16*16还原之前的除法
        rms = (uint32_t)((sum / (float)length) * 256); // 256 = 16 * 16
    }

    // 如果信号太小，直接返回最小值
    if (rms < 1) {
        return -32768;
    }

    // 使用查表法计算log10
    // 预计算的分贝表，每个元素表示index对应的分贝值*100
    // 表的大小可以根据需要调整，这里只是示例
    static const int16_t db_lookup_table[] = {
        -9000, -8500, -8000, -7500, -7000, -6500, -6000, -5500,
        -5000, -4500, -4000, -3500, -3000, -2500, -2000, -1500,
        -1000,  -500,     0
    };

    // 找到最接近的分贝值
    int32_t db = -9000;  // 默认最小值
    uint32_t ref_value = (32768 * 32768) / 256; // 满刻度参考值

    for (int i = 0; i < sizeof(db_lookup_table) / sizeof(db_lookup_table[0]); i++) {
        uint32_t threshold = ref_value >> (i + 1);
        if (rms >= threshold) {
            db = db_lookup_table[i];
            break;
        }
    }

    return db;
}

