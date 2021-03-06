#include "stdafx.h"
#include "PSObject.h"
#include "hudmanager.h"
#include "WeaponHUD.h"
#include "WeaponMagazined.h"
#include "entity.h"
#include "actor.h"
#include "ParticlesObject.h"
#include "scope.h"
#include "silencer.h"
#include "GrenadeLauncher.h"
#include "inventory.h"
#include "xrserver_objects_alife_items.h"
#include "ActorEffector.h"
#include "EffectorZoomInertion.h"
#include "xr_level_controller.h"
#include "level.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CWeaponMagazined::CWeaponMagazined(LPCSTR name, ESoundTypes eSoundType) : CWeapon(name)
{
	m_eSoundShow		= ESoundTypes(SOUND_TYPE_ITEM_TAKING | eSoundType);
	m_eSoundHide		= ESoundTypes(SOUND_TYPE_ITEM_HIDING | eSoundType);
	m_eSoundShot		= ESoundTypes(SOUND_TYPE_WEAPON_SHOOTING | eSoundType);
	m_eSoundEmptyClick	= ESoundTypes(SOUND_TYPE_WEAPON_EMPTY_CLICKING | eSoundType);
	m_eSoundReload		= ESoundTypes(SOUND_TYPE_WEAPON_RECHARGING | eSoundType);
	
	m_pSndShotCurrent = NULL;
	m_sSilencerFlameParticles = m_sSilencerSmokeParticles = NULL;

	m_bFireSingleShot = false;
	m_iShotNum = 0;
	m_iQueueSize = WEAPON_ININITE_QUEUE;
	m_bLockType = false;
}

CWeaponMagazined::~CWeaponMagazined()
{
	// sounds
	HUD_SOUND::DestroySound(sndShow);
	HUD_SOUND::DestroySound(sndHide);
	HUD_SOUND::DestroySound(sndShot);
	HUD_SOUND::DestroySound(sndEmptyClick);
	HUD_SOUND::DestroySound(sndReload);
}


void CWeaponMagazined::StopHUDSounds		()
{
	HUD_SOUND::StopSound(sndShow);
	HUD_SOUND::StopSound(sndHide);
	
	HUD_SOUND::StopSound(sndEmptyClick);
	HUD_SOUND::StopSound(sndReload);

	HUD_SOUND::StopSound(sndShot);
//.	if(sndShot.enable && sndShot.snd.feedback)
//.		sndShot.snd.feedback->switch_to_3D();

	inherited::StopHUDSounds();
}

void CWeaponMagazined::net_Destroy()
{
	inherited::net_Destroy();
}


void CWeaponMagazined::Load	(LPCSTR section)
{
	inherited::Load		(section);
		
	// Sounds
	HUD_SOUND::LoadSound(section,"snd_draw"		, sndShow		, TRUE, m_eSoundShow		);
	HUD_SOUND::LoadSound(section,"snd_holster"	, sndHide		, TRUE, m_eSoundHide		);
	HUD_SOUND::LoadSound(section,"snd_shoot"	, sndShot		, TRUE, m_eSoundShot		);
	HUD_SOUND::LoadSound(section,"snd_empty"	, sndEmptyClick	, TRUE, m_eSoundEmptyClick	);
	HUD_SOUND::LoadSound(section,"snd_reload"	, sndReload		, TRUE, m_eSoundReload		);
	
	m_pSndShotCurrent = &sndShot;
		
	
	// HUD :: Anims
	R_ASSERT			(m_pHUD);
	animGet				(mhud_idle,		pSettings->r_string(*hud_sect, "anim_idle"));
	animGet				(mhud_reload,	pSettings->r_string(*hud_sect, "anim_reload"));
	animGet				(mhud_show,		pSettings->r_string(*hud_sect, "anim_draw"));
	animGet				(mhud_hide,		pSettings->r_string(*hud_sect, "anim_holster"));
	animGet				(mhud_shots,	pSettings->r_string(*hud_sect, "anim_shoot"));

	if(pSettings->line_exist(*hud_sect,"anim_idle_sprint"))
		animGet				(mhud_idle_sprint,	pSettings->r_string(*hud_sect, "anim_idle_sprint"));

	if(IsZoomEnabled())
		animGet				(mhud_idle_aim,		pSettings->r_string(*hud_sect, "anim_idle_aim"));
	

	//????? ? ???????? ?????????, ????? ????? ????
	if(m_eSilencerStatus == ALife::eAddonAttachable)
	{
		if(pSettings->line_exist(section, "silencer_flame_particles"))
			m_sSilencerFlameParticles = pSettings->r_string(section, "silencer_flame_particles");
		if(pSettings->line_exist(section, "silencer_smoke_particles"))
			m_sSilencerSmokeParticles = pSettings->r_string(section, "silencer_smoke_particles");
		HUD_SOUND::LoadSound(section,"snd_silncer_shot", sndSilencerShot, TRUE, m_eSoundShot);
	}
	//  [7/20/2005]
	if (pSettings->line_exist(section, "dispersion_start"))
		m_iShootEffectorStart = pSettings->r_u8(section, "dispersion_start");
	else
		m_iShootEffectorStart = 0;
	//  [7/20/2005]
	//  [7/21/2005]
	if (pSettings->line_exist(section, "fire_modes"))
	{
		m_bHasDifferentFireModes = true;
		shared_str FireModesList = pSettings->r_string(section, "fire_modes");
		int ModesCount = _GetItemCount(FireModesList.c_str());
		m_aFireModes.clear();
		for (int i=0; i<ModesCount; i++)
		{
			string16 sItem;
			_GetItem(FireModesList.c_str(), i, sItem);
			int FireMode = atoi(sItem);
			m_aFireModes.push_back(FireMode);			
		}
		m_iCurFireMode = ModesCount - 1;
	}
	else
		m_bHasDifferentFireModes = false;
	//  [7/21/2005]
}

