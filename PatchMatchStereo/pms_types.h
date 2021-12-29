/* -*-c++-*- PatchMatchStereo - Copyright (C) 2020.
* Author	: Yingsong Li(Ethan Li) <ethan.li.whu@gmail.com>
*			  https://github.com/ethan-li-coding
* Describe	: header of pms_types
*/

#ifndef PATCH_MATCH_STEREO_TYPES_H_
#define PATCH_MATCH_STEREO_TYPES_H_

#include <cstdint>
#include <limits>
#include <vector>
using std::vector;
using std::pair;

#ifndef SAFE_DELETE
#define SAFE_DELETE(P) {if(P) delete[](P);(P)=nullptr;}
#endif

/** \brief 基础类型别名 */
typedef int8_t			sint8;		// 有符号8位整数
typedef uint8_t			uint8;		// 无符号8位整数
typedef int16_t			sint16;		// 有符号16位整数
typedef uint16_t		uint16;		// 无符号16位整数
typedef int32_t			sint32;		// 有符号32位整数
typedef uint32_t		uint32;		// 无符号32位整数
typedef int64_t			sint64;		// 有符号64位整数
typedef uint64_t		uint64;		// 无符号64位整数
typedef float			float32;	// 单精度浮点
typedef double			float64;	// 双精度浮点

/** \brief float32无效值 */
constexpr auto Invalid_Float = std::numeric_limits<float32>::infinity();

/** \brief PMS参数结构体 */
struct PMSOption {
	sint32	patch_size;			// patch尺寸，局部窗口为 patch_size*patch_size
	sint32  min_disparity;		// 最小视差
	sint32	max_disparity;		// 最大视差

	float32	gamma;				// gamma 权值因子
	float32	alpha;				// alpha 相似度平衡因子
	float32	tau_col;			// tau for color	相似度计算颜色空间的绝对差的下截断阈值
	float32	tau_grad;			// tau for gradient 相似度计算梯度空间的绝对差下截断阈值

	sint32	num_iters;			// 传播迭代次数

	bool	is_check_lr;		// 是否检查左右一致性
	float32	lrcheck_thres;		// 左右一致性约束阈值

	bool	is_fill_holes;		// 是否填充视差空洞

	bool	is_fource_fpw;		// 是否强制为Frontal-Parallel Window
	bool	is_integer_disp;	// 是否为整像素视差
	
	PMSOption() : patch_size(35), min_disparity(0), max_disparity(64), gamma(10.0f), alpha(0.9f), tau_col(10.0f),
	              tau_grad(2.0f), num_iters(3),
	              is_check_lr(false),
	              lrcheck_thres(0),
	              is_fill_holes(false), is_fource_fpw(false), is_integer_disp(false) { }
};

/**
 * \brief 颜色结构体
 */
struct PColor {
	uint8 r, g, b;
	PColor() : r(0), g(0), b(0) {}
	PColor(uint8 _b, uint8 _g, uint8 _r) {
		r = _r; g = _g; b = _b;
	}
};
/**
 * \brief 梯度结构体
 */
struct PGradient {
	sint16 x, y;
	PGradient() : x(0), y(0) {}
	PGradient(sint16 _x, sint16 _y) {
		x = _x; y = _y;
	}
};

/**
* \brief 二维矢量结构体
*/
struct PVector2f {

	float32 x = 0.0f, y = 0.0f;

	PVector2f() = default;
	PVector2f(const float32& _x, const float32& _y) {
		x = _x; y = _y;
	}
	PVector2f(const sint16& _x, const sint16& _y) {
		x = float32(_x); y = float32(_y);
	}
	PVector2f(const PVector2f& v) {
		x = v.x; y = v.y;
	}

	// ···operators
	// operator +
	PVector2f operator+(const PVector2f& v) const {
		return PVector2f(x + v.x, y + v.y);
	}
	// operator -
	PVector2f operator-(const PVector2f& v) const {
		return PVector2f(x - v.x, y - v.y);
	}
	// operator -t
	PVector2f operator-() const {
		return PVector2f(-x, -y);
	}
	// operator =
	PVector2f& operator=(const PVector2f& v) {
		if (this == &v) {
			return *this;
		}
		else {
			x = v.x; y = v.y;
			return *this;
		}
	}
};

