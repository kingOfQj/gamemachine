﻿#include "stdafx.h"
#include "gmgl_renders_particle.h"
#include "gmgl/shader_constants.h"
#include "gmgl/gmglshaderprogram.h"

void GMGLRenders_Particle::beginShader(Shader& shader, GMDrawMode mode)
{
	D(d);
	d->shader = &shader;

	// 应用Shader
	activateShader();

	// 只选择环境光纹理
	GMTextureFrames& textures = shader.getTexture().getTextureFrames(GMTextureType::AMBIENT, 0);

	// 获取序列中的这一帧
	ITexture* texture = getTexture(textures);
	if (texture)
	{
		// 激活动画序列
		activateTexture(GMTextureType::AMBIENT, 0);
		texture->drawTexture(&textures);
	}
}