void CWeaponMagazined::FireStart		()
{
	if(IsValid() && !IsMisfire()) 
	{
		if(!IsWorking() || AllowFireWhileWorking())
		{
			if(STATE==eReload) return;
			if(STATE==eShowing) return;
			if(STATE==eHiding) return;
			if(STATE==eMisfire) return;

			inherited::FireStart();
			
			if (iAmmoElapsed == 0) 
				OnMagazineEmpty();
			else
				SwitchState(eFire);
		}
	} 
	else 
	{
		if(eReload!=STATE && eMisfire!=STATE) OnMagazineEmpty();
	}
}

void CWeaponMagazined::FireEnd() 
{
	inherited::FireEnd();

	CActor	*actor = smart_cast<CActor*>(H_Parent());
	if(!iAmmoElapsed && actor) 
		Reload();
}

void CWeaponMagazined::Reload() 
{
	inherited::Reload();

	TryReload();
}

bool CWeaponMagazined::TryReload() 
{
	if(m_pInventory) 
	{
		m_pAmmo = smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(*m_ammoTypes[m_ammoType] ));

		
		if(IsMisfire())
		{
			m_bPending = true;
			SwitchState(eReload); 
			return true;
		}

		if(m_pAmmo || unlimited_ammo())  
		{
			m_bPending = true;
			SwitchState(eReload); 
			return true;
		} 
		else for(u32 i = 0; i < m_ammoTypes.size(); ++i) 
		{
			m_pAmmo = smart_cast<CWeaponAmmo*>(m_pInventory->GetAny( *m_ammoTypes[i] ));
			if(m_pAmmo) 
			{ 
				m_ammoType = i; 
				m_bPending = true;
				SwitchState(eReload);
				return true; 
			}
		}
	}
	
	SwitchState(eIdle);

	return false;
}

bool CWeaponMagazined::IsAmmoAvailable()
{
	if (smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(*m_ammoTypes[m_ammoType])))
		return	(true);
	else
		for(u32 i = 0; i < m_ammoTypes.size(); ++i)
			if (smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(*m_ammoTypes[i])))
				return	(true);
	return		(false);
}

void CWeaponMagazined::OnMagazineEmpty() 
{
	//??????? ???????? ????? ??? ????????
	if(STATE == eIdle) 
	{
		OnEmptyClick			();
		return;
	}

	if( NEXT_STATE != eMagEmpty && NEXT_STATE != eReload)
	{
		SwitchState(eMagEmpty);
	}

	inherited::OnMagazineEmpty();
}

