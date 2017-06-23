#include "FPlacingManager.h"
#include "FD3d12Renderer.h"
#include "d3dx12.h"
#include "FCamera.h"
#include "FGameEntity.h"
#include "FRenderableObject.h"
#include "FPrimitiveGeometry.h"
#include "FPostProcessEffect.h"
#include "FPrimitiveBox.h"

using namespace DirectX;

FPlacingManager::FPlacingManager()
{
	myObject = nullptr;
	myIgnoreNextMouseDown = 0;
}

void FPlacingManager::UpdateMousePos(const FVector3& aPos)
{
	if (myObject)
	{
		myObject->SetPos(aPos);
	}
}

void FPlacingManager::MouseDown(const FVector3& aPos)
{
	if (myIgnoreNextMouseDown)
	{
		myIgnoreNextMouseDown--;
		return;
	}

	if (!myObject)
		return;

	if(myPlaceCallback)
		myPlaceCallback(aPos);

	ClearPlacable();
}

void FPlacingManager::SetPlacable(bool anIsCube, FVector3 aScale, FDelegate2<void(FVector3)>& aCB)
{
	myIgnoreNextMouseDown = 1;
	ClearPlacable();

	if(anIsCube)
		myObject = new FPrimitiveBox(FD3d12Renderer::GetInstance(), FVector3(0, 0, 0), aScale, FPrimitiveBox::PrimitiveType::Box);
	else
		myObject = new FPrimitiveBox(FD3d12Renderer::GetInstance(), FVector3(0, 0, 0), aScale, FPrimitiveBox::PrimitiveType::Sphere);

	myObject->SetShader("primitiveshader_deferred.hlsl");
	FD3d12Renderer::GetInstance()->GetSceneGraph().AddObject(myObject, false);

	myPlaceCallback = aCB;
}

void FPlacingManager::ClearPlacable()
{
	if (myObject)
	{
		FD3d12Renderer::GetInstance()->GetSceneGraph().RemoveObject(myObject);
		delete myObject;
	}

	myObject = nullptr;
	myPlaceCallback.reset();
}

FPlacingManager::~FPlacingManager()
{
}
