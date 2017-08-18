﻿#include "stdafx.h"
#include "gmglgbuffer.h"
#include "gmglgraphic_engine.h"

#define TEXTURE_NUM (GMuint) GBufferTextureType::EndOfTextureType

GMGLGBuffer::GMGLGBuffer()
{
}

GMGLGBuffer::~GMGLGBuffer()
{
	dispose();
}

void GMGLGBuffer::dispose()
{
	D(d);
	if (d->fbo)
	{
		glDeleteFramebuffers(1, &d->fbo);
		d->fbo = 0;
	}

	if (d->textures[0] != 0)
	{
		glDeleteTextures(TEXTURE_NUM, d->textures);
		d->textures[0] = 0;
	}

	if (d->depthTexture != 0)
	{
		glDeleteTextures(1, &d->depthTexture);
		d->depthTexture = 0;
	}
}

bool GMGLGBuffer::init(GMuint windowWidth, GMuint windowHeight)
{
	D(d);
	GLenum errCode;

	d->windowWidth = windowWidth;
	d->windowHeight = windowHeight;

	// Create the FBO
	glGenFramebuffers(1, &d->fbo);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, d->fbo);

	// Create the gbuffer textures
	glGenTextures(TEXTURE_NUM, d->textures);
	glGenTextures(1, &d->depthTexture);

	for (GMuint i = 0; i < TEXTURE_NUM; i++)
	{
		glBindTexture(GL_TEXTURE_2D, d->textures[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, windowWidth, windowHeight, 0, GL_RGB, GL_FLOAT, NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, d->textures[i], 0);
		ASSERT((errCode = glGetError()) == GL_NO_ERROR);
	}

	// depth
	glBindTexture(GL_TEXTURE_2D, d->depthTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, windowWidth, windowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, d->depthTexture, 0);
	ASSERT((errCode = glGetError()) == GL_NO_ERROR);

	Vector<GLenum> drawBuffers;
	for (GMuint i = 0; i < TEXTURE_NUM; i++)
	{
		drawBuffers.push_back(GL_COLOR_ATTACHMENT0 + i);
	}

	glDrawBuffers(TEXTURE_NUM, drawBuffers.data());
	ASSERT((errCode = glGetError()) == GL_NO_ERROR);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		gm_error("FB error, status: 0x%x\n", status);
		return false;
	}

	// restore default FBO
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	return true;
}

void GMGLGBuffer::bindForWriting()
{
	D(d);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, d->fbo);
}

void GMGLGBuffer::bindForReading()
{
	D(d);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, d->fbo);
}

void GMGLGBuffer::releaseBind()
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}

void GMGLGBuffer::setReadBuffer(GBufferTextureType TextureType)
{
	glReadBuffer(GL_COLOR_ATTACHMENT0 + (GMuint) TextureType);
}

void GMGLGBuffer::newFrame()
{
	bindForWriting();
	GMGLGraphicEngine::newFrameOnCurrentContext();
	releaseBind();
}

void GMGLGBuffer::activateTextures()
{
	D(d);
	for (GMuint i = 0; i < TEXTURE_NUM; i++)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, d->textures[i]);
	}
}