void CWeaponMagazined::UnloadMagazine() 
{
	xr_map<LPCSTR, u16> l_ammo;
	
	while(!m_magazine.empty()) 
	{
		CCartridge &l_cartridge = m_magazine.top();
		xr_map<LPCSTR, u16>::iterator l_it;
		for(l_it = l_ammo.begin(); l_ammo.end() != l_it; ++l_it) 
		{
            if(!xr_strcmp(*l_cartridge.m_ammoSect, l_it->first)) 
            { 
				 ++(l_it->second); 
				 break; 
			}
		}

		if(l_it == l_ammo.end()) l_ammo[*l_cartridge.m_ammoSect] = 1;
		m_magazine.pop(); 
		--iAmmoElapsed;
	}

	VERIFY((u32)iAmmoElapsed == m_magazine.size());
	
	xr_map<LPCSTR, u16>::iterator l_it;
	for(l_it = l_ammo.begin(); l_ammo.end() != l_it; ++l_it) 
	{
		CWeaponAmmo *l_pA = smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(l_it->first));
		if(l_pA) 
		{
			u16 l_free = l_pA->m_boxSize - l_pA->m_boxCurr;
			l_pA->m_boxCurr = l_pA->m_boxCurr + (l_free < l_it->second ? l_free : l_it->second);
			l_it->second = l_it->second - (l_free < l_it->second ? l_free : l_it->second);
		}
		if(l_it->second && !unlimited_ammo()) SpawnAmmo(l_it->second, l_it->first);
	}
}

void CWeaponMagazined::ReloadMagazine() 
{
	m_dwAmmoCurrentCalcFrame = 0;	

	//????????? ?????? ??? ???????????
	if(IsMisfire())	bMisfire = false;
	
	//?????????? ????????? ?????????????
	//?????? ?????? ????? ????????
//	static bool l_lockType = false;
	if (!m_bLockType) {
		m_ammoName	= NULL;
		m_pAmmo		= NULL;
	}
	
	if (!m_pInventory) return;

	if(!unlimited_ammo()) 
	{
		//?????????? ????? ? ????????? ??????? ???????? ???? 
		m_pAmmo = smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(*m_ammoTypes[m_ammoType]));
		
		if(!m_pAmmo && !m_bLockType) 
		{
			for(u32 i = 0; i < m_ammoTypes.size(); ++i) 
			{
				//????????? ??????? ???? ?????????? ?????
				m_pAmmo = smart_cast<CWeaponAmmo*>(m_pInventory->GetAny(*m_ammoTypes[i]));
				if(m_pAmmo) 
				{ 
					m_ammoType = i; 
					break; 
				}
			}
		}
	}
	else
		m_ammoType = 0;

//	VERIFY((u32)iAmmoElapsed == m_magazine.size());
	if((u32)iAmmoElapsed != m_magazine.size()){
		Msg("iAmmoElapsed = %d, m_magazine.size() = %d", iAmmoElapsed, m_magazine.size() );	
		Msg("weapon=%s",*cNameSect());
		if(H_Parent()){
			Msg("owner=%s",*H_Parent()->cNameSect());
		}
		VERIFY((u32)iAmmoElapsed == m_magazine.size());
	}

	//??? ???????? ??? ???????????
	if(!m_pAmmo && !unlimited_ammo() ) return;

	//????????? ???????, ???? ????????? ????????? ??????? ????
	if(!m_bLockType && !m_magazine.empty() && 
		(!m_pAmmo || xr_strcmp(m_pAmmo->cNameSect(), 
					 *m_magazine.top().m_ammoSect)))
		UnloadMagazine();

	VERIFY((u32)iAmmoElapsed == m_magazine.size());
	
	CCartridge l_cartridge = m_DefaultCartridge;
	while(iAmmoElapsed < iMagazineSize)
	{
		if (!unlimited_ammo())
		{
			if (!m_pAmmo->Get(l_cartridge)) break;
		}
		++iAmmoElapsed;
		m_magazine.push(l_cartridge);
		m_fCurrentCartirdgeDisp = l_cartridge.m_kDisp;
	}
	m_ammoName = (m_pAmmo) ? m_pAmmo->m_nameShort : NULL;

	VERIFY((u32)iAmmoElapsed == m_magazine.size());

	//???????? ??????? ????????, ???? ??? ??????
	if(m_pAmmo && !m_pAmmo->m_boxCurr && OnServer()) m_pAmmo->Drop();

	if(iMagazineSize > iAmmoElapsed) 
	{ 
		m_bLockType = true; 
		ReloadMagazine(); 
		m_bLockType = false; 
	}

	VERIFY((u32)iAmmoElapsed == m_magazine.size());
}

