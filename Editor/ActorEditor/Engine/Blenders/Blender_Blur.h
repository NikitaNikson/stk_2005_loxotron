#ifndef BLENDER_BLUR_H
#define BLENDER_BLUR_H
#pragma once

#include "Blender.h"

class ENGINE_API CBlender_Blur : public CBlender  
{
public:
	virtual		LPCSTR		getComment()	{ return "INTERNAL: blur";	}

	virtual		void		Save			(CFS_Base&  FS);
	virtual		void		Load			(CStream&	FS, WORD version);

	virtual		void		Compile			(CBlender_Compile& C);

	CBlender_Blur();
	virtual ~CBlender_Blur();
};

#endif //BLENDER_BLUR_H
