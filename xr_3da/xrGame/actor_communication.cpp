// actor_communication.cpp:	 ????? ?? PDA ? ???????
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"

#include "actor.h"

#include "UIGameSP.h"
#include "UI.h"
#include "PDA.h"
#include "HUDManager.h"
#include "level.h"
#include "string_table.h"
#include "PhraseDialog.h"
#include "character_info.h"
#include "relation_registry.h"


#include "ai_space.h"
#include "alife_simulator.h"
#include "alife_registry_container.h"
#include "alife_news_registry.h"

#include "script_game_object.h"

#include "game_cl_base.h"

#include "xrServer.h"
#include "xrServer_Objects_ALife_Monsters.h"
#include "alife_registry_wrappers.h"

#include "map_manager.h"

#include "ui/UIMainIngameWnd.h"
#include "ui/UIPdaWnd.h"
#include "ui/UIDiaryWnd.h"
#include "ui/UITalkWnd.h"
#include "game_object_space.h"
#include "script_callback_ex.h"
#include "encyclopedia_article.h"
#include "GameTaskManager.h"
#include "GameTaskdefs.h"
#include "infoportion.h"

class FindByIDPred
{
public:
	FindByIDPred(ARTICLE_ID id){object_id = id;}
	bool operator() (const ARTICLE_DATA& item)
	{
		if(item.article_id == object_id)
			return true;
		else
			return false;
	}
private:
	ARTICLE_ID object_id;
};

void CActor::AddEncyclopediaArticle	 (const CInfoPortion* info_portion) const
{
	VERIFY(info_portion);
	ARTICLE_VECTOR& article_vector = encyclopedia_registry->registry().objects();

	ARTICLE_VECTOR::iterator last_end = article_vector.end();
	ARTICLE_VECTOR::iterator B = article_vector.begin();
	ARTICLE_VECTOR::iterator E = last_end;

	for(ARTICLE_ID_VECTOR::const_iterator it = info_portion->ArticlesDisable().begin();
									it != info_portion->ArticlesDisable().end(); it++)
	{
		FindByIDPred pred(*it);
		last_end = std::remove_if(B, last_end, pred);
	}
	article_vector.erase(last_end, E);


	for(ARTICLE_ID_VECTOR::const_iterator it = info_portion->Articles().begin();
									it != info_portion->Articles().end(); it++)
	{
		FindByIDPred pred(*it);
		if( std::find_if(article_vector.begin(), article_vector.end(), pred) != article_vector.end() ) continue;

		CEncyclopediaArticle article;

		article.Load(*it);

		article_vector.push_back(ARTICLE_DATA(*it, Level().GetGameTime(), article.data()->articleType));
		LPCSTR g,n;
		g = *(article.data()->group);
		n = *(article.data()->name);
		callback(GameObject::eArticleInfo)(lua_game_object(), g, n);

		if( HUD().GetUI() ){
			CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(HUD().GetUI()->UIGame());
			pda_section::part p = pda_section::encyclopedia;
			switch (article.data()->articleType){
				case ARTICLE_DATA::eEncyclopediaArticle:	p = pda_section::encyclopedia;	break;
				case ARTICLE_DATA::eJournalArticle:			p = pda_section::journal;		break;
				case ARTICLE_DATA::eInfoArticle:			p = pda_section::info;			break;
				case ARTICLE_DATA::eTaskArticle:			p = pda_section::quests;		break;
				default: NODEFAULT;
			};
			pGameSP->PdaMenu->PdaContentsChanged			(p);
		}

	}

}


void CActor::AddGameTask			 (const CInfoPortion* info_portion) const
{
	VERIFY(info_portion);

	if(info_portion->GameTasks().empty()) return;
	for(TASK_ID_VECTOR::const_iterator it = info_portion->GameTasks().begin();
		it != info_portion->GameTasks().end(); it++)
	{
		GameTaskManager().GiveGameTaskToActor(*it);
	}
}


void  CActor::AddGameNews			 (GAME_NEWS_DATA& news_data)
{
	if(news_data.news_id != NOT_SIMULATION_NEWS)
	{
		const CALifeNews* pNewsItem  = ai().alife().news().news(news_data.news_id); VERIFY(pNewsItem);
		//????????? ? ????? ?????? ???????
		if(ALife::eNewsTypeKill == pNewsItem->m_news_type/* || ALife::eNewsTypeRetreat == pNewsItem->m_news_type*/)
		{
			CSE_Abstract* E = Level().Server->game->get_entity_from_eid(pNewsItem->m_object_id[1]);
			CSE_ALifeTraderAbstract	 *pTA	= smart_cast<CSE_ALifeTraderAbstract*>(E); 
			if(!pTA) return;
		}
		else return;
	}
		

	GAME_NEWS_VECTOR& news_vector = game_news_registry->registry().objects();
	news_data.receive_time = Level().GetGameTime();
	news_vector.push_back(news_data);

	if(HUD().GetUI()){
		HUD().GetUI()->UIMainIngameWnd->ReceiveNews(news_data);
		CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(HUD().GetUI()->UIGame());
		if(pGameSP) 
			pGameSP->PdaMenu->PdaContentsChanged	(pda_section::news);
	}
}