void CWeaponMagazined::OnStateSwitch	(u32 S)
{
	switch (S)
	{
	case eIdle:
		switch2_Idle	();
		break;
	case eFire:
		switch2_Fire	();
		break;
	case eFire2:
		switch2_Fire2	();
		break;
	case eMisfire:
		break;
	case eMagEmpty:
		switch2_Empty	();
		break;
	case eReload:
		switch2_Reload	();
		break;
	case eShowing:
		switch2_Showing	();
		break;
	case eHiding:
		switch2_Hiding	();
		break;
	case eHidden:
		switch2_Hidden	();
		break;
	}

	inherited::OnStateSwitch(S);
}

void CWeaponMagazined::UpdateCL			()
{
	inherited::UpdateCL	();
	float dt = Device.fTimeDelta;

	

	//????? ?????????? ?????? ????????? ??????
	//?????? ??????? ?? ??????
	if(NEXT_STATE == STATE)
	{
		switch (STATE)
		{
		case eShowing:
		case eHiding:
		case eReload:
		case eIdle:
			fTime			-=	dt;
			if (fTime<0)	
				fTime = 0;
			break;
		case eFire:			
			if(iAmmoElapsed>0)
				state_Fire		(dt);
			
			if(fTime<=0)
			{
				if(iAmmoElapsed == 0)
					OnMagazineEmpty();
				StopShooting();
			}
			else
			{
				fTime			-=	dt;
			}

			break;
		case eMisfire:		state_Misfire	(dt);	break;
		case eMagEmpty:		state_MagEmpty	(dt);	break;
		case eHidden:		break;
		}
	}

	UpdateSounds		();
}

void CWeaponMagazined::UpdateSounds	()
{
	if (Device.dwFrame == dwUpdateSounds_Frame)  
		return;
	
	dwUpdateSounds_Frame = Device.dwFrame;

	// ref_sound positions
	if (sndShow.playing			())	sndShow.set_position		(get_LastFP());
	if (sndHide.playing			())	sndHide.set_position		(get_LastFP());
	if (sndShot.playing			()) sndShot.set_position		(get_LastFP());
	if (sndReload.playing		()) sndReload.set_position		(get_LastFP());
	if (sndEmptyClick.playing	())	sndEmptyClick.set_position	(get_LastFP());
}

void CWeaponMagazined::state_Fire	(float dt)
{
	VERIFY(fTimeToFire>0.f);

	Fvector					p1, d; 
	p1.set(get_LastFP());
	d.set(get_LastFD());

	if (!H_Parent()) return;
	smart_cast<CEntity*>	(H_Parent())->g_fireParams	(this, p1,d);
	if (m_iShotNum == 0)
	{
		m_vStartPos = p1;
		m_vStartDir = d;
	};
		
	VERIFY(!m_magazine.empty());
//	Msg("%d && %d && (%d || %d) && (%d || %d)", !m_magazine.empty(), fTime<=0, IsWorking(), m_bFireSingleShot, m_iQueueSize < 0, m_iShotNum < m_iQueueSize);
	while (!m_magazine.empty() && fTime<=0 && (IsWorking() || m_bFireSingleShot) && (m_iQueueSize < 0 || m_iShotNum < m_iQueueSize))
	{
		m_bFireSingleShot = false;

		VERIFY(fTimeToFire>0.f);
		fTime			+=	fTimeToFire;
		
		++m_iShotNum;
		OnShot			();
		static int i = 0;
		if (i||m_iShotNum>m_iShootEffectorStart)
			FireTrace		(p1,d);
		else
			FireTrace		(m_vStartPos, m_vStartDir);
	}
	
	if(m_iShotNum == m_iQueueSize)
		m_bStopedAfterQueueFired = true;


	UpdateSounds			();
}

void CWeaponMagazined::state_Misfire	(float /**dt/**/)
{
	OnEmptyClick			();
	SwitchState				(eIdle);
	
	bMisfire				= true;

	UpdateSounds			();
}

void CWeaponMagazined::state_MagEmpty	(float dt)
{
}

void CWeaponMagazined::SetDefaults	()
{
	CWeapon::SetDefaults		();
}

void CWeaponMagazined::Hide		()
{
	// add shot effector
	//SwitchState(eHiding);
	OnZoomOut();
	SwitchState(eHidden);
}
void CWeaponMagazined::Show		()
{
	SwitchState(eShowing);
}


