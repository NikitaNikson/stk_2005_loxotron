// exxZERO Time Stamp AddIn. Document modified at : Thursday, March 07, 2002 14:14:04 , by user : Oles , from computer : OLES
// Level.cpp: implementation of the CLevel class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "../fdemorecord.h"
#include "../fdemoplay.h"
#include "../environment.h"
#include "../igame_persistent.h"
#include "ParticlesObject.h"

#include "Level.h"
#include "xrServer.h"
#include "net_queue.h"
#include "game_cl_base.h"

#include "entity_alive.h"
#include "hudmanager.h"
#include "ai_space.h"
#include "ai_debug.h"

// events
#include "PHdynamicdata.h"
#include "Physics.h"

#include "ShootingObject.h"

#include "LevelFogOfWar.h"
#include "Level_Bullet_Manager.h"

#include "script_process.h"
#include "script_engine.h"
#include "script_engine_space.h"
#include "team_base_zone.h"

#include "infoportion.h"

#include "patrol_path_storage.h"
#include "date_time.h"

#include "space_restriction_manager.h"
#include "seniority_hierarchy_holder.h"
#include "space_restrictor.h"
#include "client_spawn_manager.h"
#include "autosave_manager.h"

#include "ClimableObject.h"

#include "level_navigation_graph.h"

#include "mt_config.h"

#include "phcommander.h"
#include "map_manager.h"
#include "../CameraManager.h"

#ifdef DEBUG
#include "level_debug.h"
#endif

#include "level_sounds.h"

#ifdef DEBUG
#	include "ai/stalker/ai_stalker.h"
#endif

//#define DEBUG_PRECISE_PATH

		CPHWorld*	ph_world						= 0;
		float		g_cl_lvInterp					= 0;
		u32			lvInterpSteps					= 0;
extern	BOOL		g_bDebugDumpPhysicsStep			;
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLevel::CLevel():IPureClient	(Device.GetTimerGlobal())
{
	g_bDebugEvents				= strstr(Core.Params,"-debug_ge")?TRUE:FALSE;

//	XML_DisableStringCaching();
	Server						= NULL;

	game						= NULL;
//	game						= xr_new<game_cl_GameState>();
	game_events					= xr_new<NET_Queue_Event>();

	game_configured				= FALSE;

	eChangeRP					= Engine.Event.Handler_Attach	("LEVEL:ChangeRP",this);
	eDemoPlay					= Engine.Event.Handler_Attach	("LEVEL:PlayDEMO",this);
	eChangeTrack				= Engine.Event.Handler_Attach	("LEVEL:PlayMusic",this);
	eEnvironment				= Engine.Event.Handler_Attach	("LEVEL:Environment",this);

	eEntitySpawn				= Engine.Event.Handler_Attach	("LEVEL:spawn",this);

	//by Dandy
//	m_pFogOfWar					= NULL;
//	m_pFogOfWar					= xr_new<CFogOfWar>();

	m_pBulletManager			= xr_new<CBulletManager>();
	m_map_manager				= xr_new<CMapManager>();
	m_pFogOfWarMngr				= xr_new<CFogOfWarMngr>();
//----------------------------------------------------
	m_bNeed_CrPr					= false;
	m_bIn_CrPr						= false;
	m_dwNumSteps				= 0;
	m_dwDeltaUpdate				= u32(fixed_step*1000);
	m_dwLastNetUpdateTime		= 0;

	physics_step_time_callback	= (PhysicsStepTimeCallback*) &PhisStepsCallback;

	m_level_sound_manager		= xr_new<CLevelSoundManager>();

	m_space_restriction_manager = xr_new<CSpaceRestrictionManager>();

	m_seniority_hierarchy_holder= xr_new<CSeniorityHierarchyHolder>();

	m_client_spawn_manager		= xr_new<CClientSpawnManager>();

	m_autosave_manager			= xr_new<CAutosaveManager>();
	
	m_ph_commander				= xr_new<CPHCommander>();
	m_ph_commander_scripts		= xr_new<CPHCommander>();
#ifdef DEBUG
	m_bSynchronization			= false;
#endif	
	//---------------------------------------------------------
	pStatGraph = NULL;
	//---------------------------------------------------------
	pObjects4CrPr.clear();
	pActors4CrPr.clear();
	//---------------------------------------------------------
	pCurrentControlEntity = NULL;

#ifdef DEBUG
	m_level_debug	= xr_new<CLevelDebug>();
#endif
	//---------------------------------------------------------
	m_dwCL_PingLastSendTime = 0;
	m_dwCL_PingDeltaSend = 1000;
	m_dwRealPing = 0;

	//---------------------------------------------------------	
	m_sDemoName[0] = 0;
	m_bDemoSaveMode = FALSE;
	m_dwStoredDemoDataSize = 0;
	m_pStoredDemoData = NULL;
	m_pOldCrashHandler = NULL;
	if (!strstr(Core.Params,"-tdemo ") && !strstr(Core.Params,"-tdemof "))
	{		
		Demo_PrepareToStore();
	};
	//---------------------------------------------------------
	m_bDemoPlayMode = FALSE;
	m_aDemoData.clear();
	m_bDemoStarted	= FALSE;

	if (strstr(Core.Params,"-tdemo ") || strstr(Core.Params,"-tdemof ")) {		
		string1024				f_name;
		if (strstr(Core.Params,"-tdemo "))
		{
			sscanf					(strstr(Core.Params,"-tdemo ")+7,"%[^ ] ",f_name);
			m_bDemoPlayByFrame = FALSE;
		}
		else
		{
			sscanf					(strstr(Core.Params,"-tdemof ")+8,"%[^ ] ",f_name);
			m_bDemoPlayByFrame = TRUE;
		};
		
		Demo_Load	(f_name);		
	}	
	//---------------------------------------------------------	
}

