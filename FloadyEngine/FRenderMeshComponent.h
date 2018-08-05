#pragma once
#include "FGameEntityComponent.h"
#include "FRenderableObject.h"
#include "FVector3.h"

class FRenderableObject;
struct ID3D12Resource;

class FRenderMeshComponent :	public FGameEntityComponent
{
public:
	enum class RenderMeshType : char
	{
		Sphere = 0,
		Box = 1,
		Obj = 2
	};

	REGISTER_GAMEENTITYCOMPONENT(FRenderMeshComponent);

	FRenderMeshComponent();
	~FRenderMeshComponent();

	void Init(const FJsonObject& anObj) override;
	void Update(double aDeltaTime) override {}
	void PostPhysicsUpdate() override;
	FRenderableObject* GetGraphicsObject() { return myGraphicsObject; }
	void SetPos(const FVector3& aPos);
	FAABB GetAABB() const;
	const char* GetTexture();
	float* GetRotMatrix();
	const FVector3& GetScale();

protected:
	FRenderableObject* myGraphicsObject; // for non instances only
	FRenderableObjectInstanceData myInstanceData; // for instances
	unsigned int myMeshInstanceId;
	bool myIsInstanced;

	ID3D12Resource* m_vertexBuffer;
	ID3D12Resource* m_indexBuffer;
	//std::vector<FPrimitiveGeometry::Vertex> vertices;
	std::vector<int> indices;
	string myModelInstanceName;
	FVector3 myOffset;
};