void CWeaponMagazined::OnShot		()
{
	// Sound
	PlaySound			(*m_pSndShotCurrent,get_LastFP());

	// Camera	
	AddShotEffector		();

	// Animation
	PlayAnimShoot		();
	
	// Shell Drop
	Fvector vel; 
	PHGetLinearVell(vel);
	OnShellDrop					(get_LastSP(), vel);
	
	// ????? ?? ??????
	StartFlameParticles	();

	//??? ?? ??????
	StartSmokeParticles			(get_LastFP(), vel);
}


void CWeaponMagazined::OnEmptyClick	()
{
	PlaySound	(sndEmptyClick,get_LastFP());
}
void CWeaponMagazined::OnAnimationEnd() 
{
	switch(STATE) 
	{
		case eReload:	ReloadMagazine();	SwitchState(eIdle);	break;	// End of reload animation
		case eHiding:	SwitchState(eHidden);   break;	// End of Hide
		case eShowing:	SwitchState(eIdle);		break;	// End of Show
		case eIdle:		switch2_Idle();			break;  // Keep showing idle

	}
}
void CWeaponMagazined::switch2_Idle	()
{
	m_bPending = false;
	PlayAnimIdle();
}


void CWeaponMagazined::switch2_Fire	()
{
	m_bStopedAfterQueueFired = false;
	m_bFireSingleShot = true;
	m_iShotNum = 0;

    if(OnClient() && !IsWorking())
		FireStart();

/*	if(SingleShotMode())
	{
		m_bFireSingleShot = true;
		bWorking = false;
	}*/
}
void CWeaponMagazined::switch2_Empty()
{
	OnZoomOut();
	
	if(!TryReload())
	{
		OnEmptyClick();
	}
	else
	{
		inherited::FireEnd();
	}
}
void CWeaponMagazined::PlayReloadSound()
{
	PlaySound	(sndReload,get_LastFP());
}

void CWeaponMagazined::switch2_Reload()
{
	CWeapon::FireEnd();

	PlayReloadSound	();
	PlaySound		(sndReload,get_LastFP());
	PlayAnimReload	();
	m_bPending = true;
}
void CWeaponMagazined::switch2_Hiding()
{
	CWeapon::FireEnd();
	
	PlaySound	(sndHide,get_LastFP());

	PlayAnimHide();
	m_bPending = true;
}

void CWeaponMagazined::switch2_Hidden()
{
	CWeapon::FireEnd();

	signal_HideComplete		();
	RemoveShotEffector		();
}
void CWeaponMagazined::switch2_Showing()
{
	PlaySound	(sndShow,get_LastFP());

	m_bPending = true;
	PlayAnimShow();
}

bool CWeaponMagazined::Action(s32 cmd, u32 flags) 
{
	if(inherited::Action(cmd, flags)) return true;
	
	//???? ?????? ???-?? ??????, ?? ?????? ?? ??????
	if(IsPending()) return false;
	
	switch(cmd) 
	{
	case kWPN_RELOAD:
		{
			if(flags&CMD_START) 
				if(iAmmoElapsed < iMagazineSize || IsMisfire()) 
					Reload();
		} 
		return true;
	case kWPN_FIREMODE_PREV:
		{
			if(flags&CMD_START) 
			{
				OnPrevFireMode();
				return true;
			};
		}break;
	case kWPN_FIREMODE_NEXT:
		{
			if(flags&CMD_START) 
			{
				OnNextFireMode();
				return true;
			};
		}break;
	}
	return false;
}

bool CWeaponMagazined::CanAttach(PIItem pIItem)
{
	CScope*				pScope				= smart_cast<CScope*>(pIItem);
	CSilencer*			pSilencer			= smart_cast<CSilencer*>(pIItem);
	CGrenadeLauncher*	pGrenadeLauncher	= smart_cast<CGrenadeLauncher*>(pIItem);

	if(			pScope &&
				 m_eScopeStatus == ALife::eAddonAttachable &&
				(m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonScope) == 0 &&
				(m_sScopeName == pIItem->object().cNameSect()) )
       return true;
	else if(	pSilencer &&
				m_eSilencerStatus == ALife::eAddonAttachable &&
				(m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonSilencer) == 0 &&
				(m_sSilencerName == pIItem->object().cNameSect()) )
       return true;
	else if (	pGrenadeLauncher &&
				m_eGrenadeLauncherStatus == ALife::eAddonAttachable &&
				(m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) == 0 &&
				(m_sGrenadeLauncherName  == pIItem->object().cNameSect()) )
		return true;
	else
		return inherited::CanAttach(pIItem);
}

