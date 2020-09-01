/* -*-c++-*- PatchMatchStereo - Copyright (C) 2020.
* Author	: Yingsong Li(Ethan Li) <ethan.li.whu@gmail.com>
*			  https://github.com/ethan-li-coding
* Describe	: implement of pms_util
*/

#include "stdafx.h"
#include "pms_util.h"
#include <vector>
#include <algorithm>

PColor pms_util::GetColor(const uint8* img_data, const sint32& width, const sint32& height, const sint32& x, const sint32& y)
{
	auto* pixel = img_data + y * width * 3 + 3 * x;
	return {pixel[0], pixel[1], pixel[2]};
}

void pms_util::MedianFilter(const float32* in, float32* out, const sint32& width, const sint32& height,
	const sint32 wnd_size)
{
	const sint32 radius = wnd_size / 2;
	const sint32 size = wnd_size * wnd_size;

	// 存储局部窗口内的数据
	std::vector<float32> wnd_data;
	wnd_data.reserve(size);

	for (sint32 y = 0; y < height; y++) {
		for (sint32 x = 0; x < width; x++) {
			wnd_data.clear();

			// 获取局部窗口数据
			for (sint32 r = -radius; r <= radius; r++) {
				for (sint32 c = -radius; c <= radius; c++) {
					const sint32 row = y + r;
					const sint32 col = x + c;
					if (row >= 0 && row < height && col >= 0 && col < width) {
						wnd_data.push_back(in[row * width + col]);
					}
				}
			}

			// 排序
			std::sort(wnd_data.begin(), wnd_data.end());

			if (!wnd_data.empty()) {
				// 取中值
				out[y * width + x] = wnd_data[wnd_data.size() / 2];
			}
		}
	}
}


void pms_util::WeightedMedianFilter(const uint8* img_data, const sint32& width, const sint32& height, const sint32& wnd_size, const float32& gamma, const vector<pair<int, int>>& filter_pixels, float32* disparity_map)
{
	const sint32 wnd_size2 = wnd_size / 2;

	// 带权视差集
	vector<pair<float32,float32>> disps;
	disps.reserve(wnd_size * wnd_size);

	for (auto& pix : filter_pixels) {
		const sint32 x = pix.first;
		const sint32 y = pix.second;	
		// weighted median filter
		disps.clear();
		const auto& col_p = GetColor(img_data, width, height, x, y);
		float32 total_w = 0.0f;
		for (sint32 r = -wnd_size2; r <= wnd_size2; r++) {
			for (sint32 c = -wnd_size2; c <= wnd_size2; c++) {
				const sint32 yr = y + r;
				const sint32 xc = x + c;
				if (yr < 0 || yr >= height || xc < 0 || xc >= width) {
					continue;
				}
				const auto& disp = disparity_map[yr * width + xc];
				if(disp == Invalid_Float) {
					continue;
				}
				// 计算权值
				const auto& col_q = GetColor(img_data, width, height, xc, yr);
				const auto dc = abs(col_p.r - col_q.r) + abs(col_p.g - col_q.g) + abs(col_p.b - col_q.b);
				const auto w = exp(-dc / gamma);
				total_w += w;

				// 存储带权视差
				disps.emplace_back(disp, w);
			}
		}

		// --- 取加权中值
		// 按视差值排序
		std::sort(disps.begin(), disps.end());
		const float32 median_w = total_w / 2;
		float32 w = 0.0f;
		for (auto& wd : disps) {
			w += wd.second;
			if (w >= median_w) {
				disparity_map[y * width + x] = wd.first;
				break;
			}
		}
	}
}


