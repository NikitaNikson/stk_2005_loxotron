#include "stdafx.h"
#include "UIEventsWnd.h"
#include "UIFrameWindow.h"
#include "UIFrameLineWnd.h"
#include "UIXmlInit.h"
#include "UIAnimatedStatic.h"
#include "UIMapWnd.h"
#include "UIScrollView.h"
#include "UITabControl.h"
#include "UITaskDescrWnd.h"
#include "../MainUI.h"
#include "../HUDManager.h"
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include "../level.h"
#include "../actor.h"
#include "../gametaskmanager.h"
#include "../gametask.h"
#include "../map_manager.h"
#include "../map_location.h"
#include "../string_table.h"
#include "UITaskItem.h"
#include "../alife_registry_wrappers.h"
#include "../encyclopedia_article.h"

CUIEventsWnd::CUIEventsWnd			()
{
	m_flags.zero			();
}

CUIEventsWnd::~CUIEventsWnd			()
{
	delete_data			(m_UIMapWnd);
	delete_data			(m_UITaskInfoWnd);
}

void CUIEventsWnd::Init				()
{
	CUIXml uiXml;
	bool xml_result					= uiXml.Init(CONFIG_PATH, UI_PATH, "pda_events.xml");
	R_ASSERT3						(xml_result, "xml file not found", "pda_events.xml");

	CUIXmlInit xml_init;
	xml_init.InitWindow				(uiXml, "main_wnd", 0, this);


	m_UILeftFrame					= xr_new<CUIFrameWindow>(); m_UILeftFrame->SetAutoDelete(true);
	AttachChild						(m_UILeftFrame);
	xml_init.InitFrameWindow		(uiXml, "main_wnd:left_frame", 0, m_UILeftFrame);


	m_UILeftHeader					= xr_new<CUIFrameLineWnd>(); m_UILeftHeader->SetAutoDelete(true);
	m_UILeftFrame->AttachChild		(m_UILeftHeader);
	xml_init.InitFrameLine			(uiXml, "main_wnd:left_frame:left_frame_header", 0, m_UILeftHeader);

	xml_init.InitAutoStaticGroup	(uiXml, "main_wnd:left_frame",m_UILeftFrame);

	m_UIAnimation					= xr_new<CUIAnimatedStatic>(); m_UIAnimation->SetAutoDelete(true);
	xml_init.InitAnimatedStatic		(uiXml, "main_wnd:left_frame:left_frame_header:anim_static", 0, m_UIAnimation);
	m_UILeftHeader->AttachChild		(m_UIAnimation);

	m_UIRightWnd					= xr_new<CUIWindow>(); m_UIRightWnd->SetAutoDelete(true);
	AttachChild						(m_UIRightWnd);
	xml_init.InitWindow				(uiXml, "main_wnd:right_frame", 0, m_UIRightWnd);

	m_UIMapWnd						= xr_new<CUIMapWnd>(); m_UIMapWnd->SetAutoDelete(false);
	m_UIMapWnd->Init				("pda_events.xml","main_wnd:right_frame:map_wnd");

	m_UITaskInfoWnd					= xr_new<CUITaskDescrWnd>(); m_UITaskInfoWnd->SetAutoDelete(false);
	m_UITaskInfoWnd->Init			(&uiXml,"main_wnd:right_frame:task_descr_view");
	

	m_ListWnd						= xr_new<CUIScrollView>(); m_ListWnd->SetAutoDelete(true);
	m_UILeftFrame->AttachChild		(m_ListWnd);
	xml_init.InitScrollView			(uiXml, "main_wnd:left_frame:list", 0, m_ListWnd);

	m_TaskFilter					= xr_new<CUITabControl>(); m_TaskFilter->SetAutoDelete(true);
	m_UILeftHeader->AttachChild		(m_TaskFilter);
	xml_init.InitTabControl			(uiXml, "main_wnd:left_frame:left_frame_header:filter_tab", 0, m_TaskFilter);
	m_TaskFilter->SetWindowName		("filter_tab");
	Register						(m_TaskFilter);
   AddCallback						("filter_tab",TAB_CHANGED,boost::bind(&CUIEventsWnd::OnFilterChanged,this,_1,_2));

   m_currFilter						= eActiveTask;
   SetDescriptionMode				(true);
}

void CUIEventsWnd::Update			()
{
	if(m_flags.test(flNeedReload) ){
		ReloadList(false);
		m_flags.set(flNeedReload,FALSE );
	}
	inherited::Update		();
}

void CUIEventsWnd::Draw				()
{
	inherited::Draw			();
}

void	CUIEventsWnd::SendMessage			(CUIWindow* pWnd, s16 msg, void* pData)
{
	CUIWndCallback::OnEvent(pWnd, msg, pData);
}

void CUIEventsWnd::OnFilterChanged			(CUIWindow* w, void*)
{
	m_currFilter			=(ETaskFilters)m_TaskFilter->GetActiveIndex();
	ReloadList				(false);
	if(!GetDescriptionMode())
		SetDescriptionMode		(true);
}

void CUIEventsWnd::Reload					()
{
		m_flags.set(flNeedReload,TRUE );
}

