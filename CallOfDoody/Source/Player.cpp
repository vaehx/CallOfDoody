#include "Player.h"
#include "Weapon.h"
#include <3DEngine\ITerrain.h>
#include <3DEngine\I3DEngine.h>
#include <3DEngine\IParticleSystem.h>
#include <Renderer\IResourcePool.h>
#include <GameFramework\IGameEngine.h>
#include <GameFramework\Implementation\SPMManager.h>
#include <Common\Camera.h>

using namespace SpeedPoint;

/////////////////////////////////////////////////////////////////////////////////

CPlayer::CPlayer()
	: m_bFreeCam(false),
	m_bReloadKeyPressed(false)
{
}

/////////////////////////////////////////////////////////////////////////////////

void CPlayer::Setup(const SPlayerParameters& params)
{
	PhysObject* pPhysObject;
	assert(m_pEntity);
	assert(pPhysObject = m_pEntity->GetComponent<PhysObject>());

	m_Params = params;

	// Setup physical component
	float radius = 0.5f * m_Params.diameter;
	Vec3f p1(0, m_Params.diameter, 0), p2(0, m_Params.height - m_Params.diameter, 0);

	pPhysObject->SetProxy(geo::capsule(p1, p2, radius));
	pPhysObject->SetBehavior(ePHYSOBJ_BEHAVIOR_LIVING);
	pPhysObject->SetMass(80.0f);

	m_pEntity->SetPos(Vec3f(0));
	m_Camera.LookAt(m_Camera.position + Vec3f(0, 0, 1.0f));
	m_Camera.RecalculateViewMatrix();

	m_pEntity->SetPos(params.pos);

	//m_bFreeCam = true;

	
	// Setup weapon:
	I3DEngine* p3DEngine = SpeedPointEnv::GetEngine()->Get3DEngine();

	IEntity* pWeaponEntity = SpeedPointEnv::GetScene()->SpawnEntity("player_weapon");

	SPMManager* pSPMMgr = SpeedPointEnv::GetEngine()->GetSPMManager();
	CRenderMesh* pWeaponMesh = pWeaponEntity->AddComponent(p3DEngine->CreateMesh());
	pWeaponMesh->Init(pSPMMgr->LoadAsGeometry("/models/weapons/mp42.spm"));

	pWeaponEntity->SetPos(Vec3f(-0.25f, -0.30f, 0.6f));
	//pWeaponEntity->SetRotation(Quat::FromEuler(Vec3f(-SP_PI * 0.5f, 0, SP_PI)));
	//pWeaponEntity->SetRotation(Quat::FromEuler(Vec3f(-SP_PI * 0.5f, SP_PI, SP_PI)));
	pWeaponEntity->SetRotation(Quat::FromEuler(Vec3f(0, SP_PI, 0)));
	m_pEntity->AddChild(pWeaponEntity);

	m_pWeapon = pWeaponEntity->CreateComponent<CWeapon>();
	m_pWeapon->Init(this);

	m_ShootCooldown = 0.0f;
	m_WeaponOrigPosition = pWeaponEntity->GetPos();
	m_WeaponOrigRotation = pWeaponEntity->GetRotation();
}

/////////////////////////////////////////////////////////////////////////////////

void CPlayer::OnRelease()
{
	// Weapon instance will be deleted by the weapon entity
	m_pWeapon = 0;
}

/////////////////////////////////////////////////////////////////////////////////

void CPlayer::Reset()
{
	Setup(m_Params);
}

/////////////////////////////////////////////////////////////////////////////////

SCamera* CPlayer::GetCamera()
{
	return &m_Camera;
}

/////////////////////////////////////////////////////////////////////////////////

bool CPlayer::CamMovedLastFrame() const
{
	return m_bCamMoved;
}

/////////////////////////////////////////////////////////////////////////////////
void CPlayer::OnEntityTransformed()
{
	assert(m_pEntity);

	if (!m_bFreeCam)
		m_Camera.position = m_pEntity->GetPos() + Vec3f(0, m_Params.height - 0.15f, 0);
}

