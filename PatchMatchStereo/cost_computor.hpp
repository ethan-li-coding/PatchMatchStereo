/* -*-c++-*- PatchMatchStereo - Copyright (C) 2020.
* Author	: Yingsong Li(Ethan Li) <ethan.li.whu@gmail.com>
*			  https://github.com/ethan-li-coding
* Describe	: implement of cost computer
*/

#ifndef PATCH_MATCH_STEREO_COST_HPP_
#define PATCH_MATCH_STEREO_COST_HPP_
#include "pms_types.h"
#include <algorithm>

#define COST_PUNISH 120.0f  // NOLINT(cppcoreguidelines-macro-usage)

#define USE_FAST_EXP
/* 快速exp*/
inline double fast_exp(double x) {
	x = 1.0 + x / 1024;
	x *= x; x *= x; x *= x; x *= x;
	x *= x; x *= x; x *= x; x *= x;
	x *= x; x *= x;
	return x;
}

/**
 * \brief 代价计算器基类
 */
class CostComputer {
public:
	/** \brief 代价计算器默认构造 */
	CostComputer(): img_left_(nullptr), img_right_(nullptr), width_(0), height_(0), patch_size_(0), min_disp_(0),
	                max_disp_(0) {}

	/**
	 * \brief 代价计算器初始化
	 * \param img_left		左影像数据 
	 * \param img_right		右影像数据
	 * \param width			影像宽
	 * \param height		影像高
	 * \param patch_size	局部Patch大小
	 * \param min_disp		最小视差
	 * \param max_disp		最大视差
	 */
	CostComputer(const uint8* img_left, const uint8* img_right, const sint32& width,const sint32& height,const sint32& patch_size,const sint32& min_disp, const sint32& max_disp){
		img_left_ = img_left;
		img_right_ = img_right;
		width_ = width;
		height_ = height;
		patch_size_ = patch_size;
		min_disp_ = min_disp;
		max_disp_ = max_disp;
	}

	/** \brief 代价计算器析构 */
	virtual ~CostComputer() = default;

public:

	/**
	 * \brief 计算左影像p点视差为d时的代价值
	 * \param i		p点纵坐标
	 * \param j		p点横坐标
	 * \param d		视差值
	 * \return 代价值
	 */
	virtual float32 Compute(const sint32& i, const sint32& j, const float32& d) = 0;

public:
	/** \brief 左影像数据 */
	const uint8* img_left_;
	/** \brief 右影像数据 */
	const uint8* img_right_;

	/** \brief 影像宽 */
	sint32 width_;
	/** \brief 影像高 */
	sint32 height_;
	/** \brief 局部窗口Patch大小 */
	sint32 patch_size_;

	/** \brief 最小最大视差 */
	sint32 min_disp_;
	sint32 max_disp_;
};


/**
 * \brief 代价计算器：PatchMatchStero原文代价计算器
 */
class CostComputerPMS : public CostComputer {
public:

	/** \brief PMS代价计算器默认构造 */
	CostComputerPMS(): grad_left_(nullptr), grad_right_(nullptr), gamma_(0), alpha_(0), tau_col_(0), tau_grad_(0) {} ;

	/**
	 * \brief PMS代价计算器带参构造
	 * \param img_left		左影像数据
	 * \param img_right		右影像数据
	 * \param grad_left		左梯度数据
	 * \param grad_right	右梯度数据
	 * \param width			影像宽
	 * \param height		影像高
	 * \param patch_size	局部Patch大小
	 * \param min_disp		最小视差
	 * \param max_disp		最大视差
	 * \param gamma			参数gamma值
	 * \param alpha			参数alpha值
	 * \param t_col			参数tau_col值
	 * \param t_grad		参数tau_grad值
	 */
	CostComputerPMS(const uint8* img_left, const uint8* img_right, const PGradient* grad_left, const PGradient* grad_right, const sint32& width, const sint32& height, const sint32& patch_size,
		const sint32& min_disp, const sint32& max_disp,
		const float32& gamma, const float32& alpha, const float32& t_col, const float32 t_grad) :
		CostComputer(img_left, img_right, width, height, patch_size, min_disp, max_disp) {
		grad_left_ = grad_left;
		grad_right_ = grad_right;
		gamma_ = gamma;
		alpha_ = alpha;
		tau_col_ = t_col;
		tau_grad_ = t_grad;
	}

	/**
	 * \brief 计算左影像p点视差为d时的代价值
	 * \param x		p点x坐标
	 * \param y		p点y坐标
	 * \param d		视差值
	 * \return 代价值
	 */
	inline float32 Compute(const sint32& x, const sint32& y, const float32& d) override
	{
		const float32 xr = x - d;
		if (xr < 0.0f || xr >= static_cast<float32>(width_)) {
			return (1 - alpha_) * tau_col_ + alpha_ * tau_grad_;
		}
		// 颜色空间距离
		const auto col_p = GetColor(img_left_, x, y);
		const auto col_q = GetColor(img_right_, xr, y);
		const auto dc = std::min(abs(col_p.b - col_q.x) + abs(col_p.g - col_q.y) + abs(col_p.r - col_q.z), tau_col_);

		// 梯度空间距离
		const auto grad_p = GetGradient(grad_left_, x, y);
		const auto grad_q = GetGradient(grad_right_, xr, y);
		const auto dg = std::min(abs(grad_p.x - grad_q.x)+ abs(grad_p.y - grad_q.y), tau_grad_);

		// 代价值
		return (1 - alpha_) * dc + alpha_ * dg;
	}