extern CAI_Space *g_ai_space;

CLevel::~CLevel()
{
//	g_pGameLevel		= NULL;
	Msg							("- Destroying level");

	Engine.Event.Handler_Detach	(eEntitySpawn,	this);

	Engine.Event.Handler_Detach	(eEnvironment,	this);
	Engine.Event.Handler_Detach	(eChangeTrack,	this);
	Engine.Event.Handler_Detach	(eDemoPlay,		this);
	Engine.Event.Handler_Detach	(eChangeRP,		this);

	if (ph_world)
	{
		ph_world->Destroy		();
		xr_delete				(ph_world);
	}

	// destroy PSs
	for (POIt p_it=m_StaticParticles.begin(); m_StaticParticles.end()!=p_it; ++p_it)
		CParticlesObject::Destroy(*p_it);
	m_StaticParticles.clear		();

	// Unload sounds
	// unload prefetched sounds
	sound_registry.clear		();

	// unload static sounds
	for (u32 i=0; i<static_Sounds.size(); ++i){
		static_Sounds[i]->destroy();
		xr_delete				(static_Sounds[i]);
	}
	static_Sounds.clear			();

	xr_delete					(m_level_sound_manager);

	xr_delete					(m_space_restriction_manager);

	xr_delete					(m_seniority_hierarchy_holder);
	
	xr_delete					(m_client_spawn_manager);

	xr_delete					(m_autosave_manager);
	
	ai().script_engine().remove_script_process(ScriptEngine::eScriptProcessorLevel);

	xr_delete					(game);
	xr_delete					(game_events);


	//by Dandy
	//destroy fog of war
//	xr_delete					(m_pFogOfWar);
	//destroy bullet manager
	xr_delete					(m_pBulletManager);
	//-----------------------------------------------------------
	xr_delete					(pStatGraph);

	//-----------------------------------------------------------
	xr_delete					(m_ph_commander);
	xr_delete					(m_ph_commander_scripts);
	//-----------------------------------------------------------
	pObjects4CrPr.clear();
	pActors4CrPr.clear();

	ai().unload					();

#ifdef DEBUG
	CInifile					*old_settings = pSettings, *new_settings;
	string256					file_name;
	FS.update_path				(file_name,"$game_config$","system.ltx");
	new_settings				= xr_new<CInifile>(file_name);
	pSettings					= new_settings;
	xr_delete					(old_settings);
#endif

	//-----------------------------------------------------------	
#ifdef DEBUG	
	xr_delete					(m_level_debug);
#endif
	//-----------------------------------------------------------
	xr_delete					(m_map_manager);
	xr_delete					(m_pFogOfWarMngr);
	//-----------------------------------------------------------
	Demo_Clear					();
	m_aDemoData.clear			();
}

