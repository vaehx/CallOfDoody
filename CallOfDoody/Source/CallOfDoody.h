#pragma once

#include "Player.h"
#include <SpeedPoint.h>
#include <EntitySystem\Implementation\Entity.h>
#include <GameFramework\IApplication.h>
#include <Windows.h>

using namespace SpeedPoint;

class CallOfDoody : public IApplication, public ILogListener
{
private:
	HINSTANCE m_hInstance;
	HWND m_hWndViewport;
	HWND m_hWndLog;

	CPlayer* m_pPlayer;
	//CEntity m_ActorEntity;

	Vec3f m_LastTerrainUpdatePlayerPos;

public:
	CallOfDoody(HINSTANCE hInstance, HWND hWndViewport, HWND logWnd);

	HWND GetHWND() { return m_hWndViewport; }

	CPlayer* GetPlayer() const { return m_pPlayer; }

	// IApplication:
public:
	virtual void OnInit(IGameEngine* pGameEngine);
	virtual void Update(float fLastFrameTime);
	virtual void Render();

public: // ILogListener:
	virtual void OnLog(SResult res, const string& msg);
};
