#pragma once
#include "FVector3.h"
#include <vector>

class FThrowableTrajectory
{
public:

	struct SharedParams
	{
		SharedParams()
			: myMinXZVelocity(10.0f)
			, myMinYVelocity(10.0f)
			, myRestCollisionNormalY(0.7f)
			, myGroundCollisionNormalY(0.7f)
			, myMaxSimulationTime(20.0f)
			, myMaxSimulationSteps(100)
			, mySimulationStepTime(0.2f)
			, myObjectHeight(2.0f)
			, myInitalRadiusPercentage(5.0f)
			, myRadiusAdjustSegmentCount(5)
		{
		}

		float myMinXZVelocity;
		float myMinYVelocity;
		float myRestCollisionNormalY;
		float myGroundCollisionNormalY;
		float myMaxSimulationTime;
		float mySimulationStepTime;
		float myObjectHeight;
		int myMaxSimulationSteps;
		float myInitalRadiusPercentage;
		int myRadiusAdjustSegmentCount;
	};

	struct TrajectoryPathSegment
	{
		FVector3 myPosition;
		FVector3 myInVelocity;
		FVector3 myOutVelocity;
		FVector3 mySurfaceNormal;
		float myTime;
		bool myBounce;

	};

	struct TrajectoryPath
	{
		FVector3 myEndOrientation;
		std::vector<FThrowableTrajectory::TrajectoryPathSegment> mySegments;
	};

	FThrowableTrajectory();
	~FThrowableTrajectory();


	void ApplyFormulaSpeed(const FVector3& anInitPos, const FVector3& anFinalPos, const FVector3& anInitVel, float& outSpeed);
	bool Simulate(const FVector3& aStartPosition, const FVector3& aStartDirection, float aInitialSpeed, TrajectoryPath& outThrowPath, const float aRuntimeTimeLeft);
	void ApplyFormula(const FVector3& anInitPos, const FVector3& anInitVel, const float aTime, FVector3& outPos, FVector3& outVel);
	void ApplyFormulaTime(const FVector3& anInitPos, const FVector3& anFinalPos, const FVector3& anInitVel, float& outTime, float& outSpeed);
	bool SimulateSegment(
		const FVector3& aStartPosition,
		const FVector3& aStartDirection,
		const FVector3& aStartNormal,
		const FVector3& aLastNormal,
		const float aInitialSpeed,
		const float aInitialTime,
		const int aRecursionCount,
		int aRadiusExpansionCount,
		TrajectoryPath& outThrowPath,
		const float aRuntimeTimeLeft);
	void EndPath(const FVector3& aNormal, const FVector3& aDirection, TrajectoryPath& outThrowPath, const float aPathTime, const FVector3& aPos, const float aSpeed);

	bool CreateBounce(
		const FVector3& aCurrentPos,
		const FVector3& aLastPos,
		const FVector3& aWeight,
		const FVector3& aStartPos,
		const FVector3& aStartVel,
		const float aInitialTime,
		const float aHitDistance,
		TrajectoryPath& outThrowPath,
		FVector3& outImpactPosition,
		FVector3& outCollisionNormal,
		FVector3& outRebound,
		float& outSpeed,
		float& outPathTime
	);
};