void CLevel::PrefetchSound		(LPCSTR name)
{
	// preprocess sound name
	string_path					tmp;
	strcpy						(tmp,name);
	xr_strlwr					(tmp);
	if (strext(tmp))			*strext(tmp)=0;
	shared_str	snd_name		= tmp;
	// find in registry
	SoundRegistryMapIt it		= sound_registry.find(snd_name);
	// if find failed - preload sound
	if (it==sound_registry.end())
		sound_registry[snd_name].create(TRUE,snd_name.c_str());
}

// Game interface ////////////////////////////////////////////////////
int	CLevel::get_RPID(LPCSTR /**name/**/)
{
	/*
	// Gain access to string
	LPCSTR	params = pLevel->r_string("respawn_point",name);
	if (0==params)	return -1;

	// Read data
	Fvector4	pos;
	int			team;
	sscanf		(params,"%f,%f,%f,%d,%f",&pos.x,&pos.y,&pos.z,&team,&pos.w); pos.y += 0.1f;

	// Search respawn point
	svector<Fvector4,maxRP>	&rp = Level().get_team(team).RespawnPoints;
	for (int i=0; i<(int)(rp.size()); ++i)
		if (pos.similar(rp[i],EPS_L))	return i;
	*/
	return -1;
}

BOOL		g_bDebugEvents = FALSE	;


void CLevel::cl_Process_Event				(u16 dest, u16 type, NET_Packet& P)
{
	//			Msg				("--- event[%d] for [%d]",type,dest);
	CObject*	 O	= Objects.net_Find	(dest);
	if (0==O)		{
		Msg("* WARNING: c_EVENT[%d] : unknown dest",dest);
		return;
	}
	CGameObject* GO = smart_cast<CGameObject*>(O);
	if (!GO)		{
		Msg("! ERROR: c_EVENT[%d] : non-game-object",dest);
		return;
	}
	if (type != GE_DESTROY_REJECT)
	{
		if (type == GE_DESTROY)
			Game().OnDestroy(GO);
		GO->OnEvent		(P,type);
	}
	else {
		u32				pos = P.r_tell();
		u16				id = P.r_u16();
		P.r_seek		(pos);

		bool			ok = true;

		CObject			*D	= Objects.net_Find	(id);
		if (0==D)		{
			Msg			("! ERROR: c_EVENT[%d] : unknown dest",id);
			ok			= false;
		}

		CGameObject		*GD = smart_cast<CGameObject*>(D);
		if (!GD)		{
			Msg			("! ERROR: c_EVENT[%d] : non-game-object",id);
			ok			= false;
		}

		GO->OnEvent		(P,GE_OWNERSHIP_REJECT);
		if (ok)
		{
			Game().OnDestroy(GD);
			GD->OnEvent	(P,GE_DESTROY);
		};
	}
};

void CLevel::ProcessGameEvents		()
{
	// Game events
	{
		NET_Packet			P;
		u32 svT				= timeServer()-NET_Latency;

		/*
		if (!game_events->queue.empty())	
			Msg("- d[%d],ts[%d] -- E[svT=%d],[evT=%d]",Device.dwTimeGlobal,timeServer(),svT,game_events->queue.begin()->timestamp);
		*/

		while	(game_events->available(svT))
		{
			u16 ID,dest,type;
			game_events->get	(ID,dest,type,P);

			switch (ID)
			{
			case M_SPAWN:
				{
					u16 dummy16;
					P.r_begin(dummy16);
					cl_Process_Spawn(P);
				}break;
			case M_EVENT:
				{
					cl_Process_Event(dest, type, P);
				}break;
			default:
				{
					VERIFY(0);
				}break;
			}
		}
	}
	if (OnServer() && GameID()!= GAME_SINGLE)
		Game().m_WeaponUsageStatistic.Send_Check_Respond();
}

