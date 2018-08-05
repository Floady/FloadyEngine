#pragma once
#include "FGameEntityFactory.h"
#include "FVector3.h"
#include <map>
#include <vector>

class FRenderableObject;
class btRigidBody;
class FJsonObject;
class FGameEntityComponent;
class FRenderMeshComponent;
class FPathfindComponent;

class FGameEntity
{
public:
	REGISTER_GAMEENTITY(FGameEntity);
	
	FGameEntity() {	myPos = FVector3(0, 0, 0); myOwner = nullptr; myCanMove = false; }
	FGameEntity(const FVector3& aPos);
	FGameEntityComponent* CreateComponent(const std::string& aName);
	virtual void Init(const FJsonObject& anObj);
	virtual void Update(double aDeltaTime);
	virtual void PostPhysicsUpdate();
	const FVector3& GetPos() const { return myPos; }
	void SetOwnerEntity(FGameEntity* anEntity) { myOwner = anEntity; }
	FGameEntity* GetOwnerEntity() { return myOwner ? myOwner->GetOwnerEntity() : this; }
	virtual void SetPos(const FVector3& aPos);
	const FVector3& GetPos() { return myPos; }

	virtual FRenderMeshComponent* GetRenderableObject();

	virtual ~FGameEntity();

	template<typename T>
	T* GetComponentInSlot(int aSlotId) {
		return static_cast<T*>(myComponentsLookup[T::GetFClassName()][aSlotId]);
	}

	template<typename T>
	int GetComponentInSlotCount() 
	{
		if(myComponentsLookup.find(T::GetFClassName()) != myComponentsLookup.end())
			return myComponentsLookup[T::GetFClassName()].size();

		return 0;
	}

protected:
	void Init(const FVector3& aPos);
	
	FVector3 myPos;
	FGameEntity* myOwner; // if !null, this entity is managed by someone else (for entity picker callbacks etc)
	bool myCanMove;
private:
	std::map<const char*, std::vector<FGameEntityComponent*>> myComponentsLookup;
	std::vector<FGameEntityComponent*> myComponents;
	std::string myName;
};

