/* -*-c++-*- PatchMatchStereo - Copyright (C) 2020.
* Author	: Yingsong Li(Ethan Li) <ethan.li.whu@gmail.com>
*			  https://github.com/ethan-li-coding
* Describe	: implement of pms_propagation
*/

#include "stdafx.h"
#include "pms_propagation.h"

PMSPropagation::PMSPropagation(const sint32 width, const sint32 height, const uint8* img_left, const uint8* img_right,
	const PGradient* grad_left, const PGradient* grad_right,
	DisparityPlane* plane_left, DisparityPlane* plane_right,
	const PMSOption& option, 
	float32* cost_left, float32* cost_right,
	float32* disparity_map)
	: cost_cpt_left_(nullptr), cost_cpt_right_(nullptr),
	  width_(width), height_(height), num_iter_(0),
	  img_left_(img_left), img_right_(img_right),
	  grad_left_(grad_left), grad_right_(grad_right),
	  plane_left_(plane_left), plane_right_(plane_right),
	  cost_left_(cost_left), cost_right_(cost_right),
	  disparity_map_(disparity_map)
{
	// 代价计算器
	cost_cpt_left_ = new CostComputerPMS(img_left, img_right, grad_left, grad_right, width, height,
	                                option.patch_size, option.min_disparity, option.max_disparity, option.gamma,
	                                option.alpha, option.tau_col, option.tau_grad);
	cost_cpt_right_ = new CostComputerPMS(img_right, img_left, grad_right, grad_left, width, height,
									option.patch_size, -option.max_disparity, -option.min_disparity, option.gamma,
									option.alpha, option.tau_col, option.tau_grad);
	option_ = option;

	// 随机数生成器
	rand_disp_ = new std::uniform_real_distribution<float32>(-1.0f, 1.0f);
	rand_norm_ = new std::uniform_real_distribution<float32>(-1.0f, 1.0f);

	// 计算初始代价数据
	ComputeCostData();
}

PMSPropagation::~PMSPropagation()
{
	if(cost_cpt_left_) {
		delete cost_cpt_left_;
		cost_cpt_left_ = nullptr;
	}
	if (cost_cpt_right_) {
		delete cost_cpt_right_;
		cost_cpt_right_ = nullptr;
	}
	if (rand_disp_) {
		delete rand_disp_;
		rand_disp_ = nullptr;
	}
	if (rand_norm_) {
		delete rand_norm_;
		rand_norm_ = nullptr;
	}
}

void PMSPropagation::DoPropagation()
{
	if(!cost_cpt_left_|| !cost_cpt_right_ || !img_left_||!img_right_||!grad_left_||!grad_right_ ||!cost_left_||!plane_left_||!plane_right_||!disparity_map_||
		!rand_disp_||!rand_norm_) {
		return;
	}

	// 偶数次迭代从左上到右下传播
	// 奇数次迭代从右下到左上传播
	const sint32 dir = (num_iter_%2==0) ? 1 : -1;
	sint32 y = (dir == 1) ? 0 : height_ - 1;
	for (sint32 i = 0; i < height_; i++) {
		sint32 x = (dir == 1) ? 0 : width_ - 1;
		for (sint32 j = 0; j < width_; j++) {

			// 空间传播
			SpatialPropagation(x, y, dir);

			// 平面优化
			if (!option_.is_fource_fpw) {
				PlaneRefine(x, y);
			}

			// 视图传播
			ViewPropagation(x, y);

			x += dir;
		}
		y += dir;
	}
	++num_iter_;
}

void PMSPropagation::ComputeCostData() const
{
	if (!cost_cpt_left_ || !cost_cpt_right_ || !img_left_ || !img_right_ || !grad_left_ || !grad_right_ || !cost_left_ || !plane_left_ || !plane_right_ || !disparity_map_ ||
		!rand_disp_ || !rand_norm_) {
		return;
	}
	auto* cost_cpt = dynamic_cast<CostComputerPMS*>(cost_cpt_left_);
	for (sint32 y = 0; y < height_; y++) {
		for (sint32 x = 0; x < width_; x++) {
			const auto& plane_p = plane_left_[y * width_ + x];
			cost_left_[y * width_ + x] = cost_cpt->ComputeA(x, y, plane_p);
		}
	}
}