void CLevel::OnFrame	()
{
	// commit events from bullet manager from prev-frame
	Device.Statistic.TEST0.Begin		();
	BulletManager().CommitEvents		();
	Device.Statistic.TEST0.End			();

	// Client receive
	if (net_isDisconnected())	
	{
		Engine.Event.Defer				("kernel:disconnect");
		return;
	} else {
		Device.Statistic.netClient.Begin();
		ClientReceive					();
		Device.Statistic.netClient.End	();
	}

	ProcessGameEvents	();

	//Net sync
	if (!ai().get_alife())
		Device.Statistic.TEST2.Begin();
	if (m_bNeed_CrPr)					make_NetCorrectionPrediction();
	if (!ai().get_alife())
		Device.Statistic.TEST2.End();

	MapManager().Update		();
	// Inherited update
	inherited::OnFrame		();

	// Draw client/server stats
	CGameFont* F = HUD().Font().pFontDI;
	if ( IsServer() )
	{
		if (psDeviceFlags.test(rsStatistic))
		{
			const IServerStatistic* S = Server->GetStatistic();
			F->SetSizeI	(0.015f);
			F->OutSetI	(0.0f,0.5f);
			F->SetColor	(D3DCOLOR_XRGB(0,255,0));
			F->OutNext	("IN:  %4d/%4d (%2.1f%%)",	S->bytes_in_real,	S->bytes_in,	100.f*float(S->bytes_in_real)/float(S->bytes_in));
			F->OutNext	("OUT: %4d/%4d (%2.1f%%)",	S->bytes_out_real,	S->bytes_out,	100.f*float(S->bytes_out_real)/float(S->bytes_out));
			F->OutNext	("client_2_sever ping: %d",	net_Statistic.getPing());
			F->OutNext	("SPS/Sended : %4d/%4d", S->dwBytesPerSec, S->dwBytesSended);
			F->OutNext	("sv_urate/cl_urate : %4d/%4d", psNET_ServerUpdate, psNET_ClientUpdate);
			
			F->SetColor	(D3DCOLOR_XRGB(255,255,255));
			for (u32 I=0; I<Server->client_Count(); ++I)	{
				IClient*	C = Server->client_Get(I);
				F->OutNext("%10s: P(%d), BPS(%2.1fK), MRR(%2d), MSR(%2d), Retried(%2d), Blocked(%2d)",
					Server->game->get_option_s(*C->Name,"name",*C->Name),
//					C->Name,
					C->stats.getPing(),
					float(C->stats.getBPS()),// /1024,
					C->stats.getMPS_Receive	(),
					C->stats.getMPS_Send	(),
					C->stats.getRetriedCount(),
					C->stats.dwTimesBlocked
					);
			}
			if (IsClient())
			{
				F->OutNext("P(%d), BPS(%2.1fK), MRR(%2d), MSR(%2d), Retried(%2d), Blocked(%2d), Sended(%2d), SPS(%2d)",
					//Server->game->get_option_s(C->Name,"name",C->Name),
					//					C->Name,
					net_Statistic.getPing(),
					float(net_Statistic.getBPS()),// /1024,
					net_Statistic.getMPS_Receive	(),
					net_Statistic.getMPS_Send	(),
					net_Statistic.getRetriedCount(),
					net_Statistic.dwTimesBlocked,
					net_Statistic.dwBytesSended,
					net_Statistic.dwBytesPerSec
					);
			}
		}
	} else {
		if (psDeviceFlags.test(rsStatistic))
		{
			F->SetSizeI	(0.015f);
			F->OutSetI	(0.0f,0.5f);
			F->SetColor	(D3DCOLOR_XRGB(0,255,0));
			F->OutNext	("client_2_sever ping: %d",	net_Statistic.getPing());
		}
	}
	
//	g_pGamePersistent->Environment.SetGameTime	(GetGameDayTimeSec(),GetGameTimeFactor());
	g_pGamePersistent->Environment.SetGameTime	(GetEnvironmentGameDayTimeSec(),GetGameTimeFactor());

	//Device.Statistic.Scripting.Begin	();
	ai().script_engine().script_process	(ScriptEngine::eScriptProcessorLevel)->update();
	//Device.Statistic.Scripting.End	();
	m_ph_commander->update				();
	m_ph_commander_scripts->update		();
//	autosave_manager().update			();

	//?????????? ????? ????
	Device.Statistic.TEST0.Begin		();
	BulletManager().CommitRenderSet		();
	Device.Statistic.TEST0.End			();

	// update static sounds
	m_level_sound_manager->Update		();

	// deffer LUA-GC-STEP
	if (g_mt_config.test(mtLUA_GC))		Device.seqParallel.push_back	(fastdelegate::FastDelegate0<>(this,&CLevel::script_gc));
	else								script_gc	()	;
}

