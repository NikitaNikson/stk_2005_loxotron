//----------------------------------------------------
// file: rpoint.h
//----------------------------------------------------

#ifndef _INCDEF_RPoint_H_
#define _INCDEF_RPoint_H_

#include "CustomObject.h"
                                                    
#define RPOINT_SIZE 0.5f

class CFrustum;

class CRPoint : public CCustomObject {
    friend class    SceneBuilder;
public:
	enum EType{
    	etPlayer,
    	etEntity,
        force_dword=(-1) 
    };
    struct Flags{
    	DWORD		bActive:1;
    };
public:
    DWORD			m_dwTeamID;
    DWORD			m_dwSquadID;
    DWORD			m_dwGroupID;
    Flags			m_Flags;
    EType			m_Type;
    string64		m_EntityRefs;
public:
	                CRPoint     ();
	                CRPoint     ( char *name );
    void            Construct   ();
	virtual         ~CRPoint    ();

	virtual void    Render      ( int priority, bool strictB2F );
	virtual bool    RayPick     ( float& distance,	Fvector& start,	Fvector& direction,
		                          SRayPickInfo* pinf = NULL );
    virtual bool 	FrustumPick	( const CFrustum& frustum );
  	virtual bool 	Load		(CStream&);
	virtual void 	Save		(CFS_Base&);
	virtual bool    GetBox      (Fbox& box);
};

#endif /*_INCDEF_Glow_H_*/

