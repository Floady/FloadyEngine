#include "FGameLevel.h"
#include "..\FJson\FJson.h"
#include "FGameEntityFactory.h"
#include "FGameEntity.h"
#include "FD3d12Renderer.h"
#include "FLightManager.h"
#include "FNavMeshManagerRecast.h"
#include "FProfiler.h"

FGameLevel::FGameLevel(const char * aLevelName)
{
	FJsonObject* level = FJson::Parse(aLevelName); //"Configs//level.txt"
	const FJsonObject* child = level->GetFirstChild();
	while (child)
	{
		FGameEntity* newEntity = FGameEntityFactory::GetInstance()->Create(child->GetName());
		newEntity->Init(*child);
		myEntityContainer.push_back(newEntity);
		child = level->GetNextChild();
	}

	// procedural lights test
	/*
	float spacingX = 26.0f;
	float spacingZ = 16.0f;
	for(int i = 0; i < 10; i++)
		for (int j = 0; j < 10; j++)
		{
			FVector3 pos(i * spacingX, 1000, j * spacingZ);
			FVector3 pos2(i * spacingX, -1000, j * spacingZ);
			FVector3 navPos = FNavMeshManagerRecast::GetInstance()->RayCast(pos, pos2);
			FLightManager::GetInstance()->AddSpotlight(navPos + FVector3(0, 5, 0), FVector3(0, -1, 0.05f), navPos.y + 6.0f, FVector3(0.2, 0.2, 0.2), 25.0f);
		}
	//*/
}

void FGameLevel::Update(double aDeltaTime)
{
	FPROFILE_FUNCTION("FGameLevel Update");

	FLightManager::GetInstance()->SortLights();
	FCamera* cam = FD3d12Renderer::GetInstance()->GetCamera();
	FAABB& aabb = FLightManager::GetInstance()->GetVisibleAABB();
	for (FGameEntity* entity : myEntityContainer)
	{
		entity->Update(aDeltaTime);

		if (cam->IsInFrustum(entity->GetRenderableObject()))
			aabb.Grow(entity->GetRenderableObject()->GetAABB());
	}
}

void FGameLevel::PostPhysicsUpdate()
{
	for (FGameEntity* entity : myEntityContainer)
	{
		entity->PostPhysicsUpdate();
	}
}

FGameLevel::~FGameLevel()
{
}