int		psLUA_GCSTEP					= 10			;
void	CLevel::script_gc				()
{
	lua_gc	(ai().script_engine().lua(), LUA_GCSTEP, psLUA_GCSTEP);
}

#ifdef DEBUG_PRECISE_PATH
void test_precise_path	();
#endif

void CLevel::OnRender()
{
	inherited::OnRender	();
	
	Game().OnRender();
	//?????????? ?????? ????
	//Device.Statistic.TEST1.Begin();
	BulletManager().Render();
	//Device.Statistic.TEST1.End();
	//?????????? ????????c ????????????
	HUD().RenderUI();

	autosave_manager().OnRender();

#ifdef DEBUG
	ph_world->OnRender	();
#endif

#ifdef DEBUG
	if (ai().get_level_graph() && (bDebug || psAI_Flags.test(aiMotion)))
		ai().level_graph().render();

#ifdef DEBUG_PRECISE_PATH
	test_precise_path		();
#endif

	CAI_Stalker				*stalker = smart_cast<CAI_Stalker*>(Level().CurrentEntity());
	if (stalker)
		stalker->OnRender	();


	if (bDebug)	{
		for (u32 I=0; I < Level().Objects.o_count(); I++) {
			CObject*	_O		= Level().Objects.o_get_by_iterator(I);
			CSpaceRestrictor	*space_restrictor = smart_cast<CSpaceRestrictor*>	(_O);
			if (space_restrictor)
				space_restrictor->OnRender();
			CClimableObject		*climable		  = smart_cast<CClimableObject*>	(_O);
			if(climable)
				climable->OnRender();
			CTeamBaseZone	*team_base_zone = smart_cast<CTeamBaseZone*>(_O);
			if (team_base_zone)
				team_base_zone->OnRender();
			if (GameID() != GAME_SINGLE)
			{
				CInventoryItem* pIItem = smart_cast<CInventoryItem*>(_O);
				if (pIItem) pIItem->OnRender();
			}
		}
		//  [7/5/2005]
		if (Server && Server->game) Server->game->OnRender();
		//  [7/5/2005]
		ObjectSpace.dbgRender	();

		//---------------------------------------------------------------------
		HUD().Font().pFontSmall->OutSet		(170,630);
		HUD().Font().pFontSmall->SetSize	(16.0f);
		HUD().Font().pFontSmall->SetColor	(0xffff0000);
		HUD().Font().pFontSmall->OutNext	("Client Objects:      [%d]",Server->GetEntitiesNum());
		HUD().Font().pFontSmall->OutNext	("Server Objects:      [%d]",Objects.o_count());
		HUD().Font().pFontSmall->OutNext	("Interpolation Steps: [%d]", Level().GetInterpolationSteps());
		HUD().Font().pFontSmall->SetSize	(8.0f);
		//---------------------------------------------------------------------
	}
#endif

#ifdef DEBUG
	if (bDebug) {
		DBG().draw_object_info				();
		DBG().draw_text						();
		DBG().draw_level_info				();
	}
#endif
}