bool CActor::OnReceiveInfo(INFO_ID info_id) const
{
	if(!CInventoryOwner::OnReceiveInfo(info_id))
		return false;

	CInfoPortion info_portion;
	info_portion.Load(info_id);

	AddEncyclopediaArticle	(&info_portion);
	AddGameTask				(&info_portion);

	callback(GameObject::eInventoryInfo)(lua_game_object(), *info_id);

	if(!HUD().GetUI())
		return false;
	//?????? ???? ????????? ? ?????? single
	CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(HUD().GetUI()->UIGame());
	if(!pGameSP) return false;

	if(pGameSP->TalkMenu->IsShown())
	{
		pGameSP->TalkMenu->NeedUpdateQuestions();
	}


	return true;
}


void CActor::OnDisableInfo(INFO_ID info_id) const
{
//	Level().RemoveMapLocationByInfo(info_index);
	CInventoryOwner::OnDisableInfo(info_id);

	if(!HUD().GetUI())
		return;

	//?????? ???? ????????? ? ?????? single
	CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(HUD().GetUI()->UIGame());
	if(!pGameSP) return;

/* 
	//???????? ?????? ?? ?????, ???? ?? ????? ? ????? ? ?????????
	if(pGameSP->PdaMenu.UIMapWnd.IsShown())
	{
		pGameSP->PdaMenu.UIMapWnd.InitGlobalMapObjectives	();
		pGameSP->PdaMenu.UIMapWnd.InitLocalMapObjectives	();
	}
*/

	if(pGameSP->TalkMenu->IsShown())
	{
		pGameSP->TalkMenu->NeedUpdateQuestions();
	}
//	else if(pGameSP->PdaMenu.UIPdaCommunication.IsShown())
//	{
//		pGameSP->PdaMenu.UIPdaCommunication.NeedUpdateQuestions();
//	}
}



void CActor::ReceivePdaMessage(u16 who, EPdaMsg msg, INFO_ID info_id)
{
	//?????? ???? ????????? ? ?????? single
	if(!HUD().GetUI())
		return;
	CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(HUD().GetUI()->UIGame());
	if(!pGameSP) return;

	//???????????? ? ??????????
	CObject* pPdaObject =  Level().Objects.net_Find(who);
	VERIFY(pPdaObject);
	CPda* pPda = smart_cast<CPda*>(pPdaObject);
	VERIFY(pPda);
	HUD().GetUI()->UIMainIngameWnd->ReceivePdaMessage(pPda->GetOriginalOwner(), msg, info_id);


	SPdaMessage last_pda_message;
	//bool prev_msg = 
	GetPDA()->GetLastMessageFromLog(who, last_pda_message);

	//???????? ?????????? ? ????????
	UpdateContact(pPda->GetOriginalOwnerID());
	CInventoryOwner::ReceivePdaMessage(who, msg, info_id);
}

void  CActor::ReceivePhrase		(DIALOG_SHARED_PTR& phrase_dialog)
{
	//?????? ???? ????????? ? ?????? single
	CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(HUD().GetUI()->UIGame());
	if(!pGameSP) return;

	if(pGameSP->TalkMenu->IsShown())
	{
		pGameSP->TalkMenu->NeedUpdateQuestions();
	}
//	else if(pGameSP->PdaMenu.UIPdaCommunication.IsShown())
//	{
//		pGameSP->PdaMenu.UIPdaCommunication.NeedUpdateQuestions();
//	}


	CPhraseDialogManager::ReceivePhrase(phrase_dialog);
}

void   CActor::UpdateAvailableDialogs	(CPhraseDialogManager* partner)
{
	m_AvailableDialogs.clear();
	m_CheckedDialogs.clear();

	if(CInventoryOwner::m_known_info_registry->registry().objects_ptr())
	{
		for(KNOWN_INFO_VECTOR::const_iterator it = CInventoryOwner::m_known_info_registry->registry().objects_ptr()->begin();
			CInventoryOwner::m_known_info_registry->registry().objects_ptr()->end() != it; ++it)
		{
			//?????????? ??????? ?????????? ? ??????? ?? ????????
			CInfoPortion info_portion;
			info_portion.Load((*it).info_id);

			for(u32 i = 0; i<info_portion.DialogNames().size(); i++)
				AddAvailableDialog(*info_portion.DialogNames()[i], partner);
		}
	}

	//???????? ????????? ?????? ???????????
	CInventoryOwner* pInvOwnerPartner = smart_cast<CInventoryOwner*>(partner); VERIFY(pInvOwnerPartner);
	
	for(u32 i = 0; i<pInvOwnerPartner->CharacterInfo().ActorDialogs().size(); i++)
		AddAvailableDialog(pInvOwnerPartner->CharacterInfo().ActorDialogs()[i], partner);

	//???????? ????????? ??????? ??????????? ?? info portions
	if(pInvOwnerPartner->m_known_info_registry->registry().objects_ptr())
	{
		for(KNOWN_INFO_VECTOR::const_iterator it = pInvOwnerPartner->m_known_info_registry->registry().objects_ptr()->begin();
			pInvOwnerPartner->m_known_info_registry->registry().objects_ptr()->end() != it; ++it)
		{
			//?????????? ??????? ?????????? ? ??????? ?? ????????
			CInfoPortion info_portion;
			info_portion.Load((*it).info_id);
			for(u32 i = 0; i<info_portion.ActorDialogNames().size(); i++)
				AddAvailableDialog(*info_portion.ActorDialogNames()[i], partner);
		}
	}

	CPhraseDialogManager::UpdateAvailableDialogs(partner);
}

