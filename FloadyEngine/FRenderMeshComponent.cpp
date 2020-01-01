#include "FRenderMeshComponent.h"
#include "FMeshInstanceManager.h"
#include "FDebugDrawer.h"
#include "FCamera.h"
#include "FLightManager.h"
#include "FGameEntity.h"
#include "FProfiler.h"
#include "FUtilities.h"
#include "FPrimitiveBoxMultiTex.h"
#include "FPrimitiveGeometry.h"
#include "FMeshManager.h"
#include <unordered_map>
#include "FGame.h"
#include "FPrimitiveBoxInstanced.h"

REGISTER_GAMEENTITYCOMPONENT2(FRenderMeshComponent);

void hash_combine2(size_t &seed, size_t hash)
{
	hash += 0x9e3779b9 + (seed << 6) + (seed >> 2);
	seed ^= hash;
}

namespace std {
	template<> struct hash<FPrimitiveGeometry::Vertex2> {
		size_t operator()(FPrimitiveGeometry::Vertex2 const& vertex) const {

			size_t seed = 0;
			seed += vertex.matId;
			hash<float> hasher;
			hash_combine2(seed, hasher(vertex.position.x));
			hash_combine2(seed, hasher(vertex.position.y));
			hash_combine2(seed, hasher(vertex.position.z));
			hash_combine2(seed, hasher(vertex.normal.x));
			hash_combine2(seed, hasher(vertex.normal.y));
			hash_combine2(seed, hasher(vertex.normal.z));
			hash_combine2(seed, hasher(vertex.uv.x));
			hash_combine2(seed, hasher(vertex.uv.y));
			return seed;
		}
	};

}

FRenderMeshComponent::FRenderMeshComponent()
{
	myMeshInstanceId = 0;
	myGraphicsObject = nullptr;
	m_indexBuffer = nullptr;
	m_vertexBuffer = nullptr;
	myModelInstanceName = "";
	myIsInstanced = false;
}

FRenderMeshComponent::~FRenderMeshComponent()
{
	if(myGraphicsObject)
	{
		FGame::GetInstance()->GetRenderer()->GetSceneGraph().RemoveObject(myGraphicsObject);
		delete myGraphicsObject;
		myGraphicsObject = nullptr;
	}

	if (myIsInstanced)
	{
		FRenderableObjectInstanceData& data = FMeshInstanceManager::GetInstance()->GetInstanceData(myModelInstanceName, myMeshInstanceId);
		data.myIsVisible = false;
	}
}