void CLevel::OnEvent(EVENT E, u64 P1, u64 /**P2/**/)
{
	if (E==eEntitySpawn)	{
		char	Name[128];	Name[0]=0;
		sscanf	(LPCSTR(P1),"%s", Name);
		Level().g_cl_Spawn	(Name,0xff, M_SPAWN_OBJECT_LOCAL, Fvector().set(0,0,0));
	} else if (E==eChangeRP && P1) {
	} else if (E==eDemoPlay && P1) {
		char* name = (char*)P1;
		char RealName [256];
		strcpy(RealName,name);
		strcat(RealName,".xrdemo");
		Cameras().AddCamEffector(xr_new<CDemoPlay> (RealName,1.3f,0));
	} else if (E==eChangeTrack && P1) {
		// int id = atoi((char*)P1);
		// Environment->Music_Play(id);
	} else if (E==eEnvironment) {
		// int id=0; float s=1;
		// sscanf((char*)P1,"%d,%f",&id,&s);
		// Environment->set_EnvMode(id,s);
	} else return;
}

void	CLevel::AddObject_To_Objects4CrPr	(CGameObject* pObj)
{
	if (!pObj) return;
	for	(OBJECTS_LIST_it OIt = pObjects4CrPr.begin(); OIt != pObjects4CrPr.end(); OIt++)
	{
		if (*OIt == pObj) return;
	}
	pObjects4CrPr.push_back(pObj);

}
void	CLevel::AddActor_To_Actors4CrPr		(CGameObject* pActor)
{
	if (!pActor) return;
	for	(OBJECTS_LIST_it AIt = pActors4CrPr.begin(); AIt != pActors4CrPr.end(); AIt++)
	{
		if (*AIt == pActor) return;
	}
	pActors4CrPr.push_back(pActor);
}

void CLevel::make_NetCorrectionPrediction	()
{
	m_bNeed_CrPr	= false;
	m_bIn_CrPr		= true;
	u64 NumPhSteps = ph_world->m_steps_num;
	ph_world->m_steps_num -= m_dwNumSteps;
	if(g_bDebugDumpPhysicsStep&&m_dwNumSteps>20)Msg("!!!TOO MANY PHYSICS STEPS FOR CORRECTION PREDICTION = %d !!!",m_dwNumSteps);
//////////////////////////////////////////////////////////////////////////////////
	ph_world->Freeze();

	//setting UpdateData and determining number of PH steps from last received update
	for	(OBJECTS_LIST_it OIt = pObjects4CrPr.begin(); OIt != pObjects4CrPr.end(); OIt++)
	{
		CGameObject* pObj = *OIt;
		if (!pObj) continue;
		pObj->PH_B_CrPr();
	};
//////////////////////////////////////////////////////////////////////////////////
	//first prediction from "delivered" to "real current" position
	//making enought PH steps to calculate current objects position based on their updated state	
	
	for (u32 i =0; i<m_dwNumSteps; i++)	
	{
		ph_world->Step();

		for	(OBJECTS_LIST_it AIt = pActors4CrPr.begin(); AIt != pActors4CrPr.end(); AIt++)
		{
			CGameObject* pActor = *AIt;
			if (!pActor || pActor->CrPr_IsActivated()) continue;
			pActor->PH_B_CrPr();
		};
	};
//////////////////////////////////////////////////////////////////////////////////
	for	(OBJECTS_LIST_it OIt = pObjects4CrPr.begin(); OIt != pObjects4CrPr.end(); OIt++)
	{
		CGameObject* pObj = *OIt;
		if (!pObj) continue;
		pObj->PH_I_CrPr();
	};
//////////////////////////////////////////////////////////////////////////////////
	if (!InterpolationDisabled())
	{
		for (u32 i =0; i<lvInterpSteps; i++)	//second prediction "real current" to "future" position
		{
			ph_world->Step();
#ifdef DEBUG
/*
			for	(OBJECTS_LIST_it OIt = pObjects4CrPr.begin(); OIt != pObjects4CrPr.end(); OIt++)
			{
				CGameObject* pObj = *OIt;
				if (!pObj) continue;
				pObj->PH_Ch_CrPr();
			};
*/
#endif
		}
		//////////////////////////////////////////////////////////////////////////////////
		for	(OBJECTS_LIST_it OIt = pObjects4CrPr.begin(); OIt != pObjects4CrPr.end(); OIt++)
		{
			CGameObject* pObj = *OIt;
			if (!pObj) continue;
			pObj->PH_A_CrPr();
		};
	};
	ph_world->UnFreeze();

	ph_world->m_steps_num = NumPhSteps;
	m_dwNumSteps = 0;
	m_bIn_CrPr = false;

	pObjects4CrPr.clear();
	pActors4CrPr.clear();
};

