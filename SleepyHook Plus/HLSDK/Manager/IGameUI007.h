class IGameUI007
{
public:
	int GameMessageBox(const wchar_t* title, const wchar_t* text)
	{
		typedef int(__thiscall* fo)(void*, const wchar_t* title, const wchar_t* text);
		return reinterpret_cast<fo>(((int*)(*(int*)(this)))[41])(this, title, text);
	};
	int GameMessageBox(const char* title, const char* text)
	{
		typedef int(__thiscall* fo)(void*, const char* title, const char* text);
		return reinterpret_cast<fo>(((int*)(*(int*)(this)))[42])(this, title, text);
	};

private:
};