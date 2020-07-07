/* -*-c++-*- PatchMatchStereo - Copyright (C) 2020.
* Author	: Ethan Li <ethan.li.whu@gmail.com>
*			  https://github.com/ethan-li-coding
* Describe	: implement of patch match stereo class
*/

#include "stdafx.h"
#include "PatchMatchStereo.h"
#include <ctime>
#include <random>
#include "pms_propagation.h"
#include "pms_util.h"

PatchMatchStereo::PatchMatchStereo(): width_(0), height_(0), img_left_(nullptr), img_right_(nullptr),
                                      gray_left_(nullptr), gray_right_(nullptr),
                                      grad_left_(nullptr), grad_right_(nullptr),
                                      cost_left_(nullptr), cost_right_(nullptr), 
                                      disp_left_(nullptr), disp_right_(nullptr),
                                      plane_left_(nullptr), plane_right_(nullptr),
                                      is_initialized_(false) { }


PatchMatchStereo::~PatchMatchStereo()
{
	Release();
}

bool PatchMatchStereo::Initialize(const sint32 & width, const sint32 & height, const PMSOption & option)
{
	// ··· 赋值

	// 影像尺寸
	width_ = width;
	height_ = height;
	// PMS参数
	option_ = option;

	if (width <= 0 || height <= 0) {
		return false;
	}

	//··· 开辟内存空间
	const sint32 img_size = width * height;
	const sint32 disp_range = option.max_disparity - option.min_disparity;
	// 灰度数据
	gray_left_ = new uint8[img_size];
	gray_right_ = new uint8[img_size];
	// 梯度数据
	grad_left_ = new PGradient[img_size]();
	grad_right_ = new PGradient[img_size]();
	// 代价数据
	cost_left_ = new float32[img_size];
	cost_right_ = new float32[img_size];
	// 视差图
	disp_left_ = new float32[img_size];
	disp_right_ = new float32[img_size];
	// 平面集
	plane_left_ = new DisparityPlane[img_size];
	plane_right_ = new DisparityPlane[img_size];

	is_initialized_ = grad_left_ && grad_right_ && disp_left_ && disp_right_  && plane_left_ && plane_right_;

	return is_initialized_;
}

void PatchMatchStereo::Release()
{
	SAFE_DELETE(grad_left_);
	SAFE_DELETE(grad_right_);
	SAFE_DELETE(cost_left_);
	SAFE_DELETE(cost_right_);
	SAFE_DELETE(disp_left_);
	SAFE_DELETE(disp_right_);
	SAFE_DELETE(plane_left_);
	SAFE_DELETE(plane_right_);
}

bool PatchMatchStereo::Match(const uint8* img_left, const uint8* img_right, float32* disp_left)
{
	if (!is_initialized_) {
		return false;
	}
	if (img_left == nullptr || img_right == nullptr) {
		return false;
	}

	img_left_ = img_left;
	img_right_ = img_right;

	// 随机初始化
	RandomInitialization();

	// 计算灰度图
	ComputeGray();

	// 计算梯度图
	ComputeGradient();

	// 迭代传播
	Propagation();

	// 平面转换成视差
	PlaneToDisparity();

	// 左右一致性检查
	if (option_.is_check_lr) {
		// 一致性检查
		LRCheck();
	}

	// 视差填充
	if (option_.is_fill_holes) {
		FillHolesInDispMap();
	}

	// 输出视差图
	if (disp_left && disp_left_) {
		memcpy(disp_left, disp_left_, height_ * width_ * sizeof(float32));
	}
	return true;
}

bool PatchMatchStereo::Reset(const uint32& width, const uint32& height, const PMSOption& option)
{
	// 释放内存
	Release();
	
	// 重置初始化标记
	is_initialized_ = false;

	return Initialize(width, height, option);
}

float* PatchMatchStereo::GetDisparityMap(const sint32& view) const
{
	switch (view) {
	case 0:
		return disp_left_;
	case 1:
		return disp_right_;
	default:
		return nullptr;
	}
}

PGradient* PatchMatchStereo::GetGradientMap(const sint32& view) const
{
	switch (view) {
	case 0:
		return grad_left_;
	case 1:
		return grad_right_;
	default:
		return nullptr;
	}
}

