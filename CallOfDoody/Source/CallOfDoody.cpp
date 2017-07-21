#include "CallOfDoody.h"
#include "Player.h"
#include <GameFramework\ISettings.h>
#include <EntitySystem\IScene.h>
#include <EntitySystem\IEntityClass.h>
#include <3DEngine\I3DEngine.h>
#include <3DEngine\IMaterial.h>
#include <3DEngine\ISkyBox.h>
#include <Renderer\IRenderer.h>
#include <Renderer\IResourcePool.h>
#include <Shlwapi.h>

// ---------------------------------------------------------------------------------------------

CallOfDoody::CallOfDoody(HINSTANCE hInstance, HWND hWndViewport, HWND logWnd)
	: m_hInstance(hInstance),
	m_hWndViewport(hWndViewport),
	m_hWndLog(logWnd)
{
}

void CallOfDoody::OnInit(IGameEngine* pEngine)
{
	pEngine->GetEntityClassManager()->RegisterEntityClass<CPlayerEntityClass>();

	I3DEngine* p3DEngine = pEngine->Get3DEngine();

	// -------------------------------------
	// Load World
	char execPath[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, execPath);

	pEngine->GetResources()->SetResourceRootPath(string(execPath) + "\\Workspace");

	string worldFile = "/worlds/New-Test/newtest.spw";
	CLog::Log(S_DEBUG, "Loading World %s", worldFile.c_str());
	pEngine->LoadWorld(worldFile);

	// -------------------------------------
	// Set World Environment
	SEnvironmentSettings environment;
	environment.sunPosition = Vec3f(0.25f, 0.3f, 0.25f);
	environment.sunIntensity = SColor(6.0f, 6.0f, 6.0f);
	environment.fogStart = 50.0f;
	environment.fogEnd = pEngine->GetRenderer()->GetTargetViewport()->GetProjectionDesc().farZ;

	pEngine->Get3DEngine()->SetEnvironmentSettings(environment);

	// -------------------------------------
	// Create player
	IEntityClass* pPlayerEntityClass = pEngine->GetEntityClassManager()->GetClass("Player");
	IEntity* pPlayerEntity = pEngine->GetScene()->SpawnEntity("player", pPlayerEntityClass);

	m_pPlayer = pPlayerEntity->GetComponent<CPlayer>();

	// Get spawn pos from scene
	Vec3f spawnPos = Vec3f(-13.f, 15.f, -10.f);
	IEntity* pSpawn = pEngine->GetScene()->GetFirstEntityByName("spawn");
	if (pSpawn)
		spawnPos = pSpawn->GetPos();
	else
		CLog::Log(S_WARN, "No spawnpoint found in world (Entity with name 'spawn')! Using default (%.2f, %.2f, %.2f)",
			spawnPos.x, spawnPos.y, spawnPos.z);

	// Setup player
	SPlayerParameters playerParams;
	playerParams.pos = spawnPos;
	playerParams.height = 1.60f;
	playerParams.diameter = 0.6f;
	m_pPlayer->Setup(playerParams);

	SCamera* pCamera = m_pPlayer->GetCamera();
	pEngine->GetRenderer()->GetTargetViewport()->SetCamera(m_pPlayer->GetCamera());

	m_LastTerrainUpdatePlayerPos = m_pPlayer->GetEntity()->GetPos();

	// -------------------------------------
	// Setup terrain
	pEngine->Get3DEngine()->GetTerrain()->GenLodLevelChunks(pCamera);
	//pEngine->GetRenderer()->EnableWireframe(true);

	// -------------------------------------
	// Setup skybox
	pEngine->Get3DEngine()->GetSkyBox()->InitGeometry(pEngine->GetRenderer());

	// -------------------------------------
	// Setup crosshair
	SHUDElement* pCrossHair = pEngine->Get3DEngine()->CreateHUDElement();
	SIZE vpSz = pEngine->GetRenderer()->GetTargetViewport()->GetSize();
	pCrossHair->pos[0] = (unsigned int)(vpSz.cx / 2);
	pCrossHair->pos[1] = (unsigned int)(vpSz.cy / 2);
	pCrossHair->size[0] = 6;
	pCrossHair->size[1] = 6;
	pCrossHair->pTexture = pEngine->GetResources()->GetTexture("/textures/hud/crosshair.png");

	// -------------------------------------
	// Setup actor

	//SInitialGeometryDesc* pActorGeom = p3DEngine->GetGeometryManager()->LoadGeometry("res\\models\\export\\soldier.spm");
	//m_ActorEntity.AddComponent(p3DEngine->CreateMesh(SRenderMeshParams(pActorGeom)));
	//m_ActorEntity.SetPos(Vec3f(0, 4.0f, 0));
	//m_ActorEntity.SetRotation(Quat::FromAxisAngle(Vec3f(1.0f, 0, 0), -SP_PI * 0.5f));
}

void CallOfDoody::OnLog(SResult res, const string& msg)
{
	// Output to visual studio console
	OutputDebugString("[CallOfDoody] ");
	OutputDebugString(msg.c_str());

	// Output to our ingame log
	if (m_hWndLog)
	{
		DWORD startPos, endPos;
		SendMessage(m_hWndLog, EM_GETSEL, reinterpret_cast<WPARAM>(&startPos), reinterpret_cast<WPARAM>(&endPos));
		int outLength = GetWindowTextLength(m_hWndLog);
		SendMessage(m_hWndLog, EM_SETSEL, outLength, outLength);
		SendMessage(m_hWndLog, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(msg.c_str()));
		SendMessage(m_hWndLog, EM_SETSEL, startPos, endPos);
	}
}

void CallOfDoody::Update(float fLastFrameTime)
{
	if (m_pPlayer)
	{
		RECT viewRect;
		::GetWindowRect(m_hWndViewport, &viewRect);

		int viewportOffset[] = { viewRect.left, viewRect.top };
		int viewportSz[] = { viewRect.right - viewRect.left, viewRect.bottom - viewRect.top };

		m_pPlayer->Update(fLastFrameTime, viewportOffset, viewportSz);

		// Update terrain if necessary
		float minTerrainUpdateDist = 3.0f;
		Vec3f dcampos = m_pPlayer->GetEntity()->GetPos() - m_LastTerrainUpdatePlayerPos;
		if (dcampos.LengthSq() > minTerrainUpdateDist * minTerrainUpdateDist)
		{
			ITerrain* pTerrain = SpeedPointEnv::Get3DEngine()->GetTerrain();
			pTerrain->GenLodLevelChunks(m_pPlayer->GetCamera());
			m_LastTerrainUpdatePlayerPos = m_pPlayer->GetEntity()->GetPos();
		}
	}

	ISkyBox* pSkyBox = SpeedPointEnv::GetEngine()->Get3DEngine()->GetSkyBox();
	if (pSkyBox)
		pSkyBox->SetPosition(m_pPlayer->GetEntity()->GetPos());
}

void CallOfDoody::Render()
{

}