void CActor::TryToTalk()
{
	VERIFY(m_pPersonWeLookingAt);

	if(!IsTalking())
	{
		RunTalkDialog(m_pPersonWeLookingAt);
	}
}

void CActor::RunTalkDialog(CInventoryOwner* talk_partner)
{
	//?????????? ?????????? ? ????
	if(talk_partner->OfferTalk(this))
	{	
		StartTalk(talk_partner);
		//?????? ???? ????????? ? ?????? single
		CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(HUD().GetUI()->UIGame());
		if(pGameSP)
		{
			if(pGameSP->MainInputReceiver())
				Game().StartStopMenu(pGameSP->MainInputReceiver(),true);
			pGameSP->StartTalk();
		}
	}
}

void CActor::StartTalk (CInventoryOwner* talk_partner)
{
	CGameObject* GO = smart_cast<CGameObject*>(talk_partner); VERIFY(GO);
	//???????? ?????????? ? ????????
	UpdateContact(GO->ID());

	CInventoryOwner::StartTalk(talk_partner);
}

void CActor::UpdateContact		(u16 contact_id)
{
	if(ID() == contact_id) return;

	TALK_CONTACT_VECTOR& contacts = contacts_registry->registry().objects();
	for(TALK_CONTACT_VECTOR_IT it = contacts.begin(); contacts.end() != it; ++it)
		if((*it).id == contact_id) break;

	if(contacts.end() == it)
	{
		TALK_CONTACT_DATA contact_data(contact_id, Level().GetGameTime());
		contacts.push_back(contact_data);
	}
	else
	{
		(*it).time = Level().GetGameTime();
	}
}
void CActor::NewPdaContact		(CInventoryOwner* pInvOwner)
{	
	if(Game().Type() != GAME_SINGLE) return;
		
	HUD().GetUI()->UIMainIngameWnd->AnimateContacts();

	Level().MapManager().AddRelationLocation		( pInvOwner );

	if( HUD().GetUI() ){
		CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(HUD().GetUI()->UIGame());

		if(pGameSP)
			pGameSP->PdaMenu->PdaContentsChanged	(pda_section::contacts);
	}
}

void CActor::LostPdaContact		(CInventoryOwner* pInvOwner)
{

	CGameObject* GO = smart_cast<CGameObject*>(pInvOwner);
	if (GO){

		for(int t = ALife::eRelationTypeFriend; t<ALife::eRelationTypeLast; ++t){
			ALife::ERelationType tt = (ALife::ERelationType)t;
			Level().MapManager().RemoveMapLocation(RELATION_REGISTRY().GetSpotName(tt),	GO->ID());
		}
		Level().MapManager().RemoveMapLocation("deadbody_location",	GO->ID());
	};

	if( HUD().GetUI() ){
		CUIGameSP* pGameSP = smart_cast<CUIGameSP*>(HUD().GetUI()->UIGame());
		if(pGameSP){
			pGameSP->PdaMenu->PdaContentsChanged	(pda_section::contacts);
		}
	}

}

void CActor::AddGameNews_deffered	 (GAME_NEWS_DATA& news_data, u32 delay)
{
	GAME_NEWS_DATA * d = xr_new<GAME_NEWS_DATA>(news_data);
	//*d = news_data;
	m_defferedMessages.push_back( SDefNewsMsg() );
	m_defferedMessages.back().news_data = d;
	m_defferedMessages.back().time = Device.dwTimeGlobal+delay;
	std::sort(m_defferedMessages.begin(), m_defferedMessages.end() );
}
void CActor::UpdateDefferedMessages()
{
	while( m_defferedMessages.size() ){
		SDefNewsMsg& M = m_defferedMessages.back();
		if(M.time <=Device.dwTimeGlobal){
			AddGameNews(*M.news_data);		
			xr_delete(M.news_data);
			m_defferedMessages.pop_back();
		}else
			break;
	}
}
