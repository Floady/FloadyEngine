#include "FRenderMeshComponent.h"
#include "FRenderableObject.h"
#include "FMeshInstanceManager.h"
#include "FDebugDrawer.h"
#include "FCamera.h"
#include "FLightManager.h"
#include "FPrimitiveBox.h"
#include "FGameEntity.h"
#include "FProfiler.h"
#include "FUtilities.h"
#include "FPrimitiveBoxMultiTex.h"
#include "FPrimitiveGeometry.h"
#include "FObjLoader.h"
#include "FMeshManager.h"
#include <unordered_map>
#include "FGame.h"
#include "FPrimitiveBoxInstanced.h"

REGISTER_GAMEENTITYCOMPONENT2(FRenderMeshComponent);

using namespace DirectX;


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
}

FRenderMeshComponent::~FRenderMeshComponent()
{
	if(myGraphicsObject)
	{
		FGame::GetInstance()->GetRenderer()->GetSceneGraph().RemoveObject(myGraphicsObject);
		delete myGraphicsObject;
		myGraphicsObject = nullptr;
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

	FVector3 pos, scale;

	pos.x = anObj.GetItem("posX").GetAs<double>();
	pos.y = anObj.GetItem("posY").GetAs<double>();
	pos.z = anObj.GetItem("posZ").GetAs<double>();

	scale.x = anObj.GetItem("scaleX").GetAs<double>();
	scale.y = anObj.GetItem("scaleY").GetAs<double>();
	scale.z = anObj.GetItem("scaleZ").GetAs<double>();

	myIsInstanced = false;
	if (anObj.HasItem("instanced"))
		myIsInstanced = anObj.GetItem("instanced").GetAs<bool>();

	RenderMeshType type = static_cast<FRenderMeshComponent::RenderMeshType>(anObj.GetItem("type").GetAs<int>()); // 0 spehere, 1 box
	switch (type)
	{
	case RenderMeshType::Sphere:
	{
		if(myIsInstanced)
			myMeshInstanceId = FMeshInstanceManager::GetInstance()->GetMeshInstanceId("sphere");
		else
			myGraphicsObject = new FPrimitiveBoxInstanced(FGame::GetInstance()->GetRenderer(), pos, scale, FPrimitiveBoxInstanced::PrimitiveType::Sphere);
	}
	break;
	case RenderMeshType::Box:
	{
		myGraphicsObject = new FPrimitiveBox(FGame::GetInstance()->GetRenderer(), pos, scale, FPrimitiveBox::PrimitiveType::Box);
	}
	break;
	case RenderMeshType::Obj:
	{
		/*
		myGraphicsObject = new FPrimitiveBox(FGame::GetInstance()->GetRenderer(), pos, scale, FPrimitiveBox::PrimitiveType::Sphere);
		/*/
		myGraphicsObject = new FPrimitiveBoxMultiTex(FD3d12Renderer::GetInstance(), pos, scale, FPrimitiveBox::PrimitiveType::Sphere);

		FObjLoader::FObjMesh& m = dynamic_cast<FPrimitiveBoxMultiTex*>(myGraphicsObject)->myObjMesh;
		FObjLoader objLoader;
		string model = anObj.GetItem("model").GetAs<string>();
		string path = "models/";
		path.append(model);

		FMeshManager::FMeshObject* mesh = FMeshManager::GetInstance()->GetMesh(path);
		m = mesh->myMeshData;

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

		myGraphicsObject->myAABB. myMin.x = min(myGraphicsObject->myAABB.myMin.x, vert.position.x * myGraphicsObject->GetScale().x);
		myGraphicsObject->myAABB.myMin.y = min(myGraphicsObject->myAABB.myMin.y, vert.position.y * myGraphicsObject->GetScale().y);
		myGraphicsObject->myAABB.myMin.z = min(myGraphicsObject->myAABB.myMin.z, vert.position.z * myGraphicsObject->GetScale().z);
		}

		//FUtilities::FLog("Obj model #vtx: %i\n", mesh->myVertices.size());
		//FUtilities::FLog("Obj model #idx: %i\n", indices.size());
		//*/

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
		string tex = anObj.GetItem("tex").GetAs<string>();

		FMeshInstanceManager::GetInstance()->GetInstance("sphere", myMeshInstanceId)->SetTexture(tex.c_str()); ;
		//FMeshInstanceManager::GetInstance()->GetInstance("sphere", myMeshInstanceId)->SetScale(scale);
		myInstanceData.myScale = scale;

		myInstanceData.myAABB = FMeshInstanceManager::GetInstance()->GetInstance("sphere", myMeshInstanceId)->GetLocalAABB();

		FPrimitiveBoxInstanced::PerInstanceData& data = FMeshInstanceManager::GetInstance()->GetInstanceData("sphere", myMeshInstanceId);

		data.myIsVisible = true;
		const FVector3& pos = myInstanceData.myPos;
		const FVector3& scale2 = GetScale();
		XMFLOAT4X4 m = XMFLOAT4X4();
		XMMATRIX mtxRot = XMLoadFloat4x4(&m);
		XMMATRIX scale = XMMatrixScaling(scale2.x, scale2.y, scale2.z);
		XMMATRIX offset = XMMatrixTranslationFromVector(XMVectorSet(pos.x, pos.y, pos.z, 1));
		offset = scale * offset;

		offset = XMMatrixTranspose(offset);
		XMStoreFloat4x4(&data.myModelMatrix, offset);
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
		FDebugDrawer* debugDrawer = FD3d12Renderer::GetInstance()->GetDebugDrawer();
		

		if (ourShouldRecalc || FD3d12Renderer::GetInstance()->GetCamera()->IsInFrustum(myInstanceData.GetAABB()))
		{
			//if (debugDrawer)
			//{
			//	FAABB& aabb = myInstanceData.GetAABB();
			//	debugDrawer->drawAABB(aabb.myMin, aabb.myMax, FVector3(0.2, 0.2, 1));
			//}

			FPrimitiveBoxInstanced::PerInstanceData& data = FMeshInstanceManager::GetInstance()->GetInstanceData("sphere", myMeshInstanceId);
			
			data.myIsVisible = true;

			const FVector3& pos = myInstanceData.myPos;
			const FVector3& scale2 = GetScale();
			XMFLOAT4X4 m = XMFLOAT4X4(myInstanceData.myRotMatrix);
			XMMATRIX mtxRot = XMLoadFloat4x4(&m);
			XMMATRIX scale = XMMatrixScaling(scale2.x, scale2.y, scale2.z);
			XMMATRIX offset = XMMatrixTranslationFromVector(XMVectorSet(pos.x, pos.y, pos.z, 1));
			offset = scale * mtxRot * offset;

			offset = XMMatrixTranspose(offset);
			XMStoreFloat4x4(&data.myModelMatrix, offset);
		}
		else
		{
			//if (debugDrawer)
			//{
			//	FAABB& aabb = myInstanceData.GetAABB();
			//	debugDrawer->drawAABB(aabb.myMin, aabb.myMax, FVector3(1, 0.2, 0.2));
			//}

			FPrimitiveBoxInstanced::PerInstanceData& data = FMeshInstanceManager::GetInstance()->GetInstanceData("sphere", myMeshInstanceId);
			data.myIsVisible = false;
		}
	}
}

void FRenderMeshComponent::SetPos(const FVector3 & aPos)
{
	if (myGraphicsObject)
		myGraphicsObject->SetPos(aPos);
	else
	{
		myInstanceData.myPos = aPos;
		FMeshInstanceManager::GetInstance()->GetInstance("sphere", myMeshInstanceId)->RecalcModelMatrix();
	}
}

FAABB FRenderMeshComponent::GetAABB() const
{
	if (myGraphicsObject)
		return myGraphicsObject->GetAABB();
	else
		return myInstanceData.GetAABB();
}

const char * FRenderMeshComponent::GetTexture()
{
	if (myGraphicsObject)
		return myGraphicsObject->GetTexture();
	else
		return FMeshInstanceManager::GetInstance()->GetInstance("sphere", myMeshInstanceId)->GetTexture();
}

float * FRenderMeshComponent::GetRotMatrix()
{
	if (myGraphicsObject)
		return myGraphicsObject->myRotMatrix;
	else
		return myInstanceData.myRotMatrix;
}

const FVector3 & FRenderMeshComponent::GetScale()
{
	if (myGraphicsObject)
		return myGraphicsObject->GetScale();
	else
		return myInstanceData.myScale;
}