/**
* \brief 三维矢量结构体
*/
struct PVector3f {

	float32 x = 0.0f, y = 0.0f, z = 0.0f;

	PVector3f() = default;
	PVector3f(const float32& _x, const float32& _y, const float32& _z) {
		x = _x; y = _y; z = _z;
	}
	PVector3f(const uint8& _x, const uint8& _y, const uint8& _z) {
		x = float32(_x); y = float32(_y); z = float32(_z);
	}
	PVector3f(const PVector3f& v) {
		x = v.x; y = v.y; z = v.z;
	}

	// normalize
	void normalize() {
		if (x == 0.0f && y == 0.0f && z == 0.0f) {
			return;
		}
		else {
			const float32 sq = x * x + y * y + z * z;
			const float32 sqf = sqrt(sq);
			x /= sqf; y /= sqf; z /= sqf;
		}
	}

	// ···operators
	// operator +
	PVector3f operator+(const PVector3f& v) const {
		return PVector3f(x + v.x, y + v.y, z + v.z);
	}
	// operator -
	PVector3f operator-(const PVector3f& v) const {
		return PVector3f(x - v.x, y - v.y, z - v.z);
	}
	// operator -t
	PVector3f operator-() const {
		return PVector3f(-x, -y, -z);
	}
	// operator =
	PVector3f& operator=(const PVector3f& v) {
		if (this == &v) {
			return *this;
		}
		else {
			x = v.x;
			y = v.y;
			z = v.z;
			return *this;
		}
	}
	// operator ==
	bool operator==(const PVector3f& v) const {
		return (x == v.x) && (y == v.y) && (z == v.z);
	}
	// operator !=
	bool operator!=(const PVector3f& v) const {
		return (x != v.x) || (y != v.y) || (z != v.z);
	}

	// dot
	float32 dot(const PVector3f& v) const {
		return x * v.x + y * v.y + z * v.z;
	}
};

typedef  PVector3f PPoint3f;


/**
 * \brief 视差平面
 */
struct DisparityPlane {
	PVector3f p;
	DisparityPlane() = default;
	DisparityPlane(const float32& x,const float32& y,const float32& z) {
		p.x = x; p.y = y; p.z = z;
	}
	DisparityPlane(const sint32& x, const sint32& y, const PVector3f& n, const float32& d) {
		p.x = -n.x / n.z;
		p.y = -n.y / n.z;
		p.z = (n.x * x + n.y * y + n.z * d) / n.z;
	}

	/**
	 * \brief 获取该平面下像素(x,y)的视差
	 * \param x		像素x坐标
	 * \param y		像素y坐标
	 * \return 像素(x,y)的视差
	 */
	float32 to_disparity(const sint32& x,const sint32& y) const
	{
		return p.dot(PVector3f(float32(x), float32(y), 1.0f));
	}

	/** \brief 获取平面的法线 */
	PVector3f to_normal() const
	{
		PVector3f n(p.x, p.y, -1.0f);
		n.normalize();
		return n;
	}

	/**
	 * \brief 将视差平面转换到另一视图
	 * 假设左视图平面方程为 d = a_p*xl + b_p*yl + c_p
	 * 左右视图满足：(1) xr = xl - d_p; (2) yr = yl; (3) 视差符号相反(本代码左视差为正值，右视差为负值)
	 * 代入左视图视差平面方程就可得到右视图坐标系下的平面方程: d = a_p/(a_p-1)*xr + b_p/(a_p-1)*yr + c_p/(a_p-1)
	 * 右至左同理
	 * \param x		像素x坐标
	 * \param y 	像素y坐标
	 * \return 转换后的平面
	 */
	DisparityPlane to_another_view(const sint32& x, const sint32& y) const
	{
		float denom = 1 / (p.x - 1.f);
		return { p.x * denom, p.y * denom, p.z * denom };
	}

	// operator ==
	bool operator==(const DisparityPlane& v) const {
		return p == v.p;
	}
	// operator !=
	bool operator!=(const DisparityPlane& v) const {
		return p != v.p;
	}
};

#endif
