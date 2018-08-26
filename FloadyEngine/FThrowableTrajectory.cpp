#include "FThrowableTrajectory.h"
#include <cstdint>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <random>
#include "FD3d12Renderer.h"
#include "FDebugDrawer.h"
#include "FPhysicsWorld.h"
#include "FGame.h"
#include "FGameEntity.h"

FVector3 myStartPosition;
FVector3 myStartVelocity;
const FVector3 myWeight = FVector3(0.0f, -10.0f, 0.0f);

FThrowableTrajectory::SharedParams mySharedParameters;

FThrowableTrajectory::TrajectoryPath myPath;

float F_Clamp(float n, float lower, float upper)
{
	return fmax(lower, fmin(n, upper));
}

float F_Lerp(float t, float a, float b) {
	return (1 - t)*a + t*b;
}

FThrowableTrajectory::FThrowableTrajectory()
{
	
}

FThrowableTrajectory::~FThrowableTrajectory()
{
}

float InitalRadiusPercentage()
{
	return mySharedParameters.myInitalRadiusPercentage;
}

int RadiusAdjustSegmentCount()
{
	return mySharedParameters.myRadiusAdjustSegmentCount;
}

float RestCollisionNormalY()
{
	return mySharedParameters.myRestCollisionNormalY;
}

float GroundCollisionNormalY()
{
	return mySharedParameters.myGroundCollisionNormalY;
}


bool FThrowableTrajectory::Simulate(const FVector3& aStartPosition, const FVector3& aStartDirection, float aInitialSpeed, TrajectoryPath& outThrowPath, const float aRuntimeTimeLeft)
{
	bool success = false;

	TrajectoryPathSegment segment;
	segment.myPosition = aStartPosition;
	segment.myTime = 0.0f;
	segment.myOutVelocity = aStartDirection*aInitialSpeed;
	segment.myBounce = false;
	outThrowPath.mySegments.push_back(segment);
	outThrowPath.myEndOrientation = FVector3(0.0f, 0.0f, 0.0f);

	float pathTime = 0.0f;
	success = SimulateSegment(aStartPosition, aStartDirection, FVector3(0.0f, 1.0f, 0.0f), FVector3(0.0f, 1.0f, 0.0f), aInitialSpeed, pathTime, 0, 0, outThrowPath, aRuntimeTimeLeft);
	success = success && outThrowPath.mySegments.size() > 0;

	//if (success)
	//{
	//	if (ShouldLandOnNavMesh())
	//	{
	//		SnapToValidNavMeshPosition(aStartPosition, outThrowPath);
	//	}
	//}
	//
	return success;
}

FVector3 ComputeReboundVector(const FVector3& aDirection, const FVector3 &aNormal)
{
	float dot = -2.0f * aDirection.Dot(aNormal);
	FVector3 v((aNormal * dot) + aDirection);
	v.Normalize();
	return v;
}

float AngleBetweenNormalisedVectors(FVector3& a, FVector3& b)
{
	float x1 = a.x;
	float x2 = b.x;
	float y1 = a.y;
	float y2 = b.y;
	float z1 = a.z;
	float z2 = b.z;
	float dot = x1*x2 + y1*y2 + z1*z2; //   between[x1, y1, z1] and [x2, y2, z2]
	float lenSq1 = x1*x1 + y1*y1 + z1*z1;
	float lenSq2 = x2*x2 + y2*y2 + z2*z2;
	float angle = acos(dot / sqrt(lenSq1 * lenSq2));
	
	return angle;
}


