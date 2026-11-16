#include "YogFogComputeCaptureLibrary.h"

#include "CoreGlobals.h"
#include "Dom/JsonObject.h"
#include "DynamicRHI.h"
#include "Engine/TextureRenderTargetVolume.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformTime.h"
#include "ImageUtils.h"
#include "Math/Float16Color.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "RenderingThread.h"
#include "RHI.h"
#include "RHICommandList.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "TextureResource.h"

DEFINE_LOG_CATEGORY_STATIC(LogYogFogComputeCapture, Log, All);

namespace YogFogComputeCapture
{
constexpr int32 MaxAxis = 512;
constexpr int64 MaxVoxelCount = 16ll * 1024ll * 1024ll;
constexpr int32 ChannelCount = 4;
static_assert(sizeof(FFloat16Color) == ChannelCount * sizeof(uint16), "Unexpected RGBA16F readback layout.");
const TCHAR* const Channels[ChannelCount] = { TEXT("R"), TEXT("G"), TEXT("B"), TEXT("A") };

bool Fail(const FString& Message)
{
    UE_LOG(LogYogFogComputeCapture, Error, TEXT("CaptureVolume: %s"), *Message);
    return false;
}

// A lexical Saved-dir boundary alone would allow a junction inside that boundary to escape it.
bool HasLinkedAncestor(const FString& Directory, const FString& AllowedRoot)
{
    IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
    FString Candidate = Directory;
    for (;;)
    {
        if (PlatformFile.IsSymlink(*Candidate) == ESymlinkResult::Symlink) return true;
        if (FPaths::IsSamePath(Candidate, AllowedRoot)) return false;
        const FString Parent = FPaths::GetPath(Candidate);
        if (Parent.IsEmpty() || Parent == Candidate) return true;
        Candidate = Parent;
    }
}

struct FChannelStats
{
    double Min = MAX_dbl;
    double Max = -MAX_dbl;
    double Sum = 0.;
    int64 FiniteCount = 0;
    int64 NonFiniteCount = 0;

    void Add(float Value)
    {
        if (!FMath::IsFinite(Value)) { ++NonFiniteCount; return; }
        Min = FMath::Min(Min, static_cast<double>(Value));
        Max = FMath::Max(Max, static_cast<double>(Value));
        Sum += Value;
        ++FiniteCount;
    }

    FColor Preview(float Value) const
    {
        if (!FMath::IsFinite(Value)) return FColor(255, 0, 255, 255);
        const double Range = Max - Min;
        const double Normalized = FiniteCount > 0 && Range > 0.
            ? (static_cast<double>(Value) - Min) / Range : (Value == 0.f ? 0. : .5);
        const uint8 Gray = static_cast<uint8>(FMath::Clamp(FMath::RoundToInt(Normalized * 255.), 0, 255));
        return FColor(Gray, Gray, Gray, 255);
    }