bool CWeaponMagazined::CanDetach(const char* item_section_name)
{
	if( m_eScopeStatus == CSE_ALifeItemWeapon::eAddonAttachable &&
	   0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonScope) &&
	   (m_sScopeName	== item_section_name))
       return true;
	else if(m_eSilencerStatus == CSE_ALifeItemWeapon::eAddonAttachable &&
	   0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonSilencer) &&
	   (m_sSilencerName == item_section_name))
       return true;
	else if(m_eGrenadeLauncherStatus == CSE_ALifeItemWeapon::eAddonAttachable &&
	   0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) &&
	   (m_sGrenadeLauncherName == item_section_name))
       return true;
	else
		return inherited::CanDetach(item_section_name);
}

bool CWeaponMagazined::Attach(PIItem pIItem)
{
	bool result = false;

	CScope*				pScope					= smart_cast<CScope*>(pIItem);
	CSilencer*			pSilencer				= smart_cast<CSilencer*>(pIItem);
	CGrenadeLauncher*	pGrenadeLauncher		= smart_cast<CGrenadeLauncher*>(pIItem);
	
	if(pScope &&
	   m_eScopeStatus == CSE_ALifeItemWeapon::eAddonAttachable &&
	   (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonScope) == 0 &&
	   (m_sScopeName == pIItem->object().cNameSect()))
	{
		m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonScope;
		result = true;
	}
	else if(pSilencer &&
	   m_eSilencerStatus == CSE_ALifeItemWeapon::eAddonAttachable &&
	   (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonSilencer) == 0 &&
	   (m_sSilencerName == pIItem->object().cNameSect()))
	{
		m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonSilencer;
		result = true;
	}
	else if(pGrenadeLauncher &&
	   m_eGrenadeLauncherStatus == CSE_ALifeItemWeapon::eAddonAttachable &&
	   (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) == 0 &&
	   (m_sGrenadeLauncherName == pIItem->object().cNameSect()))
	{
		m_flagsAddOnState |= CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher;
		result = true;
	}

	if(result)
	{
		if (OnServer())
		{
			//?????????? ?????????????? ???? ?? ?????????
			pIItem->Drop					();
			pIItem->object().DestroyObject	();
		};

		UpdateAddonsVisibility();
		InitAddons();

		return true;
	}
	else
        return inherited::Attach(pIItem);
}


bool CWeaponMagazined::Detach(const char* item_section_name)
{
	if(		m_eScopeStatus == CSE_ALifeItemWeapon::eAddonAttachable &&
			0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonScope) &&
			(m_sScopeName == item_section_name))
	{
		m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonScope;
		
		UpdateAddonsVisibility();
		InitAddons();

		return CInventoryItemObject::Detach(item_section_name);
	}
	else if(m_eSilencerStatus == CSE_ALifeItemWeapon::eAddonAttachable &&
			0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonSilencer) &&
			(m_sSilencerName == item_section_name))
	{
		m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonSilencer;

		UpdateAddonsVisibility();
		InitAddons();
		return CInventoryItemObject::Detach(item_section_name);
	}
	else if(m_eGrenadeLauncherStatus == CSE_ALifeItemWeapon::eAddonAttachable &&
			0 != (m_flagsAddOnState&CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher) &&
			(m_sGrenadeLauncherName == item_section_name))
	{
		m_flagsAddOnState &= ~CSE_ALifeItemWeapon::eWeaponAddonGrenadeLauncher;

		UpdateAddonsVisibility();
		InitAddons();
		return CInventoryItemObject::Detach(item_section_name);
	}
	else
		return inherited::Detach(item_section_name);;
}