void PMSPropagation::SpatialPropagation(const sint32& x, const sint32& y, const sint32& direction) const
{
	// ---
	// 空间传播

	// 偶数次迭代从左上到右下传播
	// 奇数次迭代从右下到左上传播
	const sint32 dir = direction;

	// 获取p当前的视差平面并计算代价
	auto& plane_p = plane_left_[y * width_ + x];
	auto& cost_p = cost_left_[y * width_ + x];
	auto* cost_cpt = dynamic_cast<CostComputerPMS*>(cost_cpt_left_);

	// 获取p左(右)侧像素的视差平面，计算将平面分配给p时的代价，取较小值
	const sint32 xd = x - dir;
	if (xd >= 0 && xd < width_) {
		auto& plane = plane_left_[y * width_ + xd];
		if (plane != plane_p) {
			const auto cost = cost_cpt->ComputeA(x, y, plane);
			if (cost < cost_p) {
				plane_p = plane;
				cost_p = cost;
			}
		}
	}

	// 获取p上(下)侧像素的视差平面，计算将平面分配给p时的代价，取较小值
	const sint32 yd = y - dir;
	if (yd >= 0 && yd < height_) {
		auto& plane = plane_left_[yd * width_ + x];
		if (plane != plane_p) {
			const auto cost = cost_cpt->ComputeA(x, y, plane);
			if (cost < cost_p) {
				plane_p = plane;
				cost_p = cost;
			}
		}
	}
}

void PMSPropagation::ViewPropagation(const sint32& x, const sint32& y) const
{
	// --
	// 视图传播
	// 搜索p在右视图的同名点q，更新q的平面

	// 左视图匹配点p的位置及其视差平面 
	const sint32 p = y * width_ + x;
	const auto& plane_p = plane_left_[p];
	auto* cost_cpt = dynamic_cast<CostComputerPMS*>(cost_cpt_right_);

	const float32 d_p = plane_p.to_disparity(x, y);

	// 计算右视图列号
	const sint32 xr = lround(x - d_p);
	if (xr < 0 || xr >= width_) {
		return;
	}

	const sint32 q = y * width_ + xr;
	auto& plane_q = plane_right_[q];
	auto& cost_q = cost_right_[q];

	// 将左视图的视差平面转换到右视图
	const auto plane_p2q = plane_p.to_another_view(x, y);
	const float32 d_q = plane_p2q.to_disparity(xr,y);
	const auto cost = cost_cpt->ComputeA(xr, y, plane_p2q);
	if (cost < cost_q) {
		plane_q = plane_p2q;
		cost_q = cost;
	}
}

void PMSPropagation::PlaneRefine(const sint32& x, const sint32& y) const
{
	// --
	// 平面优化
	const auto max_disp = static_cast<float32>(option_.max_disparity);
	const auto min_disp = static_cast<float32>(option_.min_disparity);

	// 随机数生成器
	std::random_device rd;
	std::mt19937 gen(rd());
	const auto& rand_d = *rand_disp_;
	const auto& rand_n= *rand_norm_;

	// 像素p的平面、代价、视差、法线
	auto& plane_p = plane_left_[y * width_ + x];
	auto& cost_p = cost_left_[y * width_ + x];
	auto* cost_cpt = dynamic_cast<CostComputerPMS*>(cost_cpt_left_);

	float32 d_p = plane_p.to_disparity(x, y);
	PVector3f norm_p = plane_p.to_normal();

	float32 disp_update = (max_disp - min_disp) / 2.0f;
	float32 norm_update = 1.0f;
	const float32 stop_thres = 0.1f;

	// 迭代优化
	while (disp_update > stop_thres) {

		// 在 -disp_update ~ disp_update 范围内随机一个视差增量
		float32 disp_rd = rand_d(gen) * disp_update;
		if (option_.is_integer_disp) {
			disp_rd = static_cast<float32>(round(disp_rd));
		}

		// 计算像素p新的视差
		const float32 d_p_new = d_p + disp_rd;
		if (d_p_new < min_disp || d_p_new > max_disp) {
			disp_update /= 2;
			norm_update /= 2;
			continue;
		}

		// 在 -norm_update ~ norm_update 范围内随机三个值作为法线增量的三个分量
		PVector3f norm_rd;
		if (!option_.is_fource_fpw) {
			norm_rd.x = rand_n(gen) * norm_update;
			norm_rd.y = rand_n(gen) * norm_update;
			float32 z = rand_n(gen) * norm_update;
			while (z == 0.0f) {
				z = rand_n(gen) * norm_update;
			}
			norm_rd.z = z;
		}
		else {
			norm_rd.x = 0.0f; norm_rd.y = 0.0f;	norm_rd.z = 0.0f;
		}

		// 计算像素p新的法线
		auto norm_p_new = norm_p + norm_rd;
		norm_p_new.normalize();

		// 计算新的视差平面
		auto plane_new = DisparityPlane(x, y, norm_p_new, d_p_new);

		// 比较Cost
		if (plane_new != plane_p) {
			const float32 cost = cost_cpt->ComputeA(x, y, plane_new);

			if (cost < cost_p) {
				plane_p = plane_new;
				cost_p = cost;
				d_p = d_p_new;
				norm_p = norm_p_new;
			}
		}

		disp_update /= 2.0f;
		norm_update /= 2.0f;
	}
}
