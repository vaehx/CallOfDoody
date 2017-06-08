#include "Weapon.h"
#include "Player.h"
#include <3DEngine\IParticleSystem.h>
#include <Renderer\IResourcePool.h>

CWeapon::CWeapon()
	: m_pPlayer(0),
	m_pAmmoText(0)
{
}

void CWeapon::Init(CPlayer* pPlayer)
{
	IResourcePool* pResources = SpeedPointEnv::GetEngine()->GetResources();
	I3DEngine* p3DEngine = SpeedPointEnv::Get3DEngine();

	m_pPlayer = pPlayer;

	m_Ammo.available = 250;
	m_Ammo.magazineCapacity = 50;
	m_Ammo.current = m_Ammo.magazineCapacity;

	IViewport* pViewport = p3DEngine->GetRenderer()->GetTargetViewport();
	SIZE viewportSz = pViewport->GetSize();

	m_pAmmoText = p3DEngine->GetRenderer()->GetFontRenderSlot();
	m_pAmmoText->alignRight = false;
	m_pAmmoText->screenPos[0] = viewportSz.cx - 250;
	m_pAmmoText->screenPos[1] = viewportSz.cy - 180;
	m_pAmmoText->fontSize = eFONTSIZE_LARGE;
	m_pAmmoText->color = SColor::White();
	m_pAmmoText->keep = true;
	m_pAmmoText->render = true;
	m_pAmmoText->text = "??";

	SParticleEmitterParams emitterParams;
	emitterParams.numConcurrentParticles = 20;
	emitterParams.particleMaxDistance = 0.3f;
	emitterParams.particleSpeed = 2.5f;
	emitterParams.particleSize = 0.15f;
	emitterParams.particleTexture = pResources->GetTexture("/textures/particles/fire.png");
	emitterParams.position = Vec3f(0.2f, 0.0f, -1.0f); // relative to entity
	emitterParams.spawnAutomatically = false;

	m_pExplosionEmitter = m_pEntity->AddComponent(p3DEngine->GetParticleSystem()->CreateEmitter(emitterParams));

	m_pExplosionHelper = p3DEngine->AddHelper<CPointHelper>(CPointHelper::Params(emitterParams.position));
	m_pExplosionHelper->Hide();
}

void CWeapon::OnRelease()
{
	SpeedPointEnv::GetRenderer()->ReleaseFontRenderSlot(&m_pAmmoText);
	
	SP_SAFE_RELEASE(m_pExplosionHelper);
	m_pPlayer = 0;
}

bool CWeapon::CanShoot() const
{
	return m_Ammo.current > 0;
}

void CWeapon::Shoot()
{
	if (!CanShoot())
		return;
	
	for (unsigned int i = 0; i < 5; ++i)
		m_pExplosionEmitter->SpawnParticle();

	m_Ammo.current--;
}

void CWeapon::Reload()
{
	m_Ammo.current = min(m_Ammo.magazineCapacity - m_Ammo.current, m_Ammo.available);
	m_Ammo.available -= m_Ammo.current;
}

void CWeapon::Update(float fTime)
{
	if (m_bHelpersShown)
	{
		Vec3f worldExplosionPos = (GetEntity()->GetTransform() * Vec4f(m_pExplosionEmitter->GetPos(), 1.0f)).xyz();
		m_pExplosionHelper->SetPos(worldExplosionPos);
	}

	m_pAmmoText->text = string("Ammo: ") + std::to_string(m_Ammo.current) + "/" + std::to_string(m_Ammo.available);
}

void CWeapon::ShowHelpers(bool show)
{
	m_bHelpersShown = show;
	m_pExplosionHelper->Show(m_bHelpersShown);
}
