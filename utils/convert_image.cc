#include "utils/convert_image.h"
#include <optional>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "three_party/stb_image.h"
#include "webp/encode.h"
#include <fstream>

// 该函数将内存中的图片数据转换为 WebP 格式
// input_data: 原始图片数据（如 PNG/JPG 文件的字节流）
// input_size: 原始数据的大小
// quality:    压缩质量 (0 = 最小质量/最小体积, 100 = 最高质量/最大体积)
// output_data: 输出的 WebP 数据
// 返回值:      成功返回 true
bool ConvertToWebP(const uint8_t *input_data, size_t input_size, int quality,
                   std::vector<uint8_t> &output_data) {
    int width, height, channels;
    // 1. 使用 stb_image 从内存中解码图片，强制转换为 4 通道 (RGBA)
    //    这样处理起来最统一，libwebp 也原生支持 RGBA 输入。
    uint8_t *rgba_data = stbi_load_from_memory(input_data, input_size, &width,
                                               &height, &channels, 4);
    if (rgba_data == nullptr) {
        // 解码失败，可能是图片格式不受支持
        return false;
    }

    // 2. 使用 libwebp 将 RGBA 数据编码为 WebP
    uint8_t *webp_data = nullptr;
    size_t webp_size = WebPEncodeRGBA(rgba_data, width, height, width * 4,
                                      quality, &webp_data);

    if (webp_size == 0) {
        // 编码失败
        stbi_image_free(rgba_data);
        return false;
    }

    // 3. 将编码后的数据复制到输出 vector 中
    output_data.assign(webp_data, webp_data + webp_size);

    // 4. 释放 libwebp 和 stb_image 分配的内存
    free(webp_data);
    stbi_image_free(rgba_data);
    return true;
}

// 一个函数，如果成功，那么返回空字符串，如果失败，返回错误描述
std::string convertToWebpFile(const std::string in, const std::string out) {
    // 读取文件
    std::ifstream is(in, std::ios::binary);
    if (!is) {
        return "读取文件失败";
    }
    std::vector<uint8_t> input_data((std::istreambuf_iterator<char>(is)),
                                    std::istreambuf_iterator<char>());
    is.close();

    std::vector<uint8_t> output_data;

    if (!ConvertToWebP(input_data.data(), input_data.size(), 75.0f,
                       output_data)) {
        return "转换格式失败";
    }
    std::ofstream os(out, std::ios::binary);
    os.write(reinterpret_cast<const char *>(output_data.data()),
             output_data.size());
    os.close();

    return "";
}
