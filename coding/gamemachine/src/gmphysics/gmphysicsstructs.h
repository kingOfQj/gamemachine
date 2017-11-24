﻿#ifndef __PHYSICSSTRUCTS_H__
#define __PHYSICSSTRUCTS_H__
#include <gmcommon.h>
#include <map>
#include <linearmath.h>
BEGIN_NS

typedef GMint GMCommand;

struct GMCommandVector3
{
	GMCommandVector3() = default;
	GMCommandVector3(GMfloat x, GMfloat y, GMfloat z)
	{
		p[0] = x;
		p[1] = y;
		p[2] = z;
	}

	GMfloat& operator[](GMint i)
	{
		return p[i];
	}

private:
	GMfloat p[3];
};

typedef Map<GMCommand, Vector<GMCommandVector3> > CommandParams;

#define USELESS_PARAM 0
#define CMD_NONE 0x0000
#define CMD_MOVE 0x0001
#define CMD_JUMP 0x0002

GM_ALIGNED_STRUCT(GMShapeProperties)
{
	glm::vec3 bounding[2]; //最小边界和最大边界
	GMfloat stepHeight;
};

GM_ALIGNED_STRUCT(GMMotionProperties)
{
	glm::vec3 translation;
	glm::vec3 velocity;
	glm::vec3 jumpSpeed;
	GMfloat moveSpeed;
};

GM_ALIGNED_STRUCT(GMCollisionObject)
{
	GMMotionProperties motions;
	GMShapeProperties shapeProps;
};

END_NS
#endif