void FRenderMeshComponent::Init(const FJsonObject & anObj)
{
	if (myGraphicsObject)
	{
		FGame::GetInstance()->GetRenderer()->GetSceneGraph().RemoveObject(myGraphicsObject);
		delete myGraphicsObject;
		myGraphicsObject = nullptr;
	}
	
	myOffset = FVector3(0, 0, 0);

	FVector3 pos, scale;

	pos.x = anObj.GetItem("posX").GetAs<double>();
	pos.y = anObj.GetItem("posY").GetAs<double>();
	pos.z = anObj.GetItem("posZ").GetAs<double>();

	scale.x = anObj.GetItem("scaleX").GetAs<double>();
	scale.y = anObj.GetItem("scaleY").GetAs<double>();
	scale.z = anObj.GetItem("scaleZ").GetAs<double>();

	myModelInstanceName = "";

	myIsInstanced = false;
	if (anObj.HasItem("instanced"))
		myIsInstanced = anObj.GetItem("instanced").GetAs<bool>();

	if (anObj.HasItem("offsetX"))
	{
		myOffset.x = anObj.GetItem("offsetX").GetAs<double>();
		myOffset.y = anObj.GetItem("offsetY").GetAs<double>();
		myOffset.z = anObj.GetItem("offsetZ").GetAs<double>();
	}

	RenderMeshType type = static_cast<FRenderMeshComponent::RenderMeshType>(anObj.GetItem("type").GetAs<int>()); // 0 spehere, 1 box
	switch (type)
	{
	case RenderMeshType::Sphere:
	{
		myModelInstanceName = "sphere";

		if(myIsInstanced)
			myMeshInstanceId = FMeshInstanceManager::GetInstance()->GetMeshInstanceId("sphere");
		else
			myGraphicsObject = new FPrimitiveBoxInstanced(FGame::GetInstance()->GetRenderer(), pos, scale, FPrimitiveBoxInstanced::PrimitiveType::Sphere);
	}
	break;
	case RenderMeshType::Box:
	{
		myModelInstanceName = "box";

		if (myIsInstanced)
			myMeshInstanceId = FMeshInstanceManager::GetInstance()->GetMeshInstanceId("box");
		else
			myGraphicsObject = new FPrimitiveBoxInstanced(FGame::GetInstance()->GetRenderer(), pos, scale, FPrimitiveBoxInstanced::PrimitiveType::Box);
	}
	break;
	case RenderMeshType::Obj:
	{
		myModelInstanceName = anObj.GetItem("model").GetAs<string>();

		if(myIsInstanced)
			myMeshInstanceId = FMeshInstanceManager::GetInstance()->GetMeshInstanceId(myModelInstanceName);
		else
		{
			myGraphicsObject = new FPrimitiveBoxMultiTex(FD3d12Renderer::GetInstance(), pos, scale, FPrimitiveBoxInstanced::PrimitiveType::Sphere, 1);
			string path = "models/";
			path.append(myModelInstanceName);

			FMeshManager::FMeshObject* mesh = FMeshManager::GetInstance()->GetMesh(path, FDelegate2<void(const FMeshManager::FMeshLoadObject&)>::from<FPrimitiveBoxMultiTex, &FPrimitiveBoxMultiTex::ObjectLoadingDone>(dynamic_cast<FPrimitiveBoxMultiTex*>(myGraphicsObject)));
			
			dynamic_cast<FPrimitiveBoxMultiTex*>(myGraphicsObject)->myMesh = mesh;

			myGraphicsObject->myIndicesCount = mesh->myIndicesCount;
			myGraphicsObject->m_vertexBufferView.BufferLocation = mesh->myVertexBuffer->GetGPUVirtualAddress();
			myGraphicsObject->m_vertexBufferView.StrideInBytes = sizeof(FPrimitiveGeometry::Vertex2);
			myGraphicsObject->m_vertexBufferView.SizeInBytes = sizeof(FPrimitiveGeometry::Vertex2) * mesh->myVertices.size();

			myGraphicsObject->m_indexBufferView.BufferLocation = mesh->myIndexBuffer->GetGPUVirtualAddress();
			myGraphicsObject->m_indexBufferView.Format = DXGI_FORMAT_R32_UINT;  // get from primitive manager
			myGraphicsObject->m_indexBufferView.SizeInBytes = sizeof(int) * mesh->myIndicesCount;

			// setup AABB
			for (FPrimitiveGeometry::Vertex2& vert : mesh->myVertices)
			{
				myGraphicsObject->myAABB.myMax.x = max(myGraphicsObject->myAABB.myMax.x, vert.position.x * myGraphicsObject->GetScale().x);
				myGraphicsObject->myAABB.myMax.y = max(myGraphicsObject->myAABB.myMax.y, vert.position.y * myGraphicsObject->GetScale().y);
				myGraphicsObject->myAABB.myMax.z = max(myGraphicsObject->myAABB.myMax.z, vert.position.z * myGraphicsObject->GetScale().z);

				myGraphicsObject->myAABB.myMin.x = min(myGraphicsObject->myAABB.myMin.x, vert.position.x * myGraphicsObject->GetScale().x);
				myGraphicsObject->myAABB.myMin.y = min(myGraphicsObject->myAABB.myMin.y, vert.position.y * myGraphicsObject->GetScale().y);
				myGraphicsObject->myAABB.myMin.z = min(myGraphicsObject->myAABB.myMin.z, vert.position.z * myGraphicsObject->GetScale().z);
			}
		}
	}
	}

	if(!myIsInstanced)
	{
		string tex = anObj.GetItem("tex").GetAs<string>();
		myGraphicsObject->SetTexture(tex.c_str());

		FGame::GetInstance()->GetRenderer()->GetSceneGraph().AddObject(myGraphicsObject, false);
	}
	else
	{
		FRenderableObjectInstanceData& data = FMeshInstanceManager::GetInstance()->GetInstanceData(myModelInstanceName, myMeshInstanceId);
		
		string tex = anObj.GetItem("tex").GetAs<string>();

		FMeshInstanceManager::GetInstance()->GetInstance(myModelInstanceName, myMeshInstanceId)->SetTexture(tex.c_str());
		data.myScale = scale;

		data.myAABB = FMeshInstanceManager::GetInstance()->GetInstance(myModelInstanceName, myMeshInstanceId)->GetLocalAABB();

		
		data.myIsVisible = true;
		data.myPos = pos;
		const FVector3& scale2 = scale;

		//
		FMatrix scaleMatrix;
		scaleMatrix.cell[FMatrix::SX] = scale2.x;
		scaleMatrix.cell[FMatrix::SY] = scale2.y;
		scaleMatrix.cell[FMatrix::SZ] = scale2.z;
		FMatrix rotMatrix;
		memcpy(data.myRotMatrix, rotMatrix.cell, sizeof(float) * 16);
		
		rotMatrix.SetTranslation(pos);
		rotMatrix.Concatenate(scaleMatrix);

		memcpy(data.myModelMatrix, rotMatrix.cell, sizeof(float) * 16);
	}
}

extern bool ourShouldRecalc;