void CUIEventsWnd::ReloadList				(bool bClearOnly)
{
	m_ListWnd->Clear	();
	if(bClearOnly)		return;

	if(!Actor()) return;

	GameTasks& tasks = Actor()->GameTaskManager().GameTasks();
	GameTasks::reverse_iterator it =  tasks.rbegin();
	CGameTask* task = NULL;
	for(;it!=tasks.rend();++it){
		task = (*it).game_task;
		R_ASSERT(task);
		R_ASSERT(task->m_Objectives.size() > 0);

		if( !Filter(task) ) continue;

		CStringTable		stbl;
		CUITaskItem			*pTaskItem = NULL;
		if(task->m_Objectives[0].TaskState()==eTaskUserDefined){
			VERIFY(task->m_Objectives.size()==1);
			pTaskItem = xr_new<CUIUserTaskItem>(this);
			pTaskItem->SetGameTask			(task, 0);
			m_ListWnd->AddWindow			(pTaskItem);
		}else
		for (u32 i = 0; i < task->m_Objectives.size(); ++i)
		{
			if(i==0)
				pTaskItem = xr_new<CUITaskRootItem>(this);
			else
				pTaskItem = xr_new<CUITaskSubItem>(this);

			pTaskItem->SetGameTask			(task, i);
			m_ListWnd->AddWindow			(pTaskItem);
		}

	}

}
/*int i=0;*/
void CUIEventsWnd::Show					(bool status)
{
/*	if(i==0&&status){
		CActor *pActor = smart_cast<CActor*>(Level().CurrentEntity());
		pActor->encyclopedia_registry->registry().objects().push_back(ARTICLE_DATA("user_task_article", Level().GetGameTime(), ARTICLE_DATA::eTaskArticle));
		pActor->encyclopedia_registry->registry().objects().push_back(ARTICLE_DATA("user_task_article_1", Level().GetGameTime(), ARTICLE_DATA::eTaskArticle));
		pActor->encyclopedia_registry->registry().objects().push_back(ARTICLE_DATA("user_task_article_2", Level().GetGameTime(), ARTICLE_DATA::eTaskArticle));
		pActor->encyclopedia_registry->registry().objects().push_back(ARTICLE_DATA("user_task_article_3", Level().GetGameTime(), ARTICLE_DATA::eTaskArticle));
		pActor->encyclopedia_registry->registry().objects().push_back(ARTICLE_DATA("user_task_article_4", Level().GetGameTime(), ARTICLE_DATA::eTaskArticle));
		pActor->encyclopedia_registry->registry().objects().push_back(ARTICLE_DATA("user_task_article_5", Level().GetGameTime(), ARTICLE_DATA::eTaskArticle));
		pActor->encyclopedia_registry->registry().objects().push_back(ARTICLE_DATA("user_task_article_6", Level().GetGameTime(), ARTICLE_DATA::eTaskArticle));
		i=1;
	};
*/

	inherited::Show			(status);
	m_UIMapWnd->Show		(status);
	m_UITaskInfoWnd->Show	(status);

	ReloadList				(status == false);

}

bool CUIEventsWnd::Filter(CGameTask* t)
{
	ETaskState task_state = t->m_Objectives[0].TaskState();
	
	return (m_currFilter==eActiveTask			&& task_state==eTaskStateInProgress )||
			(m_currFilter==eAccomplishedTask	&& task_state==eTaskStateCompleted )||
			(m_currFilter==eFailedTask			&& task_state==eTaskStateFail )||
			(m_currFilter==eOwnTask				&& task_state==eTaskUserDefined );
}


void CUIEventsWnd::SetDescriptionMode		(bool bMap)
{
	if(bMap){
		m_UIRightWnd->DetachChild		(m_UITaskInfoWnd);
		m_UIRightWnd->AttachChild		(m_UIMapWnd);
	}else{
		m_UIRightWnd->DetachChild		(m_UIMapWnd);
		m_UIRightWnd->AttachChild		(m_UITaskInfoWnd);
	}
	m_flags.set(flMapMode, bMap);
}

bool CUIEventsWnd::GetDescriptionMode		()
{
	return !!m_flags.test(flMapMode);
}

void CUIEventsWnd::ShowDescription			(CGameTask* t, int idx)
{
	SGameTaskObjective& o		= t->Objective(idx);
	if(GetDescriptionMode()){//map
		CMapLocation* ml = o.HasMapLocation();
		if(ml&&ml->SpotEnabled())
			m_UIMapWnd->SetTargetMap(ml->LevelName(), ml->Position(), true);
	}else{//articles
		m_UITaskInfoWnd->ClearAll	();

	if(Actor()->encyclopedia_registry->registry().objects_ptr())
	{
		string512	need_group;
		if(0==idx){
			strcpy(need_group,*t->m_ID);
		}else
		if(o.article_key.size()){
			sprintf(need_group, "%s/%s", *t->m_ID, *o.article_key);
		}else{
			sprintf(need_group, "%s/%d", *t->m_ID, idx);
		}

		ARTICLE_VECTOR::const_iterator it = Actor()->encyclopedia_registry->registry().objects_ptr()->begin();
		for(; it != Actor()->encyclopedia_registry->registry().objects_ptr()->end(); it++)
		{
			if (ARTICLE_DATA::eTaskArticle == it->article_type)
			{
				CEncyclopediaArticle	A;
				A.Load					(it->article_id);

				if(strstr(*(A.data()->group), need_group)== *(A.data()->group))
					m_UITaskInfoWnd->AddArticle(&A);
			}else
			if(o.article_id.size() && it->article_id ==o.article_id){
				CEncyclopediaArticle			A;
				A.Load							(it->article_id);
				m_UITaskInfoWnd->AddArticle		(&A);
			}
		}
	}
	}

	int sz = m_ListWnd->GetSize		();
	for(int i=0; i<sz;++i){
		CUITaskItem* itm = (CUITaskItem*)m_ListWnd->GetItem(i);
		if((itm->GameTask()==t) && (itm->ObjectiveIdx()==idx) )	
			itm->MarkSelected	(true);
		else
			itm->MarkSelected	(false);
	}

}