bool FThrowableTrajectory::CreateBounce(
	const FVector3& aCurrentPos,
	const FVector3& aLastPos,
	const FVector3& aWeight,
	const FVector3& aStartPos,
	const FVector3& aStartVel,
	const float aHitDistance,
	const float aInitialTime,
	TrajectoryPath& outThrowPath,
	FVector3& outImpactPosition,
	FVector3& outCollisionNormal,
	FVector3& outRebound,
	float& outSpeed,
	float& outPathTime
)
{
	outCollisionNormal = FVector3(0, 1, 0);// aHit.myNormal;

	const bool isGroundCollision = (outCollisionNormal.y >= GroundCollisionNormalY());

	FVector3 impactDirection = (aCurrentPos - aLastPos);
	const float originalSegmentLength = impactDirection.Length();
	impactDirection.Normalize();

	outImpactPosition = aLastPos + impactDirection*(aHitDistance - 0.005f);

	const float segmentLength = (outImpactPosition - aLastPos).Length();

	// Calculate rebound direction
	outRebound = ComputeReboundVector(impactDirection, outCollisionNormal);
	
	// Check angle constrain (between impact direction and the aHit surface)
	const float angleCollision = PI*0.5f - AngleBetweenNormalisedVectors(outCollisionNormal, outRebound);
	const bool isRealizableBounce = true;

	if (!isRealizableBounce || originalSegmentLength <= 0.0f)
		return false;

	float bounceTime, bounceSpeed;
	ApplyFormulaTime(aStartPos, outImpactPosition, aStartVel, bounceTime, bounceSpeed);

	if (bounceSpeed <= 0.f)
		return false;

	// Creating the control point
	const float restitution = 0.5f; /*isGroundCollision ? GroundBounceRestitution() : WallBounceRestitution()*/; // TODO: FIX - restitutaion values?
	outSpeed = bounceSpeed * restitution;

	TrajectoryPathSegment segment;
	segment.myPosition = outImpactPosition;
	segment.myTime = bounceTime + aInitialTime;
	segment.mySurfaceNormal = outCollisionNormal;
	segment.myBounce = true;

	// Detect slide on slops and try and push the object away
	if (!isGroundCollision)
	{
		/*
		static const float probeDistancePercentage = 0.1f;
		FVector3 testProbeFinalPos = outImpactPosition + aWeight * probeDistancePercentage;

		bool slideCollisionDetected = false;

		Core_RayCastResult probeRes;
		if (ShapeCast(aPhysics, outImpactPosition, testProbeFinalPos, myConfig->myObject.myRadius, myConfig->myObject.myHeight, probeRes))
		{
			const Core_RayCastHit& probeHit = probeRes.GetFinalHit();
			FVector3 probeCollisionNormal = probeHit.myNormal;
			if (probeCollisionNormal.y < GroundCollisionNormalY())
			{
				slideCollisionDetected = true;
			}
		}

		if (slideCollisionDetected)
		{
			static const  float minSpeed = 1.8f;
			outSpeed = fmax(minSpeed, outSpeed);
			static const float extraNormalPercent = 0.4f;
			outRebound += (outCollisionNormal * extraNormalPercent);
			outRebound.Normalize();
		}

		if (debugDrawer)
		{
			debugDrawer->CreateLine("grenadeTrajectory", outImpactPosition, testProbeFinalPos, slideCollisionDetected ? 0x22FF0000 : 0x2200FF00);
			if (slideCollisionDetected)
				debugDrawer->CreateLine("grenadeTrajectory", outImpactPosition, outImpactPosition + (outRebound * 0.2f), 0xFF0000FF);
		}
		*/
	}

	// Final Velocity value
	segment.myOutVelocity = outRebound*outSpeed;

	// Next initial current time simulation
	outPathTime = segment.myTime;

	outThrowPath.mySegments.push_back(segment);

	return true;
}


bool FThrowableTrajectory::SimulateSegment(
	const FVector3& aStartPosition,
	const FVector3& aStartDirection,
	const FVector3& aStartNormal,
	const FVector3& aLastNormal,
	const float aInitialSpeed,
	const float aInitialTime,
	const int aRecursionCount,
	int aRadiusExpansionCount,
	TrajectoryPath& outThrowPath,
	const float aRuntimeTimeLeft)
{

	const float radius = 10.0f; //TODO: FIX THIS
	const FVector3 startVelocity = aStartDirection * aInitialSpeed;
	FVector3 currentVelocity = startVelocity;
	float XZSpeed = FVector3(currentVelocity.x, 0.0f, currentVelocity.z).Length();

	// Simulation ends?
	if ((aRecursionCount > 0
		&& XZSpeed < mySharedParameters.myMinXZVelocity
		&& currentVelocity.y < mySharedParameters.myMinYVelocity
		&& (aStartNormal.y >= RestCollisionNormalY())
		&& (aLastNormal.y >= RestCollisionNormalY())
		)
		|| aInitialTime >= mySharedParameters.myMaxSimulationTime
		|| aRecursionCount > 2 /*MaxBounces()*/) // TODO: FIX THIS
	{
		EndPath(aStartNormal, aStartDirection, outThrowPath, aInitialTime, aStartPosition, aInitialSpeed);
		return true;
	}

	FVector3 currentPosition = aStartPosition;
	FVector3 lastPosition = currentPosition;

	// replace bullet physics
	//Core_PhysicsWorld* pPhysics = myWorldModel.GetPhysicsWorld()->GetCorePhysicsWorld();
	FPhysicsWorld* pPhysics = FGame::GetInstance()->GetPhysics();

	const float stepTime = mySharedParameters.mySimulationStepTime;
	float pathTime = aInitialTime + stepTime;
	for (int simulationIndex = 0, maxSimulationSteps = mySharedParameters.myMaxSimulationSteps; simulationIndex < maxSimulationSteps; ++simulationIndex)
	{
		const float expansionPerc = F_Clamp((float)aRadiusExpansionCount / RadiusAdjustSegmentCount(), 0.0f, 1.0f);
		float adjustedRadius = F_Lerp((radius * InitalRadiusPercentage()), radius, expansionPerc);
		float adjustedHeight = F_Lerp((mySharedParameters.myObjectHeight * InitalRadiusPercentage()), mySharedParameters.myObjectHeight, expansionPerc);

		lastPosition = currentPosition;

		// Calculate current position/velocity
		ApplyFormula(aStartPosition, startVelocity, pathTime - aInitialTime, currentPosition, currentVelocity);

		FD3d12Renderer::GetInstance()->GetDebugDrawer()->DrawSphere(currentPosition, 1.0f, FVector3(1.0f, 0,0));


		//*
		FPhysicsWorld::RayCastHit hitResult;
		bool hasHit = pPhysics->RayCast(lastPosition, currentPosition, hitResult);
		
		// Bounce?
		if (hasHit)
		{
			FVector3 impactPosition = hitResult.myPos;
			FVector3 collisionNormal = hitResult.myNormal;
			FVector3 rebound = FVector3(0,1,0);
			float speed;

			FD3d12Renderer::GetInstance()->GetDebugDrawer()->DrawSphere(impactPosition, 1.0f, FVector3(1.0f, 1.0f, 0));

			bool bounceCreated = CreateBounce(
				currentPosition,
				lastPosition,
				myWeight,
				aStartPosition,
				startVelocity,
				(impactPosition-lastPosition).Length(),
				aInitialTime,
				outThrowPath,
				impactPosition,
				collisionNormal,
				rebound,
				speed,
				pathTime
			);

			FD3d12Renderer::GetInstance()->GetDebugDrawer()->DrawSphere(impactPosition, 1.0f, FVector3(1.0f, 1.0f, 0));
			FD3d12Renderer::GetInstance()->GetDebugDrawer()->DrawSphere(lastPosition, 1.0f, FVector3(1.0f, 0.0f, 1.0f));
			FD3d12Renderer::GetInstance()->GetDebugDrawer()->DrawSphere(currentPosition, 1.0f, FVector3(0.0f, 1.0f, 0));

			if (bounceCreated)
			{
				return SimulateSegment(impactPosition, rebound, collisionNormal, aStartNormal, speed, pathTime, aRecursionCount + 1, aRadiusExpansionCount, outThrowPath, aRuntimeTimeLeft);
			}
			else
			{
				// The segment length is 0 i.e. we have not moved so lets end the path
				EndPath(aStartNormal, aStartDirection, outThrowPath, pathTime, lastPosition, aInitialSpeed);
				return true;
			}

		}

		// Regular path
		else
		//*/

		{
			// It's important to increase the expansion count here, only if it isn't bouncing. Bounce must maintain the expansion count or the bounced segment test will bounce (incorrectly) as well if the shape size is growing
			++aRadiusExpansionCount;

			pathTime += stepTime;

			const float timeLeftOnRuntime = aRuntimeTimeLeft;
			if (timeLeftOnRuntime > 0.0f && pathTime >= timeLeftOnRuntime)
			{
				pathTime = timeLeftOnRuntime;
				break;
			}
		}
	}

	EndPath(aStartNormal, currentVelocity.Normalized(), outThrowPath, pathTime, currentPosition, currentVelocity.Length());
	return true;
}

