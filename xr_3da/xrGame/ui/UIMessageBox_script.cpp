#include "stdafx.h"
#include "UIMessageBox.h"
#include "UIMessageBoxEx.h"
#include "../script_space.h"

using namespace luabind;

void CUIMessageBox::script_register(lua_State *L)
{
	module(L)
	[
		class_<CUIMessageBox,CUIStatic>("CUIMessageBox")
		.def(					constructor<>())
		.def("Init",	&CUIMessageBox::Init)
		.def("SetText",	&CUIMessageBox::SetText),

		class_<CUIMessageBoxEx, CUIDialogWnd>("CUIMessageBoxEx")
		.def(constructor<>())
		.def("Init",	&CUIMessageBoxEx::Init)
//		.enum_("style")
//		[
//			value("STYLE_OK",					int(CUIMessageBox::MESSAGEBOX_OK)),
///			value("STYLE_YES_NO",				int(CUIMessageBox::MESSAGEBOX_YES_NO)),
//			value("STYLE_YES_NO_CANCEL",		int(CUIMessageBox::MESSAGEBOX_YES_NO_CANCEL))
//		]
//		.def("SetText",					(void(CUIMessageBox::*)(LPCSTR))CUIMessageBox::SetText)
//		.def("Show",					(void(CUIMessageBox::*)(void))CUIMessageBox::Show)
//		.def("Hide",					&CUIMessageBox::Hide)
//		.def("AutoCenter",				&CUIMessageBox::AutoCenter)
//		.def("SetStyle",				&CUIMessageBox::SetStyle_script)
//		.def("",					&CUIMessageBox::)

	];

}