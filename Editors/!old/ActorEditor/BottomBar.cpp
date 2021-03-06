//---------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "BottomBar.h"
#include "UI_main.h"
#include "leftbar.h"
#include "LogForm.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TfraBottomBar *fraBottomBar=0;
//---------------------------------------------------------------------------
__fastcall TfraBottomBar::TfraBottomBar(TComponent* Owner)
    : TFrame(Owner)
{
	DEFINE_INI(fsStorage);
}
//---------------------------------------------------------------------------

void __fastcall TfraBottomBar::ClickOptionsMenuItem(TObject *Sender)
{
    TMenuItem* mi = dynamic_cast<TMenuItem*>(Sender);
    if (mi){
        mi->Checked = !mi->Checked;
        if (mi==miDrawGrid)     			psDeviceFlags.set(rsDrawGrid,mi->Checked);
        else if (mi==miRenderWithTextures)	psDeviceFlags.set(rsRenderTextures,mi->Checked);
        else if (mi==miLightScene)  		psDeviceFlags.set(rsLighting,mi->Checked);
        else if (mi==miRenderLinearFilter)	psDeviceFlags.set(rsFilterLinear,mi->Checked);
        else if (mi==miRenderEdgedFaces)	psDeviceFlags.set(rsEdgedFaces,mi->Checked);
//        else if (mi==miFog)					psDeviceFlags.set(rsFog,mi->Checked);
        else if (mi==miRealTime)			psDeviceFlags.set(rsRenderRealTime,mi->Checked);
//        else if (mi==miDODrawObjects)		psDeviceFlags.set(rsDetails,mi->Checked);
        else if (mi==miDrawSafeRect)		UI.Command(COMMAND_TOGGLE_SAFE_RECT);
        else if (mi==miRenderFillPoint)		Device.dwFillMode 	= D3DFILL_POINT;
        else if (mi==miRenderFillWireframe)	Device.dwFillMode 	= D3DFILL_WIREFRAME;
        else if (mi==miRenderFillSolid)		Device.dwFillMode 	= D3DFILL_SOLID;
        else if (mi==miRenderShadeFlat)		Device.dwShadeMode	= D3DSHADE_FLAT;
        else if (mi==miRenderShadeGouraud)	Device.dwShadeMode	=D3DSHADE_GOURAUD;
        else if (mi==miRenderHWTransform){	HW.Caps.bForceGPU_SW = !mi->Checked; UI.Resize(); }
    }
    UI.RedrawScene();
    UI.Command(COMMAND_UPDATE_TOOLBAR);
}
//---------------------------------------------------------------------------
void __fastcall TfraBottomBar::QualityClick(TObject *Sender)
{
    UI.SetRenderQuality((float)(((TMenuItem*)Sender)->Tag)/100);
    ((TMenuItem*)Sender)->Checked = true;
    UI.Resize();
}
//---------------------------------------------------------------------------
void __fastcall TfraBottomBar::fsStorageRestorePlacement(TObject *Sender)
{
	Device.dwFillMode 	= fsStorage->ReadInteger("dwFillMode",Device.dwFillMode);
	Device.dwShadeMode 	= fsStorage->ReadInteger("dwShadeMode",Device.dwShadeMode);
    // fill mode
    if (Device.dwFillMode==D3DFILL_POINT)			miRenderFillPoint->Checked 		= true;
    else if (Device.dwFillMode==D3DFILL_WIREFRAME)	miRenderFillWireframe->Checked  = true;
    else if (Device.dwFillMode==D3DFILL_SOLID) 		miRenderFillSolid->Checked		= true;
    // shade mode
    if (Device.dwShadeMode==D3DSHADE_FLAT)			miRenderShadeFlat->Checked		= true;
	else if (Device.dwShadeMode==D3DSHADE_GOURAUD)	miRenderShadeGouraud->Checked	= true;

    // hw transform
    HW.Caps.bForceGPU_SW 		= !miRenderHWTransform->Checked;

    // quality
    if 		(N200->Checked)	QualityClick(N200);
    else if (N150->Checked)	QualityClick(N150);
    else if (N125->Checked)	QualityClick(N125);
    else if (N100->Checked)	QualityClick(N100);
    else if (N75->Checked)	QualityClick(N75);
    else if (N50->Checked)	QualityClick(N50);
    else if (N25->Checked)	QualityClick(N25);
    // statistic
    ebStatClick(Sender);
}
//---------------------------------------------------------------------------

void __fastcall TfraBottomBar::fsStorageSavePlacement(TObject *Sender)
{
	fsStorage->WriteInteger("dwFillMode",Device.dwFillMode);
	fsStorage->WriteInteger("dwShadeMode",Device.dwShadeMode);
}
//---------------------------------------------------------------------------

void __fastcall TfraBottomBar::ebLogClick(TObject *Sender)
{
	TfrmLog::ChangeVisible();
}
//---------------------------------------------------------------------------


void __fastcall TfraBottomBar::ebStatClick(TObject *Sender)
{
	psDeviceFlags.set(rsStatistic,!psDeviceFlags.is(rsStatistic));
    UI.RedrawScene();
}
//---------------------------------------------------------------------------

void __fastcall TfraBottomBar::ebOptionsMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
    POINT pt;
    GetCursorPos(&pt);
    pmOptions->Popup(pt.x,pt.y);
    TExtBtn* btn = dynamic_cast<TExtBtn*>(Sender); VERIFY(btn); btn->MouseManualUp();
}
//---------------------------------------------------------------------------


void __fastcall TfraBottomBar::pmOptionsPopup(TObject *Sender)
{
    miRenderWithTextures->Checked	= psDeviceFlags.is(rsRenderTextures);
    miLightScene->Checked			= psDeviceFlags.is(rsLighting);
    miRenderLinearFilter->Checked	= psDeviceFlags.is(rsFilterLinear);
    miRenderEdgedFaces->Checked		= psDeviceFlags.is(rsEdgedFaces);
    miRealTime->Checked				= psDeviceFlags.is(rsRenderRealTime);
//    miFog->Checked					= psDeviceFlags.is(rsFog);
    miDrawGrid->Checked				= psDeviceFlags.is(rsDrawGrid);
    miDrawSafeRect->Checked			= psDeviceFlags.is(rsDrawSafeRect);
//    miDODrawObjects->Checked		= psDeviceFlags.is(rsDetails);
}
//---------------------------------------------------------------------------


