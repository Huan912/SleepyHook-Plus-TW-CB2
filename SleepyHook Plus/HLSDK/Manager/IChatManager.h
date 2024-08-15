
class IChatManager
{
public:
	int MessageToChat(const wchar_t* Message, int channel = 1)
	{
		typedef int(__thiscall* fo)(void*, int pChannel, const wchar_t* pMessage);
		return reinterpret_cast<fo>(((int*)(*(int*)(this)))[2])(this, channel, Message);
	};

private:
};