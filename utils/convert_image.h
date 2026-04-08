#pragma once

#include <vector>

bool ConvertToWebP(const uint8_t *input_data, size_t input_size, int quality,
                   std::vector<uint8_t> &output_data);

std::string convertToWebpFile(const std::string in, const std::string out);
