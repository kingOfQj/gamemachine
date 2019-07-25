﻿#ifndef __GMDX11_MAIN_H__
#define __GMDX11_MAIN_H__
#include <gmcommon.h>

BEGIN_NS
class GMWaveGameObject;
END_NS

extern "C"
{
	GM_DECL_EXPORT gm::IFactory* gmdx11_createDirectX11Factory();
	GM_DECL_EXPORT void gmdx11_loadShader(const gm::IRenderContext*, const gm::GMString& fileName);
	GM_DECL_EXPORT void gmdx11_loadExtensionShaders(const gm::IRenderContext*);
	GM_DECL_EXPORT void gmdx11_ext_renderWaveObjectShader(gm::GMWaveGameObject* waveObject, gm::IShaderProgram* shaderProgram);
}

#endif