
#include <comdef.h>
//VGUI_Localize003
class IVGUI_Localize003
{
public:
	const char* GameTitleToStr(const char* text)
	{
		typedef wchar_t*(__thiscall* fo)(void*, const char*);
		wchar_t* a = reinterpret_cast<fo>(((int*)(*(int*)(this)))[3])(this, text);
		_bstr_t b(a);
		const char* pTitle = b;
		return pTitle;
	};
private:
};