/////////////////////////////////////////////////////////////////////////////////
bool CPlayer::EnableFreeCam(bool enable)
{
	return (m_bFreeCam = enable);
}

/////////////////////////////////////////////////////////////////////////////////

void CPlayer::Update(float deltaTime, int viewportOffset[2], int viewportSz[2])
{
	HandleMovement(deltaTime, viewportOffset, viewportSz);
	HandleActions(deltaTime);

	m_pWeapon->Update(deltaTime);
}

/////////////////////////////////////////////////////////////////////////////////

void CPlayer::HandleMovement(float fTime, int viewportOffset[2], int viewportSz[2])
{
	PhysObject* pPhysObject = m_pEntity->GetComponent<PhysObject>();
	SPhysObjectState* pState = pPhysObject->GetState();

	m_bOnFloor = pState->livingOnGround;

	// --------------------------- Keyboard Input --------------------------------

	const Mat44& camMtx = m_Camera.viewMatrix;
	Vec3f left = Vec3f(camMtx._11, 0.0f, camMtx._31).Normalized();
	Vec3f forward = m_Camera.GetForward();
	forward = Vec3f(forward.x, 0, forward.z).Normalized();

	float step = 8200.0f;
	if (!m_bOnFloor && !m_bFreeCam)
		step *= 0.f;

	if (GetAsyncKeyState(VK_SHIFT) < 0)
		step *= 2.5f;

	step *= fTime;

	SVector3 vMove;
	bool anyKeyPressed = false;
	if (GetAsyncKeyState(0x41) < 0) { // A
		vMove -= left * step;
		anyKeyPressed = true;
	}

	if (GetAsyncKeyState(0x44) < 0) { // D		
		vMove += left  * step;
		anyKeyPressed = true;
	}

	if (GetAsyncKeyState(0x57) < 0) { // W
		vMove -= forward * step;
		anyKeyPressed = true;
	}

	if (GetAsyncKeyState(0x53) < 0) { // S
		vMove += forward * step;
		anyKeyPressed = true;
	}

	// Up/Down movement when in free-cam mode
	if (m_bFreeCam)
	{
		Vec3f up(0, 1.0f, 0);
		if (GetAsyncKeyState(0x45) < 0) { // E
			vMove += up * step;
			anyKeyPressed = true;
		}

		if (GetAsyncKeyState(0x51) < 0) { // Q
			vMove -= up * step;
			anyKeyPressed = true;
		}
	}

	const float fCmpEpsilon = 0.01f; // threshold to consider camera moved
	m_bCamMoved = (fabsf(vMove.x) > fCmpEpsilon || fabsf(vMove.y) > fCmpEpsilon || fabsf(vMove.z) > fCmpEpsilon);

	if (m_bFreeCam)
	{
		m_Camera.position += vMove * 0.001f;
	}

	float MAX_SPEED = 3.6f;
	if (GetAsyncKeyState(VK_SHIFT) < 0)
		MAX_SPEED = 5.8f;

	if (anyKeyPressed)
	{
		pState->livingMoves = true;

		// Accelerate
		pState->F = Vec3f(0);
		pState->P += Vec3f(vMove.x, 0, vMove.z);

		// Limit speed of player
		Vec3f velocity = pState->P * pState->Minv;
		if (Vec2Length(Vec2f(velocity.x, velocity.z)) > MAX_SPEED)
		{
			Vec2f vel(velocity.x, velocity.z);
			vel = Vec2Normalize(vel) * MAX_SPEED;
			velocity.x = vel.x;
			velocity.z = vel.y;

			pState->P = velocity * pState->M;
		}
	}
	else
	{
		// Apply anti-acceleration to slow down player
		//pState->P.x = -0.5f * pState->P.x / fTime;
		//pState->P.z = -0.5f * pState->P.z / fTime;

		pState->F = Vec3f(0);
		pState->livingMoves = false;

		//if (Vec2Length(Vec2f(velocity.x, velocity.z)) <= 20.0f)
		//{
		//	pState->P = Vec3f(0, pState->P.y, 0);
		//	pState->v = Vec3f(0, pState->v.y, 0);
		//}
	}


	// Jumping:
	if (m_bOnFloor && GetAsyncKeyState(VK_SPACE) < 0)
	{
		// Friction when jumping off
		pState->P -= 0.2f * Vec3f(pState->P.x, 0, pState->P.z);

		pState->P.y += 300.0f/* * deltaTime*/;
		pState->v = pState->P * pState->Minv;
	}


	// --------------------------- Mouse Input --------------------------------	

	const float xDiff = 0.06f;
	const float yDiff = 0.06f;

	POINT MousePos;
	::GetCursorPos(&MousePos);

	float xMouse = (float)((MousePos.x - viewportOffset[0]) - viewportSz[0] / 2);
	float yMouse = (float)((MousePos.y - viewportOffset[1]) - viewportSz[1] / 2);

	Vec3f vRot;
	if (xMouse < 0) vRot.y += xMouse * xDiff * fTime;
	if (xMouse > 0) vRot.y += xMouse * xDiff * fTime;

	forward = m_Camera.GetForward();
	float dot = Vec3Dot(forward, Vec3f(0, 1.0f, 0));

	if (yMouse < 0)
	{
		if (dot > -0.96f)
			vRot.x += yMouse * yDiff * fTime;
	}

	if (yMouse > 0)
	{
		if (dot < 0.96f)
			vRot.x += yMouse * yDiff * fTime;
	}

	// Reset cursor to middle of viewport
	if (!::SetCursorPos((int)(viewportOffset[0] + viewportSz[0] * 0.5f), (int)(viewportOffset[1] + viewportSz[1] * 0.5f)))
		return;

	// Assign	
	m_Camera.Turn(vRot.y, -vRot.x);
	m_pEntity->Rotate(Quat::FromRotationY(-vRot.y));
	m_pEntity->Rotate(Quat::FromAxisAngle(m_pEntity->GetLeft(), vRot.x));
}