void PatchMatchStereo::RandomInitialization() const
{
	const sint32 width = width_;
	const sint32 height = height_;
	if (width <= 0 || height <= 0 ||
		disp_left_ == nullptr || disp_right_ == nullptr ||
		plane_left_ == nullptr || plane_right_ == nullptr) {
		return;
	}
	const auto& option = option_;
	const sint32 min_disparity = option.min_disparity;
	const sint32 max_disparity = option.max_disparity;

	// 随机数生成器
	std::random_device rd;
	std::mt19937 gen(rd());
	const std::uniform_real_distribution<float32> rand_d(static_cast<float32>(min_disparity), static_cast<float32>(max_disparity));
	const std::uniform_real_distribution<float32> rand_n(-1.0f, 1.0f);

	for (int k = 0; k < 2; k++) {
		auto* disp_ptr = k == 0 ? disp_left_ : disp_right_;
		auto* plane_ptr = k == 0 ? plane_left_ : plane_right_;
		sint32 sign = (k == 0) ? 1:-1;
		for (sint32 y = 0; y < height; y++) {
			for (sint32 x = 0; x < width; x++) {
				const sint32 p = y * width + x;
				// 随机视差值
				float32 disp = sign * rand_d(gen);
				if (option.is_integer_disp) {
					disp = static_cast<float32>(round(disp));
				}
				disp_ptr[p] = disp;

				// 随机法向量
				PVector3f norm;
				if (!option.is_fource_fpw) {
					norm.x = rand_n(gen);
					norm.y = rand_n(gen);
					float32 z = rand_n(gen);
					while (z == 0.0f) {
						z = rand_n(gen);
					}
					norm.z = z;
					norm.normalize();
				}
				else {
					norm.x = 0.0f; norm.y = 0.0f; norm.z = 1.0f;
				}

				// 计算视差平面
				plane_ptr[p] = DisparityPlane(x, y, norm, disp);
			}
		}
	}
}

void PatchMatchStereo::ComputeGray() const
{
	const sint32 width = width_;
	const sint32 height = height_;
	if (width <= 0 || height <= 0 ||
		img_left_ == nullptr || img_right_ == nullptr ||
		gray_left_ == nullptr || gray_right_ == nullptr) {
		return;
	}

	// 彩色转灰度
	for (sint32 n = 0; n < 2; n++) {
		auto* color = (n == 0) ? img_left_ : img_right_;
		auto* gray = (n == 0) ? gray_left_ : gray_right_;
		for (sint32 i = 0; i < height; i++) {
			for (sint32 j = 0; j < width; j++) {
				const auto b = color[i * width * 3 + 3 * j];
				const auto g = color[i * width * 3 + 3 * j + 1];
				const auto r = color[i * width * 3 + 3 * j + 2];
				gray[i * width + j] = uint8(r * 0.299 + g * 0.587 + b * 0.114);
			}
		}
	}
}

void PatchMatchStereo::ComputeGradient() const
{
	const sint32 width = width_;
	const sint32 height = height_;
	if (width <= 0 || height <= 0 ||
		grad_left_ == nullptr || grad_right_ == nullptr ||
		gray_left_ == nullptr || gray_right_ == nullptr) {
		return;
	}

	// Sobel梯度算子
	for (sint32 n = 0; n < 2; n++) {
		auto* gray = (n == 0) ? gray_left_ : gray_right_;
		auto* grad = (n == 0) ? grad_left_ : grad_right_;
		for (int y = 1; y < height - 1; y++) {
			for (int x = 1; x < width - 1; x++) {
				const auto grad_x = (-gray[(y - 1) * width + x - 1] + gray[(y - 1) * width + x + 1]) +
					(-2 * gray[y * width + x - 1] + 2 * gray[y * width + x + 1]) +
					(-gray[(y + 1) * width + x - 1] + gray[(y + 1) * width + x + 1]);
				const auto grad_y = (-gray[(y - 1) * width + x - 1] - 2 * gray[(y - 1) * width + x] - gray[(y - 1) * width + x + 1]) +
					(gray[(y + 1) * width + x - 1] + 2 * gray[(y + 1) * width + x] + gray[(y + 1) * width + x + 1]);
				grad[y * width + x].x = grad_x / 8;
				grad[y * width + x].y = grad_y / 8;
			}
		}
	}
}

void PatchMatchStereo::Propagation() const
{
	const sint32 width = width_;
	const sint32 height = height_;
	if (width <= 0 || height <= 0 ||
		img_left_ == nullptr || img_right_ == nullptr ||
		grad_left_ == nullptr || grad_right_ == nullptr ||
		disp_left_ == nullptr || disp_right_ == nullptr ||
		plane_left_ == nullptr || plane_right_ == nullptr) {
		return;
	}

	// 左右视图匹配参数
	const auto opion_left = option_;
	auto option_right = option_;
	option_right.min_disparity = -opion_left.max_disparity;
	option_right.max_disparity = -opion_left.min_disparity;

	// 左右视图传播实例
	PMSPropagation propa_left(width, height, img_left_, img_right_, grad_left_, grad_right_, plane_left_, plane_right_, opion_left,cost_left_,cost_right_, disp_left_);
	PMSPropagation propa_right(width, height, img_right_, img_left_, grad_right_, grad_left_, plane_right_, plane_left_, option_right, cost_right_, cost_left_, disp_right_);

	// 迭代传播
	for (int k = 0; k < option_.num_iters; k++) {
		propa_left.DoPropagation();
		propa_right.DoPropagation();
	}
}