u32			CLevel::GetInterpolationSteps	()
{
	return lvInterpSteps;
};

void		CLevel::UpdateDeltaUpd	( u32 LastTime )
{
	u32 CurrentDelta = LastTime - m_dwLastNetUpdateTime;
	if (CurrentDelta < m_dwDeltaUpdate) 
		CurrentDelta = iFloor(float(m_dwDeltaUpdate * 10 + CurrentDelta) / 11);

	m_dwLastNetUpdateTime = LastTime;
	m_dwDeltaUpdate = CurrentDelta;

	if (0 == g_cl_lvInterp) ReculcInterpolationSteps();
	else 
		if (g_cl_lvInterp>0)
		{
			lvInterpSteps = iCeil(g_cl_lvInterp / fixed_step);
		}
};

void		CLevel::ReculcInterpolationSteps ()
{
	lvInterpSteps			= iFloor(float(m_dwDeltaUpdate) / (fixed_step*1000));
	if (lvInterpSteps > 60) lvInterpSteps = 60;
	if (lvInterpSteps < 3)	lvInterpSteps = 3;
};

bool		CLevel::InterpolationDisabled	()
{
	return g_cl_lvInterp < 0; 
};

void 		CLevel::PhisStepsCallback	( u32 Time0, u32 Time1 )
{
	if (GameID() == GAME_SINGLE)	return;

//#pragma todo("Oles to all: highly inefficient and slow!!!")
//fixed (Andy)
	/*
	for (xr_vector<CObject*>::iterator O=Level().Objects.objects.begin(); O!=Level().Objects.objects.end(); ++O) 
	{
		if( (*O)->CLS_ID == CLSID_OBJECT_ACTOR){
			CActor* pActor = smart_cast<CActor*>(*O);
			if (!pActor || pActor->Remote()) continue;
				pActor->UpdatePosStack(Time0, Time1);
		}
	};
	*/
};

void				CLevel::SetNumCrSteps		( u32 NumSteps )
{
	m_bNeed_CrPr = true;
	if (m_dwNumSteps > NumSteps) return;
	m_dwNumSteps = NumSteps;
	if (m_dwNumSteps > 1000000)
	{
		VERIFY(0);
	}
};


ALife::_TIME_ID CLevel::GetGameTime()
{
	return			(game->GetGameTime());
//	return			(Server->game->GetGameTime());
}

ALife::_TIME_ID CLevel::GetEnvironmentGameTime()
{
	return			(game->GetEnvironmentGameTime());
//	return			(Server->game->GetGameTime());
}

u8 CLevel::GetDayTime() 
{ 
	u32 dummy32;
	u32 hours;
	GetGameDateTime(dummy32, dummy32, dummy32, hours, dummy32, dummy32, dummy32);
	VERIFY	(hours<256);
	return	u8(hours); 
}

float CLevel::GetGameDayTimeSec()
{
	return	(float(s64(GetGameTime() % (24*60*60*1000)))/1000.f);
}

u32 CLevel::GetGameDayTimeMS()
{
	return	(u32(s64(GetGameTime() % (24*60*60*1000))));
}

float CLevel::GetEnvironmentGameDayTimeSec()
{
	return	(float(s64(GetEnvironmentGameTime() % (24*60*60*1000)))/1000.f);
}

void CLevel::GetGameDateTime	(u32& year, u32& month, u32& day, u32& hours, u32& mins, u32& secs, u32& milisecs)
{
	split_time(GetGameTime(), year, month, day, hours, mins, secs, milisecs);
}


float CLevel::GetGameTimeFactor()
{
	return			(game->GetGameTimeFactor());
//	return			(Server->game->GetGameTimeFactor());
}

void CLevel::SetGameTimeFactor(const float fTimeFactor)
{
	game->SetGameTimeFactor(fTimeFactor);
//	Server->game->SetGameTimeFactor(fTimeFactor);
}

