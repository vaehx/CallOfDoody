#pragma once

#include <SpeedPoint.h>
#include <EntitySystem\IEntity.h>
#include <Renderer\IFont.h>
#include <3DEngine\IParticleSystem.h>

using namespace SpeedPoint;

class CPlayer;

struct SAmmoInfo
{
	unsigned int current;
	unsigned int available;
	unsigned int magazineCapacity;
};

class CWeapon : public IComponent
{
public:
	DEFINE_COMPONENT("Weapon");

private:
	CPlayer* m_pPlayer;
	
	CParticleEmitter* m_pExplosionEmitter;
	CPointHelper* m_pExplosionHelper;
	bool m_bHelpersShown;

	SAmmoInfo m_Ammo;
	SFontRenderSlot* m_pAmmoText;

public:
	CWeapon();

	void Init(CPlayer* m_pPlayer);
	bool CanShoot() const;
	void Shoot();
	void Reload();
	void Update(float fTime);

	SAmmoInfo& GetAmmoInfo() { return m_Ammo; }

	void ShowHelpers(bool show);
	bool AreHelpersShown() const { return m_bHelpersShown; }
	
	CPlayer* GetPlayer() const { return m_pPlayer; };
	virtual void OnRelease();
};
