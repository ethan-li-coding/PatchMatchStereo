/* -*-c++-*- SemiGlobalMatching - Copyright (C) 2020.
* Author	: Ethan Li <ethan.li.whu@gmail.com>
*			  https://github.com/ethan-li-coding
* Describe	: implement of pms_util
*/

#pragma once
#include "pms_types.h"

namespace pms_util
{

	/**
	* \brief 获取像素(i,j)的颜色值
	* \param img_data	颜色数组
	* \param width		影像宽
	* \param height		影像高
	* \param i			像素行坐标
	* \param j			像素列坐标
	* \return 像素(i,j)的颜色值
	*/
	PColor GetColor(const uint8* img_data, const sint32& width, const sint32& height, const sint32& i,const sint32& j);


	/**
	 * \brief 对视差图进行加权中值滤波
	 * \param img_data		颜色数组
	 * \param width			影像宽
	 * \param height		影像高
	 * \param wnd_size		窗口大小
	 * \param gamma			gamma值
	 * \param filter_pixels 需要滤波的像素集
	 * \param disparity_map 视差图
	 */
	void WeightedMedianFilter(const uint8* img_data, const sint32& width, const sint32& height, const sint32& wnd_size, const float32& gamma,const vector<pair<int, int>>& filter_pixels, float32* disparity_map);
}