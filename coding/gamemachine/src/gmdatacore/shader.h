﻿#ifndef __SHADER_H__
#define __SHADER_H__
#include "common.h"
#include "foundation/linearmath.h"
BEGIN_NS

// 表示一套纹理，包括普通纹理、漫反射纹理、法线贴图、光照贴图，以后可能还有高光贴图等
enum TextureIndex
{
	TEXTURE_INDEX_AMBIENT,
	TEXTURE_INDEX_AMBIENT_2,
	TEXTURE_INDEX_AMBIENT_3,
	TEXTURE_INDEX_DIFFUSE,
	TEXTURE_INDEX_NORMAL_MAPPING,
	TEXTURE_INDEX_LIGHTMAP,
	TEXTURE_INDEX_MAX,
};

enum
{
	MAX_ANIMATION_FRAME = 16,
	MAX_TEX_MOD = 3,
};

enum GMS_BlendFunc
{
	GMS_ZERO = 0,
	GMS_ONE,
	GMS_DST_COLOR,
	GMS_SRC_ALPHA,
	GMS_ONE_MINUS_SRC_ALPHA,
};

enum class GMS_Cull
{
	CULL = 0,
	NONE,
};

enum GMS_TextureFilter
{
	GMS_LINEAR = 0,
	GMS_NEAREST,
	GMS_LINEAR_MIPMAP_LINEAR,
	GMS_NEAREST_MIPMAP_LINEAR,
	GMS_LINEAR_MIPMAP_NEAREST,
	GMS_NEAREST_MIPMAP_NEAREST,
};

enum GMS_Wrap
{
	GMS_REPEAT = 0,
	GMS_CLAMP_TO_EDGE,
	GMS_CLAMP_TO_BORDER,
	GMS_MIRRORED_REPEAT,
};

enum GMS_TextureModType
{
	GMS_NO_TEXTURE_MOD = 0,
	GMS_SCROLL,
	GMS_SCALE,
};

GM_ALIGNED_STRUCT(GMS_TextureMod)
{
	GMS_TextureModType type = GMS_NO_TEXTURE_MOD;
	GMfloat p1 = 0;
	GMfloat p2 = 0;
};

// 表示一系列动画帧
GM_ALIGNED_STRUCT(TextureFrames)
{
	TextureFrames()
	{
		memset(frames, 0, MAX_ANIMATION_FRAME * sizeof(ITexture*));
	}

	ITexture* frames[MAX_ANIMATION_FRAME];
	GMS_TextureMod texMod[MAX_TEX_MOD];
	GMint frameCount = 0;
	GMint animationMs = 1; //每一帧动画间隔 (ms)
	GMS_TextureFilter magFilter = GMS_LINEAR;
	GMS_TextureFilter minFilter = GMS_LINEAR_MIPMAP_LINEAR;
	GMS_Wrap wrapS = GMS_REPEAT;
	GMS_Wrap wrapT = GMS_REPEAT;
};

GM_ALIGNED_STRUCT(TextureInfo)
{
	// 每个texture由TEXTURE_INDEX_MAX个纹理动画组成。静态纹理的纹理动画数量为1
	TextureFrames textures[TEXTURE_INDEX_MAX];
	GMuint autorelease = 0;
};

#define DECLARE_GETTER(name, memberName, paramType) \
	inline paramType& get##name() { D(d); return d-> memberName; } \
	inline const paramType& get##name() const { D(d); return d-> memberName; }

#define DECLARE_SETTER(name, memberName, paramType) \
	inline void set##name(const paramType & arg) { D(d); d-> memberName = arg; }

#define DECLARE_PROPERTY(name, memberName, paramType) \
	DECLARE_GETTER(name, memberName, paramType) \
	DECLARE_SETTER(name, memberName, paramType)

GM_PRIVATE_OBJECT(GMLight)
{
	GMfloat shininess = 0;
	bool enabled = false;
	bool useGlobalLightColor = false; // true表示使用全局的光的颜色
	linear_math::Vector3 lightPosition;
	linear_math::Vector3 lightColor;
	linear_math::Vector3 ka;
	linear_math::Vector3 ks;
	linear_math::Vector3 kd;
};

class GMLight : public GMObject
{
	DECLARE_PRIVATE(GMLight)

public:
	DECLARE_PROPERTY(LightPosition, lightPosition, linear_math::Vector3);
	DECLARE_PROPERTY(LightColor, lightColor, linear_math::Vector3);
	DECLARE_PROPERTY(Ka, ka, linear_math::Vector3);
	DECLARE_PROPERTY(Ks, ks, linear_math::Vector3);
	DECLARE_PROPERTY(Kd, kd, linear_math::Vector3);
	DECLARE_PROPERTY(Shininess, shininess, GMfloat);
	DECLARE_PROPERTY(Enabled, enabled, bool);
	DECLARE_PROPERTY(UseGlobalLightColor, useGlobalLightColor, bool);
	
	inline GMLight& operator=(const GMLight& rhs)
	{
		D(d);
		*d = *rhs.data();
		return *this;
	}
};

enum LightType
{
	LT_BEGIN = 0,
	LT_AMBIENT = 0,
	LT_SPECULAR,
	LT_END,
};

GM_PRIVATE_OBJECT(Shader)
{
	GMuint surfaceFlag = 0;
	GMS_Cull cull = GMS_Cull::CULL;
	GMS_BlendFunc blendFactorSrc = GMS_ZERO;
	GMS_BlendFunc blendFactorDest = GMS_ZERO;
	GMLight lights[LT_END];
	bool blend = false;
	bool nodraw = false;
	bool noDepthTest = false;
	TextureInfo texture;
};

class Shader : public GMObject
{
	DECLARE_PRIVATE(Shader)

public:
	DECLARE_PROPERTY(SurfaceFlag, surfaceFlag, GMuint);
	DECLARE_PROPERTY(Cull, cull, GMS_Cull);
	DECLARE_PROPERTY(BlendFactorSource, blendFactorSrc, GMS_BlendFunc);
	DECLARE_PROPERTY(BlendFactorDest, blendFactorDest, GMS_BlendFunc);
	DECLARE_PROPERTY(Blend, blend, bool);
	DECLARE_PROPERTY(Nodraw, nodraw, bool);
	DECLARE_PROPERTY(NoDepthTest, noDepthTest, bool);
	DECLARE_PROPERTY(Texture, texture, TextureInfo);
	
	inline GMLight& getLight(LightType type) { D(d); return d->lights[type]; }
	inline void setLight(LightType type, const GMLight& light) { D(d); d->lights[type] = light; }

	inline Shader& operator=(const Shader& rhs)
	{
		D(d);
		*d = *rhs.data();
		for (GMint i = LT_BEGIN; i < LT_END; i++)
		{
			d->lights[i] = rhs.data()->lights[i];
		}
		return *this;
	}
};
END_NS
#endif