void PatchMatchStereo::LRCheck()
{
	const sint32 width = width_;
	const sint32 height = height_;

	const float32& threshold = option_.lrcheck_thres;

	// k==0 : 左视图一致性检查
	// k==1 : 右视图一致性检查
	for (int k = 0; k < 2; k++) {
		auto* disp_left = (k == 0) ? disp_left_ : disp_right_;
		auto* disp_right = (k == 0) ? disp_right_ : disp_left_;
		auto& mismatches = (k == 0) ? mismatches_left_ : mismatches_right_;
		mismatches.clear();

		// ---左右一致性检查
		for (sint32 y = 0; y < height; y++) {
			for (sint32 x = 0; x < width; x++) {

				// 左影像视差值
				auto& disp = disp_left[y * width + x];

				if (disp == Invalid_Float) {
					mismatches.emplace_back(x, y);
					continue;
				}

				// 根据视差值找到右影像上对应的同名像素
				const auto col_right = lround(x - disp);

				if (col_right >= 0 && col_right < width) {
					// 右影像上同名像素的视差值
					auto& disp_r = disp_right[y * width + col_right];

					// 判断两个视差值是否一致（差值在阈值内为一致）
					// 在本代码里，左右视图的视差值符号相反
					if (abs(disp + disp_r) > threshold) {
						// 让视差值无效
						disp = Invalid_Float;
						mismatches.emplace_back(x, y);
					}
				}
				else {
					// 通过视差值在右影像上找不到同名像素（超出影像范围）
					disp = Invalid_Float;
					mismatches.emplace_back(x, y);
				}
			}
		}
	}
}

void PatchMatchStereo::FillHolesInDispMap()
{
	const sint32 width = width_;
	const sint32 height = height_;
	if (width <= 0 || height <= 0 ||
		disp_left_ == nullptr || disp_right_ == nullptr ||
		plane_left_ == nullptr || plane_right_ == nullptr) {
		return;
	}

	const auto& option = option_;

	// k==0 : 左视图视差填充
	// k==1 : 右视图视差填充
	for (int k = 0; k < 2; k++) {
		auto& mismatches = (k == 0) ? mismatches_left_ : mismatches_right_;
		if(mismatches.empty()) {
			continue;
		}
		const auto* img_ptr = (k == 0) ? img_left_ : img_right_;
		const auto* plane_ptr = (k == 0) ? plane_left_ : plane_right_;
		auto* disp_ptr = (k == 0) ? disp_left_ : disp_right_;
		vector<float32> fill_disps(mismatches.size());		// 存储每个待填充像素的视差
		for (auto n = 0u; n < mismatches.size();n++) {
			auto& pix = mismatches[n];
			const sint32 x = pix.first;
			const sint32 y = pix.second;
			vector<DisparityPlane> planes;

			// 向左向右各搜寻第一个有效像素，记录平面
			sint32 xs = x + 1;
			while (xs < width) {
				if (disp_ptr[y * width + xs] != Invalid_Float) {
					planes.push_back(plane_ptr[y * width + xs]);
					break;
				}
				xs++;
			}
			xs = x - 1;
			while (xs >= 0) {
				if (disp_ptr[y * width + xs] != Invalid_Float) {
					planes.push_back(plane_ptr[y * width + xs]);
					break;
				}
				xs--;
			}

			if(planes.empty()) {
				continue;
			}
			else if (planes.size() == 1u) {
				fill_disps[n] = planes[0].to_disparity(x, y);
			}
			else {
				// 选择较小的视差
				const auto d1 = planes[0].to_disparity(x, y);
				const auto d2 = planes[1].to_disparity(x, y);
				fill_disps[n] = abs(d1) < abs(d2) ? d1 : d2;
			}
		}
		for (auto n = 0u; n < mismatches.size(); n++) {
			auto& pix = mismatches[n];
			const sint32 x = pix.first;
			const sint32 y = pix.second;
			disp_ptr[y * width + x] = fill_disps[n];
		}

		// 加权中值滤波
		pms_util::WeightedMedianFilter(img_ptr, width, height, option.patch_size, option.gamma, mismatches, disp_ptr);
	}
}

void PatchMatchStereo::PlaneToDisparity() const
{
	const sint32 width = width_;
	const sint32 height = height_;
	if (width <= 0 || height <= 0 ||
		disp_left_ == nullptr || disp_right_ == nullptr ||
		plane_left_ == nullptr || plane_right_ == nullptr) {
		return;
	}
	for (int k = 0; k < 2; k++) {
		auto* plane_ptr = (k == 0) ? plane_left_ : plane_right_;
		auto* disp_ptr = (k == 0) ? disp_left_ : disp_right_;
		for (sint32 y = 0; y < height; y++) {
			for (sint32 x = 0; x < width; x++) {
				const sint32 p = y * width + x;
				const auto& plane = plane_ptr[p];
				disp_ptr[p] = plane.to_disparity(x, y);
			}
		}
	}
}