void CWeaponMagazined::InitAddons()
{
	//////////////////////////////////////////////////////////////////////////
	// ??????
	if(IsScopeAttached())
	{
		if(m_eScopeStatus == ALife::eAddonAttachable)
		{
			m_sScopeName = pSettings->r_string(cNameSect(), "scope_name");
			m_iScopeX	 = pSettings->r_s32(cNameSect(),"scope_x");
			m_iScopeY	 = pSettings->r_s32(cNameSect(),"scope_y");

			shared_str scope_tex_name;
			scope_tex_name = pSettings->r_string(*m_sScopeName, "scope_texture");
			m_fScopeZoomFactor = pSettings->r_float	(*m_sScopeName, "scope_zoom_factor");
			if(m_UIScope.GetShader())
				m_UIScope.GetShader().destroy();	
			m_UIScope.Init(*scope_tex_name, "hud\\default", 0, 0, alNone);
		}
		else if(m_eScopeStatus == ALife::eAddonPermanent)
		{
			m_fScopeZoomFactor = pSettings->r_float	(cNameSect(), "scope_zoom_factor");
			shared_str scope_tex_name;
			scope_tex_name = pSettings->r_string(cNameSect(), "scope_texture");

			if(m_UIScope.GetShader())
				m_UIScope.GetShader().destroy();	
			m_UIScope.Init(*scope_tex_name, "hud\\default", 0, 0, alNone);
		}
	}
	else
	{
		if(m_UIScope.GetShader())
			m_UIScope.GetShader().destroy();	
		
		if(IsZoomEnabled())
			m_fScopeZoomFactor = pSettings->r_float	(cNameSect(), "scope_zoom_factor");
	}

	

	//////////////////////////////////////////////////////////////////////////
	// ?????????
	if(IsSilencerAttached() && SilencerAttachable())
	{		
		m_sFlameParticlesCurrent = m_sSilencerFlameParticles;
		m_sSmokeParticlesCurrent = m_sSilencerSmokeParticles;
		m_pSndShotCurrent = &sndSilencerShot;


		//???? ????????
//		LoadFireParams	(*cNameSect(), "silencer_");
		LoadFireParams	(*cNameSect(), "");

		//????????? ?? ????????
		LoadLights		(*cNameSect(), "silencer_");
		//  [7/26/2005]
		ApplySilencerKoeffs();
		//  [7/26/2005]
	}
	else
	{
		m_sFlameParticlesCurrent = m_sFlameParticles;
		m_sSmokeParticlesCurrent = m_sSmokeParticles;
		m_pSndShotCurrent = &sndShot;

		//???? ????????
		LoadFireParams	(*cNameSect(), "");
		//????????? ?? ????????
		LoadLights		(*cNameSect(), "");
	}

	inherited::InitAddons();
}

void CWeaponMagazined::ApplySilencerKoeffs	()
{
	float BHPk = 1.0f, BSk = 1.0f;
	float FDB_k = 1.0f, CD_k = 1.0f;
	
	if (pSettings->line_exist(m_sSilencerName, "bullet_hit_power_k"))
	{
		BHPk = pSettings->r_float(m_sSilencerName, "bullet_hit_power_k");
		clamp(BHPk, 0.0f, 1.0f);
	};
	if (pSettings->line_exist(m_sSilencerName, "bullet_speed_k"))
	{
		BSk = pSettings->r_float(m_sSilencerName, "bullet_speed_k");
		clamp(BSk, 0.0f, 1.0f);
	};
	if (pSettings->line_exist(m_sSilencerName, "fire_dispersion_base_k"))
	{
		FDB_k = pSettings->r_float(m_sSilencerName, "fire_dispersion_base_k");
//		clamp(FDB_k, 0.0f, 1.0f);
	};
	if (pSettings->line_exist(m_sSilencerName, "cam_dispersion_k"))
	{
		CD_k = pSettings->r_float(m_sSilencerName, "cam_dispersion_k");
		clamp(CD_k, 0.0f, 1.0f);
	};

	iHitPower			= int(iHitPower*BHPk);
	fHitImpulse			*= BSk;
	m_fStartBulletSpeed *= BSk;
	fireDispersionBase	*= FDB_k;
	camDispersion		*= CD_k;
	camDispersionInc	*= CD_k;
}

//??????????? ??????? ??? ???????????? ???????? HUD
void CWeaponMagazined::PlayAnimShow()
{
	m_pHUD->animPlay(mhud_show[Random.randI(mhud_show.size())],FALSE,this);
}

void CWeaponMagazined::PlayAnimHide()
{
	m_pHUD->animPlay (mhud_hide[Random.randI(mhud_hide.size())],TRUE,this);
}


