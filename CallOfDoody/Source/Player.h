#pragma once

#include <EntitySystem\Implementation\Entity.h>
#include <EntitySystem\IEntityClass.h>
#include <3DEngine\ITerrain.h>
#include <Common\Camera.h>
#include <Common\geo.h>

using namespace SpeedPoint;

class CWeapon;

struct SPlayerParameters
{
	Vec3f pos;
	float height;
	float diameter;
};

class CPlayer : public IComponent
{
public:
	DEFINE_COMPONENT("Player");

private:
	SPlayerParameters m_Params;
	SCamera m_Camera;
	Vec3f m_Forward; // player forward != camera forward

	bool m_bCamMoved; // Camera was moved last frame
	bool m_bOnFloor;
	bool m_bFreeCam;

	CWeapon* m_pWeapon;
	float m_ShootCooldown; // seconds
	Vec3f m_WeaponOrigPosition;
	Quat m_WeaponOrigRotation;
	bool m_bReloadKeyPressed;

	void HandleMovement(float deltaTime, int viewportOffset[2], int viewportSz[2]);
	void HandleActions(float deltaTime);
	void HandleShooting(float deltaTime);
	void HandleReload(float deltaTime);

public:
	CPlayer();

	// pos - the very bottom of the player
	// height - distance from bottom pos to camera
	// diameter - player width will be the diameter of the capsule. The greater, the thicker you are.
	void Setup(const SPlayerParameters& params);

	// Reset to initial player parameters (set on first call of Setup())
	void Reset();

	void Update(float fTime, int viewportOffs[2], int viewportSz[2]);

	SCamera* GetCamera();
	const SPlayerParameters& GetParams() const { return m_Params; }
	bool CamMovedLastFrame() const;
	CWeapon* GetEquippedWeapon() const;
	
	bool EnableFreeCam(bool freeCamEnabled);
	bool IsFreeCamEnabled() const { return m_bFreeCam; }

	// IComponent:
public:
	virtual void OnEntityTransformed();
	virtual void OnRelease();
};

class CPlayerEntityClass : public IEntityClass
{
public:
	virtual const char* GetName() const { return "Player"; };
	virtual bool Apply(IEntity* entity);
};
