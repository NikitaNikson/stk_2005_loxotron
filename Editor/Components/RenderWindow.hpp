// Borland C++ Builder
// Copyright (c) 1995, 2002 by Borland Software Corporation
// All rights reserved

// (DO NOT EDIT: machine generated header) 'RenderWindow.pas' rev: 6.00

#ifndef RenderWindowHPP
#define RenderWindowHPP

#pragma delphiheader begin
#pragma option push -w-
#pragma option push -Vx
#include <Dialogs.hpp>	// Pascal unit
#include <Graphics.hpp>	// Pascal unit
#include <Menus.hpp>	// Pascal unit
#include <Forms.hpp>	// Pascal unit
#include <SysUtils.hpp>	// Pascal unit
#include <Controls.hpp>	// Pascal unit
#include <StdCtrls.hpp>	// Pascal unit
#include <Messages.hpp>	// Pascal unit
#include <ExtCtrls.hpp>	// Pascal unit
#include <Classes.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------

namespace Renderwindow
{
//-- type declarations -------------------------------------------------------
class DELPHICLASS TCustomRenderWindow;
class PASCALIMPLEMENTATION TCustomRenderWindow : public Controls::TWinControl 
{
	typedef Controls::TWinControl inherited;
	
protected:
	Classes::TNotifyEvent FOnChangeFocus;
	Forms::TFormBorderStyle FBorderStyle;
	void __fastcall SetBorderStyle(Forms::TBorderStyle Value);
	virtual void __fastcall CreateParams(Controls::TCreateParams &Params);
	virtual void __fastcall CreateWindowHandle(const Controls::TCreateParams &Params);
	void __fastcall ChangeFocus(bool p);
	__property ParentColor  = {default=0};
	__property Forms::TBorderStyle BorderStyle = {read=FBorderStyle, write=SetBorderStyle, default=1};
	
public:
	__fastcall virtual TCustomRenderWindow(Classes::TComponent* AOwner);
	virtual void __fastcall DefaultHandler(void *Message);
	
__published:
	__property TabStop  = {default=1};
	__property Classes::TNotifyEvent OnChangeFocus = {read=FOnChangeFocus, write=FOnChangeFocus};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TCustomRenderWindow(HWND ParentWindow) : Controls::TWinControl(ParentWindow) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TWinControl.Destroy */ inline __fastcall virtual ~TCustomRenderWindow(void) { }
	#pragma option pop
	
};


class DELPHICLASS TD3DWindow;
class PASCALIMPLEMENTATION TD3DWindow : public TCustomRenderWindow 
{
	typedef TCustomRenderWindow inherited;
	
private:
	Classes::TNotifyEvent FOnPaint;
	HIDESBASE MESSAGE void __fastcall WMPaint(Messages::TWMPaint &Message);
	
protected:
	virtual void __fastcall Paint(void);
	
public:
	__fastcall virtual TD3DWindow(Classes::TComponent* AOwner);
	
__published:
	__property Align  = {default=0};
	__property BorderStyle  = {default=1};
	__property Color  = {default=-2147483643};
	__property Enabled  = {default=1};
	__property Font ;
	__property PopupMenu ;
	__property ShowHint ;
	__property TabOrder  = {default=-1};
	__property Visible  = {default=1};
	__property OnKeyDown ;
	__property OnKeyPress ;
	__property OnKeyUp ;
	__property OnMouseDown ;
	__property OnMouseMove ;
	__property OnMouseUp ;
	__property OnResize ;
	__property Classes::TNotifyEvent OnPaint = {read=FOnPaint, write=FOnPaint};
public:
	#pragma option push -w-inl
	/* TWinControl.CreateParented */ inline __fastcall TD3DWindow(HWND ParentWindow) : TCustomRenderWindow(ParentWindow) { }
	#pragma option pop
	#pragma option push -w-inl
	/* TWinControl.Destroy */ inline __fastcall virtual ~TD3DWindow(void) { }
	#pragma option pop
	
};


//-- var, const, procedure ---------------------------------------------------
#define RWStyle (System::Set<Controls__11, csAcceptsControls, csMenuEvents> () << Controls__11(0) << Controls__11(1) << Controls__11(3) << Controls__11(6) << Controls__11(11) )

}	/* namespace Renderwindow */
using namespace Renderwindow;
#pragma option pop	// -w-
#pragma option pop	// -Vx

#pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// RenderWindow