/////////////////////////////////////////////////////////////////////////////////

void CPlayer::HandleActions(float deltaTime)
{
	HandleShooting(deltaTime);
	HandleReload(deltaTime);
}

void CPlayer::HandleShooting(float deltaTime)
{
	I3DEngine* p3DEngine = SpeedPointEnv::GetEngine()->Get3DEngine();
	IResourcePool* pResources = SpeedPointEnv::GetEngine()->GetResources();

	// Shooting
	const float SHOOT_COOLDOWN = 0.1f; // sec
	
	float oldShootCooldown = m_ShootCooldown;
	m_ShootCooldown = max(0.0f, m_ShootCooldown - deltaTime);

	bool bFireKeyPressed = GetAsyncKeyState(VK_LBUTTON) < 0;
	if (bFireKeyPressed && m_ShootCooldown <= 0.0f && m_pWeapon->CanShoot())
	{
		// Shoot!
		m_ShootCooldown = SHOOT_COOLDOWN; // 100ms cooldown

		m_pWeapon->Shoot();


		// Trace shooting line

		//
		// 1. Create bullet and Line-Object to intersect with the world
		//

		/*

		void Player::Shoot(const Vec3f& direction)
		{
			CEntity bulletEntity;
			Bullet* bullet = bulletEntity.CreateComponent<Bullet>();
			bullet->Init();
			bullet->Shoot();
		}

		class Bullet : public IComponent
		{
		private:
			CPhysObject* m_pRay;
		public:
			void Init()
			{
				SPhysObjectParams physObjectParams;
				physObjectParams.collisionShape = SLineSegment(m_Weapon.GetPos(), m_Weapon.GetPos() + m_Weapon.GetForward() * 1000.0f);
				m_pRay = m_pEntity->AddComponent(m_pPhysics->CreatePhysObject(physObjectParams));
			}

			void Shoot()
			{
				m_pPhysics->IntersectWithScene(m_pRay);
			}
		}

		*/

		//
		// 2. Broad-phase test:
		//		- Test line against all bound boxes
		//
		// 3. Narrow-phase test:
		//		- Test line against individual objects
		//			- collect objects that actually DO intersect with the line
		//			- determine closest of these objects
		//
	
		/*

		bool Physics::IntersectWithScene(CPhysObject* pObject)
		{
			// Broadphase:
			vector<CPhysObject*> candidates;
			IntersectWithSceneBroadphase(pObject, &candidates);

			if (candidates.empty())
				return false;

			// Narrow phase:
			struct SCollidedObject
			{
				CPhysObject* pObject;
				SContactInfo contact;
			};

			vector<SCollidedObject> collisions;
			for (auto itCandidate = candidates.begin(); itCandidate != candidates.end(); ++itCandidate)
			{
				SContactInfo contact;
				if (Intersect(pObject, *itCandidate, &contact))
					collisions.push_back(SCollidedObject(*itCandidate, contact));
			}

			if (collisions.empty())
				return false;

			// Determine closest collision - to what ???
			for (auto collision

			// Handle collision
			closestCollision->pObject->OnCollision(pObject, closestCollision->contact);

		}

		*/

		//
		// 4. If there was a collision: Handle it
		//		- Call PhysObject::OnCollision(PhysObject* pOther)
		//			- High-level components will register listeners via PhysObject::RegisterEventHandler(IPhysObjEventHandler* handler)
		//			-> High-level components can handle event, e.g. IPhysObjEventHandler::OnCollision(IPhysObject* other)
		//				- e.g. Actor implements IPhysObjEventListener and registers itself to physical component
		
		/*
		
		class Actor : public IComponent, public IPhysObjEventHandler
		{
		public:
			virtual void OnCollision(IPhysObject* other, const SContactInfo& contact)
			{
				CPhysObjComponent* pOtherComponent = dynamic_cast<CPhysObjComponent*>(other);
				IEntity* pOtherEntity = pOtherComponent->GetEntity();

				Bullet* pBullet = pOtherEntity->GetComponent<Bullet>();
				if (pBullet)
				{
					// It actually is a bullet
					m_Health -= pBullet->GetDamage();
				}
			}
		};
		
		
		
		*/


	}


	///////// WEAPON ANIMATION ///////

	IEntity* pWeaponEntity = m_pWeapon->GetEntity();

	if (m_ShootCooldown > SHOOT_COOLDOWN * 0.5f)
	{
		// Whip weapon up
		pWeaponEntity->Rotate(Quat::FromAxisAngle(Vec3f(1.0f, 0, 0), -1.0f * deltaTime));
		pWeaponEntity->Translate(Vec3f(0, 0, -1.0f * deltaTime));
	}
	else if (m_ShootCooldown > 0.0f)
	{
		// Whip weapon down
		pWeaponEntity->Rotate(Quat::FromAxisAngle(Vec3f(1.0f, 0, 0), 1.0f * deltaTime));
		pWeaponEntity->Translate(Vec3f(0, 0, 1.0f * deltaTime));
	}
	else if (oldShootCooldown > m_ShootCooldown)
	{
		// Reset to correct position, because cooldown just ended
		pWeaponEntity->SetPos(m_WeaponOrigPosition);
		pWeaponEntity->SetRotation(m_WeaponOrigRotation);
	}
}

void CPlayer::HandleReload(float deltaTime)
{
	bool reloadKeyState = (GetAsyncKeyState(0x52) < 0);  // R
	if (!reloadKeyState)
	{
		m_bReloadKeyPressed = false;
		return;
	}

	if (m_bReloadKeyPressed)
		return; // still pressed

	m_bReloadKeyPressed = true;

	m_pWeapon->Reload();
}


/////////////////////////////////////////////////////////////////////////////////

CWeapon* CPlayer::GetEquippedWeapon() const
{
	return m_pWeapon;
}




/////////////////////////////////////////////////////////////////////////////////

bool CPlayerEntityClass::Apply(IEntity* entity)
{
	//entity->AddComponent(SpeedPointEnv::Get3DEngine()->CreateMesh());
	entity->AddComponent(SpeedPointEnv::GetPhysics()->CreatePhysObject());
	entity->CreateComponent<CPlayer>();

	return true;
}