void FThrowableTrajectory::ApplyFormula(const FVector3& anInitPos, const FVector3& anInitVel, const float aTime, FVector3& outPos, FVector3& outVel)
{
	outVel = anInitVel + myWeight * aTime;
	outPos = anInitPos + anInitVel * aTime + ((myWeight * (aTime*aTime) * 0.5f));
}

void FThrowableTrajectory::ApplyFormulaTime(const FVector3& anInitPos, const FVector3& anFinalPos, const FVector3& anInitVel, float& outTime, float& outSpeed)
{

	ApplyFormulaSpeed(anInitPos, anFinalPos, anInitVel, outSpeed);

	// t = Pf - P0 / v0
	if (anInitVel.x != 0.f)
		outTime = fabs((anFinalPos.x - anInitPos.x) / anInitVel.x); // because accelX == 0
	else if (anInitVel.z != 0.f)
		outTime = fabs((anFinalPos.z - anInitPos.z) / anInitVel.z); // because accelZ == 0
	else if (myWeight.y != 0.f)
	{
		// t = vf - v0 / a
		outTime = fabs((outSpeed - anInitVel.y) / myWeight.y);
	}
	else
		outTime = 0.f;
}

void FThrowableTrajectory::ApplyFormulaSpeed(const FVector3& anInitPos, const FVector3& anFinalPos, const FVector3& anInitVel, float& outSpeed)
{
	// vf = sqrt( v0^2 + 2a(Pf-P0) )

	float initSpeed = anInitVel.Length();
	float accel = myWeight.Length();
	float d = (anFinalPos - anInitPos).Length();
	outSpeed = sqrt((initSpeed*initSpeed) + accel * d * 2.0f);
}

void FThrowableTrajectory::EndPath(const FVector3& aNormal, const FVector3& aDirection, TrajectoryPath& outThrowPath, const float aPathTime, const FVector3& aPos, const float aSpeed)
{
	TrajectoryPathSegment segment;
	segment.myPosition = aPos;
	segment.myInVelocity = aDirection*aSpeed;
	segment.mySurfaceNormal = aNormal;
	segment.myTime = aPathTime;
	segment.myBounce = false;
	outThrowPath.mySegments.push_back(segment);

	FVector3 endUp = aNormal;
	if (true /*SnapToUp()*/)
	{
		endUp = FVector3(0.0f, 1.0f, 0.0f);
	}
	outThrowPath.myEndOrientation = FVector3(1.0f, 0.0f, 0.0f);  //locThrowableItemBallisticMovementEndOrientation(endUp, aDirection);
}