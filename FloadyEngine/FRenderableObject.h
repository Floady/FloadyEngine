#pragma once
#include "FVector3.h"
#include <d3d12.h>


struct FAABB
{
	FAABB() { Reset(); }

	void Grow(const FVector3& aPoint)
	{
		myMin.x = min(aPoint.x, myMin.x);
		myMin.y = min(aPoint.y, myMin.y);
		myMin.z = min(aPoint.z, myMin.z);
		myMax.x = max(aPoint.x, myMax.x);
		myMax.y = max(aPoint.y, myMax.y);
		myMax.z = max(aPoint.z, myMax.z);
	}

	void Grow(float aX, float aY, float aZ)
	{
		myMin.x = min(aX, myMin.x);
		myMin.y = min(aY, myMin.y);
		myMin.z = min(aZ, myMin.z);
		myMax.x = max(aX, myMax.x);
		myMax.y = max(aY, myMax.y);
		myMax.z = max(aZ, myMax.z);
	}

	void Grow(const FAABB& anAABB)
	{
		Grow(anAABB.myMax);
		Grow(anAABB.myMin);
	}

	bool IsInside(const FVector3& aPoint) const;
	bool IsInside(const FAABB& anAABB) const;

	void Reset() { myMin = FVector3(99999, 99999, 999999); myMax = FVector3(-99999,-99999,-9999);}
	FVector3 myMin;
	FVector3 myMax;
};

struct ID3D12Resource;
class FRenderableObjectInstanceData
{
public:
	FAABB myAABB;
	float myRotMatrix[16];
	FVector3 myPos;
	FVector3 myScale;
	FAABB GetAABB() const
	{
		FAABB aabb = myAABB;
		aabb.myMax += myPos;
		aabb.myMin += myPos;
		return aabb;
	}

};

class FRenderableObject
{
public:
	virtual void Init() = 0;
	virtual void Render() = 0;
	virtual void RenderShadows() = 0;
	virtual void PopulateCommandListAsync() = 0;
	virtual void PopulateCommandListAsyncShadows() = 0;
	virtual void PopulateCommandListInternalShadows(ID3D12GraphicsCommandList* aCmdList) = 0;
	virtual void PopulateCommandListInternal(ID3D12GraphicsCommandList* aCmdList) = 0;
	bool CastsShadows() { return myCastsShadows; }
	void SetCastsShadows(bool aShouldCastShadows) { myCastsShadows = aShouldCastShadows; }

	virtual void SetTexture(const char* aFilename) = 0;
	virtual const char* GetTexture() = 0;
	virtual void SetShader(const char* aFilename) = 0;
	virtual void RecalcModelMatrix() {}
	virtual void UpdateConstBuffers() {}

	virtual FAABB GetAABB() const;
	virtual FAABB GetLocalAABB() const;

	// todo: this needs some base implementation (pull modelmatrix here, any renderable has a matrix to set and to render with)
	virtual void SetPos(const FVector3& aPos) { myPos = aPos; }
	const FVector3& GetPos() { return myPos; }
	const FVector3& GetScale() { return myScale; }
	void SetScale(FVector3 aScale) { myScale = aScale; }
	virtual void SetRotMatrix(float* m) { }
	FRenderableObject();
	virtual ~FRenderableObject();
	virtual bool GetIsVisible() { return myIsVisible; }
	void SetIsVisible(bool anIsVisible) { myIsVisible = anIsVisible; }
	ID3D12Resource* GetModelViewMatrix() { return m_ModelProjMatrix; }
public:
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
	int myIndicesCount;
	bool myRenderCCW;
	float myRotMatrix[16];

protected:
	ID3D12Resource* m_ModelProjMatrix;
	FVector3 myPos;
	FVector3 myScale;
	bool myCastsShadows;
	bool myIsVisible;
public: // todo wrap this up nicely
	FAABB myAABB;
};