    TSharedPtr<FJsonObject> ToJson() const
    {
        TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
        Result->SetNumberField(TEXT("finite_count"), static_cast<double>(FiniteCount));
        Result->SetNumberField(TEXT("nonfinite_count"), static_cast<double>(NonFiniteCount));
        if (FiniteCount > 0)
        {
            Result->SetNumberField(TEXT("min"), Min);
            Result->SetNumberField(TEXT("max"), Max);
            Result->SetNumberField(TEXT("avg"), Sum / static_cast<double>(FiniteCount));
        }
        else
        {
            Result->SetField(TEXT("min"), MakeShared<FJsonValueNull>());
            Result->SetField(TEXT("max"), MakeShared<FJsonValueNull>());
            Result->SetField(TEXT("avg"), MakeShared<FJsonValueNull>());
        }
        return Result;
    }
};

float ReadChannel(const FFloat16Color& Sample, int32 Channel)
{
    switch (Channel)
    {
    case 0: return Sample.R.GetFloat();
    case 1: return Sample.G.GetFloat();
    case 2: return Sample.B.GetFloat();
    default: return Sample.A.GetFloat();
    }
}

TArray<TSharedPtr<FJsonValue>> Dimensions(int32 X, int32 Y, int32 Z)
{
    return { MakeShared<FJsonValueNumber>(X), MakeShared<FJsonValueNumber>(Y), MakeShared<FJsonValueNumber>(Z) };
}

bool SavePNG(const FString& Filename, const TArray<FColor>& Pixels, int32 Width, int32 Height)
{
    // Engine's helper encodes FColor/BGRA8 correctly; no dependency on an image wrapper plugin.
    const FImageView Image(Pixels.GetData(), Width, Height, EGammaSpace::sRGB);
    return FImageUtils::SaveImageByExtension(*Filename, Image);
}

bool SaveSlice(const FString& OutputDirectory, const TCHAR* Plane, int32 FixedAxis,
    const FIntVector& Size, const TArray<FFloat16Color>& Samples,
    const FChannelStats (&Stats)[ChannelCount], TArray<TSharedPtr<FJsonValue>>& OutMetadata)
{
    const int32 Width = FixedAxis == 0 ? Size.Y : Size.X;
    const int32 Height = FixedAxis == 2 ? Size.Y : Size.Z;
    const int32 FixedIndex = (FixedAxis == 0 ? Size.X : FixedAxis == 1 ? Size.Y : Size.Z) / 2;
    TArray<FColor> RPreview;
    TArray<FColor> Panels;
    RPreview.SetNumUninitialized(Width * Height);
    Panels.SetNumUninitialized(Width * Height * ChannelCount);
    for (int32 V = 0; V < Height; ++V)
    {
        for (int32 U = 0; U < Width; ++U)
        {
            const int32 X = FixedAxis == 0 ? FixedIndex : U;
            const int32 Y = FixedAxis == 0 ? U : FixedAxis == 1 ? FixedIndex : V;
            const int32 Z = FixedAxis == 2 ? FixedIndex : V;
            const FFloat16Color& Sample = Samples[X + Size.X * (Y + Size.Y * Z)];
            RPreview[U + Width * V] = Stats[0].Preview(ReadChannel(Sample, 0));
            for (int32 C = 0; C < ChannelCount; ++C)
                Panels[C * Width + U + Width * ChannelCount * V] = Stats[C].Preview(ReadChannel(Sample, C));
        }
    }
    const FString RFile = FString(Plane) + TEXT(".png");
    const FString PanelsFile = FString(Plane) + TEXT("_RGBA.png");
    if (!SavePNG(FPaths::Combine(OutputDirectory, RFile), RPreview, Width, Height)
        || !SavePNG(FPaths::Combine(OutputDirectory, PanelsFile), Panels, Width * ChannelCount, Height)) return false;
    TSharedPtr<FJsonObject> Info = MakeShared<FJsonObject>();
    Info->SetStringField(TEXT("plane"), Plane);
    Info->SetNumberField(TEXT("fixed_axis_xyz"), FixedAxis);
    Info->SetNumberField(TEXT("fixed_index"), FixedIndex);
    Info->SetNumberField(TEXT("width"), Width);
    Info->SetNumberField(TEXT("height"), Height);
    Info->SetStringField(TEXT("horizontal_axis"), FixedAxis == 0 ? TEXT("Y") : TEXT("X"));
    Info->SetStringField(TEXT("vertical_axis"), FixedAxis == 2 ? TEXT("Y") : TEXT("Z"));
    Info->SetStringField(TEXT("r_preview"), RFile);
    Info->SetStringField(TEXT("rgba_panels_left_to_right"), PanelsFile);
    OutMetadata.Add(MakeShared<FJsonValueObject>(Info));
    return true;
}
}

