/* -*-c++-*- PatchMatchStereo - Copyright (C) 2020.
* Author	: Yingsong Li(Ethan Li) <ethan.li.whu@gmail.com>
*			  https://github.com/ethan-li-coding
* Describe	: header of pms_propagation
*/

#ifndef PATCH_MATCH_STEREO_PROPAGATION_H_
#define PATCH_MATCH_STEREO_PROPAGATION_H_
#include "pms_types.h"

#include "cost_computor.hpp"
#include <random>

/**
 * \brief 传播类
 */
class PMSPropagation final{
public:
	PMSPropagation(const sint32 width, const sint32 height,
		const uint8* img_left, const uint8* img_right,
		const PGradient* grad_left, const PGradient* grad_right,
		DisparityPlane* plane_left, DisparityPlane* plane_right,
		const PMSOption& option,
		float32* cost_left, float32* cost_right,
		float32* disparity_map);

	~PMSPropagation();

public:
	/** \brief 执行传播一次 */
	void DoPropagation();

private:
	/** \brief 计算代价数据 */
	void ComputeCostData() const;

	/**
	 * \brief 空间传播
	 * \param x 像素x坐标
	 * \param y 像素y坐标
	 * \param direction 传播方向
	 */
	void SpatialPropagation(const sint32& x, const sint32& y, const sint32& direction) const;
	
	/**
	 * \brief 视图传播
	 * \param x 像素x坐标
	 * \param y 像素y坐标
	 */
	void ViewPropagation(const sint32& x, const sint32& y) const;
	
	/**
	 * \brief 平面优化
	 * \param x 像素x坐标
	 * \param y 像素y坐标
	 */
	void PlaneRefine(const sint32& x, const sint32& y) const;
private:
	/** \brief 代价计算器 */
	CostComputer* cost_cpt_left_;
	CostComputer* cost_cpt_right_;

	/** \brief PMS算法参数*/
	PMSOption option_;

	/** \brief 影像宽高 */
	sint32 width_;  
	sint32 height_;

	/** \brief 传播迭代次数 */
	sint32 num_iter_;

	/** \brief 影像数据 */
	const uint8* img_left_;
	const uint8* img_right_;

	/** \brief 梯度数据 */
	const PGradient* grad_left_;
	const PGradient* grad_right_;

	/** \brief 平面数据 */
	DisparityPlane* plane_left_;
	DisparityPlane* plane_right_;

	/** \brief 代价数据	 */
	float32* cost_left_;
	float32* cost_right_;

	/** \brief 视差数据 */
	float32* disparity_map_;

	/** \brief 随机数生成器 */
	std::uniform_real_distribution<float32>* rand_disp_;
	std::uniform_real_distribution<float32>* rand_norm_;
};

#endif