void CLevel::SetGameTimeFactor(ALife::_TIME_ID GameTime, const float fTimeFactor)
{
	game->SetGameTimeFactor(GameTime, fTimeFactor);
//	Server->game->SetGameTimeFactor(fTimeFactor);
}
void CLevel::SetEnvironmentGameTimeFactor(ALife::_TIME_ID GameTime, const float fTimeFactor)
{
	game->SetEnvironmentGameTimeFactor(GameTime, fTimeFactor);
//	Server->game->SetGameTimeFactor(fTimeFactor);
}/*
void CLevel::SetGameTime(ALife::_TIME_ID GameTime)
{
	game->SetGameTime(GameTime);
//	Server->game->SetGameTime(GameTime);
}
*/
bool CLevel::IsServer ()
{
//	return (!!Server);
	if (IsDemoPlay()) return false;
	if (!Server) return false;
	return (Server->client_Count() != 0);

}

bool CLevel::IsClient ()
{
//	return (!Server);
	if (IsDemoPlay()) return true;
	if (!Server) return true;
	return (Server->client_Count() == 0);
}

u32	GameID()
{
	return Game().Type();
}

bool	IsGameTypeSingle()
{
	return (GameID()==GAME_SINGLE);
}

#ifdef DEBUG_PRECISE_PATH
#include "path_manager_level_precise.h"
#include "graph_engine.h"
#include "graph_engine_space.h"

void test_precise_path	()
{
	xr_vector<u32>	path;

	CLevelPathManagerPrecise<
		CGraphEngine::CAlgorithm::CDataStorage,
		GraphEngineSpace::_dist_type,
		GraphEngineSpace::_index_type,
		GraphEngineSpace::_iteration_type
	>				path_manager;

	bool			failed = !ai().graph_engine().search(
		ai().level_graph(),
		0,
		ai().level_graph().header().vertex_count() - 1,
		&path,
		GraphEngineSpace::CBaseParameters()
//		,
//		path_manager
	);

	if (failed)
		return;
	
	if (path.empty())
		return;
	
	xr_vector<Fvector>		point_path;

	{
		xr_vector<u32>::const_iterator	I = path.begin(), J;
		xr_vector<u32>::const_iterator	E = path.end();
		for ( ; I != E; ) {
			Fvector			current = ai().level_graph().vertex_position(*I);
			for (J = I + 1; J != E; ++J) {
				if (ai().level_graph().check_vertex_in_direction(*I,current,*J))
					continue;
				break;
			}
			
			if (J == E) {
				point_path.push_back(ai().level_graph().vertex_position(path.back()));
				break;
			}

			point_path.push_back(ai().level_graph().vertex_position(*I));
			I				= J-1;
		}
	}

#if 1
	{
		xr_vector<u32>::const_iterator	I = path.begin();
		xr_vector<u32>::const_iterator	E = path.end();
		for ( ; I != E; ++I)
			RCache.dbg_DrawAABB(
				Fvector(ai().level_graph().vertex_position(*I)).add(Fvector().set(0.f,0.05f,0.f)),
				.05f,
				.05f,
				.05f,
				0xff00ff00
			);
	}
#endif
	{
		Fvector							t1, t2;
		xr_vector<Fvector>::iterator	I = point_path.begin(), J = I;
		xr_vector<Fvector>::iterator	E = point_path.end();
		for (++I; I != E; ++I, ++J) {
			RCache.dbg_DrawAABB(
				Fvector(*J).add(Fvector().set(0.f,0.25f,0.f)),
				.25f,
				.25f,
				.25f,
				0xff0000ff
			);

			t1 = Fvector(*J).add(Fvector().set(0.f,0.25f,0.f));
			t2 = Fvector(*I).add(Fvector().set(0.f,0.25f,0.f));
			RCache.dbg_DrawLINE(Fidentity,t1,t2,0xff00ff00);
		}

		RCache.dbg_DrawAABB(
			Fvector(point_path.back()).add(Fvector().set(0.f,0.25f,0.f)),
			.25f,
			.25f,
			.25f,
			0xff0000ff
		);
	}
}
#endif