void CWeaponMagazined::PlayAnimReload()
{
	m_pHUD->animPlay(mhud_reload[Random.randI(mhud_reload.size())],TRUE,this);
}

void CWeaponMagazined::PlayAnimIdle()
{
	MotionSVec* m = NULL;
	if(IsZoomed())
	{
		m = &mhud_idle_aim;
//		m_pHUD->animPlay(mhud_idle_aim[Random.randI(mhud_idle_aim.size())], TRUE);
	}
	else{
		m = &mhud_idle;
		CActor* pActor = smart_cast<CActor*>(H_Parent());
		if(pActor){
			CEntity::SEntityState st;
			pActor->g_State(st);
			if(st.bSprint && mhud_idle_sprint.size())	m = &mhud_idle_sprint;
		}
	}
	m_pHUD->animPlay((*m)[Random.randI(m->size())], TRUE);
}
void CWeaponMagazined::PlayAnimShoot()
{
	m_pHUD->animPlay(mhud_shots[Random.randI(mhud_shots.size())],TRUE,this);
}




void CWeaponMagazined::OnZoomIn			()
{
	inherited::OnZoomIn();

	if(STATE == eIdle)
		PlayAnimIdle();


	CActor* pActor = smart_cast<CActor*>(H_Parent());
	if(pActor)
	{
		CEffectorZoomInertion* S = smart_cast<CEffectorZoomInertion*>	(pActor->EffectorManager().GetEffector(eCEZoom));
		if (!S)	
		{
			S = (CEffectorZoomInertion*)pActor->EffectorManager().AddEffector(xr_new<CEffectorZoomInertion> ());
			S->Init(this);
		};
		S->SetRndSeed(pActor->GetZoomRndSeed());
		R_ASSERT				(S);
	}
}
void CWeaponMagazined::OnZoomOut		()
{
	if(!m_bZoomMode) return;

	inherited::OnZoomOut();

	if(STATE == eIdle)
		PlayAnimIdle();

	CActor* pActor = smart_cast<CActor*>(H_Parent());
	if(pActor)
		pActor->EffectorManager().RemoveEffector(eCEZoom);

}

//???????????? ??????? ???????? ?????????? ? ?????????
void CWeaponMagazined::SwitchMode			()
{
	if(eIdle != STATE || IsPending()) return;

	if(SingleShotMode())
		m_iQueueSize = WEAPON_ININITE_QUEUE;
	else
		m_iQueueSize = 1;
	
	PlaySound	(sndEmptyClick, get_LastFP());
}
 
void CWeaponMagazined::StartIdleAnim			()
{
	if(IsZoomed())	m_pHUD->animDisplay(mhud_idle_aim[Random.randI(mhud_idle_aim.size())], TRUE);
	else			m_pHUD->animDisplay(mhud_idle[Random.randI(mhud_idle.size())], TRUE);
}

void CWeaponMagazined::onMovementChanged	(ACTOR_DEFS::EMoveCommand cmd)
{
	if( (cmd == ACTOR_DEFS::mcSprint)&&(STATE==eIdle)  )
		PlayAnimIdle						();
}

void	CWeaponMagazined::OnNextFireMode		()
{
	if (!m_bHasDifferentFireModes) return;
	if (STATE != eIdle) return;
	m_iCurFireMode = (m_iCurFireMode+1+m_aFireModes.size()) % m_aFireModes.size();
	SetQueueSize(GetCurrentFireMode());
};

void	CWeaponMagazined::OnPrevFireMode		()
{
	if (!m_bHasDifferentFireModes) return;
	if (STATE != eIdle) return;
	m_iCurFireMode = (m_iCurFireMode-1+m_aFireModes.size()) % m_aFireModes.size();
	SetQueueSize(GetCurrentFireMode());	
};

void	CWeaponMagazined::OnH_A_Chield		()
{
	if (m_bHasDifferentFireModes)
	{
		CActor	*actor = smart_cast<CActor*>(H_Parent());
		if (!actor) SetQueueSize(-1);
		else SetQueueSize(GetCurrentFireMode());
	};	
	inherited::OnH_A_Chield();
};

void	CWeaponMagazined::SetQueueSize			(int size)  
{
	m_iQueueSize = size; 
	if (m_iQueueSize == -1)
		strcpy(m_sCurFireMode, " (A)");
	else
		sprintf(m_sCurFireMode, " (%d)", m_iQueueSize);
};