void FRenderMeshComponent::PostPhysicsUpdate()
{
	//FPROFILE_FUNCTION("PostPhysicsUpdate");
	if (myGraphicsObject)
	{
		myGraphicsObject->RecalcModelMatrix();
		myGraphicsObject->SetIsVisible(FD3d12Renderer::GetInstance()->GetCamera()->IsInFrustum(myGraphicsObject));
	}

	if (myIsInstanced)
	{
		FRenderableObjectInstanceData& data = FMeshInstanceManager::GetInstance()->GetInstanceData(myModelInstanceName, myMeshInstanceId);

		FDebugDrawer* debugDrawer = FD3d12Renderer::GetInstance()->GetDebugDrawer();
		
		
		FAABB aabb2 = data.GetAABB();
		if (FD3d12Renderer::GetInstance()->GetCamera()->IsInFrustum(aabb2) || ourShouldRecalc)
		{
			//if (debugDrawer)
			//{
			//	debugDrawer->drawAABB(aabb2.myMin, aabb2.myMax, FVector3(0.2, 0.2, 1));
			//}

			data.myIsVisible = true;

			const FVector3& pos2 = data.myPos;
			const FVector3& scale2 = GetScale();

			FMatrix scaleMatrix;
			scaleMatrix.cell[FMatrix::SX] = scale2.x;
			scaleMatrix.cell[FMatrix::SY] = scale2.y;
			scaleMatrix.cell[FMatrix::SZ] = scale2.z;
			FMatrix rotMatrix;
			memcpy(rotMatrix.cell, data.myRotMatrix, sizeof(float) * 16);
			rotMatrix.Invert2();
			rotMatrix.SetTranslation(pos2);
			rotMatrix.Concatenate(scaleMatrix);

			memcpy(data.myModelMatrix, rotMatrix.cell, sizeof(float) * 16);
		}
		else
		{
			//if (debugDrawer)
			//{
			//	debugDrawer->drawAABB(aabb2.myMin, aabb2.myMax, FVector3(1, 0.2, 0.2));
			//}

			data.myIsVisible = false;
		}
	}
}

void FRenderMeshComponent::SetPos(const FVector3 & aPos)
{
	if (myGraphicsObject)
		myGraphicsObject->SetPos(myOffset + aPos);
	else
	{
		FRenderableObjectInstanceData& data = FMeshInstanceManager::GetInstance()->GetInstanceData(myModelInstanceName, myMeshInstanceId);
		data.myPos = myOffset + aPos;
		FMeshInstanceManager::GetInstance()->GetInstance(myModelInstanceName, myMeshInstanceId)->RecalcModelMatrix();
	}
}

FAABB FRenderMeshComponent::GetAABB() const
{
	FAABB aabb;
	if (myGraphicsObject)
		aabb = myGraphicsObject->GetAABB();
	else
	{
		FRenderableObjectInstanceData& data = FMeshInstanceManager::GetInstance()->GetInstanceData(myModelInstanceName, myMeshInstanceId);
		aabb = data.GetAABB();
	}

	if (false)
	{
		FDebugDrawer* debugDrawer = FD3d12Renderer::GetInstance()->GetDebugDrawer();
		debugDrawer->drawAABB(aabb, FVector3(1, 0, 0));
	}

	return aabb;
}

const char * FRenderMeshComponent::GetTexture()
{
	if (myGraphicsObject)
		return myGraphicsObject->GetTexture();
	else
		return FMeshInstanceManager::GetInstance()->GetInstance(myModelInstanceName, myMeshInstanceId)->GetTexture();
}

float * FRenderMeshComponent::GetRotMatrix()
{
	if (myGraphicsObject)
		return myGraphicsObject->myRotMatrix;
	else
		return FMeshInstanceManager::GetInstance()->GetInstanceData(myModelInstanceName, myMeshInstanceId).myRotMatrix;
}

const FVector3 & FRenderMeshComponent::GetScale()
{
	if (myGraphicsObject)
		return myGraphicsObject->GetScale();
	else
		return FMeshInstanceManager::GetInstance()->GetInstanceData(myModelInstanceName, myMeshInstanceId).myScale;
}

const FVector3 & FRenderMeshComponent::GetPos()
{
	if (myGraphicsObject)
		return myGraphicsObject->GetPos();
	else
		return FMeshInstanceManager::GetInstance()->GetInstanceData(myModelInstanceName, myMeshInstanceId).myPos;
}

const D3D12_VERTEX_BUFFER_VIEW & FRenderMeshComponent::GetVertexBufferView()
{
	if (myGraphicsObject)
		return myGraphicsObject->GetVertexBufferView();
	else
		return FMeshInstanceManager::GetInstance()->GetInstance(myModelInstanceName, myMeshInstanceId)->GetVertexBufferView();
}

const D3D12_INDEX_BUFFER_VIEW & FRenderMeshComponent::GetIndexBufferView()
{
	if (myGraphicsObject)
		return myGraphicsObject->GetIndexBufferView();
	else
		return FMeshInstanceManager::GetInstance()->GetInstance(myModelInstanceName, myMeshInstanceId)->GetIndexBufferView();
}

int FRenderMeshComponent::GetIndicesCount()
{
	if (myGraphicsObject)
		return myGraphicsObject->GetIndicesCount();
	else
		return FMeshInstanceManager::GetInstance()->GetInstance(myModelInstanceName, myMeshInstanceId)->GetIndicesCount();
}