	/**
	 * \brief 计算左影像p点视差为d时的代价值
	 * \param col_p		p点颜色值
	 * \param grad_p	p点梯度值
	 * \param x			p点x坐标
	 * \param y			p点y坐标
	 * \param d			视差值
	 * \return 代价值
	 */
	inline float32 Compute(const PColor& col_p,const PGradient& grad_p, const sint32& x, const sint32& y, const float32& d) const
	{
		const float32 xr = x - d;
		if (xr < 0.0f || xr >= static_cast<float32>(width_)) {
			return (1 - alpha_) * tau_col_ + alpha_ * tau_grad_;
		}
		// 颜色空间距离
		const auto col_q = GetColor(img_right_, xr, y);
		const auto dc = std::min(abs(col_p.b - col_q.x) + abs(col_p.g - col_q.y) + abs(col_p.r - col_q.z), tau_col_);

		// 梯度空间距离
		const auto grad_q = GetGradient(grad_right_, xr, y);
		const auto dg = std::min(abs(grad_p.x - grad_q.x) + abs(grad_p.y - grad_q.y), tau_grad_);

		// 代价值
		return (1 - alpha_) * dc + alpha_ * dg;
	}


	/**
	 * \brief 计算左影像p点视差平面为p时的聚合代价值
	 * \param x		p点x坐标
	 * \param y 	p点y坐标
	 * \param p		平面参数
	 * \return 聚合代价值
	 */
	inline float32 ComputeA(const sint32& x, const sint32& y, const DisparityPlane& p) const
	{
		const auto pat = patch_size_ / 2;
		const auto& col_p = GetColor(img_left_, x, y);
		float32 cost = 0.0f;
		for (sint32 r = -pat; r <= pat; r++) {
			const sint32 yr = y + r;
			for (sint32 c = -pat; c <= pat; c++) {
				const sint32 xc = x + c;
				if (yr < 0 || yr > height_ - 1 || xc < 0 || xc > width_ - 1) {
					continue;
				}
				// 计算视差值
				const float32 d = p.to_disparity(xc,yr);
				if (d < min_disp_ || d > max_disp_) {
					cost += COST_PUNISH;
					continue;
				}

				// 计算权值
				const auto& col_q = GetColor(img_left_, xc, yr);
				const auto dc = abs(col_p.r - col_q.r) + abs(col_p.g - col_q.g) + abs(col_p.b - col_q.b);
#ifdef USE_FAST_EXP
				const auto w = fast_exp(double(-dc / gamma_));
#else
				const auto w = exp(-dc / gamma_);
#endif

				// 聚合代价
				const auto grad_q = GetGradient(grad_left_, xc, yr);
				cost += w * Compute(col_q, grad_q, xc, yr, d);
			}
		}
		return cost;
	}

	/**
	* \brief 获取像素点的颜色值
	* \param img_data	颜色数组,3通道
	* \param x			像素x坐标
	* \param y			像素y坐标
	* \return 像素(x,y)的颜色值
	*/
	inline PColor GetColor(const uint8* img_data, const sint32& x, const sint32& y) const
	{
		auto* pixel = img_data + y * width_ * 3 + 3 * x;
		return { pixel[0], pixel[1], pixel[2] };
	}

	/**
	* \brief 获取像素点的颜色值
	* \param img_data	颜色数组
	* \param x			像素x坐标，实数，线性内插得到颜色值
	* \param y			像素y坐标
	* \return 像素(x,y)的颜色值
	*/
	inline PVector3f GetColor(const uint8* img_data, const float32& x,const sint32& y) const
	{
		float32 col[3];
		const auto x1 = static_cast<sint32>(x);
		const sint32 x2 = x1 + 1;
		const float32 ofs = x - x1;

		for (sint32 n = 0; n < 3; n++) {
			const auto& g1 = img_data[y * width_ * 3 + 3 * x1 + n];
			const auto& g2 = (x2 < width_) ? img_data[y * width_ * 3 + 3 * x2 + n] : g1;
			col[n] = (1 - ofs) * g1 + ofs * g2;
		}

		return { col[0], col[1], col[2] };
	}

	/**
	* \brief 获取像素点的梯度值
	* \param grad_data	梯度数组
	* \param x			像素x坐标
	* \param y			像素y坐标
	* \return 像素(x,y)的梯度值
	*/
	inline PGradient GetGradient(const PGradient* grad_data, const sint32& x, const sint32& y) const
	{
		return grad_data[y * width_ + x];
	}

	/**
	* \brief 获取像素点的梯度值
	* \param grad_data	梯度数组
	* \param x			像素x坐标，实数，线性内插得到梯度值
	* \param y			像素y坐标
	* \return 像素(x,y)的梯度值
	*/
	inline PVector2f GetGradient(const PGradient* grad_data, const float32& x, const sint32& y) const
	{
		const auto x1 = static_cast<sint32>(x);
		const sint32 x2 = x1 + 1;
		const float32 ofs = x - x1;

		const auto& g1 = grad_data[y * width_ + x1];
		const auto& g2 = (x2 < width_) ? grad_data[y * width_ + x2] : g1;

		return { (1 - ofs) * g1.x + ofs * g2.x,(1 - ofs) * g1.y + ofs * g2.y };
	}

private:
	/** \brief 左影像梯度数据 */
	const PGradient* grad_left_;
	/** \brief 右影像梯度数据 */
	const PGradient* grad_right_;

	/** \brief 参数gamma */
	float gamma_;
	/** \brief 参数alpha */
	float32 alpha_;
	/** \brief 参数tau_col */
	float32 tau_col_;
	/** \brief 参数tau_grad */
	float32 tau_grad_;
};

// ↓↓↓在此通过派生类的方式实现你想实现的代价计算器↓↓↓

#endif
