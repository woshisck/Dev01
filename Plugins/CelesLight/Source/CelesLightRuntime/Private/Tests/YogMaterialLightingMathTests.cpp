#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace YogMaterialLightingMathTests
{
	// CPU mirrors of Engine/Shaders/Private/YogMaterialLighting.ush. These
	// regress the mathematical contract, not shader compilation or rendered GPU
	// output. GPU material/light/temporal acceptance remains a separate gate.
	float Saturate(float Value) { return FMath::Clamp(Value, 0.0f, 1.0f); }
	float SmoothStep(float Lower, float Upper, float Value)
	{
		const float T = Saturate((Value - Lower) / (Upper - Lower));
		return T * T * (3.0f - 2.0f * T);
	}

	FVector3f Response(FVector3f Value, FVector4f Parameters)
	{
		if (Parameters.Z <= 0.0f || Parameters.X == 1.0f) { return Value; }
		const float Y = FMath::Max(Value.X * 0.2126f + Value.Y * 0.7152f + Value.Z * 0.0722f, 0.0f);
		const float Pivot = FMath::Max(Parameters.Y, 1.0e-20f);
		const float Position = Y / (Y + Pivot);
		const float Gain = FMath::Clamp(1.0f + (Parameters.X - 1.0f) * (2.0f * Position - 1.0f), 0.35f, 2.0f);
		return Value * FMath::Lerp(1.0f, Gain, Saturate(Parameters.Z));
	}

	float DiffuseGain(float NoL, FVector4f Parameters)
	{
		if (Parameters.X == 1.0f && Parameters.Z <= 0.0f) { return 1.0f; }
		const float T = Saturate(NoL);
		const float Continuous = FMath::Clamp(1.0f + (Parameters.X - 1.0f) * (2.0f * T - 1.0f), 0.35f, 2.0f);
		const float Steps = FMath::Max(FMath::RoundToFloat(Parameters.Y) - 1.0f, 1.0f);
		const float Position = T * Steps;
		const float Fraction = Position - FMath::FloorToFloat(Position);
		const float Softness = FMath::Clamp(Parameters.W, 0.05f, 0.5f);
		const float Ramp = (FMath::FloorToFloat(Position) + SmoothStep(0.5f - Softness, 0.5f + Softness, Fraction)) / Steps;
		const float RampGain = FMath::Clamp(1.0f + 2.0f * (Ramp - T), 0.35f, 2.0f);
		return FMath::Clamp(Continuous * FMath::Lerp(1.0f, RampGain, Saturate(Parameters.Z)), 0.35f, 2.0f);
	}

	float ShadowVisibility(float Visibility, float Contrast)
	{
		if (Contrast == 1.0f || Visibility <= 0.0f || Visibility >= 1.0f) { return Visibility; }
		const float A = FMath::Pow(Visibility, Contrast);
		const float B = FMath::Pow(1.0f - Visibility, Contrast);
		return A / FMath::Max(A + B, 1.0e-6f);
	}

	// Double precision is intentional for numerical quadrature. The formula
	// and clamps mirror HLSL; this does not establish GPU float precision.
	double GGXDistribution(double AlphaSquared, double NoH, double Falloff)
	{
		const double A2 = FMath::Clamp(AlphaSquared, 1.0e-8, 1.0);
		const double P = 2.0 * FMath::Clamp(Falloff, 1.0, 4.0);
		if (A2 > 0.9999) { return 0.31830988618; }
		const double T = FMath::Clamp(NoH, 0.0, 1.0);
		const double NoH2 = T * T;
		const double D = FMath::Max((1.0 - NoH2) + A2 * NoH2, A2);
		const double LogA = FMath::Loge(A2);
		const double Normalization = (P - 1.0) * (1.0 - A2) * 0.31830988618;
		return Normalization * FMath::Exp((P - 1.0) * LogA - P * FMath::Loge(D))
			/ FMath::Max(1.0 - FMath::Exp((P - 1.0) * LogA), 1.0e-6);
	}

	float GGXDistributionFloat(float AlphaSquared, float NoH, float Falloff)
	{
		const float A2 = FMath::Clamp(AlphaSquared, 1.0e-8f, 1.0f);
		const float P = 2.0f * FMath::Clamp(Falloff, 1.0f, 4.0f);
		if (A2 > 0.9999f) { return 0.31830988618f; }
		const float T = Saturate(NoH);
		const float NoH2 = T * T;
		const float D = FMath::Max((1.0f - NoH2) + A2 * NoH2, A2);
		const float LogA = FMath::Loge(A2);
		const float Normalization = (P - 1.0f) * (1.0f - A2) * 0.31830988618f;
		return Normalization * FMath::Exp((P - 1.0f) * LogA - P * FMath::Loge(D))
			/ FMath::Max(1.0f - FMath::Exp((P - 1.0f) * LogA), 1.0e-6f);
	}

	double NativeGGXDistribution(double AlphaSquared, double NoH)
	{
		const double D = 1.0 + (AlphaSquared - 1.0) * NoH * NoH;
		return AlphaSquared / (UE_DOUBLE_PI * D * D);
	}

	double AdaptiveProjectedIntegral(double A2, double Falloff, double Lower, double Upper,
		double FL, double FM, double FU, double Previous, double Tolerance, int32 Depth)
	{
		const double Middle = (Lower + Upper) * 0.5;
		// u = cos(theta)^2, so 2*pi*D*cos(theta)*dcos(theta) = pi*D*du.
		const double LeftMiddle = UE_DOUBLE_PI * GGXDistribution(A2, FMath::Sqrt((Lower + Middle) * 0.5), Falloff);
		const double RightMiddle = UE_DOUBLE_PI * GGXDistribution(A2, FMath::Sqrt((Middle + Upper) * 0.5), Falloff);
		const double Left = (Middle - Lower) * (FL + 4.0 * LeftMiddle + FM) / 6.0;
		const double Right = (Upper - Middle) * (FM + 4.0 * RightMiddle + FU) / 6.0;
		const double Delta = Left + Right - Previous;
		if (Depth <= 0 || FMath::Abs(Delta) <= 15.0 * Tolerance) { return Left + Right + Delta / 15.0; }
		return AdaptiveProjectedIntegral(A2, Falloff, Lower, Middle, FL, LeftMiddle, FM, Left, Tolerance * 0.5, Depth - 1)
			+ AdaptiveProjectedIntegral(A2, Falloff, Middle, Upper, FM, RightMiddle, FU, Right, Tolerance * 0.5, Depth - 1);
	}

	double ProjectedIntegral(double A2, double Falloff)
	{
		const double FL = UE_DOUBLE_PI * GGXDistribution(A2, 0.0, Falloff);
		const double FM = UE_DOUBLE_PI * GGXDistribution(A2, FMath::Sqrt(0.5), Falloff);
		const double FU = UE_DOUBLE_PI * GGXDistribution(A2, 1.0, Falloff);
		return AdaptiveProjectedIntegral(A2, Falloff, 0.0, 1.0, FL, FM, FU, (FL + 4.0 * FM + FU) / 6.0, 1.0e-7, 26);
	}

	bool RelativeNear(float Actual, float Expected, float Tolerance = 3.0e-6f)
	{
		return FMath::Abs(Actual - Expected) <= FMath::Max(FMath::Abs(Expected), 1.0e-20f) * Tolerance;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogMaterialLightingIndirectMathTest,
	"CelesLight.MaterialLightingMath.IndirectLobeContract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogMaterialLightingIndirectMathTest::RunTest(const FString& Parameters)
{
	using namespace YogMaterialLightingMathTests;
	TestTrue(TEXT("Zero GI stays exactly zero"), Response(FVector3f::ZeroVector, FVector4f(2.0f, 0.18f, 1.0f, 0.0f)) == FVector3f::ZeroVector);
	const FVector3f Low(0.0001f, 0.0001f, 0.0001f);
	const FVector3f LowResult = Response(Low, FVector4f(2.0f, 0.18f, 1.0f, 0.0f));
	TestTrue(TEXT("Low-energy GI is not rounded to black"), LowResult.X > 0.0f && LowResult.Y > 0.0f && LowResult.Z > 0.0f);
	const FVector3f Color(0.041f, 0.137f, 0.293f);
	TestTrue(TEXT("Contrast-one response is exact identity"), Response(Color, FVector4f(1.0f, 0.18f, 1.0f, 0.0f)) == Color);
	TestTrue(TEXT("Amount-zero response is exact identity"), Response(Color, FVector4f(2.0f, 0.18f, 0.0f, 0.0f)) == Color);
	bool bBounded = true, bHuePreserved = true;
	for (float Contrast : {0.5f, 1.0f, 1.3f, 2.0f, 4.0f})
	for (float Amount : {0.0f, 0.25f, 1.0f})
	for (float Energy : {0.0001f, 0.01f, 0.18f, 1.0f, 10.0f, 1000.0f})
	{
		const FVector3f Input = Color * Energy;
		const FVector3f Output = Response(Input, FVector4f(Contrast, 0.18f, Amount, 0.0f));
		const float Gain = Output.X / Input.X;
		bBounded &= FMath::IsFinite(Gain) && Gain >= 0.35f - 1.0e-6f && Gain <= 2.0f + 1.0e-6f;
		bHuePreserved &= RelativeNear(Output.Y / Input.Y, Gain) && RelativeNear(Output.Z / Input.Z, Gain);
	}
	TestTrue(TEXT("Indirect gain is finite and bounded 0.35 to 2"), bBounded);
	TestTrue(TEXT("RGB ratios and hue are preserved"), bHuePreserved);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogMaterialLightingExposureMathTest,
	"CelesLight.MaterialLightingMath.PreExposureScaleInvariant", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogMaterialLightingExposureMathTest::RunTest(const FString& Parameters)
{
	using namespace YogMaterialLightingMathTests;
	bool bInvariant = true, bNeutralExact = true;
	for (float Contrast : {0.5f, 1.0f, 1.3f, 2.0f})
	for (float Energy : {0.0001f, 0.01f, 0.18f, 1.0f, 1000.0f})
	for (float PreExposure : {1.0e-4f, 0.01f, 1.0f, 100.0f, 1.0e4f})
	{
		const FVector3f Input = FVector3f(0.11f, 0.31f, 0.73f) * Energy;
		const FVector3f Expected = Response(Input, FVector4f(Contrast, 0.18f, 1.0f, 0.0f)) * PreExposure;
		const FVector3f Actual = Response(Input * PreExposure, FVector4f(Contrast, 0.18f * PreExposure, 1.0f, 0.0f));
		bInvariant &= RelativeNear(Actual.X, Expected.X) && RelativeNear(Actual.Y, Expected.Y) && RelativeNear(Actual.Z, Expected.Z);
		bNeutralExact &= Response(Input * PreExposure, FVector4f(1.0f, 0.18f * PreExposure, 1.0f, 0.0f)) == Input * PreExposure;
	}
	TestTrue(TEXT("Pivot follows PreExposure without changing scene-linear response"), bInvariant);
	TestTrue(TEXT("Neutral pre-exposed input is exactly unchanged"), bNeutralExact);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogMaterialLightingDiffuseShadowMathTest,
	"CelesLight.MaterialLightingMath.DiffuseAndVisibilityContract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogMaterialLightingDiffuseShadowMathTest::RunTest(const FString& Parameters)
{
	using namespace YogMaterialLightingMathTests;
	bool bBounded = true, bNeutral = true;
	for (float Contrast : {0.5f, 1.0f, 2.0f, 4.0f})
	for (float Bands : {2.0f, 3.0f, 5.0f, 12.0f})
	for (float Ramp : {0.0f, 0.35f, 1.0f})
	for (float Softness : {0.05f, 0.18f, 0.5f})
	for (int32 Index = 0; Index <= 128; ++Index)
	{
		const float NoL = Index / 128.0f;
		const float Gain = DiffuseGain(NoL, FVector4f(Contrast, Bands, Ramp, Softness));
		bBounded &= FMath::IsFinite(Gain) && Gain >= 0.35f && Gain <= 2.0f;
		bNeutral &= DiffuseGain(NoL, FVector4f(1.0f, Bands, 0.0f, Softness)) == 1.0f;
	}
	TestTrue(TEXT("Soft/floor ramps yield positive bounded gain, not zero-light replacements"), bBounded);
	TestTrue(TEXT("Neutral direct response is exact one"), bNeutral);
	bool bEndpoints = true, bMonotonic = true, bNativeExact = true;
	for (float Contrast : {0.5f, 1.0f, 1.5f, 2.0f, 4.0f})
	{
		bEndpoints &= ShadowVisibility(0.0f, Contrast) == 0.0f && ShadowVisibility(1.0f, Contrast) == 1.0f;
		float Previous = 0.0f;
		for (int32 Index = 0; Index <= 128; ++Index)
		{
			const float Visibility = Index / 128.0f;
			const float Result = ShadowVisibility(Visibility, Contrast);
			bMonotonic &= FMath::IsFinite(Result) && Result >= Previous && Result >= 0.0f && Result <= 1.0f;
			bNativeExact &= ShadowVisibility(Visibility, 1.0f) == Visibility;
			Previous = Result;
		}
	}
	TestTrue(TEXT("Fully shadowed and visible endpoints are unchanged"), bEndpoints);
	TestTrue(TEXT("Partial visibility response is finite bounded and monotonic"), bMonotonic);
	TestTrue(TEXT("Neutral visibility is exact native"), bNativeExact);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FYogMaterialLightingGGXMathTest,
	"CelesLight.MaterialLightingMath.GeneralizedGGXNormalization", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FYogMaterialLightingGGXMathTest::RunTest(const FString& Parameters)
{
	using namespace YogMaterialLightingMathTests;
	bool bFinite = true, bNativeMatch = true, bNormalized = true;
	double MaxNormalizationError = 0.0, MaxNativeRelativeError = 0.0;
	for (double Roughness : {0.1, 0.2, 0.4, 0.7, 0.9, 1.0})
	{
		const double A2 = Roughness * Roughness * Roughness * Roughness;
		for (double Falloff : {1.0, 1.5, 2.0, 3.0, 4.0})
		{
			const double Integral = ProjectedIntegral(A2, Falloff);
			MaxNormalizationError = FMath::Max(MaxNormalizationError, FMath::Abs(Integral - 1.0));
			bNormalized &= FMath::IsFinite(Integral) && FMath::Abs(Integral - 1.0) < 1.0e-4;
			for (int32 Index = 0; Index <= 128; ++Index)
			{
				const double NoH = Index / 128.0;
				const double Value = GGXDistribution(A2, NoH, Falloff);
				bFinite &= FMath::IsFinite(Value) && Value > 0.0;
				const double Native = NativeGGXDistribution(A2, NoH);
				const double RelativeError = FMath::Abs(GGXDistribution(A2, NoH, 1.0) - Native) / Native;
				MaxNativeRelativeError = FMath::Max(MaxNativeRelativeError, RelativeError);
				bNativeMatch &= RelativeError < 1.0e-8;
			}
		}
	}
	TestTrue(TEXT("GGX is finite positive for roughness 0.1 to 1 and falloff 1 to 4"), bFinite);
	TestTrue(TEXT("Falloff one matches native D_GGX within numerical epsilon"), bNativeMatch);
	TestTrue(TEXT("Projected hemisphere integral remains normalized to one"), bNormalized);
	bool bThinFloatFinite = true, bThinFloatAccurate = true;
	double MaxThinFloatRelativeError = 0.0;
	for (float Roughness : {0.02f, 0.1f, 0.4f, 1.0f})
	for (float Falloff : {1.0f, 1.5f, 2.0f, 3.0f, 4.0f})
	for (float NoH : {0.0f, 0.9f, 0.99f, 0.999f, 0.9999f, 0.999999f, 1.0f})
	{
		const float ThinA2 = Roughness * Roughness * Roughness * Roughness;
		const float Actual = GGXDistributionFloat(ThinA2, NoH, Falloff);
		const double Reference = GGXDistribution(ThinA2, NoH, Falloff);
		// Very narrow/high-falloff far tails can underflow float/flush to zero;
		// this is negligible NDF energy, not a zeroed diffuse GI band.
		const double RelativeError = Reference >= 1.17549435e-38
			? FMath::Abs(static_cast<double>(Actual) - Reference) / Reference : 0.0;
		MaxThinFloatRelativeError = FMath::Max(MaxThinFloatRelativeError, RelativeError);
		bThinFloatFinite &= FMath::IsFinite(Actual) && Actual >= 0.0f && (NoH < 1.0f || Actual > 0.0f);
		// NoH-squared float rounding is amplified in extremely narrow lobes;
		// allow 0.1 percent while still catching unstable/collapsed peaks.
		bThinFloatAccurate &= RelativeError < 1.0e-3;
	}
	TestTrue(TEXT("Float GGX remains finite nonnegative down to roughness 0.02, with positive NoH-one peaks"), bThinFloatFinite);
	TestTrue(TEXT("Stable denominator and log-space GGX agree with double reference"), bThinFloatAccurate);
	const double A2 = 0.4 * 0.4 * 0.4 * 0.4;
	TestTrue(TEXT("Increased falloff narrows the normalized peak"), GGXDistribution(A2, 1.0, 2.0) > GGXDistribution(A2, 1.0, 1.0));
	TestTrue(TEXT("Increased falloff suppresses the tail"), GGXDistribution(A2, 0.6, 2.0) < GGXDistribution(A2, 0.6, 1.0));
	TestTrue(TEXT("Larger GGX width lowers the normalized peak"), GGXDistribution(A2 * 4.0, 1.0, 1.0) < GGXDistribution(A2, 1.0, 1.0));
	TestTrue(TEXT("Larger GGX width broadens the tail"), GGXDistribution(A2 * 4.0, 0.6, 1.0) > GGXDistribution(A2, 0.6, 1.0));
	AddInfo(FString::Printf(TEXT("CPU mathematical mirror only: max projected integral error %.9g, max native GGX relative error %.9g, max thin float GGX relative error %.9g; GPU rendering is separately verified."), MaxNormalizationError, MaxNativeRelativeError, MaxThinFloatRelativeError));
	return true;
}
#endif
