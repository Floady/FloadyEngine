#pragma once
#include <vector>
#include <string>
#include "FGameEntity.h"
#include "FGameEntityFactory.h"

class FJsonObject;
class FGameBuildingBlueprint;

class FGameBuilding : public FGameEntity
{
public:
	REGISTER_GAMEENTITY(FGameBuilding);

	FGameBuilding() : FGameEntity() {}
	FGameBuilding(FGameBuildingBlueprint* aBluePrint);
	~FGameBuilding();
	virtual void Init(const FJsonObject& anObj) override;
	virtual void Update(double aDeltaTime) override;
	FVector3 GetRallyPointPos() const { return myRallyPointPos; }
	void SetRallyPointPos(FVector3 aPos) { myRallyPointPos = aPos; }
	void SetRallyPointPosWorld(FVector3 aPos) { myRallyPointPos = aPos - myPos; }
	const FGameBuildingBlueprint* GetBluePrint() const { return myBluePrint; }
	FGameEntity* GetRepresentation() const { return myRepresentation; }
	FRenderMeshComponent* GetRenderableObject() override { return myRepresentation ? myRepresentation->GetRenderableObject() : GetRenderableObject(); }
	void SetPos(const FVector3& aPos) override { FGameEntity::SetPos(aPos); myRepresentation->SetPos(aPos); }
	void PostPhysicsUpdate() override;
	
	FGameEntity* myRepresentation;
private:
	FGameBuildingBlueprint* myBluePrint;
	FVector3 myRallyPointPos;
};


class FMenuItemBase
{
public:
	FMenuItemBase(const std::string& anIcon) : myIcon(anIcon) {} 
	virtual void Execute() = 0;
	virtual std::string GetIcon() const { return myIcon; }

protected:
	std::string myIcon;
};

class FGameBuildingBlueprint
{
public:
	class CreateAgentMenuItem : public FMenuItemBase
	{
	public:
		CreateAgentMenuItem(const std::string& aClassName, const std::string& anIcon) : FMenuItemBase(anIcon), myClassName(aClassName) {}
		virtual std::string GetClassName() const { return myClassName; }
		virtual void Execute() override;
		float myBuildTime;
		const FJsonObject* mySetup;
		std::string myClassName;
		// add callback to execute this item, make it a global UI thing FGUIMenuItem
	};

	class SetRallyPointMenuItem : public FMenuItemBase
	{
	public:
		SetRallyPointMenuItem(const std::string& anIcon) : FMenuItemBase(anIcon) {}
		virtual void Execute() override;
	};

	const std::string& GetEntityClassName() { return myEntityClassName; }

	FGameBuildingBlueprint(const char* aSetup);
	~FGameBuildingBlueprint();
	const std::vector<FMenuItemBase*>& GetMenuItems() const { return myMenuItems; }
	void ExecuteMenuItem(int aIdx) const; // @todo: support FGameBuilding* aBuilding,  ... delegates are shit :c
	FGameEntity* GetEntity() const { return myEntity; }
	FGameBuilding* CreateBuilding();
	FVector3 GetRallyPointPos() const { return myRallyPointPos; }
	const FJsonObject* GetBuildingRepresentation() const;

protected:
	FGameEntity* myEntity; // representation of building
	std::vector<FMenuItemBase*> myMenuItems;
	FVector3 myRallyPointPos;
	std::string myEntityClassName;
	FJsonObject* myBuilding;
};

