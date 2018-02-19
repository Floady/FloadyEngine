#include "FGame.h"
#include "FGameEntity.h"
#include "FPathfindComponent.h"
#include "FProfiler.h"
#include "FUtilities.h"
#include "FGameEntityComponent.h"
#include "FRenderMeshComponent.h"
#include "FPhysicsComponent.h"
#include "..\FJson\FJsonObject.h"

REGISTER_GAMEENTITY2(FGameEntity);

FGameEntity::FGameEntity(const FVector3& aPos)
{
	myCanMove = false;
	myOwner = nullptr;
	Init(aPos);
}

FGameEntityComponent* FGameEntity::CreateComponent(const std::string & aName)
{
	FGameEntityComponent* comp = FGameEntityFactory::GetInstance()->CreateComponent(aName);
	comp->SetOwner(this);
	const char* key = FGameEntityFactory::GetInstance()->GetEntityComponentKey(aName);
	myComponentsLookup[key].push_back(comp);
	myComponents.push_back(comp);

	return comp;
}

void FGameEntity::Init(const FVector3& aPos)
{
	myOwner = nullptr;
	myPos = aPos;
}

void FGameEntity::Init(const FJsonObject & anObj)
{
	const FJsonObject* components = anObj.GetChildByName("Components");
	if(components)
	{
		const FJsonObject* child = components->GetFirstChild();
		while (child)
		{
			FGameEntityComponent* comp = CreateComponent(child->GetName());
			comp->SetOwner(this);
			comp->Init(*child);
			FLOG("component name: %s", child->GetName().c_str());
			child = components->GetNextChild();
		}
	}

	FVector3 pos, scale;

	pos.x = anObj.GetItem("posX").GetAs<double>();
	pos.y = anObj.GetItem("posY").GetAs<double>();
	pos.z = anObj.GetItem("posZ").GetAs<double>();
	myCanMove = anObj.GetItem("canMove").GetAs<bool>();

	Init(pos);
}

void FGameEntity::Update(double aDeltaTime)
{
	//FPROFILE_FUNCTION("FGameEntity Update");

	for (FGameEntityComponent* component : myComponents)
	{
		component->Update(aDeltaTime);
	}
}
void FGameEntity::PostPhysicsUpdate()
{
	if (myCanMove)
	{
		myPos = GetComponentInSlot<FPhysicsComponent>(0)->GetPos();
		GetComponentInSlot<FPhysicsComponent>(0)->GetTransform(GetRenderableObject()->GetRotMatrix());
	}

	GetComponentInSlot<FRenderMeshComponent>(0)->SetPos(myPos);

	for (FGameEntityComponent* component : myComponents)
	{
		component->PostPhysicsUpdate();
	}
}

void FGameEntity::SetPos(const FVector3& aPos)
{
	myPos = aPos;

	GetRenderableObject()->SetPos(aPos);

	if(myCanMove)
		GetComponentInSlot<FPhysicsComponent>(0)->SetPos(aPos);
}

FRenderMeshComponent * FGameEntity::GetRenderableObject()
{
	return GetComponentInSlot<FRenderMeshComponent>(0);
}

FGameEntity::~FGameEntity()
{
	for (FGameEntityComponent* component : myComponents)
	{
		delete component;
	}

	myComponents.clear();
}