bool UYogFogComputeCaptureLibrary::CaptureVolume(UTextureRenderTargetVolume* Field, const FString& AbsoluteOutputDirectory)
{
    using namespace YogFogComputeCapture;
    if (!IsInGameThread()) return Fail(TEXT("Explicit captures must be called on the game thread."));
    if (!IsValid(Field)) return Fail(TEXT("A valid render-target volume is required."));
    if (!GDynamicRHI || GUsingNullRHI) return Fail(TEXT("GPU readback requires an initialized, non-Null RHI."));
    // These desktop implementations were inspected for an exact, tightly packed RGBA16F readback.
    if (RHIGetInterfaceType() != ERHIInterfaceType::D3D12 && RHIGetInterfaceType() != ERHIInterfaceType::D3D11)
        return Fail(TEXT("This debug capture currently supports D3D12/D3D11 only."));
    if (Field->GetFormat() != PF_FloatRGBA) return Fail(TEXT("Only PF_FloatRGBA / RGBA16F volumes are supported."));
    const FIntVector Size(Field->SizeX, Field->SizeY, Field->SizeZ);
    if (Size.X <= 0 || Size.Y <= 0 || Size.Z <= 0 || Size.X > MaxAxis || Size.Y > MaxAxis || Size.Z > MaxAxis)
        return Fail(TEXT("Volume dimensions must be between 1 and 512 on every axis."));
    const int64 VoxelCount = static_cast<int64>(Size.X) * Size.Y * Size.Z;
    if (VoxelCount > MaxVoxelCount) return Fail(TEXT("Capture exceeds the 16-million-voxel debug allocation limit."));

    FString OutputDirectory = AbsoluteOutputDirectory.TrimStartAndEnd();
    if (OutputDirectory.IsEmpty() || FPaths::IsRelative(OutputDirectory)) return Fail(TEXT("Output directory must be absolute."));
    FPaths::NormalizeDirectoryName(OutputDirectory);
    if (!FPaths::CollapseRelativeDirectories(OutputDirectory)) return Fail(TEXT("Invalid output directory."));
    OutputDirectory = FPaths::ConvertRelativePathToFull(OutputDirectory);
    FPaths::NormalizeDirectoryName(OutputDirectory);
    TArray<FString> PathParts;
    OutputDirectory.ParseIntoArray(PathParts, TEXT("/"), true);
    for (int32 I = 0; I < PathParts.Num(); ++I)
    {
        // Windows strips trailing dots/spaces and supports colon streams; reject aliases before IO.
        const FString& Part = PathParts[I];
        const bool bDrive = I == 0 && Part.Len() == 2 && FChar::IsAlpha(Part[0]) && Part[1] == TEXT(':');
        if (Part.EndsWith(TEXT(".")) || Part.EndsWith(TEXT(" "))
            || (!bDrive && Part.Contains(TEXT(":"))) || Part.Contains(TEXT("*")) || Part.Contains(TEXT("?"))
            || Part.Contains(TEXT("\"")) || Part.Contains(TEXT("<")) || Part.Contains(TEXT(">")) || Part.Contains(TEXT("|")))
            return Fail(TEXT("Output directory contains an invalid/ambiguous path component."));
    }
    FString AllowedRoot = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Stage12/Compute")));
    FPaths::NormalizeDirectoryName(AllowedRoot);
    if (FPaths::IsSamePath(OutputDirectory, AllowedRoot) || !FPaths::IsUnderDirectory(OutputDirectory, AllowedRoot))
        return Fail(TEXT("Output must be a child directory of this project's Saved/Stage12/Compute."));
    if (HasLinkedAncestor(OutputDirectory, AllowedRoot)) return Fail(TEXT("Output path contains a symlink/junction."));
    if (!IFileManager::Get().MakeDirectory(*OutputDirectory, true) || HasLinkedAncestor(OutputDirectory, AllowedRoot))
        return Fail(TEXT("Could not create a safe capture directory."));
    const TCHAR* const OutputFiles[] = { TEXT("Field.rgba16f.raw"), TEXT("Metadata.json"), TEXT("XY.png"), TEXT("XZ.png"),
        TEXT("YZ.png"), TEXT("XY_RGBA.png"), TEXT("XZ_RGBA.png"), TEXT("YZ_RGBA.png") };
    for (const TCHAR* File : OutputFiles)
    {
        const FString Filename = FPaths::Combine(OutputDirectory, File);
        if (FPlatformFileManager::Get().GetPlatformFile().IsSymlink(*Filename) == ESymlinkResult::Symlink
            || IFileManager::Get().FileExists(*Filename))
            return Fail(TEXT("Capture files already exist. Use a fresh per-capture directory; captures are not overwritten."));
    }

    FTextureRenderTargetResource* Resource = Field->GameThread_GetRenderTargetResource();
    if (!Resource || !Resource->GetTextureRenderTargetVolumeResource()) return Fail(TEXT("Volume render resource is unavailable."));
    TArray<FFloat16Color> Samples;
    FString ReadbackError;
    const double ReadbackStart = FPlatformTime::Seconds();
    const uint64 CaptureFrame = GFrameCounter;
    const FString CaptureTime = FDateTime::UtcNow().ToIso8601();
    // References below remain valid because the game thread waits for this command before returning.
    // The RHI helper also submits/blocks the GPU copy; this is intentionally never a Tick path.
    ENQUEUE_RENDER_COMMAND(YogCaptureFogComputeVolume)(
        [Resource, Size, &Samples, &ReadbackError](FRHICommandListImmediate& RHICmdList)
        {
            const FTextureRHIRef Texture = Resource->GetRenderTargetTexture();
            if (!Texture.IsValid() || !Texture->GetDesc().IsTexture3D() || Texture->GetFormat() != PF_FloatRGBA
                || Texture->GetSizeXYZ() != Size)
            {
                ReadbackError = TEXT("Actual RHI volume format/dimensions do not match the requested RGBA16F resource.");
                return;
            }
            // D3D readback removes row/depth pitch. RCM_MinMax avoids value remapping.
            RHICmdList.Read3DSurfaceFloatData(Texture.GetReference(), FIntRect(0, 0, Size.X, Size.Y),
                FIntPoint(0, Size.Z), Samples, FReadSurfaceDataFlags(RCM_MinMax));
        });
    FlushRenderingCommands();
    const double ReadbackSeconds = FPlatformTime::Seconds() - ReadbackStart;
    if (!ReadbackError.IsEmpty()) return Fail(ReadbackError);
    if (Samples.Num() != VoxelCount) return Fail(TEXT("GPU readback returned an unexpected voxel count."));

    FChannelStats Stats[ChannelCount];
    TArray64<uint8> Raw;
    Raw.SetNumUninitialized(VoxelCount * ChannelCount * sizeof(uint16));
    for (int32 I = 0; I < Samples.Num(); ++I)
    {
        const uint16* Halves = Samples[I].GetFourHalves();
        for (int32 C = 0; C < ChannelCount; ++C)
        {
            // Explicit LE stores preserve the original binary16 payload, including NaN/Inf/sign bits.
            const int64 Byte = (static_cast<int64>(I) * ChannelCount + C) * sizeof(uint16);
            Raw[Byte] = static_cast<uint8>(Halves[C] & 0xff);
            Raw[Byte + 1] = static_cast<uint8>(Halves[C] >> 8);
            Stats[C].Add(ReadChannel(Samples[I], C));
        }
    }
    const FString RawHash = FSHA1::HashBuffer(Raw.GetData(), static_cast<uint64>(Raw.Num())).ToString().ToLower();
    if (!FFileHelper::SaveArrayToFile(Raw, *FPaths::Combine(OutputDirectory, TEXT("Field.rgba16f.raw"))))
        return Fail(TEXT("Could not write raw GPU volume data."));
    TArray<TSharedPtr<FJsonValue>> Slices;
    if (!SaveSlice(OutputDirectory, TEXT("XY"), 2, Size, Samples, Stats, Slices)
        || !SaveSlice(OutputDirectory, TEXT("XZ"), 1, Size, Samples, Stats, Slices)
        || !SaveSlice(OutputDirectory, TEXT("YZ"), 0, Size, Samples, Stats, Slices))
        return Fail(TEXT("Could not write orthogonal PNG slices. No completion metadata was written."));

    TSharedPtr<FJsonObject> Metadata = MakeShared<FJsonObject>();
    Metadata->SetNumberField(TEXT("schema_version"), 1);
    Metadata->SetStringField(TEXT("capture_utc"), CaptureTime);
    Metadata->SetNumberField(TEXT("game_frame_counter"), static_cast<double>(CaptureFrame));
    Metadata->SetStringField(TEXT("field_object"), Field->GetPathName());
    Metadata->SetStringField(TEXT("rhi"), GDynamicRHI->GetName());
    Metadata->SetStringField(TEXT("format"), TEXT("PF_FloatRGBA / RGBA16F"));
    Metadata->SetArrayField(TEXT("dimensions_xyz"), Dimensions(Size.X, Size.Y, Size.Z));
    TArray<TSharedPtr<FJsonValue>> NumpyShape = Dimensions(Size.Z, Size.Y, Size.X);
    NumpyShape.Add(MakeShared<FJsonValueNumber>(ChannelCount));
    Metadata->SetArrayField(TEXT("array_shape_zyxc"), NumpyShape);
    Metadata->SetStringField(TEXT("storage"), TEXT("IEEE-754 binary16 little-endian; interleaved RGBA; X fastest, then Y, then Z; no padding"));
    Metadata->SetStringField(TEXT("raw_file"), TEXT("Field.rgba16f.raw"));
    Metadata->SetNumberField(TEXT("raw_byte_count"), static_cast<double>(Raw.Num()));
    Metadata->SetStringField(TEXT("hash_algorithm"), TEXT("SHA-1"));
    Metadata->SetStringField(TEXT("sha1"), RawHash);
    Metadata->SetStringField(TEXT("numpy_read"), FString::Printf(TEXT("np.fromfile('Field.rgba16f.raw', dtype='<f2').reshape((%d,%d,%d,4))"), Size.Z, Size.Y, Size.X));
    Metadata->SetStringField(TEXT("channel_semantics"), TEXT("Generic capture does not assign density/velocity semantics; use the simulation's documented channel contract."));
    Metadata->SetBoolField(TEXT("synchronous_gpu_readback"), true);
    Metadata->SetNumberField(TEXT("readback_and_flush_seconds"), ReadbackSeconds);
    Metadata->SetBoolField(TEXT("usable_as_realtime_performance_measurement"), false);
    TSharedPtr<FJsonObject> ChannelStatistics = MakeShared<FJsonObject>();
    bool bAllFinite = true;
    for (int32 C = 0; C < ChannelCount; ++C)
    {
        ChannelStatistics->SetObjectField(Channels[C], Stats[C].ToJson());
        bAllFinite &= Stats[C].NonFiniteCount == 0;
    }
    Metadata->SetObjectField(TEXT("channel_statistics"), ChannelStatistics);
    Metadata->SetBoolField(TEXT("all_values_finite"), bAllFinite);
    Metadata->SetArrayField(TEXT("middle_slices"), Slices);
    Metadata->SetStringField(TEXT("png_display"), TEXT("R grayscale and left-to-right RGBA grayscale panels. Each channel uses this capture's full-volume finite min/max; constant nonzero=0.5, constant zero=0; nonfinite=magenta. PNGs are qualitative, opaque, and not raw-value measurements; no row/axis flip."));
    FString Json;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
    if (!FJsonSerializer::Serialize(Metadata.ToSharedRef(), Writer)
        || !FFileHelper::SaveStringToFile(Json, *FPaths::Combine(OutputDirectory, TEXT("Metadata.json")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
        return Fail(TEXT("Could not write completion metadata."));
    UE_LOG(LogYogFogComputeCapture, Display, TEXT("Captured GPU volume %dx%dx%d into %s (SHA1 %s, finite=%s)."),
        Size.X, Size.Y, Size.Z, *OutputDirectory, *RawHash, bAllFinite ? TEXT("true") : TEXT("false"));
    return true;
}
