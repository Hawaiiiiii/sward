#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>

#include <sward/ui_runtime/frontend_screen_reference.hpp>
#include <sward/ui_runtime/sgfx_templates.hpp>
#include <sward/ui_runtime/sonic_hud_reference.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cwchar>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace
{
using sward::ui_runtime::SgfxScreenTemplate;
using sward::ui_runtime::SonicHudScenePolicy;
using sward::ui_runtime::defaultFrontendRuntimeAlignment;
using sward::ui_runtime::findSgfxScreenTemplate;
using sward::ui_runtime::formatFrontendRuntimeAlignment;
using sward::ui_runtime::frontendScreenPolicies;
using sward::ui_runtime::sgfxScreenTemplates;
using sward::ui_runtime::sampleSonicHudTimeline;
using sward::ui_runtime::sonicHudOwnerReference;
using sward::ui_runtime::sonicHudScenePolicies;

inline constexpr int kDesignWidth = 1280;
inline constexpr int kDesignHeight = 720;
inline constexpr int kRendererChromeHeight = 44;
inline constexpr double kPi = 3.14159265358979323846;
inline constexpr int kPrevButtonId = 1001;
inline constexpr int kNextButtonId = 1002;
inline constexpr int kScreenLabelId = 1003;
inline constexpr int kAtlasPrevButtonId = 1004;
inline constexpr int kAtlasNextButtonId = 1005;

struct TextureSourceCandidate
{
    std::string_view textureFileName;
    std::string_view relativePath;
};

struct DdsTextureImage
{
    std::filesystem::path path;
    std::string fileName;
    std::string format;
    int width = 0;
    int height = 0;
    std::vector<std::uint32_t> argbPixels;
};

struct SuUiRenderCast
{
    std::string_view sceneName;
    std::string_view castName;
    std::string_view textureName;
    int sourceX = 0;
    int sourceY = 0;
    int sourceWidth = 0;
    int sourceHeight = 0;
    int destinationX = 0;
    int destinationY = 0;
    int destinationWidth = 0;
    int destinationHeight = 0;
};

enum class RendererScreenKind
{
    CastCatalog,
    AtlasGallery,
    TitleLoopReconstruction,
    SonicHudReconstruction,
    CsdReferencePipeline,
    SonicHudReferencePipeline,
};

struct SuUiRendererScreen
{
    std::string_view id;
    std::string_view displayName;
    std::string_view contractFileName;
    const SuUiRenderCast* casts = nullptr;
    std::size_t castCount = 0;
    Gdiplus::Color background = Gdiplus::Color(255, 0, 0, 0);
    RendererScreenKind kind = RendererScreenKind::CastCatalog;
};

struct SgfxPlaceholderAssetSlot
{
    std::string_view slotName;
    std::string_view textureName;
    std::string_view sourceFamily;
};

struct SgfxTemplateRenderBinding
{
    std::string_view templateId;
    std::string_view rendererScreenId;
    const SgfxPlaceholderAssetSlot* slots = nullptr;
    std::size_t slotCount = 0;
    std::string_view requiredEventId;
    std::string_view timelineBandId;
    std::string_view timelineEventLabel;
};

struct CsdPipelineSceneSummary
{
    std::string sceneName;
    int castCount = 0;
    int subimageCount = 0;
    std::vector<std::string> textureNames;
    double frameStart = 0.0;
    double frameEnd = 0.0;
};

struct CsdPipelineTimelineHook
{
    std::string sceneName;
    std::string animationName;
    double frameCount = 0.0;
    double timelineSeconds = 0.0;
    int totalKeyframes = 0;
};

struct CsdPipelineEvidence
{
    std::filesystem::path sourcePath;
    std::string layoutFileName;
    std::vector<std::string> textureNames;
    std::vector<CsdPipelineSceneSummary> scenes;
    std::vector<CsdPipelineTimelineHook> timelines;
};

struct CsdPipelineTemplateBinding
{
    std::string_view templateId;
    std::string_view layoutFileName;
    std::string_view primarySceneName;
    std::string_view timelineSceneName;
    std::string_view timelineAnimationName;
};

struct CsdReferenceViewerLane
{
    std::string laneId;
    std::string rendererScreenId;
    std::string contractFileName;
    std::string nativeTargetId;
    std::vector<CsdPipelineTemplateBinding> scenes;
    std::vector<SgfxPlaceholderAssetSlot> slots;
    std::string requiredEventId;
    std::string timelineBandId;
    std::string timelineEventLabel;
    std::string runtimeAlignment;
    std::string runtimeAlignmentSource;
    std::string runtimeAlignmentEvidence;
    std::string runtimeAlignmentLiveStatePath;
    std::string runtimeAlignmentFieldStatus;
    std::string runtimeAlignmentProbe;
    std::string runtimeAlignmentBridgePipe;
    bool runtimeAlignmentBridgeConnected = false;
    std::string runtimeAlignmentBridgeFallback;
    std::string runtimeAlignmentBridgeError;
    int uiOracleRuntimeFrame = 0;
    int uiOraclePlaybackFrame = 0;
    std::string uiOraclePlaybackClock = "frontend-policy";
    std::string uiOracleFrameSource = "policy-sample-frame";
    std::string uiOracleSource = "missing";
    std::string uiOracleProbe = "missing";
    std::string uiOracleActiveMotionName;
    std::string materialSemantics;
    std::string policySource;
};

struct FrontendLiveBridgeProbeResult
{
    bool attempted = false;
    bool connected = false;
    std::string pipeName = "sward_ui_lab_live";
    std::string command = "state";
    std::string responseJson;
    std::string error;
    std::string fallbackSource;
};

struct FrontendLiveStateAlignmentEvidence
{
    bool found = false;
    std::filesystem::path liveStatePath;
    sward::ui_runtime::FrontendRuntimeAlignment alignment;
    std::string fieldStatus;
    std::string route;
    std::string nativeCaptureStatus;
    std::string fallbackSource;
    FrontendLiveBridgeProbeResult bridgeProbe;
};

struct FrontendUiOracleEvidence
{
    bool found = false;
    std::string source = "missing";
    std::string probe = "missing";
    std::string target;
    std::string activeProject;
    std::string activeMotionName;
    std::string cursorOwner;
    std::string transitionBand;
    std::string inputLockState;
    std::string runtimeDrawListStatus = "missing";
    int runtimeFrame = 0;
    int runtimeSceneMotionFrame = -1;
    int sceneCount = 0;
    int layerCount = 0;
    std::vector<std::string> activeScenes;
    std::filesystem::path liveStatePath;
    FrontendLiveBridgeProbeResult bridgeProbe;
};

struct FrontendUiOraclePlaybackClock
{
    bool found = false;
    std::string source = "missing";
    std::string probe = "missing";
    std::string playbackClock = "frontend-policy";
    std::string frameSource = "policy-sample-frame";
    std::string activeMotionName;
    int runtimeFrame = 0;
    int playbackFrame = 0;
    FrontendUiOracleEvidence oracle;
};

struct FrontendRuntimeDrawableOracleScene
{
    std::string runtimeScenePath;
    std::string localSceneName;
    std::string animationName;
    int timelineFrame = 0;
    int timelineFrameCount = 0;
    std::size_t commandCount = 0;
    std::size_t drawnCommandCount = 0;
    std::size_t sampledTrackCount = 0;
    std::size_t textureCount = 0;
    std::vector<std::string> textureNames;
};

struct FrontendRuntimeDrawableOracle
{
    bool found = false;
    std::string screenId;
    std::string source = "missing";
    std::string probe = "missing";
    std::string activeProject;
    std::string drawableSceneSource = "ui-oracle-active-scenes";
    std::string runtimeDrawableOracleStatus = "runtime-csd-tree-local-material";
    std::string gpuDrawListStatus = "pending";
    int runtimeFrame = 0;
    std::size_t activeSceneCount = 0;
    std::vector<FrontendRuntimeDrawableOracleScene> scenes;
};

struct FrontendRuntimeDrawCall
{
    std::string project;
    std::string layerPath;
    std::string sceneName;
    std::string primitive = "quad";
    std::string colorSample;
    bool textured = false;
    int vertexCount = 0;
    int vertexStride = 0;
    double minX = 0.0;
    double minY = 0.0;
    double maxX = 0.0;
    double maxY = 0.0;
};

struct FrontendRuntimeDrawListEvidence
{
    bool found = false;
    std::string screenId;
    std::string source = "missing";
    std::string probe = "missing";
    std::string activeProject;
    std::string runtimeDrawListStatus = "missing";
    std::string backendSubmitStatus = "pending";
    int runtimeFrame = 0;
    std::vector<FrontendRuntimeDrawCall> calls;
    FrontendLiveBridgeProbeResult bridgeProbe;
};

struct FrontendRuntimeDrawListTriageScene
{
    std::string sceneName;
    std::size_t localCommandCount = 0;
    std::size_t localTextureCount = 0;
    std::size_t runtimeRectCount = 0;
    std::size_t rectMatchCandidates = 0;
};

struct FrontendRuntimeDrawListTriage
{
    std::string screenId;
    std::string source = "missing";
    std::string probe = "missing";
    std::string runtimeDrawListSource = "ui-draw-list";
    std::string materialTriage = "runtime-rectangles-vs-local-csd";
    std::string backendSubmitStatus = "pending";
    std::size_t runtimeCallCount = 0;
    std::size_t runtimeRectCount = 0;
    std::size_t localCommandCount = 0;
    std::size_t localTextureCount = 0;
    std::size_t rectMatchCandidates = 0;
    std::vector<FrontendRuntimeDrawListTriageScene> scenes;
};

struct FrontendGpuSubmitCall
{
    std::string source;
    bool indexed = false;
    bool inlineVertexStream = false;
    int vertexCount = 0;
    int indexCount = 0;
    int instanceCount = 0;
    int texture2DDescriptorIndex = 0;
    int samplerDescriptorIndex = 0;
    bool alphaBlendEnable = false;
};

struct FrontendGpuSubmitEvidence
{
    bool found = false;
    std::string screenId;
    std::string source = "missing";
    std::string probe = "missing";
    std::string backendSubmitStatus = "missing";
    int runtimeFrame = 0;
    std::vector<FrontendGpuSubmitCall> calls;
    FrontendLiveBridgeProbeResult bridgeProbe;
};

struct FrontendGpuSubmitMaterialTriage
{
    std::string screenId;
    std::string source = "missing";
    std::string probe = "missing";
    std::string gpuSubmitSource = "ui-gpu-submit";
    std::string materialTriage = "backend-submit-vs-runtime-rectangles";
    std::string backendSubmitStatus = "render-thread-material-submit";
    std::size_t backendSubmitCount = 0;
    std::size_t texturedSubmitCount = 0;
    std::size_t alphaBlendSubmitCount = 0;
    std::size_t drawRectCount = 0;
    std::size_t localCommandCount = 0;
};

struct FrontendMaterialCorrelationPair
{
    int uiDrawSequence = 0;
    int gpuSubmitSequence = 0;
    std::string blendSemantic;
    std::string samplerSemantic;
    std::string addressSemantic;
    bool alphaBlendEnable = false;
    bool additiveBlend = false;
    bool linearFilter = false;
    bool pointFilter = false;
};

struct FrontendMaterialCorrelationEvidence
{
    bool found = false;
    std::string screenId;
    std::string source = "missing";
    std::string probe = "missing";
    std::string correlationStatus = "missing";
    std::string rawBackendCommandStatus = "missing";
    std::string resolvedBackendStatus = "missing";
    int runtimeFrame = 0;
    std::size_t drawCallCount = 0;
    std::size_t backendSubmitCount = 0;
    std::size_t backendResolvedSubmitCount = 0;
    std::vector<FrontendMaterialCorrelationPair> pairs;
    FrontendLiveBridgeProbeResult bridgeProbe;
};

struct FrontendMaterialCorrelationTriage
{
    std::string screenId;
    std::string source = "missing";
    std::string probe = "missing";
    std::string materialCorrelationSource = "ui-material-correlation";
    std::string blendSemantics = "runtime-submit-named";
    std::string samplerSemantics = "runtime-submit-named";
    std::string rawBackendCommandStatus = "missing";
    std::size_t pairCount = 0;
    std::size_t drawCallCount = 0;
    std::size_t backendSubmitCount = 0;
    std::size_t alphaBlendPairCount = 0;
    std::size_t additivePairCount = 0;
    std::size_t filterLinearPairCount = 0;
    std::size_t filterPointPairCount = 0;
    std::size_t localCommandCount = 0;
};

struct FrontendBackendResolvedSubmit
{
    std::string backend;
    std::string nativeCommand;
    std::string materialParityHint = "missing";
    std::string textureDescriptorSemantic = "missing";
    std::string samplerDescriptorSemantic = "missing";
    bool textureDescriptorKnown = false;
    bool samplerDescriptorKnown = false;
    bool indexed = false;
    bool resolvedPipelineKnown = false;
    bool activeFramebufferKnown = false;
    bool blendEnabled = false;
    bool framebufferRegistered = false;
    int vertexCount = 0;
    int indexCount = 0;
    int instanceCount = 0;
    int renderTargetFormat0 = 0;
    int depthTargetFormat = 0;
    int primitiveTopology = 0;
    int renderTargetCount = 0;
};

struct FrontendBackendResolvedEvidence
{
    bool found = false;
    std::string screenId;
    std::string source = "missing";
    std::string probe = "missing";
    std::string resolvedBackendStatus = "missing";
    std::string materialParityStatus = "missing";
    std::string blendParityPolicy = "missing";
    std::string framebufferParityPolicy = "missing";
    std::string textureViewSamplerGap = "pending";
    std::string textureViewSamplerStatus = "missing";
    std::string textureDescriptorPolicy = "missing";
    std::string samplerDescriptorPolicy = "missing";
    std::string vendorDescriptorCaptureGap = "pending-native-descriptor-dump";
    std::string vendorResourceCaptureStatus = "missing";
    std::string vendorResourceCapturePolicy = "missing";
    std::string uiOnlyLayerCaptureStatus = "pending-runtime-ui-render-target-copy";
    std::string nativeCommandCaptureGap = "pending-full-vendor-command-buffer-dump";
    std::string textMovieSfxGap = "pending";
    int runtimeFrame = 0;
    std::size_t materialPairCount = 0;
    std::size_t textureDescriptorKnownCount = 0;
    std::size_t samplerDescriptorKnownCount = 0;
    std::size_t linearSamplerDescriptorCount = 0;
    std::size_t pointSamplerDescriptorCount = 0;
    std::size_t wrapSamplerDescriptorCount = 0;
    std::size_t clampSamplerDescriptorCount = 0;
    std::size_t textureResourceViewKnownCount = 0;
    std::size_t samplerResourceViewKnownCount = 0;
    std::size_t resourceViewPairCount = 0;
    std::vector<FrontendBackendResolvedSubmit> submits;
    FrontendLiveBridgeProbeResult bridgeProbe;
};

struct FrontendBackendResolvedTriage
{
    std::string screenId;
    std::string source = "missing";
    std::string probe = "missing";
    std::string backendResolvedSource = "ui-backend-resolved";
    std::string materialCorrelationBackendResolved = "joined";
    std::string resolvedPsoBlendFramebuffer = "runtime-backend";
    std::string resolvedBackendStatus = "missing";
    std::size_t backendResolvedSubmitCount = 0;
    std::size_t resolvedPipelineSubmitCount = 0;
    std::size_t blendEnabledSubmitCount = 0;
    std::size_t renderTargetFormatKnownCount = 0;
    std::size_t framebufferKnownCount = 0;
    std::size_t materialPairCount = 0;
    std::size_t localCommandCount = 0;
};

struct FrontendBackendMaterialParityTriage
{
    std::string screenId;
    std::string source = "missing";
    std::string probe = "missing";
    std::string materialParityPolicy = "backend-resolved-pso-blend-framebuffer";
    std::string textureViewSamplerGap = "pending";
    std::string textMovieSfxGap = "pending";
    std::string materialParityStatus = "missing";
    std::size_t backendResolvedSubmitCount = 0;
    std::size_t sourceOverSubmitCount = 0;
    std::size_t additiveSubmitCount = 0;
    std::size_t opaqueSubmitCount = 0;
    std::size_t framebufferRegisteredSubmitCount = 0;
    std::size_t localCommandCount = 0;
};

struct FrontendDescriptorSemanticsTriage
{
    std::string screenId;
    std::string source = "missing";
    std::string probe = "missing";
    std::string textureSamplerPolicy = "runtime-descriptor-state";
    std::string vendorDescriptorGap = "pending-native-descriptor-dump";
    std::string textMovieSfxGap = "pending";
    std::string textureViewSamplerStatus = "missing";
    std::size_t textureDescriptorKnownCount = 0;
    std::size_t samplerDescriptorKnownCount = 0;
    std::size_t linearSamplerDescriptorCount = 0;
    std::size_t pointSamplerDescriptorCount = 0;
    std::size_t wrapSamplerDescriptorCount = 0;
    std::size_t clampSamplerDescriptorCount = 0;
    std::size_t localCommandCount = 0;
};

struct FrontendVendorResourceCaptureTriage
{
    std::string screenId;
    std::string source = "missing";
    std::string probe = "missing";
    std::string vendorResourcePolicy = "native-rhi-resource-view-sampler";
    std::string uiOnlyLayerStatus = "pending-runtime-ui-render-target-copy";
    std::string nativeCommandGap = "pending-full-vendor-command-buffer-dump";
    std::string vendorResourceCaptureStatus = "missing";
    std::size_t textureResourceViewKnownCount = 0;
    std::size_t samplerResourceViewKnownCount = 0;
    std::size_t resourceViewPairCount = 0;
    std::size_t localCommandCount = 0;
};

struct CsdColorRgba
{
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
    std::uint8_t a = 255;
};

struct CsdDrawableCommand
{
    std::string sceneName;
    std::string castName;
    std::string textureName;
    int groupIndex = 0;
    int castIndex = 0;
    int subimageIndex = -1;
    int textureIndex = -1;
    int textureWidth = 0;
    int textureHeight = 0;
    int sourceX = 0;
    int sourceY = 0;
    int sourceWidth = 0;
    int sourceHeight = 0;
    int castWidth = 0;
    int castHeight = 0;
    int destinationX = 0;
    int destinationY = 0;
    int destinationWidth = 0;
    int destinationHeight = 0;
    double translationX = 0.0;
    double translationY = 0.0;
    double scaleX = 1.0;
    double scaleY = 1.0;
    double rotation = 0.0;
    int drawType = 1;
    std::uint32_t castFlags = 0;
    std::uint32_t colorPackedRgba = 0xFFFFFFFF;
    CsdColorRgba colorRgba{};
    CsdColorRgba gradientTopLeftRgba{};
    CsdColorRgba gradientBottomLeftRgba{};
    CsdColorRgba gradientTopRightRgba{};
    CsdColorRgba gradientBottomRightRgba{};
    bool colorKnown = false;
    bool gradientKnown = false;
    bool gradientVarying = false;
    bool additiveBlend = false;
    bool linearFiltering = false;
    bool hidden = false;
    bool flipX = false;
    bool flipY = false;
    bool textureResolved = false;
    bool sourceFits = false;
    bool sourceFreeStructural = false;
};

struct CsdDrawableScene
{
    std::filesystem::path sourcePath;
    std::string layoutFileName;
    std::string sceneName;
    int castCount = 0;
    int subimageCount = 0;
    std::vector<std::string> textureNames;
    std::vector<CsdDrawableCommand> commands;
};

struct CsdCastDictionaryEntry
{
    int groupIndex = 0;
    int castIndex = 0;
    std::string name;
};

struct CsdSubimageBinding
{
    int textureIndex = -1;
    double left = 0.0;
    double top = 0.0;
    double right = 0.0;
    double bottom = 0.0;
};

struct DdsTextureInfo
{
    std::string format;
    int width = 0;
    int height = 0;
};

struct CsdTimelineKeyframe
{
    double frame = 0.0;
    double value = 0.0;
    std::string interpolationType;
};

struct CsdTimelinePackedRgbaKeyframe
{
    double frame = 0.0;
    std::uint32_t packedRgba = 0xFFFFFFFF;
    CsdColorRgba color{};
    std::string interpolationType;
};

struct CsdTimelineTrackSample
{
    std::string sceneName;
    std::string castName;
    std::string trackType;
    int groupIndex = 0;
    int castIndex = 0;
    int sampleFrame = 0;
    int keyframeCount = 0;
    double value = 0.0;
};

struct CsdTimelinePackedRgbaTrackSample
{
    std::string sceneName;
    std::string castName;
    std::string trackType;
    int groupIndex = 0;
    int castIndex = 0;
    int sampleFrame = 0;
    int keyframeCount = 0;
    std::uint32_t packedRgba = 0xFFFFFFFF;
    CsdColorRgba color{};
};

struct CsdTimelinePlayback
{
    std::filesystem::path sourcePath;
    std::string layoutFileName;
    std::string sceneName;
    std::string animationName;
    int animationIndex = 0;
    int sampleFrame = 0;
    double frameCount = 0.0;
    int trackCount = 0;
    int numericTrackCount = 0;
    int keyframeCount = 0;
    int colorTrackCount = 0;
    int gradientTrackCount = 0;
    int packedColorTrackCount = 0;
    int packedGradientTrackCount = 0;
    int decodedPackedColorTrackCount = 0;
    int decodedPackedGradientTrackCount = 0;
    int decodedPackedKeyframeCount = 0;
    int unresolvedPackedKeyframeCount = 0;
    std::vector<CsdTimelineTrackSample> samples;
    std::vector<CsdTimelinePackedRgbaTrackSample> packedRgbaSamples;
};

struct BitmapSignalStats
{
    bool loaded = false;
    int width = 0;
    int height = 0;
    std::uint64_t rgbSum = 0;
    std::uint64_t alphaSum = 0;
    std::uint64_t rgbNonBlack = 0;
};

struct CsdFullFrameDeltaStats
{
    bool computed = false;
    std::string mode = "registered-full-frame-nearest";
    int width = kDesignWidth;
    int height = kDesignHeight;
    int pixelCount = 0;
    int exactMatchPixels = 0;
    int significantDeltaPixels = 0;
    int renderNonBlackPixels = 0;
    int nativeNonBlackPixels = 0;
    double meanAbsRgb = 0.0;
    int maxAbsRgb = 0;
    double renderNonBlackRatio = 0.0;
    double nativeNonBlackRatio = 0.0;
};

struct CsdUiLayerMaskStats
{
    bool computed = false;
    std::string mode = "rendered-csd-coverage-mask";
    int width = kDesignWidth;
    int height = kDesignHeight;
    int pixelCount = 0;
    int maskedPixelCount = 0;
    int exactMatchPixels = 0;
    int significantDeltaPixels = 0;
    double maskCoverageRatio = 0.0;
    double meanAbsRgb = 0.0;
    int maxAbsRgb = 0;
    double fullFrameMeanAbsRgb = 0.0;
    double fullFrameDeltaReduction = 0.0;
};

struct BitmapComparisonStats
{
    bool nativeFound = false;
    int sampleGridWidth = 64;
    int sampleGridHeight = 36;
    int sampleCount = 0;
    std::string alignmentMode = "search-center-crop-16x9";
    int nativeAlignmentCropX = 0;
    int nativeAlignmentCropY = 0;
    int nativeAlignmentCropWidth = 0;
    int nativeAlignmentCropHeight = 0;
    int registrationOffsetX = 0;
    int registrationOffsetY = 0;
    int registrationCandidateCount = 0;
    double registrationBaseMeanAbsRgb = 0.0;
    double meanAbsRgb = 0.0;
    int maxAbsRgb = 0;
    BitmapSignalStats rendered;
    BitmapSignalStats native;
    CsdFullFrameDeltaStats fullFrame;
    CsdUiLayerMaskStats uiLayerDelta;
};

struct CsdNativeFrameRegistration
{
    int cropX = 0;
    int cropY = 0;
    int cropWidth = 0;
    int cropHeight = 0;
    int offsetX = 0;
    int offsetY = 0;
    int candidateCount = 0;
    double baseMeanAbsRgb = 0.0;
    double bestMeanAbsRgb = 0.0;
    int bestMaxAbsRgb = 0;
};

struct CsdMaterialParityTriage
{
    std::string primaryBlocker = "not-computed";
    std::vector<std::string> riskFlags;
    double coverageGap = 0.0;
    double sampledVsFullFrameGap = 0.0;
};

struct CsdHudRuntimeSceneEntry
{
    std::string path;
    std::string sceneName;
    int castCount = 0;
    int frame = 0;
};

struct CsdHudRuntimeNodeEntry
{
    std::string path;
    std::string nodeAddress;
    std::string projectAddress;
    int sceneCount = 0;
    int childNodeCount = 0;
    int frame = 0;
};

struct CsdHudRuntimeLayerEntry
{
    std::string path;
    std::string sceneName;
    std::string layerName;
    std::string layerAddress;
    std::string castNodeAddress;
    int castNodeIndex = 0;
    int castIndex = 0;
    int frame = 0;
};

struct CsdHudRuntimeSceneEvidence
{
    bool found = false;
    std::filesystem::path liveStatePath;
    std::string runtimeProject;
    std::string localLayoutFileName;
    std::string localProject;
    std::string localSceneName;
    std::string layoutStatus = "missing-live-state";
    std::string ownerPathStatus = "unknown";
    std::string ownerFieldMaturationStatus = "unknown";
    bool rawOwnerKnown = false;
    bool rawOwnerFieldsReady = false;
    bool resolvedFromCsdProjectTree = false;
    int stageReadyFrame = 0;
    int runtimeSceneCount = 0;
    int runtimeNodeCount = 0;
    int runtimeLayerCount = 0;
    std::vector<CsdHudRuntimeSceneEntry> runtimeScenes;
    std::vector<CsdHudRuntimeNodeEntry> runtimeNodes;
    std::vector<CsdHudRuntimeLayerEntry> runtimeLayers;
};

struct CsdHudRuntimeMaterialEntry
{
    std::string runtimePath;
    std::string runtimeSceneName;
    std::string localSceneName;
    std::string layerName;
    std::string castName;
    std::string textureName;
    std::string sgfxSlotLabel;
    std::string materialSourceStatus;
    std::string timelineName;
    int timelineFrame = 0;
    int timelineFrameCount = 0;
    int sourceX = 0;
    int sourceY = 0;
    int sourceWidth = 0;
    int sourceHeight = 0;
    int destinationX = 0;
    int destinationY = 0;
    int destinationWidth = 0;
    int destinationHeight = 0;
    int textureWidth = 0;
    int textureHeight = 0;
    int subimageIndex = -1;
    int textureIndex = -1;
    int castIndex = 0;
    bool textureResolved = false;
    bool sourceFits = false;
    bool timelineResolved = false;
};

struct SonicHudCompositorScene
{
    std::string runtimePath;
    std::string sceneName;
    std::string localSceneName;
    std::string sgfxSlotLabel;
    std::string activationEvent;
    std::string timelineName;
    int runtimeLayerCount = 0;
    int drawableLayerCount = 0;
    int structuralLayerCount = 0;
    int timelineFrame = 0;
    int timelineFrameCount = 0;
    int castCount = 0;
    std::vector<std::string> textureNames;
};

struct SonicHudCompositorModel
{
    std::string target = "sonic-hud";
    std::string source = "runtime-tree+exact-local-layout";
    std::string project = "ui_playscreen";
    std::string owner = "CHudSonicStage";
    std::string ownerHook = "sub_824D9308";
    std::string stateEvent = "sonic-hud-ready";
    std::string referenceStatus = "clean-readable-reference-exported";
    std::filesystem::path liveStatePath;
    int sceneCount = 0;
    int runtimeLayerCount = 0;
    int exportedLayerCount = 0;
    int drawableLayerCount = 0;
    int structuralLayerCount = 0;
    std::vector<SonicHudCompositorScene> scenes;
};

struct CsdHudSceneCoverageDiagnostic
{
    std::string sceneName;
    std::string runtimePath;
    std::string sgfxSlotLabel;
    int runtimeCastCount = 0;
    int localCommandCount = 0;
    int localCoveredPixels = 0;
    double localCoverageRatio = 0.0;
    bool runtimeSceneMatched = false;
    bool locallyRendered = false;
};

struct CsdHudCastCoverageDiagnostic
{
    std::string sceneName;
    std::string castName;
    std::string textureName;
    std::string sgfxSlotLabel;
    int destinationX = 0;
    int destinationY = 0;
    int destinationWidth = 0;
    int destinationHeight = 0;
    int localCoveredPixels = 0;
    int nativeNonBlackPixels = 0;
    double nativeOverlapRatio = 0.0;
};

struct CsdRenderedFrameComparison
{
    std::string templateId;
    std::string layoutFileName;
    std::string sceneName;
    std::string timelineSceneName;
    std::string timelineAnimationName;
    int frame = 0;
    std::filesystem::path renderedFramePath;
    std::filesystem::path diffFramePath;
    std::filesystem::path uiLayerDiffFramePath;
    std::optional<std::filesystem::path> nativeBestPath;
    std::size_t drawCommandCount = 0;
    std::size_t sampledCommandCount = 0;
    std::size_t textureBindingCount = 0;
    std::size_t colorCommandCount = 0;
    std::size_t alphaModulatedCommandCount = 0;
    std::size_t gradientCommandCount = 0;
    std::size_t gradientApproxCommandCount = 0;
    std::size_t gradientVertexColorCommandCount = 0;
    std::size_t additiveCommandCount = 0;
    std::size_t additiveSoftwareCommandCount = 0;
    std::size_t normalBlendCommandCount = 0;
    std::size_t linearFilteringCommandCount = 0;
    std::size_t softwareQuadCommandCount = 0;
    std::size_t csdPointFilterSampleCount = 0;
    std::size_t bilinearSampleCount = 0;
    std::size_t nearestSampleCount = 0;
    std::size_t gradientTrackSampleCount = 0;
    std::size_t packedColorTrackCount = 0;
    std::size_t packedGradientTrackCount = 0;
    std::size_t decodedPackedColorTrackCount = 0;
    std::size_t decodedPackedGradientTrackCount = 0;
    std::size_t decodedPackedKeyframeCount = 0;
    std::size_t unresolvedPackedKeyframeCount = 0;
    std::vector<std::string> sgfxSlots;
    BitmapComparisonStats visualDelta;
    CsdMaterialParityTriage materialTriage;
    CsdHudRuntimeSceneEvidence sonicHudRuntimeScene;
    std::vector<CsdHudSceneCoverageDiagnostic> sonicHudSceneCoverage;
    std::vector<CsdHudCastCoverageDiagnostic> sonicHudCastCoverage;
};

struct CsdSamplerStats
{
    std::size_t csdPointFilterSampleCount = 0;
    std::size_t bilinearSampleCount = 0;
    std::size_t nearestSampleCount = 0;
};

struct CsdSoftwareRenderStats
{
    std::size_t softwareQuadCommandCount = 0;
    std::size_t gradientVertexColorCommandCount = 0;
    std::size_t additiveSoftwareCommandCount = 0;
    CsdSamplerStats samplerStats;
};

[[nodiscard]] std::optional<CsdTimelinePlayback> loadTimelinePlaybackForScene(
    std::string_view templateId,
    std::string_view layoutFileName,
    std::string_view localSceneName,
    std::optional<int> sampleFrameOverride = std::nullopt);

[[nodiscard]] std::vector<CsdDrawableCommand> timelineSampledCommands(
    const CsdDrawableScene& scene,
    const CsdTimelinePlayback* playback,
    std::size_t& sampledCommandCount);

[[nodiscard]] std::string portablePath(const std::filesystem::path& path);
[[nodiscard]] std::string jsonEscape(std::string_view text);
[[nodiscard]] std::filesystem::path repoRootForOutput();
[[nodiscard]] std::optional<std::filesystem::path> findLatestFrontendLiveStatePath(std::string_view target);
[[nodiscard]] std::string discoverFrontendLiveBridgeName(std::string_view target);
[[nodiscard]] FrontendLiveBridgeProbeResult queryUiLabLiveBridgeCommand(
    std::string_view pipeName,
    std::string_view command,
    DWORD timeoutMilliseconds = 150);
[[nodiscard]] FrontendLiveBridgeProbeResult queryUiLabLiveBridgeState(
    std::string_view pipeName,
    DWORD timeoutMilliseconds = 150);
[[nodiscard]] FrontendLiveBridgeProbeResult queryUiLabLiveBridgeUiOracle(
    std::string_view pipeName,
    DWORD timeoutMilliseconds = 150);
[[nodiscard]] FrontendLiveBridgeProbeResult queryUiLabLiveBridgeUiDrawList(
    std::string_view pipeName,
    DWORD timeoutMilliseconds = 150);
[[nodiscard]] FrontendLiveBridgeProbeResult queryUiLabLiveBridgeGpuSubmit(
    std::string_view pipeName,
    DWORD timeoutMilliseconds = 150);
[[nodiscard]] FrontendLiveBridgeProbeResult queryUiLabLiveBridgeMaterialCorrelation(
    std::string_view pipeName,
    DWORD timeoutMilliseconds = 150);
[[nodiscard]] FrontendLiveBridgeProbeResult queryUiLabLiveBridgeBackendResolved(
    std::string_view pipeName,
    DWORD timeoutMilliseconds = 150);
[[nodiscard]] FrontendLiveStateAlignmentEvidence loadFrontendRuntimeAlignmentFromLiveState(
    const sward::ui_runtime::FrontendScreenPolicy& policy);
[[nodiscard]] FrontendLiveStateAlignmentEvidence loadFrontendRuntimeAlignmentFromLiveBridge(
    const sward::ui_runtime::FrontendScreenPolicy& policy);
[[nodiscard]] FrontendUiOracleEvidence loadFrontendUiOracleEvidence(
    const sward::ui_runtime::FrontendScreenPolicy& policy);
[[nodiscard]] FrontendUiOraclePlaybackClock loadFrontendUiOraclePlaybackClock(
    const sward::ui_runtime::FrontendScreenPolicy& policy);
[[nodiscard]] FrontendRuntimeDrawListEvidence loadFrontendRuntimeDrawListEvidence(
    const sward::ui_runtime::FrontendScreenPolicy& policy);
[[nodiscard]] FrontendGpuSubmitEvidence loadFrontendGpuSubmitEvidence(
    const sward::ui_runtime::FrontendScreenPolicy& policy);
[[nodiscard]] FrontendMaterialCorrelationEvidence loadFrontendMaterialCorrelationEvidence(
    const sward::ui_runtime::FrontendScreenPolicy& policy);
[[nodiscard]] FrontendBackendResolvedEvidence loadFrontendBackendResolvedEvidence(
    const sward::ui_runtime::FrontendScreenPolicy& policy);
[[nodiscard]] std::string formatFrontendRuntimeAlignmentEvidence(const FrontendLiveStateAlignmentEvidence& evidence);

inline constexpr std::array<TextureSourceCandidate, 32> kTextureSourceCandidates{{
    { "mat_load_comon_001.dds", "ui_extended_archives/Loading/mat_load_comon_001.dds" },
    { "mat_load_en_001.dds", "ui_broader_archives/Languages/English/Loading/mat_load_en_001.dds" },
    { "mat_load_en_002.dds", "ui_broader_archives/Languages/English/Loading/mat_load_en_002.dds" },
    { "mat_comon_txt_001.dds", "ui_extended_archives/Loading/mat_comon_txt_001.dds" },
    { "OPmovie_titlelogo_EN.decompressed.dds", "runtime_previews/title/decompressed/OPmovie_titlelogo_EN.decompressed.dds" },
    { "ui_mm_base.dds", "ui_frontend_archives/MainMenu/ui_mm_base.dds" },
    { "ui_mm_parts1.dds", "ui_frontend_archives/MainMenu/ui_mm_parts1.dds" },
    { "ui_mm_contentstext.dds", "ui_frontend_archives/MainMenu/ui_mm_contentstext.dds" },
    { "mat_title_en_001.dds", "ui_broader_archives/Languages/English/Title/mat_title_en_001.dds" },
    { "mat_title_en_002.dds", "ui_broader_archives/Languages/English/Title/mat_title_en_002.dds" },
    { "mat_start_en_001.dds", "phase25_commonflow_archives/Languages/English/ActionCommon/mat_start_en_001.dds" },
    { "ui_ps1_gauge1.dds", "phase135_ui_playscreen_probe/Sonic/ui_ps1_gauge1.dds" },
    { "mat_playscreen_001.dds", "phase135_ui_playscreen_probe/Sonic/mat_playscreen_001.dds" },
    { "mat_playscreen_002.dds", "phase135_ui_playscreen_probe/Sonic/mat_playscreen_002.dds" },
    { "mat_playscreen_en_001.dds", "phase135_ui_playscreen_probe/Languages/English/Sonic/mat_playscreen_en_001.dds" },
    { "mat_playscreen_en_002.dds", "phase135_ui_playscreen_probe/Languages/English/Sonic/mat_playscreen_en_002.dds" },
    { "mat_playscreen_en_003.dds", "phase135_ui_playscreen_probe/Languages/English/Sonic/mat_playscreen_en_003.dds" },
    { "mat_hit_001.dds", "phase135_ui_playscreen_probe/Sonic/mat_hit_001.dds" },
    { "mat_comon_num_001.dds", "ui_extended_archives/SystemCommonCore/mat_comon_num_001.dds" },
    { "mat_comon_001.dds", "ui_extended_archives/SystemCommonCore/mat_comon_001.dds" },
    { "mat_comon_x360_001.dds", "ui_extended_archives/SystemCommonCore/mat_comon_x360_001.dds" },
    { "mat_comon_en_001.dds", "ui_broader_archives/Languages/English/SystemCommonCore/mat_comon_en_001.dds" },
    { "mat_comon_002.dds", "phase135_ui_playscreen_probe/SystemCommonCore/mat_comon_002.dds" },
    { "mat_comon_003.dds", "phase135_ui_playscreen_probe/SystemCommonCore/mat_comon_003.dds" },
    { "mat_comon_004.dds", "phase135_ui_playscreen_probe/SystemCommonCore/mat_comon_004.dds" },
    { "mat_result_comon_001.dds", "ui_extended_archives/SystemCommonCore/mat_result_comon_001.dds" },
    { "mat_pause_en_001.dds", "ui_broader_archives/Languages/English/SystemCommonCore/mat_pause_en_001.dds" },
    { "mat_pause_en_002.dds", "ui_broader_archives/Languages/English/SystemCommonCore/mat_pause_en_002.dds" },
    { "mat_ex_common_002.dds", "ui_extended_archives/SystemCommonCore/mat_ex_common_002.dds" },
    { "ui_ps1_gauge1.dds", "phase16_support_archives/ExStageTails_Common/ui_ps1_gauge1.dds" },
    { "mat_playscreen_001.dds", "phase16_support_archives/ExStageTails_Common/mat_playscreen_001.dds" },
    { "mat_playscreen_en_001.dds", "phase25_commonflow_archives/Languages/English/ExStageTails_Common/mat_playscreen_en_001.dds" },
}};

inline constexpr std::array<SuUiRenderCast, 4> kTitleLoopReconstructionCasts{{
    { "ui_title/bg/bg", "title_movie_frame", "ui_mm_base.dds", 0, 0, 1280, 720, 0, 0, 1280, 720 },
    { "ui_title/logo", "opmovie_titlelogo_en", "OPmovie_titlelogo_EN.decompressed.dds", 0, 0, 1280, 720, 0, 0, 1280, 720 },
    { "mm_title_intro", "press_start_text", "mat_title_en_001.dds", 32, 0, 192, 24, 550, 540, 180, 24 },
    // Evidence seam: UseAlternateTitleMidAsmHook switches EN/JP title treatment.
    { "CTitleStateIntro::Update", "alternate_title_gate", "mat_title_en_001.dds", 0, 456, 256, 56, 548, 638, 184, 40 },
}};

inline constexpr std::array<SuUiRenderCast, 8> kSonicHudReconstructionCasts{{
    { "ui_prov_playscreen.yncp", "so_speed_gauge_body", "ui_ps1_gauge1.dds", 0, 0, 256, 128, 18, 512, 430, 215 },
    { "ui_prov_playscreen.yncp", "so_ring_energy_body", "ui_ps1_gauge1.dds", 0, 64, 192, 48, 40, 636, 320, 80 },
    { "ui_prov_playscreen.yncp", "so_speed_gauge_meter", "ui_ps1_gauge1.dds", 48, 64, 150, 34, 94, 632, 240, 54 },
    { "ui_prov_playscreen.yncp", "ring_get_flash", "ui_ps1_gauge1.dds", 72, 82, 24, 22, 326, 622, 54, 48 },
    { "ui_prov_playscreen.yncp", "hud_label_stack", "mat_playscreen_001.dds", 0, 0, 128, 164, 480, 116, 200, 256 },
    { "ui_prov_playscreen.yncp", "ring_energy_label", "mat_playscreen_en_001.dds", 0, 0, 128, 72, 76, 634, 260, 80 },
    { "ui_prov_playscreen.yncp", "ring_digits_zeroes", "mat_comon_num_001.dds", 0, 0, 192, 32, 112, 672, 190, 42 },
    { "ui_prov_playscreen.yncp", "so_head_life_icon", "mat_comon_001.dds", 0, 0, 96, 64, 32, 24, 96, 64 },
}};

inline constexpr std::array<SuUiRenderCast, 1> kLoadingCompositeCasts{{
    { "load_composite", "full_screen", "mat_load_comon_001.dds", 0, 0, 1280, 720, 0, 0, 1280, 720 },
}};

inline constexpr std::array<SuUiRenderCast, 3> kMainMenuCompositeCasts{{
    { "mm_bg", "base_sheet", "ui_mm_base.dds", 0, 0, 1280, 720, 0, 0, 1280, 720 },
    { "mm_bg", "parts_sheet", "ui_mm_parts1.dds", 0, 0, 1280, 640, 0, 40, 1280, 640 },
    { "mm_contents", "text_sheet", "ui_mm_contentstext.dds", 0, 0, 640, 640, 248, 40, 640, 640 },
}};

inline constexpr std::array<SuUiRenderCast, 1> kSonicTitleMenuCasts{{
    { "mm_bg_usual", "black3", "ui_mm_parts1.dds", 896, 336, 16, 16, 655, 435, 368, 464 },
}};

inline constexpr std::array<SuUiRenderCast, 2> kTitleLogoSheetCasts{{
    { "title", "logo_en_001", "mat_title_en_001.dds", 0, 0, 256, 512, 320, 104, 256, 512 },
    { "title", "logo_en_002", "mat_title_en_002.dds", 0, 0, 128, 256, 704, 232, 128, 256 },
}};

inline constexpr std::array<SuUiRenderCast, 1> kSonicStageHudCasts{{
    { "so_speed_gauge", "position_hd", "ui_ps1_gauge1.dds", 4, 64, 16, 20, 752, 357, 16, 20 },
}};

inline constexpr std::array<SgfxPlaceholderAssetSlot, 4> kTitleMenuTemplateSlots{{
    { "backdrop", "ui_mm_base.dds", "ui_title/ui_mainmenu Sonic title/menu CSD" },
    { "content", "ui_mm_contentstext.dds", "ui_title/ui_mainmenu Sonic title/menu CSD" },
    { "logo", "OPmovie_titlelogo_EN.decompressed.dds", "ui_title/ui_mainmenu Sonic title/menu CSD" },
    { "prompt_glyphs", "mat_start_en_001.dds", "ui_title/ui_mainmenu Sonic title/menu CSD" },
}};

inline constexpr std::array<SgfxPlaceholderAssetSlot, 4> kLoadingTemplateSlots{{
    { "device_frame", "mat_load_comon_001.dds", "ui_loading Sonic loading CSD" },
    { "backdrop", "mat_load_comon_001.dds", "ui_loading Sonic loading CSD" },
    { "loading_copy", "mat_load_comon_001.dds", "ui_loading Sonic loading CSD" },
    { "controller_variant", "mat_load_comon_001.dds", "ui_loading Sonic loading CSD" },
}};

inline constexpr std::array<SgfxPlaceholderAssetSlot, 5> kSonicHudTemplateSlots{{
    { "speed_gauge", "ui_ps1_gauge1.dds", "ui_playscreen Sonic HUD CSD tree" },
    { "energy_gauge", "ui_ps1_gauge1.dds", "ui_playscreen Sonic HUD CSD tree" },
    { "ring_counter", "mat_comon_num_001.dds", "ui_playscreen Sonic HUD CSD tree" },
    { "side_panel", "mat_playscreen_001.dds", "ui_playscreen Sonic HUD CSD tree" },
    { "prompt_strip", "mat_playscreen_en_001.dds", "ui_playscreen Sonic HUD CSD tree" },
}};

inline constexpr std::array<SgfxPlaceholderAssetSlot, 4> kTutorialTemplateSlots{{
    { "prompt_row", "mat_start_en_001.dds", "SonicHudGuide/ui_playscreen prompt CSD" },
    { "tutorial_panel", "mat_playscreen_001.dds", "SonicHudGuide/ui_playscreen prompt CSD" },
    { "control_glyphs", "mat_start_en_001.dds", "SonicHudGuide/ui_playscreen prompt CSD" },
    { "host_context_readout", "mat_comon_num_001.dds", "SonicHudGuide/ui_playscreen prompt CSD" },
}};

inline constexpr std::array<SgfxPlaceholderAssetSlot, 4> kTitleOptionsTemplateSlots{{
    { "backdrop", "ui_mm_base.dds", "ui_mainmenu title/options CSD" },
    { "option_carousel", "ui_mm_contentstext.dds", "ui_mainmenu title/options CSD" },
    { "option_cursor", "ui_mm_parts1.dds", "ui_mainmenu title/options CSD" },
    { "prompt_glyphs", "mat_start_en_001.dds", "ui_mainmenu title/options prompt CSD" },
}};

inline constexpr std::array<SgfxPlaceholderAssetSlot, 4> kPauseTemplateSlots{{
    { "pause_backdrop", "mat_pause_en_001.dds", "ui_pause pause CSD" },
    { "pause_chrome", "mat_pause_en_002.dds", "ui_pause pause CSD" },
    { "pause_content", "mat_comon_001.dds", "ui_pause pause CSD" },
    { "prompt_strip", "mat_ex_common_002.dds", "ui_pause pause CSD" },
}};

inline constexpr std::array<SgfxTemplateRenderBinding, 6> kSgfxTemplateRenderBindings{{
    {
        "title-menu",
        "MainMenuComposite",
        kTitleMenuTemplateSlots.data(),
        kTitleMenuTemplateSlots.size(),
        "title-menu-visible",
        "select_travel",
        "title menu visual ready",
    },
    {
        "loading",
        "LoadingComposite",
        kLoadingTemplateSlots.data(),
        kLoadingTemplateSlots.size(),
        "loading-display-active",
        "pda_intro",
        "loading display active",
    },
    {
        "sonic-hud",
        "SonicHudReconstruction",
        kSonicHudTemplateSlots.data(),
        kSonicHudTemplateSlots.size(),
        "sonic-hud-ready",
        "hud_in",
        "sonic-hud-ready",
    },
    {
        "tutorial",
        "SonicHudReconstruction",
        kTutorialTemplateSlots.data(),
        kTutorialTemplateSlots.size(),
        "tutorial-hud-owner-path-ready",
        "hud_in",
        "tutorial-ready",
    },
    {
        "title-options",
        "TitleOptionsReference",
        kTitleOptionsTemplateSlots.data(),
        kTitleOptionsTemplateSlots.size(),
        "title-options-ready",
        "select_travel",
        "title options visual ready",
    },
    {
        "pause",
        "PauseMenuReference",
        kPauseTemplateSlots.data(),
        kPauseTemplateSlots.size(),
        "pause-ready",
        "intro_medium",
        "pause menu visual ready",
    },
}};

inline constexpr std::array<CsdPipelineTemplateBinding, 4> kCsdPipelineTemplateBindings{{
    {
        "title-menu",
        "ui_mainmenu.yncp",
        "mm_bg_usual",
        "mm_donut_move",
        "DefaultAnim",
    },
    {
        "loading",
        "ui_loading.yncp",
        "pda",
        "pda_txt",
        "Usual_Anim_3",
    },
    {
        "sonic-hud",
        "ui_playscreen.yncp",
        "so_speed_gauge",
        "so_speed_gauge",
        "DefaultAnim",
    },
    {
        "tutorial",
        "ui_playscreen.yncp",
        "u_info",
        "u_info",
        "Intro_Anim",
    },
}};

[[nodiscard]] std::pair<std::string, std::string> splitTransitionBand(std::string_view transitionBand)
{
    const auto arrow = transitionBand.find("->");
    if (arrow == std::string_view::npos)
        return { std::string(transitionBand), std::string(transitionBand) };
    return {
        std::string(transitionBand.substr(0, arrow)),
        std::string(transitionBand.substr(arrow + 2)),
    };
}

[[nodiscard]] std::string frontendMaterialSemanticsDescriptor(
    const sward::ui_runtime::FrontendScreenPolicy& policy)
{
    std::ostringstream out;
    out << policy.screenId
        << ":blend=" << policy.materialSemantics.blendModel
        << ":alpha=" << policy.materialSemantics.alphaModel
        << ":color=" << policy.materialSemantics.colorModel
        << ":filter=" << policy.materialSemantics.filteringModel
        << ":offset=" << policy.materialSemantics.pixelOffsetModel
        << ":oracle=" << policy.materialSemantics.oraclePolicy;
    return out.str();
}

void applyUiOraclePlaybackFrameToLane(
    CsdReferenceViewerLane& lane,
    const FrontendUiOraclePlaybackClock& clock)
{
    lane.uiOracleRuntimeFrame = clock.runtimeFrame;
    lane.uiOraclePlaybackFrame = clock.playbackFrame;
    lane.uiOraclePlaybackClock = clock.playbackClock;
    lane.uiOracleFrameSource = clock.frameSource;
    lane.uiOracleSource = clock.source;
    lane.uiOracleProbe = clock.probe;
    lane.uiOracleActiveMotionName = clock.activeMotionName;
}

[[nodiscard]] CsdReferenceViewerLane referenceViewerLaneFromTrackedPolicy(
    const sward::ui_runtime::FrontendScreenPolicy& policy)
{
    const auto [transitionBandId, transitionReadyLabel] = splitTransitionBand(policy.transitionBand);
    CsdReferenceViewerLane lane;
    lane.laneId = policy.screenId;
    lane.rendererScreenId = policy.screenName;
    lane.contractFileName = policy.referenceContract;
    lane.nativeTargetId = policy.screenId;
    lane.requiredEventId = policy.activationEvent;
    lane.timelineBandId = transitionBandId;
    lane.timelineEventLabel = transitionReadyLabel;
    const auto alignmentEvidence = loadFrontendRuntimeAlignmentFromLiveBridge(policy);
    lane.runtimeAlignment = formatFrontendRuntimeAlignment(alignmentEvidence.alignment);
    lane.runtimeAlignmentSource = alignmentEvidence.alignment.source;
    lane.runtimeAlignmentEvidence = formatFrontendRuntimeAlignmentEvidence(alignmentEvidence);
    lane.runtimeAlignmentLiveStatePath = !alignmentEvidence.liveStatePath.empty()
        ? portablePath(alignmentEvidence.liveStatePath)
        : (alignmentEvidence.alignment.source == "ui_lab_live_bridge" ? "direct-live-bridge" : "missing");
    lane.runtimeAlignmentFieldStatus = alignmentEvidence.fieldStatus;
    lane.runtimeAlignmentProbe = alignmentEvidence.alignment.source == "ui_lab_live_bridge"
        ? "direct-live-bridge"
        : "snapshot-fallback";
    lane.runtimeAlignmentBridgePipe = alignmentEvidence.bridgeProbe.pipeName;
    lane.runtimeAlignmentBridgeConnected = alignmentEvidence.bridgeProbe.connected;
    lane.runtimeAlignmentBridgeFallback = alignmentEvidence.fallbackSource.empty()
        ? (alignmentEvidence.alignment.source == "ui_lab_live_bridge" ? "none" : "ui_lab_live_state")
        : alignmentEvidence.fallbackSource;
    lane.runtimeAlignmentBridgeError = alignmentEvidence.bridgeProbe.error;
    applyUiOraclePlaybackFrameToLane(lane, loadFrontendUiOraclePlaybackClock(policy));
    lane.materialSemantics = frontendMaterialSemanticsDescriptor(policy);
    lane.policySource = "frontend_screen_reference";

    for (const auto& scene : policy.scenes)
    {
        lane.scenes.push_back({
            policy.screenId,
            policy.layoutName,
            scene.sceneName,
            scene.sceneName,
            scene.timeline.animationName,
        });
    }

    for (const auto& slot : policy.materialSlots)
    {
        lane.slots.push_back({
            slot.slotId,
            slot.textureName,
            slot.placeholderFamily,
        });
    }

    return lane;
}

[[nodiscard]] std::vector<CsdReferenceViewerLane> buildReferenceViewerLanesFromTrackedPolicy()
{
    std::vector<CsdReferenceViewerLane> lanes;
    for (const auto& policy : frontendScreenPolicies())
        lanes.push_back(referenceViewerLaneFromTrackedPolicy(policy));
    return lanes;
}

inline const std::array<SuUiRendererScreen, 10> kRendererScreens{{
    {
        "TitleLoopReconstruction",
        "Title Loop Reconstructed",
        "title_menu_reference.json",
        kTitleLoopReconstructionCasts.data(),
        kTitleLoopReconstructionCasts.size(),
        Gdiplus::Color(255, 0, 0, 0),
        RendererScreenKind::TitleLoopReconstruction,
    },
    {
        "SonicHudReconstruction",
        "Sonic HUD Reconstructed ui_playscreen Pipeline",
        "sonic_stage_hud_reference.json",
        kSonicHudReconstructionCasts.data(),
        kSonicHudReconstructionCasts.size(),
        Gdiplus::Color(255, 0, 0, 0),
        RendererScreenKind::SonicHudReferencePipeline,
    },
    {
        "VisualAtlasGallery",
        "Visual Atlas Gallery",
        "visual_atlas/atlas_index.json",
        nullptr,
        0,
        Gdiplus::Color(255, 0, 0, 0),
        RendererScreenKind::AtlasGallery,
    },
    {
        "LoadingComposite",
        "LoadingTransition Composite",
        "loading_transition_reference.json",
        kLoadingCompositeCasts.data(),
        kLoadingCompositeCasts.size(),
        Gdiplus::Color(255, 0, 0, 0),
        RendererScreenKind::CsdReferencePipeline,
    },
    {
        "MainMenuComposite",
        "Main Menu Composite Sheets",
        "title_menu_reference.json",
        kMainMenuCompositeCasts.data(),
        kMainMenuCompositeCasts.size(),
        Gdiplus::Color(255, 0, 0, 0),
        RendererScreenKind::CsdReferencePipeline,
    },
    {
        "TitleOptionsReference",
        "Title Options CSD Reference",
        "title_menu_reference.json",
        nullptr,
        0,
        Gdiplus::Color(255, 0, 0, 0),
        RendererScreenKind::CsdReferencePipeline,
    },
    {
        "PauseMenuReference",
        "Pause Menu CSD Reference",
        "pause_menu_reference.json",
        nullptr,
        0,
        Gdiplus::Color(255, 0, 0, 0),
        RendererScreenKind::CsdReferencePipeline,
    },
    {
        "SonicTitleMenu",
        "Sonic Title Menu",
        "title_menu_reference.json",
        kSonicTitleMenuCasts.data(),
        kSonicTitleMenuCasts.size(),
        Gdiplus::Color(255, 0, 0, 0),
    },
    {
        "TitleLogoSheet",
        "Title Logo Sheets",
        "title_menu_reference.json",
        kTitleLogoSheetCasts.data(),
        kTitleLogoSheetCasts.size(),
        Gdiplus::Color(255, 0, 0, 0),
    },
    {
        "SonicStageHud",
        "Sonic Stage HUD",
        "sonic_stage_hud_reference.json",
        kSonicStageHudCasts.data(),
        kSonicStageHudCasts.size(),
        Gdiplus::Color(255, 0, 0, 0),
    },
}};

[[nodiscard]] std::size_t atlasGalleryScreenIndex()
{
    for (std::size_t index = 0; index < kRendererScreens.size(); ++index)
    {
        if (kRendererScreens[index].kind == RendererScreenKind::AtlasGallery)
            return index;
    }
    return 0;
}

[[nodiscard]] const SgfxTemplateRenderBinding* findSgfxTemplateRenderBinding(std::string_view templateId)
{
    const auto found = std::find_if(
        kSgfxTemplateRenderBindings.begin(),
        kSgfxTemplateRenderBindings.end(),
        [templateId](const SgfxTemplateRenderBinding& binding)
        {
            return binding.templateId == templateId;
        });
    return found == kSgfxTemplateRenderBindings.end() ? nullptr : &*found;
}

[[nodiscard]] const CsdPipelineTemplateBinding* findCsdPipelineTemplateBinding(std::string_view templateId)
{
    const auto found = std::find_if(
        kCsdPipelineTemplateBindings.begin(),
        kCsdPipelineTemplateBindings.end(),
        [templateId](const CsdPipelineTemplateBinding& binding)
        {
            return binding.templateId == templateId;
        });
    return found == kCsdPipelineTemplateBindings.end() ? nullptr : &*found;
}

[[nodiscard]] std::optional<CsdReferenceViewerLane> findCsdReferenceViewerLaneById(std::string_view laneId)
{
    const auto lanes = buildReferenceViewerLanesFromTrackedPolicy();
    const auto found = std::find_if(
        lanes.begin(),
        lanes.end(),
        [laneId](const CsdReferenceViewerLane& lane)
        {
            return lane.laneId == laneId;
        });
    return found == lanes.end() ? std::nullopt : std::optional<CsdReferenceViewerLane>(*found);
}

[[nodiscard]] std::optional<CsdReferenceViewerLane> findCsdReferenceViewerLaneByScreenId(std::string_view screenId)
{
    const auto lanes = buildReferenceViewerLanesFromTrackedPolicy();
    const auto found = std::find_if(
        lanes.begin(),
        lanes.end(),
        [screenId](const CsdReferenceViewerLane& lane)
        {
            return lane.rendererScreenId == screenId;
        });
    return found == lanes.end() ? std::nullopt : std::optional<CsdReferenceViewerLane>(*found);
}

[[nodiscard]] const SuUiRendererScreen* rendererScreenById(std::string_view id)
{
    const auto found = std::find_if(
        kRendererScreens.begin(),
        kRendererScreens.end(),
        [id](const SuUiRendererScreen& screen)
        {
            return screen.id == id;
        });
    return found == kRendererScreens.end() ? nullptr : &*found;
}

[[nodiscard]] std::optional<std::size_t> rendererScreenIndexById(std::string_view id)
{
    for (std::size_t index = 0; index < kRendererScreens.size(); ++index)
    {
        if (kRendererScreens[index].id == id)
            return index;
    }

    return std::nullopt;
}

[[nodiscard]] std::filesystem::path executableDirectory()
{
    std::wstring buffer(MAX_PATH, L'\0');
    DWORD size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    while (size == buffer.size())
    {
        buffer.resize(buffer.size() * 2);
        size = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
    }

    if (size == 0)
        return std::filesystem::current_path();

    buffer.resize(size);
    return std::filesystem::path(buffer).parent_path();
}

void appendAncestorAssetRoots(std::vector<std::filesystem::path>& candidates, std::filesystem::path start)
{
    std::error_code error;
    start = std::filesystem::absolute(start, error);
    if (error)
        return;

    for (auto current = start; !current.empty(); current = current.parent_path())
    {
        candidates.push_back(current / "extracted_assets");
        if (current == current.root_path())
            break;
    }
}

[[nodiscard]] std::vector<std::filesystem::path> extractedAssetRootCandidates()
{
    std::vector<std::filesystem::path> candidates;
    appendAncestorAssetRoots(candidates, std::filesystem::current_path());
    appendAncestorAssetRoots(candidates, executableDirectory());
    return candidates;
}

void appendAncestorRepoRoots(std::vector<std::filesystem::path>& candidates, std::filesystem::path start)
{
    std::error_code error;
    start = std::filesystem::absolute(start, error);
    if (error)
        return;

    for (auto current = start; !current.empty(); current = current.parent_path())
    {
        candidates.push_back(current);
        if (current == current.root_path())
            break;
    }
}

[[nodiscard]] std::vector<std::filesystem::path> repoRootCandidates()
{
    std::vector<std::filesystem::path> candidates;
    appendAncestorRepoRoots(candidates, std::filesystem::current_path());
    appendAncestorRepoRoots(candidates, executableDirectory());
    return candidates;
}

[[nodiscard]] std::optional<std::filesystem::path> layoutEvidencePath()
{
    constexpr std::string_view kLayoutEvidenceRelativePath = "research_uiux/data/layout_deep_analysis.json";
    for (const auto& root : repoRootCandidates())
    {
        std::error_code error;
        const auto path = root / std::filesystem::path(kLayoutEvidenceRelativePath);
        if (std::filesystem::is_regular_file(path, error))
            return path;
    }

    return std::nullopt;
}

[[nodiscard]] int layoutFilePreferenceScore(std::string_view layoutFileName, std::string_view sourcePath)
{
    if (layoutFileName == "ui_playscreen.yncp")
    {
        if (sourcePath.find("/phase135_ui_playscreen_probe/Sonic/ui_playscreen.yncp") != std::string_view::npos)
            return 100;
        if (sourcePath.find("/Sonic/ui_playscreen.yncp") != std::string_view::npos)
            return 90;
        if (sourcePath.find("/SuperSonic/ui_playscreen.yncp") != std::string_view::npos)
            return 80;
    }

    return 1;
}

[[nodiscard]] std::vector<std::filesystem::path> discoverAtlasSheetPaths()
{
    std::vector<std::filesystem::path> paths;
    for (const auto& root : extractedAssetRootCandidates())
    {
        std::error_code error;
        const auto sheetRoot = root / "visual_atlas" / "sheets";
        if (!std::filesystem::is_directory(sheetRoot, error))
            continue;

        for (const auto& entry : std::filesystem::directory_iterator(sheetRoot, error))
        {
            if (error)
                break;
            if (!entry.is_regular_file(error))
                continue;

            const auto extension = entry.path().extension().string();
            if (extension == ".png" || extension == ".PNG")
                paths.push_back(entry.path());
        }
    }

    std::sort(
        paths.begin(),
        paths.end(),
        [](const auto& left, const auto& right)
        {
            return left.filename().string() < right.filename().string();
        });
    paths.erase(
        std::unique(
            paths.begin(),
            paths.end(),
            [](const auto& left, const auto& right)
            {
                return left.filename() == right.filename();
            }),
        paths.end());
    return paths;
}

[[nodiscard]] const TextureSourceCandidate* textureSourceCandidateForFileName(std::string_view textureFileName)
{
    const auto found = std::find_if(
        kTextureSourceCandidates.begin(),
        kTextureSourceCandidates.end(),
        [textureFileName](const TextureSourceCandidate& candidate)
        {
            return candidate.textureFileName == textureFileName;
        });
    return found == kTextureSourceCandidates.end() ? nullptr : &*found;
}

[[nodiscard]] std::optional<std::filesystem::path> textureSourcePathForFileName(std::string_view textureFileName)
{
    const auto* candidate = textureSourceCandidateForFileName(textureFileName);
    if (!candidate)
        return std::nullopt;

    for (const auto& root : extractedAssetRootCandidates())
    {
        std::error_code error;
        const auto path = root / std::filesystem::path(candidate->relativePath);
        if (std::filesystem::is_regular_file(path, error))
            return path;
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<std::filesystem::path> findTitleMoviePreviewFramePath()
{
    // Local-only frame extracted from game/movie/evmo_title_loop.sfd with ffmpeg.
    // The binary never embeds or publishes the proprietary movie frame; it only
    // consumes the operator's generated preview if it exists beside extracted assets.
    constexpr std::string_view kTitlePreviewRelativePath =
        "runtime_previews/title/evmo_title_loop_00_00_35_000.png";

    for (const auto& root : extractedAssetRootCandidates())
    {
        std::error_code error;
        const auto path = root / std::filesystem::path(kTitlePreviewRelativePath);
        if (std::filesystem::is_regular_file(path, error))
            return path;
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<std::filesystem::path> findTitleLogoPreviewPath()
{
    // Local-only PNG decoded from Loading/OPmovie_titlelogo_EN.dds after the
    // Xbox LZX container is expanded with tools/x_decompress. This keeps the
    // operator preview exact while the native renderer grows full X360 DDS
    // decode coverage.
    constexpr std::string_view kTitleLogoPreviewRelativePath =
        "runtime_previews/title/decompressed/OPmovie_titlelogo_EN.decompressed.png";

    for (const auto& root : extractedAssetRootCandidates())
    {
        std::error_code error;
        const auto path = root / std::filesystem::path(kTitleLogoPreviewRelativePath);
        if (std::filesystem::is_regular_file(path, error))
            return path;
    }

    return std::nullopt;
}

[[nodiscard]] bool gdiplusBitmapLoads(const std::filesystem::path& path)
{
    Gdiplus::GdiplusStartupInput gdiplusInput{};
    ULONG_PTR gdiplusToken = 0;
    if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusInput, nullptr) != Gdiplus::Ok)
        return false;

    auto bitmap = std::unique_ptr<Gdiplus::Bitmap>(Gdiplus::Bitmap::FromFile(path.wstring().c_str(), FALSE));
    const bool loaded = bitmap && bitmap->GetLastStatus() == Gdiplus::Ok && bitmap->GetWidth() > 0 && bitmap->GetHeight() > 0;
    bitmap.reset();
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return loaded;
}

[[nodiscard]] std::string readTextFile(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return {};

    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

[[nodiscard]] std::size_t skipJsonWhitespace(std::string_view text, std::size_t offset)
{
    while (offset < text.size() && std::isspace(static_cast<unsigned char>(text[offset])))
        ++offset;
    return offset;
}

[[nodiscard]] std::optional<std::size_t> matchJsonContainer(
    std::string_view text,
    std::size_t openOffset,
    char openChar,
    char closeChar)
{
    if (openOffset >= text.size() || text[openOffset] != openChar)
        return std::nullopt;

    bool inString = false;
    bool escaped = false;
    int depth = 0;
    for (std::size_t index = openOffset; index < text.size(); ++index)
    {
        const char ch = text[index];
        if (inString)
        {
            if (escaped)
            {
                escaped = false;
            }
            else if (ch == '\\')
            {
                escaped = true;
            }
            else if (ch == '"')
            {
                inString = false;
            }
            continue;
        }

        if (ch == '"')
        {
            inString = true;
            continue;
        }

        if (ch == openChar)
            ++depth;
        else if (ch == closeChar)
        {
            --depth;
            if (depth == 0)
                return index;
        }
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<std::string> parseJsonStringAt(std::string_view text, std::size_t offset)
{
    if (offset >= text.size() || text[offset] != '"')
        return std::nullopt;

    std::string value;
    bool escaped = false;
    for (std::size_t index = offset + 1; index < text.size(); ++index)
    {
        const char ch = text[index];
        if (escaped)
        {
            switch (ch)
            {
            case '"': value.push_back('"'); break;
            case '\\': value.push_back('\\'); break;
            case '/': value.push_back('/'); break;
            case 'b': value.push_back('\b'); break;
            case 'f': value.push_back('\f'); break;
            case 'n': value.push_back('\n'); break;
            case 'r': value.push_back('\r'); break;
            case 't': value.push_back('\t'); break;
            default: value.push_back(ch); break;
            }
            escaped = false;
            continue;
        }

        if (ch == '\\')
        {
            escaped = true;
            continue;
        }

        if (ch == '"')
            return value;

        value.push_back(ch);
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<std::size_t> findJsonStringEnd(std::string_view text, std::size_t offset)
{
    if (offset >= text.size() || text[offset] != '"')
        return std::nullopt;

    bool escaped = false;
    for (std::size_t index = offset + 1; index < text.size(); ++index)
    {
        const char ch = text[index];
        if (escaped)
        {
            escaped = false;
            continue;
        }
        if (ch == '\\')
        {
            escaped = true;
            continue;
        }
        if (ch == '"')
            return index;
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<std::size_t> findJsonFieldValueOffset(std::string_view text, std::string_view fieldName)
{
    const std::string needle = "\"" + std::string(fieldName) + "\"";
    const auto fieldOffset = text.find(needle);
    if (fieldOffset == std::string_view::npos)
        return std::nullopt;

    const auto colonOffset = text.find(':', fieldOffset + needle.size());
    if (colonOffset == std::string_view::npos)
        return std::nullopt;

    return skipJsonWhitespace(text, colonOffset + 1);
}

[[nodiscard]] std::optional<std::size_t> findJsonTopLevelFieldValueOffset(
    std::string_view text,
    std::string_view fieldName)
{
    bool inString = false;
    bool escaped = false;
    int objectDepth = 0;
    int arrayDepth = 0;
    for (std::size_t index = 0; index < text.size(); ++index)
    {
        const char ch = text[index];
        if (inString)
        {
            if (escaped)
                escaped = false;
            else if (ch == '\\')
                escaped = true;
            else if (ch == '"')
                inString = false;
            continue;
        }

        if (ch == '"')
        {
            if (objectDepth == 1 && arrayDepth == 0)
            {
                const auto key = parseJsonStringAt(text, index);
                const auto stringEnd = findJsonStringEnd(text, index);
                if (!stringEnd)
                    return std::nullopt;

                std::size_t colonOffset = skipJsonWhitespace(text, *stringEnd + 1);
                if (colonOffset < text.size() && text[colonOffset] == ':' && key && *key == fieldName)
                    return skipJsonWhitespace(text, colonOffset + 1);

                index = *stringEnd;
                continue;
            }

            inString = true;
            continue;
        }

        if (ch == '{')
            ++objectDepth;
        else if (ch == '}')
            --objectDepth;
        else if (ch == '[')
            ++arrayDepth;
        else if (ch == ']')
            --arrayDepth;
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<std::string_view> jsonTopLevelArrayFieldSpan(
    std::string_view text,
    std::string_view fieldName)
{
    const auto valueOffset = findJsonTopLevelFieldValueOffset(text, fieldName);
    if (!valueOffset || *valueOffset >= text.size() || text[*valueOffset] != '[')
        return std::nullopt;

    const auto endOffset = matchJsonContainer(text, *valueOffset, '[', ']');
    if (!endOffset)
        return std::nullopt;

    return text.substr(*valueOffset, (*endOffset - *valueOffset) + 1);
}

[[nodiscard]] std::optional<std::string> jsonStringField(std::string_view text, std::string_view fieldName)
{
    const auto valueOffset = findJsonFieldValueOffset(text, fieldName);
    if (!valueOffset)
        return std::nullopt;
    return parseJsonStringAt(text, *valueOffset);
}

[[nodiscard]] std::optional<double> jsonNumberField(std::string_view text, std::string_view fieldName)
{
    const auto valueOffset = findJsonFieldValueOffset(text, fieldName);
    if (!valueOffset)
        return std::nullopt;

    std::size_t endOffset = *valueOffset;
    while (endOffset < text.size())
    {
        const char ch = text[endOffset];
        if (!(std::isdigit(static_cast<unsigned char>(ch)) || ch == '-' || ch == '+' || ch == '.' || ch == 'e' || ch == 'E'))
            break;
        ++endOffset;
    }

    if (endOffset == *valueOffset)
        return std::nullopt;

    try
    {
        return std::stod(std::string(text.substr(*valueOffset, endOffset - *valueOffset)));
    }
    catch (...)
    {
        return std::nullopt;
    }
}

[[nodiscard]] std::optional<bool> jsonBoolField(std::string_view text, std::string_view fieldName)
{
    const auto valueOffset = findJsonFieldValueOffset(text, fieldName);
    if (!valueOffset)
        return std::nullopt;

    if (text.substr(*valueOffset, 4) == "true")
        return true;
    if (text.substr(*valueOffset, 5) == "false")
        return false;
    return std::nullopt;
}

[[nodiscard]] std::optional<std::string_view> jsonArrayFieldSpan(std::string_view text, std::string_view fieldName)
{
    const auto valueOffset = findJsonFieldValueOffset(text, fieldName);
    if (!valueOffset || *valueOffset >= text.size() || text[*valueOffset] != '[')
        return std::nullopt;

    const auto endOffset = matchJsonContainer(text, *valueOffset, '[', ']');
    if (!endOffset)
        return std::nullopt;

    return text.substr(*valueOffset, (*endOffset - *valueOffset) + 1);
}

[[nodiscard]] std::optional<std::string_view> jsonObjectFieldSpan(std::string_view text, std::string_view fieldName)
{
    const auto valueOffset = findJsonFieldValueOffset(text, fieldName);
    if (!valueOffset || *valueOffset >= text.size() || text[*valueOffset] != '{')
        return std::nullopt;

    const auto endOffset = matchJsonContainer(text, *valueOffset, '{', '}');
    if (!endOffset)
        return std::nullopt;

    return text.substr(*valueOffset, (*endOffset - *valueOffset) + 1);
}

[[nodiscard]] std::vector<double> parseJsonNumberArray(std::string_view arraySpan)
{
    std::vector<double> values;
    std::size_t offset = 0;
    while (offset < arraySpan.size())
    {
        const char ch = arraySpan[offset];
        if (!(std::isdigit(static_cast<unsigned char>(ch)) || ch == '-' || ch == '+' || ch == '.'))
        {
            ++offset;
            continue;
        }

        std::size_t endOffset = offset + 1;
        while (endOffset < arraySpan.size())
        {
            const char numberChar = arraySpan[endOffset];
            if (!(std::isdigit(static_cast<unsigned char>(numberChar)) || numberChar == '-' || numberChar == '+'
                || numberChar == '.' || numberChar == 'e' || numberChar == 'E'))
                break;
            ++endOffset;
        }

        try
        {
            values.push_back(std::stod(std::string(arraySpan.substr(offset, endOffset - offset))));
        }
        catch (...)
        {
        }
        offset = endOffset;
    }

    return values;
}

[[nodiscard]] std::vector<double> jsonNumberArrayField(std::string_view text, std::string_view fieldName)
{
    const auto arraySpan = jsonArrayFieldSpan(text, fieldName);
    return arraySpan ? parseJsonNumberArray(*arraySpan) : std::vector<double>{};
}

[[nodiscard]] std::vector<std::string> parseJsonStringArray(std::string_view arraySpan)
{
    std::vector<std::string> values;
    bool inString = false;
    bool escaped = false;
    for (std::size_t index = 0; index < arraySpan.size(); ++index)
    {
        const char ch = arraySpan[index];
        if (inString)
        {
            if (escaped)
                escaped = false;
            else if (ch == '\\')
                escaped = true;
            else if (ch == '"')
                inString = false;
            continue;
        }

        if (ch == '"')
        {
            if (const auto value = parseJsonStringAt(arraySpan, index))
                values.push_back(*value);
            inString = true;
        }
    }

    return values;
}

[[nodiscard]] std::vector<std::string_view> jsonObjectSpansInArray(std::string_view arraySpan)
{
    std::vector<std::string_view> spans;
    bool inString = false;
    bool escaped = false;
    for (std::size_t index = 0; index < arraySpan.size(); ++index)
    {
        const char ch = arraySpan[index];
        if (inString)
        {
            if (escaped)
                escaped = false;
            else if (ch == '\\')
                escaped = true;
            else if (ch == '"')
                inString = false;
            continue;
        }

        if (ch == '"')
        {
            inString = true;
            continue;
        }

        if (ch != '{')
            continue;

        const auto endOffset = matchJsonContainer(arraySpan, index, '{', '}');
        if (!endOffset)
            break;

        spans.push_back(arraySpan.substr(index, (*endOffset - index) + 1));
        index = *endOffset;
    }

    return spans;
}

[[nodiscard]] CsdPipelineSceneSummary parseCsdPipelineSceneSummary(std::string_view objectSpan)
{
    CsdPipelineSceneSummary summary;
    summary.sceneName = jsonStringField(objectSpan, "scene_name").value_or("");
    summary.castCount = static_cast<int>(jsonNumberField(objectSpan, "cast_count").value_or(0.0));
    summary.subimageCount = static_cast<int>(jsonNumberField(objectSpan, "subimage_count").value_or(0.0));

    if (const auto textures = jsonArrayFieldSpan(objectSpan, "used_texture_names"))
        summary.textureNames = parseJsonStringArray(*textures);

    if (const auto frameRange = jsonArrayFieldSpan(objectSpan, "frame_count_range"))
    {
        std::vector<double> values;
        std::size_t offset = 0;
        while (offset < frameRange->size())
        {
            if (!(std::isdigit(static_cast<unsigned char>((*frameRange)[offset])) || (*frameRange)[offset] == '-'))
            {
                ++offset;
                continue;
            }

            std::size_t endOffset = offset + 1;
            while (endOffset < frameRange->size())
            {
                const char ch = (*frameRange)[endOffset];
                if (!(std::isdigit(static_cast<unsigned char>(ch)) || ch == '.' || ch == 'e' || ch == 'E' || ch == '+' || ch == '-'))
                    break;
                ++endOffset;
            }
            values.push_back(std::stod(std::string(frameRange->substr(offset, endOffset - offset))));
            offset = endOffset;
        }

        if (!values.empty())
            summary.frameStart = values.front();
        if (values.size() > 1)
            summary.frameEnd = values[1];
    }

    return summary;
}

[[nodiscard]] CsdPipelineTimelineHook parseCsdPipelineTimelineHook(std::string_view objectSpan)
{
    CsdPipelineTimelineHook hook;
    hook.sceneName = jsonStringField(objectSpan, "scene_name").value_or("");
    hook.animationName = jsonStringField(objectSpan, "animation_name").value_or("");
    hook.frameCount = jsonNumberField(objectSpan, "frame_count").value_or(0.0);
    hook.timelineSeconds = jsonNumberField(objectSpan, "timeline_seconds").value_or(0.0);
    hook.totalKeyframes = static_cast<int>(jsonNumberField(objectSpan, "total_keyframes").value_or(0.0));
    return hook;
}

[[nodiscard]] std::optional<CsdPipelineEvidence> loadCsdPipelineEvidence(std::string_view layoutFileName)
{
    const auto evidencePath = layoutEvidencePath();
    if (!evidencePath)
        return std::nullopt;

    const std::string document = readTextFile(*evidencePath);
    if (document.empty())
        return std::nullopt;

    const auto digestsSpan = jsonArrayFieldSpan(document, "digests");
    if (!digestsSpan)
        return std::nullopt;

    std::optional<std::string_view> digestObjectSpan;
    int bestPreferenceScore = -1;
    for (const auto objectSpan : jsonObjectSpansInArray(*digestsSpan))
    {
        if (jsonStringField(objectSpan, "file_name").value_or("") == layoutFileName)
        {
            const int preferenceScore = layoutFilePreferenceScore(layoutFileName, jsonStringField(objectSpan, "path").value_or(""));
            if (preferenceScore > bestPreferenceScore)
            {
                digestObjectSpan = objectSpan;
                bestPreferenceScore = preferenceScore;
            }
        }
    }

    if (!digestObjectSpan)
        return std::nullopt;

    const auto objectSpan = *digestObjectSpan;
    CsdPipelineEvidence evidence;
    evidence.sourcePath = *evidencePath;
    evidence.layoutFileName = jsonStringField(objectSpan, "file_name").value_or(std::string(layoutFileName));

    if (const auto textures = jsonArrayFieldSpan(objectSpan, "texture_names"))
        evidence.textureNames = parseJsonStringArray(*textures);

    if (const auto scenes = jsonArrayFieldSpan(objectSpan, "scene_summaries"))
    {
        for (const auto sceneObject : jsonObjectSpansInArray(*scenes))
            evidence.scenes.push_back(parseCsdPipelineSceneSummary(sceneObject));
    }

    if (const auto timelines = jsonArrayFieldSpan(objectSpan, "longest_animations"))
    {
        for (const auto timelineObject : jsonObjectSpansInArray(*timelines))
            evidence.timelines.push_back(parseCsdPipelineTimelineHook(timelineObject));
    }

    return evidence;
}

[[nodiscard]] const CsdPipelineSceneSummary* findCsdPipelineScene(
    const CsdPipelineEvidence& evidence,
    std::string_view sceneName)
{
    const auto found = std::find_if(
        evidence.scenes.begin(),
        evidence.scenes.end(),
        [sceneName](const CsdPipelineSceneSummary& scene)
        {
            return scene.sceneName == sceneName;
        });
    return found == evidence.scenes.end() ? nullptr : &*found;
}

[[nodiscard]] const CsdPipelineTimelineHook* findCsdPipelineTimelineHook(
    const CsdPipelineEvidence& evidence,
    std::string_view sceneName,
    std::string_view animationName)
{
    const auto found = std::find_if(
        evidence.timelines.begin(),
        evidence.timelines.end(),
        [sceneName, animationName](const CsdPipelineTimelineHook& hook)
        {
            return hook.sceneName == sceneName && hook.animationName == animationName;
        });
    return found == evidence.timelines.end() ? nullptr : &*found;
}

[[nodiscard]] std::string joinStrings(const std::vector<std::string>& values, std::size_t limit = 0)
{
    std::ostringstream joined;
    const std::size_t count = limit == 0 ? values.size() : std::min(limit, values.size());
    for (std::size_t index = 0; index < count; ++index)
    {
        if (index != 0)
            joined << ",";
        joined << values[index];
    }
    return joined.str();
}

[[nodiscard]] std::uint32_t readLe32(const std::uint8_t* data);
[[nodiscard]] std::optional<DdsTextureImage> loadDdsTextureImage(std::string_view textureFileName);

[[nodiscard]] std::optional<DdsTextureInfo> loadDdsTextureInfo(std::string_view textureFileName)
{
    const auto path = textureSourcePathForFileName(textureFileName);
    if (!path)
        return std::nullopt;

    std::ifstream file(*path, std::ios::binary);
    if (!file)
        return std::nullopt;

    std::array<std::uint8_t, 128> header{};
    file.read(reinterpret_cast<char*>(header.data()), static_cast<std::streamsize>(header.size()));
    if (!file || std::memcmp(header.data(), "DDS ", 4) != 0)
        return std::nullopt;

    const std::uint32_t headerSize = readLe32(header.data() + 4);
    const int height = static_cast<int>(readLe32(header.data() + 12));
    const int width = static_cast<int>(readLe32(header.data() + 16));
    const std::uint32_t pixelFormatSize = readLe32(header.data() + 76);
    const std::string fourCc(reinterpret_cast<const char*>(header.data() + 84), 4);
    if (headerSize != 124 || pixelFormatSize != 32 || width <= 0 || height <= 0)
        return std::nullopt;

    DdsTextureInfo info;
    info.format = fourCc;
    info.width = width;
    info.height = height;
    return info;
}

[[nodiscard]] std::optional<std::string_view> findParsedCsdFileObjectSpan(
    std::string_view document,
    std::string_view layoutFileName)
{
    const auto parsedFiles = jsonArrayFieldSpan(document, "parsed_files");
    if (!parsedFiles)
        return std::nullopt;

    std::optional<std::string_view> bestObjectSpan;
    int bestPreferenceScore = -1;
    for (const auto objectSpan : jsonObjectSpansInArray(*parsedFiles))
    {
        if (jsonStringField(objectSpan, "file_name").value_or("") == layoutFileName)
        {
            const int preferenceScore = layoutFilePreferenceScore(layoutFileName, jsonStringField(objectSpan, "path").value_or(""));
            if (preferenceScore > bestPreferenceScore)
            {
                bestObjectSpan = objectSpan;
                bestPreferenceScore = preferenceScore;
            }
        }
    }

    return bestObjectSpan;
}

[[nodiscard]] std::optional<std::string_view> firstJsonObjectInArrayField(
    std::string_view text,
    std::string_view fieldName)
{
    const auto arraySpan = jsonArrayFieldSpan(text, fieldName);
    if (!arraySpan)
        return std::nullopt;

    const auto objects = jsonObjectSpansInArray(*arraySpan);
    if (objects.empty())
        return std::nullopt;

    return objects.front();
}

[[nodiscard]] std::optional<std::string_view> jsonObjectInArrayAt(
    std::string_view arraySpan,
    std::size_t index)
{
    std::size_t objectIndex = 0;
    for (const auto objectSpan : jsonObjectSpansInArray(arraySpan))
    {
        if (objectIndex == index)
            return objectSpan;
        ++objectIndex;
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<int> csdSceneIndexForName(std::string_view rootObject, std::string_view sceneName);

[[nodiscard]] std::optional<std::string_view> findCsdSceneObjectSpanInNode(
    std::string_view nodeObject,
    std::string_view sceneName)
{
    const auto sceneIndex = csdSceneIndexForName(nodeObject, sceneName);
    const auto scenes = jsonTopLevelArrayFieldSpan(nodeObject, "scenes");
    if (sceneIndex && *sceneIndex >= 0 && scenes)
    {
        if (const auto sceneObject = jsonObjectInArrayAt(*scenes, static_cast<std::size_t>(*sceneIndex)))
            return sceneObject;
    }

    const auto children = jsonTopLevelArrayFieldSpan(nodeObject, "children");
    if (!children)
        return std::nullopt;

    for (const auto childObject : jsonObjectSpansInArray(*children))
    {
        if (const auto sceneObject = findCsdSceneObjectSpanInNode(childObject, sceneName))
            return sceneObject;
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<std::string_view> findCsdSceneObjectSpan(
    std::string_view document,
    std::string_view layoutFileName,
    std::string_view sceneName)
{
    const auto parsedFile = findParsedCsdFileObjectSpan(document, layoutFileName);
    if (!parsedFile)
        return std::nullopt;

    const auto resource = firstJsonObjectInArrayField(*parsedFile, "resources");
    const auto content = resource ? jsonObjectFieldSpan(*resource, "content") : std::optional<std::string_view>{};
    const auto project = content ? jsonObjectFieldSpan(*content, "csdm_project") : std::optional<std::string_view>{};
    const auto root = project ? jsonObjectFieldSpan(*project, "root") : std::optional<std::string_view>{};
    if (!root)
        return std::nullopt;

    return findCsdSceneObjectSpanInNode(*root, sceneName);
}

[[nodiscard]] std::optional<int> csdSceneIndexForName(std::string_view rootObject, std::string_view sceneName)
{
    const auto sceneIds = jsonTopLevelArrayFieldSpan(rootObject, "scene_ids");
    if (!sceneIds)
        return std::nullopt;

    for (const auto objectSpan : jsonObjectSpansInArray(*sceneIds))
    {
        if (jsonStringField(objectSpan, "name").value_or("") == sceneName)
            return static_cast<int>(jsonNumberField(objectSpan, "index").value_or(-1.0));
    }

    return std::nullopt;
}

[[nodiscard]] std::vector<CsdCastDictionaryEntry> parseCsdCastDictionary(std::string_view sceneObject)
{
    std::vector<CsdCastDictionaryEntry> entries;
    const auto dictionaries = jsonArrayFieldSpan(sceneObject, "cast_dictionaries");
    if (!dictionaries)
        return entries;

    for (const auto objectSpan : jsonObjectSpansInArray(*dictionaries))
    {
        CsdCastDictionaryEntry entry;
        entry.groupIndex = static_cast<int>(jsonNumberField(objectSpan, "group_index").value_or(0.0));
        entry.castIndex = static_cast<int>(jsonNumberField(objectSpan, "cast_index").value_or(0.0));
        entry.name = jsonStringField(objectSpan, "name").value_or("");
        entries.push_back(std::move(entry));
    }

    return entries;
}

[[nodiscard]] std::string csdCastNameFor(
    const std::vector<CsdCastDictionaryEntry>& dictionary,
    int groupIndex,
    int castIndex)
{
    const auto found = std::find_if(
        dictionary.begin(),
        dictionary.end(),
        [groupIndex, castIndex](const CsdCastDictionaryEntry& entry)
        {
            return entry.groupIndex == groupIndex && entry.castIndex == castIndex;
        });
    if (found != dictionary.end() && !found->name.empty())
        return found->name;

    std::ostringstream fallback;
    fallback << "Cast_" << groupIndex << "_" << castIndex;
    return fallback.str();
}

[[nodiscard]] std::vector<CsdSubimageBinding> parseCsdSubimages(std::string_view sceneObject)
{
    std::vector<CsdSubimageBinding> subimages;
    const auto subimageArray = jsonArrayFieldSpan(sceneObject, "subimages");
    if (!subimageArray)
        return subimages;

    for (const auto objectSpan : jsonObjectSpansInArray(*subimageArray))
    {
        CsdSubimageBinding subimage;
        subimage.textureIndex = static_cast<int>(jsonNumberField(objectSpan, "texture_index").value_or(-1.0));

        const auto topLeft = jsonNumberArrayField(objectSpan, "top_left");
        const auto bottomRight = jsonNumberArrayField(objectSpan, "bottom_right");
        if (topLeft.size() >= 2)
        {
            subimage.left = topLeft[0];
            subimage.top = topLeft[1];
        }
        if (bottomRight.size() >= 2)
        {
            subimage.right = bottomRight[0];
            subimage.bottom = bottomRight[1];
        }

        subimages.push_back(subimage);
    }

    return subimages;
}

[[nodiscard]] std::optional<int> firstUsedSubimageIndex(std::string_view castObject)
{
    const auto material = jsonObjectFieldSpan(castObject, "cast_material");
    if (!material)
        return std::nullopt;

    const auto usedSubimages = jsonNumberArrayField(*material, "used_subimage_indices");
    for (const double value : usedSubimages)
    {
        const int index = static_cast<int>(value);
        if (index >= 0)
            return index;
    }

    return std::nullopt;
}

[[nodiscard]] bool csdSourceRectFits(const CsdDrawableCommand& command)
{
    return command.textureResolved
        && command.sourceX >= 0
        && command.sourceY >= 0
        && command.sourceWidth > 0
        && command.sourceHeight > 0
        && command.sourceX + command.sourceWidth <= command.textureWidth
        && command.sourceY + command.sourceHeight <= command.textureHeight;
}

[[nodiscard]] std::optional<std::uint32_t> parseCsdHexColor(std::string_view text)
{
    if (text.empty())
        return std::nullopt;

    std::string value(text);
    if (value.starts_with("0x") || value.starts_with("0X"))
        value = value.substr(2);

    try
    {
        return static_cast<std::uint32_t>(std::stoul(value, nullptr, 16));
    }
    catch (...)
    {
        return std::nullopt;
    }
}

[[nodiscard]] CsdColorRgba decodeCsdPackedRgba(std::uint32_t packed)
{
    return CsdColorRgba{
        static_cast<std::uint8_t>((packed >> 24) & 0xFF),
        static_cast<std::uint8_t>((packed >> 16) & 0xFF),
        static_cast<std::uint8_t>((packed >> 8) & 0xFF),
        static_cast<std::uint8_t>(packed & 0xFF),
    };
}

[[nodiscard]] bool isDefaultWhiteRgba(const CsdColorRgba& color)
{
    return color.r == 255 && color.g == 255 && color.b == 255 && color.a == 255;
}

[[nodiscard]] bool sameRgba(const CsdColorRgba& left, const CsdColorRgba& right)
{
    return left.r == right.r && left.g == right.g && left.b == right.b && left.a == right.a;
}

void refreshCsdGradientState(CsdDrawableCommand& command)
{
    command.gradientVarying =
        command.gradientKnown
        && (!sameRgba(command.gradientTopLeftRgba, command.gradientBottomLeftRgba)
            || !sameRgba(command.gradientTopLeftRgba, command.gradientTopRightRgba)
            || !sameRgba(command.gradientTopLeftRgba, command.gradientBottomRightRgba)
            || !isDefaultWhiteRgba(command.gradientTopLeftRgba));
}

[[nodiscard]] CsdColorRgba averageGradientRgba(const CsdDrawableCommand& command)
{
    auto average = [](std::uint8_t a, std::uint8_t b, std::uint8_t c, std::uint8_t d)
    {
        return static_cast<std::uint8_t>((static_cast<int>(a) + b + c + d + 2) / 4);
    };

    return CsdColorRgba{
        average(command.gradientTopLeftRgba.r, command.gradientBottomLeftRgba.r, command.gradientTopRightRgba.r, command.gradientBottomRightRgba.r),
        average(command.gradientTopLeftRgba.g, command.gradientBottomLeftRgba.g, command.gradientTopRightRgba.g, command.gradientBottomRightRgba.g),
        average(command.gradientTopLeftRgba.b, command.gradientBottomLeftRgba.b, command.gradientTopRightRgba.b, command.gradientBottomRightRgba.b),
        average(command.gradientTopLeftRgba.a, command.gradientBottomLeftRgba.a, command.gradientTopRightRgba.a, command.gradientBottomRightRgba.a),
    };
}

[[nodiscard]] CsdColorRgba effectiveCsdDrawRgba(const CsdDrawableCommand& command)
{
    const auto gradient = averageGradientRgba(command);
    auto multiply = [](std::uint8_t left, std::uint8_t right)
    {
        return static_cast<std::uint8_t>((static_cast<int>(left) * static_cast<int>(right) + 127) / 255);
    };

    return CsdColorRgba{
        multiply(command.colorRgba.r, gradient.r),
        multiply(command.colorRgba.g, gradient.g),
        multiply(command.colorRgba.b, gradient.b),
        multiply(command.colorRgba.a, gradient.a),
    };
}

[[nodiscard]] std::optional<CsdColorRgba> parseCsdRgbaField(std::string_view object, std::string_view fieldName)
{
    const auto text = jsonStringField(object, fieldName);
    if (!text)
        return std::nullopt;
    const auto packed = parseCsdHexColor(*text);
    if (!packed)
        return std::nullopt;
    return decodeCsdPackedRgba(*packed);
}

[[nodiscard]] std::uint8_t interpolateCsdByte(std::uint8_t previous, std::uint8_t next, double t)
{
    return static_cast<std::uint8_t>(std::clamp(
        static_cast<int>(std::llround(static_cast<double>(previous) + ((static_cast<double>(next) - static_cast<double>(previous)) * t))),
        0,
        255));
}

[[nodiscard]] CsdColorRgba interpolateCsdRgba(const CsdColorRgba& previous, const CsdColorRgba& next, double t)
{
    return CsdColorRgba{
        interpolateCsdByte(previous.r, next.r, t),
        interpolateCsdByte(previous.g, next.g, t),
        interpolateCsdByte(previous.b, next.b, t),
        interpolateCsdByte(previous.a, next.a, t),
    };
}

[[nodiscard]] std::uint32_t packCsdRgba(const CsdColorRgba& color)
{
    return (static_cast<std::uint32_t>(color.r) << 24)
        | (static_cast<std::uint32_t>(color.g) << 16)
        | (static_cast<std::uint32_t>(color.b) << 8)
        | static_cast<std::uint32_t>(color.a);
}

[[nodiscard]] bool isCsdGradientTrack(std::string_view trackType)
{
    return trackType.starts_with("Gradient");
}

[[nodiscard]] bool isCsdPackedChannelTrack(std::string_view trackType)
{
    return trackType == "Color" || isCsdGradientTrack(trackType);
}

[[nodiscard]] std::vector<CsdDrawableCommand> buildCsdDrawableCommands(
    std::string_view sceneObject,
    std::string_view sceneName,
    const std::vector<std::string>& textureNames)
{
    std::vector<CsdDrawableCommand> commands;
    const auto castGroups = jsonArrayFieldSpan(sceneObject, "cast_groups");
    if (!castGroups)
        return commands;

    const auto dictionary = parseCsdCastDictionary(sceneObject);
    const auto subimages = parseCsdSubimages(sceneObject);
    const auto groupObjects = jsonObjectSpansInArray(*castGroups);
    std::vector<std::pair<std::string, std::optional<DdsTextureInfo>>> textureInfoCache;
    auto textureInfoFor = [&textureInfoCache](std::string_view textureName) -> const DdsTextureInfo*
    {
        const auto found = std::find_if(
            textureInfoCache.begin(),
            textureInfoCache.end(),
            [textureName](const std::pair<std::string, std::optional<DdsTextureInfo>>& entry)
            {
                return entry.first == textureName;
            });
        if (found != textureInfoCache.end())
            return found->second ? &*found->second : nullptr;

        textureInfoCache.emplace_back(std::string(textureName), loadDdsTextureInfo(textureName));
        return textureInfoCache.back().second ? &*textureInfoCache.back().second : nullptr;
    };

    for (std::size_t groupIndex = 0; groupIndex < groupObjects.size(); ++groupIndex)
    {
        const auto casts = jsonArrayFieldSpan(groupObjects[groupIndex], "casts");
        if (!casts)
            continue;

        const auto castObjects = jsonObjectSpansInArray(*casts);
        for (std::size_t castIndex = 0; castIndex < castObjects.size(); ++castIndex)
        {
            const auto castObject = castObjects[castIndex];
            if (static_cast<int>(jsonNumberField(castObject, "is_enabled").value_or(0.0)) == 0)
                continue;

            const auto castInfo = jsonObjectFieldSpan(castObject, "cast_info");
            if (!castInfo)
                continue;

            const auto translation = jsonNumberArrayField(*castInfo, "translation");
            const auto scale = jsonNumberArrayField(*castInfo, "scale");
            const double translationX = translation.size() >= 1 ? translation[0] : 0.0;
            const double translationY = translation.size() >= 2 ? translation[1] : 0.0;
            const double scaleX = scale.size() >= 1 ? scale[0] : 1.0;
            const double scaleY = scale.size() >= 2 ? scale[1] : 1.0;
            const int hideFlag = static_cast<int>(jsonNumberField(*castInfo, "hide_flag").value_or(0.0));
            if (hideFlag != 0)
                continue;
            const int castWidth = static_cast<int>(std::llround(jsonNumberField(castObject, "width").value_or(0.0)));
            const int castHeight = static_cast<int>(std::llround(jsonNumberField(castObject, "height").value_or(0.0)));

            CsdDrawableCommand command;
            command.sceneName = std::string(sceneName);
            command.groupIndex = static_cast<int>(groupIndex);
            command.castIndex = static_cast<int>(castIndex);
            command.castName = csdCastNameFor(dictionary, command.groupIndex, command.castIndex);
            command.drawType = static_cast<int>(jsonNumberField(castObject, "field04").value_or(1.0));
            command.castFlags = static_cast<std::uint32_t>(jsonNumberField(castObject, "field38").value_or(0.0));
            command.castWidth = castWidth;
            command.castHeight = castHeight;
            command.translationX = translationX;
            command.translationY = translationY;
            command.scaleX = scaleX;
            command.scaleY = scaleY;
            command.rotation = jsonNumberField(*castInfo, "rotation").value_or(0.0);
            command.hidden = hideFlag != 0;
            if (const auto colorText = jsonStringField(*castInfo, "color"))
            {
                if (const auto color = parseCsdHexColor(*colorText))
                {
                    command.colorPackedRgba = *color;
                    command.colorRgba = decodeCsdPackedRgba(*color);
                    command.colorKnown = true;
                }
            }

            const auto gradientTopLeft = parseCsdRgbaField(*castInfo, "gradient_top_left");
            const auto gradientBottomLeft = parseCsdRgbaField(*castInfo, "gradient_bottom_left");
            const auto gradientTopRight = parseCsdRgbaField(*castInfo, "gradient_top_right");
            const auto gradientBottomRight = parseCsdRgbaField(*castInfo, "gradient_bottom_right");
            if (gradientTopLeft && gradientBottomLeft && gradientTopRight && gradientBottomRight)
            {
                command.gradientTopLeftRgba = *gradientTopLeft;
                command.gradientBottomLeftRgba = *gradientBottomLeft;
                command.gradientTopRightRgba = *gradientTopRight;
                command.gradientBottomRightRgba = *gradientBottomRight;
                command.gradientKnown = true;
                refreshCsdGradientState(command);
            }

            command.additiveBlend = (command.castFlags & 0x1) != 0;
            command.linearFiltering = (command.castFlags & 0x1000) != 0;
            command.flipX = scaleX < 0.0 || (command.castFlags & 0x400) != 0;
            command.flipY = scaleY < 0.0 || (command.castFlags & 0x800) != 0;
            command.destinationWidth = std::max(1, static_cast<int>(std::llround(std::fabs(static_cast<double>(castWidth) * scaleX))));
            command.destinationHeight = std::max(1, static_cast<int>(std::llround(std::fabs(static_cast<double>(castHeight) * scaleY))));
            command.destinationX = static_cast<int>(std::llround(
                ((0.5 + translationX) * static_cast<double>(kDesignWidth)) - (static_cast<double>(command.destinationWidth) * 0.5)));
            command.destinationY = static_cast<int>(std::llround(
                ((0.5 + translationY) * static_cast<double>(kDesignHeight)) - (static_cast<double>(command.destinationHeight) * 0.5)));

            const auto subimageIndex = firstUsedSubimageIndex(castObject);
            if (!subimageIndex || *subimageIndex < 0 || static_cast<std::size_t>(*subimageIndex) >= subimages.size())
            {
                command.sourceFreeStructural = true;
            }
            else
            {
                const auto& subimage = subimages[static_cast<std::size_t>(*subimageIndex)];
                if (subimage.textureIndex < 0 || static_cast<std::size_t>(subimage.textureIndex) >= textureNames.size())
                {
                    command.sourceFreeStructural = true;
                }
                else
                {
                    command.subimageIndex = *subimageIndex;
                    command.textureIndex = subimage.textureIndex;
                    command.textureName = textureNames[static_cast<std::size_t>(subimage.textureIndex)];

                    const auto* textureInfo = textureInfoFor(command.textureName);
                    if (textureInfo)
                    {
                        command.textureResolved = true;
                        command.textureWidth = textureInfo->width;
                        command.textureHeight = textureInfo->height;
                        const int sourceLeft = static_cast<int>(std::llround(subimage.left * static_cast<double>(textureInfo->width)));
                        const int sourceTop = static_cast<int>(std::llround(subimage.top * static_cast<double>(textureInfo->height)));
                        const int sourceRight = static_cast<int>(std::llround(subimage.right * static_cast<double>(textureInfo->width)));
                        const int sourceBottom = static_cast<int>(std::llround(subimage.bottom * static_cast<double>(textureInfo->height)));
                        command.sourceX = sourceLeft;
                        command.sourceY = sourceTop;
                        command.sourceWidth = std::max(1, sourceRight - sourceLeft);
                        command.sourceHeight = std::max(1, sourceBottom - sourceTop);
                        command.sourceFits = csdSourceRectFits(command);
                    }
                }
            }

            commands.push_back(std::move(command));
        }
    }

    return commands;
}

[[nodiscard]] std::optional<CsdDrawableScene> loadCsdDrawableScene(const CsdPipelineTemplateBinding& binding)
{
    const auto evidencePath = layoutEvidencePath();
    if (!evidencePath)
        return std::nullopt;

    const std::string document = readTextFile(*evidencePath);
    if (document.empty())
        return std::nullopt;

    const auto pipelineEvidence = loadCsdPipelineEvidence(binding.layoutFileName);
    if (!pipelineEvidence)
        return std::nullopt;

    const auto parsedFile = findParsedCsdFileObjectSpan(document, binding.layoutFileName);
    if (!parsedFile)
        return std::nullopt;

    const auto resource = firstJsonObjectInArrayField(*parsedFile, "resources");
    const auto content = resource ? jsonObjectFieldSpan(*resource, "content") : std::optional<std::string_view>{};
    const auto project = content ? jsonObjectFieldSpan(*content, "csdm_project") : std::optional<std::string_view>{};
    const auto root = project ? jsonObjectFieldSpan(*project, "root") : std::optional<std::string_view>{};
    if (!root)
        return std::nullopt;

    const auto sceneObject = findCsdSceneObjectSpanInNode(*root, binding.primarySceneName);
    if (!sceneObject)
        return std::nullopt;

    CsdDrawableScene scene;
    scene.sourcePath = *evidencePath;
    scene.layoutFileName = std::string(binding.layoutFileName);
    scene.sceneName = std::string(binding.primarySceneName);
    if (const auto* summary = findCsdPipelineScene(*pipelineEvidence, binding.primarySceneName))
    {
        scene.castCount = summary->castCount;
        scene.subimageCount = summary->subimageCount;
    }
    else
    {
        scene.castCount = static_cast<int>(jsonNumberField(*sceneObject, "cast_count").value_or(0.0));
        scene.subimageCount = static_cast<int>(jsonNumberField(*sceneObject, "subimages_count").value_or(0.0));
    }
    scene.textureNames = pipelineEvidence->textureNames;
    scene.commands = buildCsdDrawableCommands(*sceneObject, scene.sceneName, scene.textureNames);
    return scene;
}

[[nodiscard]] int csdTimelineSampleFrameForTemplate(std::string_view templateId)
{
    if (templateId == "title-menu")
        return 10;
    if (templateId == "loading")
        return 75;
    if (templateId == "sonic-hud")
        return 99;
    return 50;
}

[[nodiscard]] std::optional<int> csdAnimationIndexForName(
    std::string_view sceneObject,
    std::string_view animationName)
{
    const auto dictionaries = jsonArrayFieldSpan(sceneObject, "animation_dictionaries");
    if (!dictionaries)
        return std::nullopt;

    for (const auto objectSpan : jsonObjectSpansInArray(*dictionaries))
    {
        if (jsonStringField(objectSpan, "name").value_or("") == animationName)
            return static_cast<int>(jsonNumberField(objectSpan, "index").value_or(-1.0));
    }

    return std::nullopt;
}

[[nodiscard]] std::vector<CsdTimelineKeyframe> parseCsdTimelineKeyframes(std::string_view trackObject)
{
    std::vector<CsdTimelineKeyframe> keyframes;
    const auto keyframeArray = jsonArrayFieldSpan(trackObject, "keyframes");
    if (!keyframeArray)
        return keyframes;

    for (const auto keyframeObject : jsonObjectSpansInArray(*keyframeArray))
    {
        const auto frame = jsonNumberField(keyframeObject, "frame");
        const auto value = jsonNumberField(keyframeObject, "value");
        if (!frame || !value || !std::isfinite(*value))
            continue;

        CsdTimelineKeyframe keyframe;
        keyframe.frame = *frame;
        keyframe.value = *value;
        keyframe.interpolationType = jsonStringField(keyframeObject, "type").value_or("Linear");
        keyframes.push_back(std::move(keyframe));
    }

    std::sort(
        keyframes.begin(),
        keyframes.end(),
        [](const CsdTimelineKeyframe& left, const CsdTimelineKeyframe& right)
        {
            return left.frame < right.frame;
        });
    return keyframes;
}

[[nodiscard]] std::optional<std::uint32_t> parseCsdPackedRgbaKeyframeValue(std::string_view keyframeObject)
{
    for (const std::string_view fieldName : { "packed_rgba", "value_packed_rgba", "value_raw_bits" })
    {
        if (const auto text = jsonStringField(keyframeObject, fieldName))
        {
            if (const auto packed = parseCsdHexColor(*text))
                return *packed;
        }
    }

    const auto value = jsonNumberField(keyframeObject, "value");
    if (!value || !std::isfinite(*value) || *value < 0.0 || *value > 4294967295.0)
        return std::nullopt;

    return static_cast<std::uint32_t>(std::llround(*value));
}

[[nodiscard]] std::vector<CsdTimelinePackedRgbaKeyframe> parseCsdTimelinePackedRgbaKeyframes(std::string_view trackObject)
{
    std::vector<CsdTimelinePackedRgbaKeyframe> keyframes;
    const auto keyframeArray = jsonArrayFieldSpan(trackObject, "keyframes");
    if (!keyframeArray)
        return keyframes;

    for (const auto keyframeObject : jsonObjectSpansInArray(*keyframeArray))
    {
        const auto frame = jsonNumberField(keyframeObject, "frame");
        const auto packed = parseCsdPackedRgbaKeyframeValue(keyframeObject);
        if (!frame || !packed)
            continue;

        CsdTimelinePackedRgbaKeyframe keyframe;
        keyframe.frame = *frame;
        keyframe.packedRgba = *packed;
        keyframe.color = decodeCsdPackedRgba(*packed);
        keyframe.interpolationType = jsonStringField(keyframeObject, "type").value_or("Linear");
        keyframes.push_back(std::move(keyframe));
    }

    std::sort(
        keyframes.begin(),
        keyframes.end(),
        [](const CsdTimelinePackedRgbaKeyframe& left, const CsdTimelinePackedRgbaKeyframe& right)
        {
            return left.frame < right.frame;
        });
    return keyframes;
}

[[nodiscard]] std::optional<double> sampleCsdTimelineTrack(
    const std::vector<CsdTimelineKeyframe>& keyframes,
    double frame)
{
    if (keyframes.empty())
        return std::nullopt;

    if (frame <= keyframes.front().frame)
        return keyframes.front().value;

    for (std::size_t index = 1; index < keyframes.size(); ++index)
    {
        const auto& previous = keyframes[index - 1];
        const auto& next = keyframes[index];
        if (frame > next.frame)
            continue;

        if (std::fabs(next.frame - previous.frame) < 0.000001 || previous.interpolationType == "Const")
            return previous.value;

        const double t = std::clamp((frame - previous.frame) / (next.frame - previous.frame), 0.0, 1.0);
        return previous.value + ((next.value - previous.value) * t);
    }

    return keyframes.back().value;
}

[[nodiscard]] std::optional<CsdColorRgba> sampleCsdPackedRgbaTimelineTrack(
    const std::vector<CsdTimelinePackedRgbaKeyframe>& keyframes,
    double frame)
{
    if (keyframes.empty())
        return std::nullopt;

    if (frame <= keyframes.front().frame)
        return keyframes.front().color;

    for (std::size_t index = 1; index < keyframes.size(); ++index)
    {
        const auto& previous = keyframes[index - 1];
        const auto& next = keyframes[index];
        if (frame > next.frame)
            continue;

        if (std::fabs(next.frame - previous.frame) < 0.000001 || previous.interpolationType == "Const")
            return previous.color;

        const double t = std::clamp((frame - previous.frame) / (next.frame - previous.frame), 0.0, 1.0);
        return interpolateCsdRgba(previous.color, next.color, t);
    }

    return keyframes.back().color;
}

[[nodiscard]] std::optional<CsdTimelinePlayback> loadCsdTimelinePlayback(
    const CsdPipelineTemplateBinding& binding,
    std::optional<int> sampleFrameOverride = std::nullopt)
{
    const auto evidencePath = layoutEvidencePath();
    if (!evidencePath)
        return std::nullopt;

    const std::string document = readTextFile(*evidencePath);
    if (document.empty())
        return std::nullopt;

    const auto sceneObject = findCsdSceneObjectSpan(document, binding.layoutFileName, binding.timelineSceneName);
    if (!sceneObject)
        return std::nullopt;

    const auto animationIndex = csdAnimationIndexForName(*sceneObject, binding.timelineAnimationName);
    const auto frameDataArray = jsonArrayFieldSpan(*sceneObject, "animation_frame_data_list");
    const auto keyframeDataArray = jsonArrayFieldSpan(*sceneObject, "animation_keyframe_data_list");
    if (!animationIndex || *animationIndex < 0 || !frameDataArray || !keyframeDataArray)
        return std::nullopt;

    const auto frameData = jsonObjectInArrayAt(*frameDataArray, static_cast<std::size_t>(*animationIndex));
    const auto keyframeData = jsonObjectInArrayAt(*keyframeDataArray, static_cast<std::size_t>(*animationIndex));
    if (!frameData || !keyframeData)
        return std::nullopt;

    CsdTimelinePlayback playback;
    playback.sourcePath = *evidencePath;
    playback.layoutFileName = std::string(binding.layoutFileName);
    playback.sceneName = std::string(binding.timelineSceneName);
    playback.animationName = std::string(binding.timelineAnimationName);
    playback.animationIndex = *animationIndex;
    playback.frameCount = jsonNumberField(*frameData, "frame_count").value_or(0.0);
    const int frameCount = std::max(0, static_cast<int>(std::llround(playback.frameCount)));
    int requestedFrame = sampleFrameOverride.value_or(csdTimelineSampleFrameForTemplate(binding.templateId));
    if (sampleFrameOverride && frameCount > 0)
        requestedFrame = requestedFrame % frameCount;
    playback.sampleFrame = std::clamp(requestedFrame, 0, frameCount);

    const auto dictionary = parseCsdCastDictionary(*sceneObject);
    const auto groups = jsonArrayFieldSpan(*keyframeData, "groups");
    if (!groups)
        return playback;

    const auto groupObjects = jsonObjectSpansInArray(*groups);
    for (std::size_t groupIndex = 0; groupIndex < groupObjects.size(); ++groupIndex)
    {
        const auto casts = jsonArrayFieldSpan(groupObjects[groupIndex], "casts");
        if (!casts)
            continue;

        const auto castObjects = jsonObjectSpansInArray(*casts);
        for (std::size_t castIndex = 0; castIndex < castObjects.size(); ++castIndex)
        {
            const auto subData = jsonArrayFieldSpan(castObjects[castIndex], "sub_data");
            if (!subData)
                continue;

            for (const auto trackObject : jsonObjectSpansInArray(*subData))
            {
                ++playback.trackCount;
                const auto trackType = jsonStringField(trackObject, "track_type").value_or("Unknown");
                const auto keyframeObjects = jsonArrayFieldSpan(trackObject, "keyframes");
                const int rawKeyframeCount = keyframeObjects
                    ? static_cast<int>(jsonObjectSpansInArray(*keyframeObjects).size())
                    : 0;
                playback.keyframeCount += rawKeyframeCount;

                const auto keyframes = parseCsdTimelineKeyframes(trackObject);
                const auto packedKeyframes = isCsdPackedChannelTrack(trackType)
                    ? parseCsdTimelinePackedRgbaKeyframes(trackObject)
                    : std::vector<CsdTimelinePackedRgbaKeyframe>{};
                const int decodedPackedKeyframes = static_cast<int>(packedKeyframes.size());
                int unresolvedKeyframes = std::max(0, rawKeyframeCount - static_cast<int>(keyframes.size()));
                if (isCsdPackedChannelTrack(trackType))
                    unresolvedKeyframes = std::max(0, rawKeyframeCount - decodedPackedKeyframes);
                if (trackType == "Color")
                {
                    ++playback.colorTrackCount;
                    if (rawKeyframeCount > 0)
                        ++playback.packedColorTrackCount;
                    if (decodedPackedKeyframes > 0)
                        ++playback.decodedPackedColorTrackCount;
                }
                if (isCsdGradientTrack(trackType))
                {
                    ++playback.gradientTrackCount;
                    if (rawKeyframeCount > 0)
                        ++playback.packedGradientTrackCount;
                    if (decodedPackedKeyframes > 0)
                        ++playback.decodedPackedGradientTrackCount;
                }
                playback.decodedPackedKeyframeCount += decodedPackedKeyframes;
                if (isCsdPackedChannelTrack(trackType))
                    playback.unresolvedPackedKeyframeCount += unresolvedKeyframes;

                if (const auto packedSample = sampleCsdPackedRgbaTimelineTrack(packedKeyframes, static_cast<double>(playback.sampleFrame)))
                {
                    CsdTimelinePackedRgbaTrackSample trackSample;
                    trackSample.sceneName = playback.sceneName;
                    trackSample.groupIndex = static_cast<int>(groupIndex);
                    trackSample.castIndex = static_cast<int>(castIndex);
                    trackSample.castName = csdCastNameFor(dictionary, trackSample.groupIndex, trackSample.castIndex);
                    trackSample.trackType = trackType;
                    trackSample.sampleFrame = playback.sampleFrame;
                    trackSample.keyframeCount = decodedPackedKeyframes;
                    trackSample.color = *packedSample;
                    trackSample.packedRgba = packCsdRgba(*packedSample);
                    playback.packedRgbaSamples.push_back(std::move(trackSample));
                }

                const auto sample = sampleCsdTimelineTrack(keyframes, static_cast<double>(playback.sampleFrame));
                if (!sample)
                    continue;

                ++playback.numericTrackCount;
                CsdTimelineTrackSample trackSample;
                trackSample.sceneName = playback.sceneName;
                trackSample.groupIndex = static_cast<int>(groupIndex);
                trackSample.castIndex = static_cast<int>(castIndex);
                trackSample.castName = csdCastNameFor(dictionary, trackSample.groupIndex, trackSample.castIndex);
                trackSample.trackType = trackType;
                trackSample.sampleFrame = playback.sampleFrame;
                trackSample.keyframeCount = static_cast<int>(keyframes.size());
                trackSample.value = *sample;
                playback.samples.push_back(std::move(trackSample));
            }
        }
    }

    return playback;
}

[[nodiscard]] std::optional<CsdDrawableCommand> applyCsdTimelineToDrawableCommand(
    const CsdDrawableCommand& command,
    const CsdTimelineTrackSample& sample)
{
    if (command.sceneName != sample.sceneName || command.castName != sample.castName)
        return std::nullopt;

    CsdDrawableCommand sampled = command;
    if (sample.trackType == "XPosition")
        sampled.translationX = sample.value;
    else if (sample.trackType == "YPosition")
        sampled.translationY = sample.value;
    else if (sample.trackType == "XScale")
        sampled.scaleX = sample.value;
    else if (sample.trackType == "YScale")
        sampled.scaleY = sample.value;
    else if (sample.trackType == "Rotation")
        sampled.rotation = sample.value;
    else
        return std::nullopt;

    const double castWidth = command.castWidth > 0
        ? static_cast<double>(command.castWidth)
        : (std::fabs(command.scaleX) > 0.000001
            ? static_cast<double>(command.destinationWidth) / std::fabs(command.scaleX)
            : static_cast<double>(command.destinationWidth));
    const double castHeight = command.castHeight > 0
        ? static_cast<double>(command.castHeight)
        : (std::fabs(command.scaleY) > 0.000001
            ? static_cast<double>(command.destinationHeight) / std::fabs(command.scaleY)
            : static_cast<double>(command.destinationHeight));

    sampled.destinationWidth = std::max(1, static_cast<int>(std::llround(std::fabs(castWidth * sampled.scaleX))));
    sampled.destinationHeight = std::max(1, static_cast<int>(std::llround(std::fabs(castHeight * sampled.scaleY))));
    sampled.destinationX = static_cast<int>(std::llround(
        ((0.5 + sampled.translationX) * static_cast<double>(kDesignWidth)) - (static_cast<double>(sampled.destinationWidth) * 0.5)));
    sampled.destinationY = static_cast<int>(std::llround(
        ((0.5 + sampled.translationY) * static_cast<double>(kDesignHeight)) - (static_cast<double>(sampled.destinationHeight) * 0.5)));
    return sampled;
}

[[nodiscard]] std::optional<CsdDrawableCommand> applyCsdPackedRgbaTimelineToDrawableCommand(
    const CsdDrawableCommand& command,
    const CsdTimelinePackedRgbaTrackSample& sample)
{
    if (command.sceneName != sample.sceneName || command.castName != sample.castName)
        return std::nullopt;

    CsdDrawableCommand sampled = command;
    if (sample.trackType == "Color")
    {
        sampled.colorPackedRgba = sample.packedRgba;
        sampled.colorRgba = sample.color;
        sampled.colorKnown = true;
    }
    else if (sample.trackType == "GradientTL")
    {
        sampled.gradientTopLeftRgba = sample.color;
        sampled.gradientKnown = true;
    }
    else if (sample.trackType == "GradientBL")
    {
        sampled.gradientBottomLeftRgba = sample.color;
        sampled.gradientKnown = true;
    }
    else if (sample.trackType == "GradientTR")
    {
        sampled.gradientTopRightRgba = sample.color;
        sampled.gradientKnown = true;
    }
    else if (sample.trackType == "GradientBR")
    {
        sampled.gradientBottomRightRgba = sample.color;
        sampled.gradientKnown = true;
    }
    else
    {
        return std::nullopt;
    }

    refreshCsdGradientState(sampled);
    return sampled;
}

[[nodiscard]] std::optional<std::filesystem::path> findRuntimeEvidenceManifestForTarget(std::string_view target)
{
    auto manifestMatchesTarget = [target](const std::filesystem::path& manifest)
    {
        const std::string text = readTextFile(manifest);
        if (text.empty())
            return false;
        const std::string fieldNeedle = "\"target\"";
        const std::string valueNeedle = "\"" + std::string(target) + "\"";
        std::size_t offset = 0;
        while ((offset = text.find(fieldNeedle, offset)) != std::string::npos)
        {
            const auto colonOffset = text.find(':', offset + fieldNeedle.size());
            if (colonOffset == std::string::npos)
                return false;
            const auto nextFieldOffset = text.find('"', colonOffset + 1);
            if (nextFieldOffset != std::string::npos && text.compare(nextFieldOffset, valueNeedle.size(), valueNeedle) == 0)
                return true;
            offset = colonOffset + 1;
        }
        return false;
    };

    std::optional<std::filesystem::path> bestManifest;
    std::string bestSessionName;
    for (const auto& root : repoRootCandidates())
    {
        std::error_code error;
        const auto evidenceRoot = root / "out" / "ui_lab_runtime_evidence";
        if (!std::filesystem::is_directory(evidenceRoot, error))
            continue;

        for (const auto& session : std::filesystem::directory_iterator(evidenceRoot, error))
        {
            if (error)
                break;
            if (!session.is_directory(error))
                continue;

            const auto sessionName = session.path().filename().string();
            const auto targetManifest = session.path() / std::filesystem::path(std::string(target)) / "capture_manifest.json";
            const auto rootManifest = session.path() / "capture_manifest.json";

            std::optional<std::filesystem::path> manifest;
            if (std::filesystem::is_regular_file(targetManifest, error))
                manifest = targetManifest;
            else if (std::filesystem::is_regular_file(rootManifest, error) && manifestMatchesTarget(rootManifest))
                manifest = rootManifest;

            if (!manifest)
                continue;

            if (!bestManifest || sessionName > bestSessionName)
            {
                bestManifest = *manifest;
                bestSessionName = sessionName;
            }
        }
    }

    return bestManifest;
}

[[nodiscard]] std::uint16_t readLe16(const std::uint8_t* data)
{
    return static_cast<std::uint16_t>(data[0] | (data[1] << 8));
}

[[nodiscard]] std::uint32_t readLe32(const std::uint8_t* data)
{
    return static_cast<std::uint32_t>(data[0])
        | (static_cast<std::uint32_t>(data[1]) << 8)
        | (static_cast<std::uint32_t>(data[2]) << 16)
        | (static_cast<std::uint32_t>(data[3]) << 24);
}

void decodeDxt5Block(const std::uint8_t* block, int blockX, int blockY, int width, int height, std::vector<std::uint32_t>& pixels)
{
    std::array<std::uint8_t, 8> alpha{};
    alpha[0] = block[0];
    alpha[1] = block[1];
    if (alpha[0] > alpha[1])
    {
        alpha[2] = static_cast<std::uint8_t>((6 * alpha[0] + alpha[1]) / 7);
        alpha[3] = static_cast<std::uint8_t>((5 * alpha[0] + 2 * alpha[1]) / 7);
        alpha[4] = static_cast<std::uint8_t>((4 * alpha[0] + 3 * alpha[1]) / 7);
        alpha[5] = static_cast<std::uint8_t>((3 * alpha[0] + 4 * alpha[1]) / 7);
        alpha[6] = static_cast<std::uint8_t>((2 * alpha[0] + 5 * alpha[1]) / 7);
        alpha[7] = static_cast<std::uint8_t>((alpha[0] + 6 * alpha[1]) / 7);
    }
    else
    {
        alpha[2] = static_cast<std::uint8_t>((4 * alpha[0] + alpha[1]) / 5);
        alpha[3] = static_cast<std::uint8_t>((3 * alpha[0] + 2 * alpha[1]) / 5);
        alpha[4] = static_cast<std::uint8_t>((2 * alpha[0] + 3 * alpha[1]) / 5);
        alpha[5] = static_cast<std::uint8_t>((alpha[0] + 4 * alpha[1]) / 5);
        alpha[6] = 0;
        alpha[7] = 255;
    }

    std::uint64_t alphaBits = 0;
    for (int index = 0; index < 6; ++index)
        alphaBits |= static_cast<std::uint64_t>(block[2 + index]) << (index * 8);

    const std::uint16_t color0 = readLe16(block + 8);
    const std::uint16_t color1 = readLe16(block + 10);
    auto unpackColor = [](std::uint16_t color)
    {
        const std::uint8_t r5 = static_cast<std::uint8_t>((color >> 11) & 0x1F);
        const std::uint8_t g6 = static_cast<std::uint8_t>((color >> 5) & 0x3F);
        const std::uint8_t b5 = static_cast<std::uint8_t>(color & 0x1F);
        return std::array<std::uint8_t, 3>{
            static_cast<std::uint8_t>((r5 << 3) | (r5 >> 2)),
            static_cast<std::uint8_t>((g6 << 2) | (g6 >> 4)),
            static_cast<std::uint8_t>((b5 << 3) | (b5 >> 2)),
        };
    };

    std::array<std::array<std::uint8_t, 3>, 4> colors{};
    colors[0] = unpackColor(color0);
    colors[1] = unpackColor(color1);
    colors[2] = {
        static_cast<std::uint8_t>((2 * colors[0][0] + colors[1][0]) / 3),
        static_cast<std::uint8_t>((2 * colors[0][1] + colors[1][1]) / 3),
        static_cast<std::uint8_t>((2 * colors[0][2] + colors[1][2]) / 3),
    };
    colors[3] = {
        static_cast<std::uint8_t>((colors[0][0] + 2 * colors[1][0]) / 3),
        static_cast<std::uint8_t>((colors[0][1] + 2 * colors[1][1]) / 3),
        static_cast<std::uint8_t>((colors[0][2] + 2 * colors[1][2]) / 3),
    };

    const std::uint32_t colorBits = readLe32(block + 12);
    for (int y = 0; y < 4; ++y)
    {
        for (int x = 0; x < 4; ++x)
        {
            const int pixelIndex = (y * 4) + x;
            const int targetX = (blockX * 4) + x;
            const int targetY = (blockY * 4) + y;
            if (targetX >= width || targetY >= height)
                continue;

            const std::uint8_t alphaValue = alpha[(alphaBits >> (3 * pixelIndex)) & 0x7];
            const auto& color = colors[(colorBits >> (2 * pixelIndex)) & 0x3];
            pixels[static_cast<std::size_t>(targetY * width + targetX)] =
                (static_cast<std::uint32_t>(alphaValue) << 24)
                | (static_cast<std::uint32_t>(color[0]) << 16)
                | (static_cast<std::uint32_t>(color[1]) << 8)
                | static_cast<std::uint32_t>(color[2]);
        }
    }
}

[[nodiscard]] std::optional<DdsTextureImage> loadDdsTextureImage(std::string_view textureFileName)
{
    const auto path = textureSourcePathForFileName(textureFileName);
    if (!path)
        return std::nullopt;

    std::ifstream file(*path, std::ios::binary | std::ios::ate);
    if (!file)
        return std::nullopt;

    const auto fileSize = file.tellg();
    if (fileSize < 128)
        return std::nullopt;

    std::vector<std::uint8_t> data(static_cast<std::size_t>(fileSize));
    file.seekg(0, std::ios::beg);
    file.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size()));
    if (!file || std::memcmp(data.data(), "DDS ", 4) != 0)
        return std::nullopt;

    const std::uint32_t headerSize = readLe32(data.data() + 4);
    const int height = static_cast<int>(readLe32(data.data() + 12));
    const int width = static_cast<int>(readLe32(data.data() + 16));
    const std::uint32_t pixelFormatSize = readLe32(data.data() + 76);
    const std::uint32_t pixelFormatFlags = readLe32(data.data() + 80);
    const std::string fourCc(reinterpret_cast<const char*>(data.data() + 84), 4);
    const std::uint32_t rgbBitCount = readLe32(data.data() + 88);
    const std::uint32_t redMask = readLe32(data.data() + 92);
    const std::uint32_t greenMask = readLe32(data.data() + 96);
    const std::uint32_t blueMask = readLe32(data.data() + 100);
    const std::uint32_t alphaMask = readLe32(data.data() + 104);
    if (headerSize != 124 || pixelFormatSize != 32 || width <= 0 || height <= 0)
        return std::nullopt;

    DdsTextureImage image;
    image.path = *path;
    image.fileName = path->filename().string();
    image.width = width;
    image.height = height;
    image.argbPixels.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), 0);

    if (fourCc == "DXT5")
    {
        const int blockCountX = (width + 3) / 4;
        const int blockCountY = (height + 3) / 4;
        const std::size_t requiredBytes = 128 + (static_cast<std::size_t>(blockCountX) * static_cast<std::size_t>(blockCountY) * 16);
        if (data.size() < requiredBytes)
            return std::nullopt;

        image.format = fourCc;
        const std::uint8_t* block = data.data() + 128;
        for (int blockY = 0; blockY < blockCountY; ++blockY)
        {
            for (int blockX = 0; blockX < blockCountX; ++blockX)
            {
                decodeDxt5Block(block, blockX, blockY, width, height, image.argbPixels);
                block += 16;
            }
        }

        return image;
    }

    constexpr std::uint32_t kDdsPixelFormatRgb = 0x40;
    constexpr std::uint32_t kDdsPixelFormatAlphaPixels = 0x1;
    const bool isBgra8 = (pixelFormatFlags & kDdsPixelFormatRgb) != 0
        && (pixelFormatFlags & kDdsPixelFormatAlphaPixels) != 0
        && rgbBitCount == 32
        && redMask == 0x00FF0000
        && greenMask == 0x0000FF00
        && blueMask == 0x000000FF
        && alphaMask == 0xFF000000;
    const std::size_t requiredBytes = 128 + (static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4);
    if (!isBgra8 || data.size() < requiredBytes)
        return std::nullopt;

    image.format = "BGRA8";
    const auto* source = data.data() + 128;
    for (std::size_t index = 0; index < image.argbPixels.size(); ++index)
    {
        const std::uint8_t blue = source[index * 4 + 0];
        const std::uint8_t green = source[index * 4 + 1];
        const std::uint8_t red = source[index * 4 + 2];
        const std::uint8_t alpha = source[index * 4 + 3];
        image.argbPixels[index] = (static_cast<std::uint32_t>(alpha) << 24)
            | (static_cast<std::uint32_t>(red) << 16)
            | (static_cast<std::uint32_t>(green) << 8)
            | static_cast<std::uint32_t>(blue);
    }
    return image;
}

[[nodiscard]] bool castSourceFits(const SuUiRenderCast& cast, const DdsTextureImage& image)
{
    return cast.sourceX >= 0
        && cast.sourceY >= 0
        && cast.sourceWidth > 0
        && cast.sourceHeight > 0
        && cast.sourceX + cast.sourceWidth <= image.width
        && cast.sourceY + cast.sourceHeight <= image.height;
}

[[nodiscard]] std::unique_ptr<Gdiplus::Bitmap> bitmapFromDdsTextureImage(const DdsTextureImage& image)
{
    auto bitmap = std::make_unique<Gdiplus::Bitmap>(image.width, image.height, PixelFormat32bppARGB);
    Gdiplus::BitmapData bitmapData{};
    Gdiplus::Rect lockRect(0, 0, image.width, image.height);
    if (bitmap->LockBits(&lockRect, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &bitmapData) != Gdiplus::Ok)
        return nullptr;

    for (int y = 0; y < image.height; ++y)
    {
        auto* destination = static_cast<std::uint8_t*>(bitmapData.Scan0) + (static_cast<std::ptrdiff_t>(bitmapData.Stride) * y);
        const auto* source = image.argbPixels.data() + (static_cast<std::size_t>(y) * static_cast<std::size_t>(image.width));
        std::memcpy(destination, source, static_cast<std::size_t>(image.width) * sizeof(std::uint32_t));
    }

    bitmap->UnlockBits(&bitmapData);
    return bitmap;
}

[[nodiscard]] std::unique_ptr<Gdiplus::Bitmap> bitmapFromArgbPixels(
    int width,
    int height,
    const std::vector<std::uint32_t>& pixels)
{
    if (width <= 0 || height <= 0 || pixels.size() < static_cast<std::size_t>(width) * static_cast<std::size_t>(height))
        return nullptr;

    auto bitmap = std::make_unique<Gdiplus::Bitmap>(width, height, PixelFormat32bppARGB);
    Gdiplus::BitmapData bitmapData{};
    Gdiplus::Rect lockRect(0, 0, width, height);
    if (bitmap->LockBits(&lockRect, Gdiplus::ImageLockModeWrite, PixelFormat32bppARGB, &bitmapData) != Gdiplus::Ok)
        return nullptr;

    for (int y = 0; y < height; ++y)
    {
        auto* destination = static_cast<std::uint8_t*>(bitmapData.Scan0) + (static_cast<std::ptrdiff_t>(bitmapData.Stride) * y);
        const auto* source = pixels.data() + (static_cast<std::size_t>(y) * static_cast<std::size_t>(width));
        std::memcpy(destination, source, static_cast<std::size_t>(width) * sizeof(std::uint32_t));
    }

    bitmap->UnlockBits(&bitmapData);
    return bitmap;
}

struct CachedTexture
{
    std::string textureFileName;
    std::optional<DdsTextureImage> image;
    std::unique_ptr<Gdiplus::Bitmap> bitmap;
};

class SwardSuUiAssetRenderer
{
public:
    SwardSuUiAssetRenderer()
        : atlasSheets_(discoverAtlasSheetPaths())
    {
    }

    [[nodiscard]] std::size_t selectedScreenIndex() const
    {
        return selectedScreenIndex_ % kRendererScreens.size();
    }

    [[nodiscard]] std::size_t screenCount() const
    {
        return kRendererScreens.size();
    }

    [[nodiscard]] const SuUiRendererScreen& selectedScreen() const
    {
        return kRendererScreens[selectedScreenIndex()];
    }

    void selectNext()
    {
        selectedScreenIndex_ = (selectedScreenIndex_ + 1) % kRendererScreens.size();
        selectedSgfxTemplateId_.reset();
    }

    void selectPrevious()
    {
        selectedScreenIndex_ = (selectedScreenIndex_ + kRendererScreens.size() - 1) % kRendererScreens.size();
        selectedSgfxTemplateId_.reset();
    }

    [[nodiscard]] bool selectScreenById(std::string_view id)
    {
        const auto index = rendererScreenIndexById(id);
        if (!index)
            return false;

        selectedScreenIndex_ = *index;
        return true;
    }

    [[nodiscard]] bool selectSgfxTemplate(std::string_view templateId)
    {
        const auto* binding = findSgfxTemplateRenderBinding(templateId);
        if (!binding || !selectScreenById(binding->rendererScreenId))
            return false;

        selectedSgfxTemplateId_ = std::string(binding->templateId);
        return true;
    }

    void selectNextAtlas()
    {
        if (atlasSheets_.empty())
            return;
        selectedScreenIndex_ = atlasGalleryScreenIndex();
        selectedSgfxTemplateId_.reset();
        selectedAtlasIndex_ = (selectedAtlasIndex_ + 1) % atlasSheets_.size();
        atlasBitmap_.reset();
    }

    void selectPreviousAtlas()
    {
        if (atlasSheets_.empty())
            return;
        selectedScreenIndex_ = atlasGalleryScreenIndex();
        selectedSgfxTemplateId_.reset();
        selectedAtlasIndex_ = (selectedAtlasIndex_ + atlasSheets_.size() - 1) % atlasSheets_.size();
        atlasBitmap_.reset();
    }

    [[nodiscard]] const SgfxTemplateRenderBinding* selectedSgfxTemplateBinding() const
    {
        if (!selectedSgfxTemplateId_)
            return nullptr;

        return findSgfxTemplateRenderBinding(*selectedSgfxTemplateId_);
    }

    [[nodiscard]] std::size_t atlasSheetCount() const
    {
        return atlasSheets_.size();
    }

    [[nodiscard]] std::string selectedAtlasFileName() const
    {
        if (atlasSheets_.empty())
            return "none";
        return atlasSheets_[selectedAtlasIndex_ % atlasSheets_.size()].filename().string();
    }

    [[nodiscard]] std::string selectedScreenIndexText() const
    {
        const auto& screen = selectedScreen();
        std::ostringstream text;
        text
            << (selectedScreenIndex() + 1)
            << "/"
            << screenCount()
            << " "
            << screen.id
            << " - "
            << screen.displayName;
        if (screen.kind == RendererScreenKind::AtlasGallery)
        {
            text << " | atlas ";
            if (atlasSheets_.empty())
            {
                text << "0/0 missing visual_atlas/sheets";
            }
            else
            {
                text
                    << ((selectedAtlasIndex_ % atlasSheets_.size()) + 1)
                    << "/"
                    << atlasSheets_.size()
                    << " "
                    << selectedAtlasFileName();
            }
        }
        if (selectedSgfxTemplateId_)
            text << " | template " << *selectedSgfxTemplateId_;
        return text.str();
    }

    [[nodiscard]] const CachedTexture* textureFor(std::string_view textureFileName)
    {
        const auto found = std::find_if(
            textureCache_.begin(),
            textureCache_.end(),
            [textureFileName](const auto& cached)
            {
                return cached->textureFileName == textureFileName;
            });
        if (found != textureCache_.end())
            return found->get();

        auto cached = std::make_unique<CachedTexture>();
        cached->textureFileName = std::string(textureFileName);
        cached->image = loadDdsTextureImage(textureFileName);
        if (cached->image)
            cached->bitmap = bitmapFromDdsTextureImage(*cached->image);

        textureCache_.push_back(std::move(cached));
        return textureCache_.back().get();
    }

    [[nodiscard]] Gdiplus::Bitmap* currentAtlasBitmap()
    {
        if (atlasSheets_.empty())
            return nullptr;

        const auto& path = atlasSheets_[selectedAtlasIndex_ % atlasSheets_.size()];
        if (atlasBitmap_ && atlasBitmapPath_ == path)
            return atlasBitmap_.get();

        auto bitmap = std::unique_ptr<Gdiplus::Bitmap>(Gdiplus::Bitmap::FromFile(path.wstring().c_str(), FALSE));
        if (!bitmap || bitmap->GetLastStatus() != Gdiplus::Ok)
        {
            atlasBitmap_.reset();
            atlasBitmapPath_.clear();
            return nullptr;
        }

        atlasBitmapPath_ = path;
        atlasBitmap_ = std::move(bitmap);
        return atlasBitmap_.get();
    }

    [[nodiscard]] Gdiplus::Bitmap* titleMovieFrameBitmap()
    {
        if (!titleMovieFrameLoadAttempted_)
        {
            titleMovieFrameLoadAttempted_ = true;
            titleMovieFramePath_ = findTitleMoviePreviewFramePath();
            if (titleMovieFramePath_)
            {
                auto bitmap = std::unique_ptr<Gdiplus::Bitmap>(Gdiplus::Bitmap::FromFile(titleMovieFramePath_->wstring().c_str(), FALSE));
                if (bitmap && bitmap->GetLastStatus() == Gdiplus::Ok)
                    titleMovieFrameBitmap_ = std::move(bitmap);
            }
        }

        return titleMovieFrameBitmap_.get();
    }

    [[nodiscard]] Gdiplus::Bitmap* titleLogoBitmap()
    {
        if (!titleLogoLoadAttempted_)
        {
            titleLogoLoadAttempted_ = true;
            titleLogoPath_ = findTitleLogoPreviewPath();
            if (titleLogoPath_)
            {
                auto bitmap = std::unique_ptr<Gdiplus::Bitmap>(Gdiplus::Bitmap::FromFile(titleLogoPath_->wstring().c_str(), FALSE));
                if (bitmap && bitmap->GetLastStatus() == Gdiplus::Ok)
                    titleLogoBitmap_ = std::move(bitmap);
            }
        }

        return titleLogoBitmap_.get();
    }

private:
    std::size_t selectedScreenIndex_ = 0;
    std::size_t selectedAtlasIndex_ = 0;
    std::optional<std::string> selectedSgfxTemplateId_;
    std::vector<std::filesystem::path> atlasSheets_;
    std::filesystem::path atlasBitmapPath_;
    std::unique_ptr<Gdiplus::Bitmap> atlasBitmap_;
    bool titleMovieFrameLoadAttempted_ = false;
    std::optional<std::filesystem::path> titleMovieFramePath_;
    std::unique_ptr<Gdiplus::Bitmap> titleMovieFrameBitmap_;
    bool titleLogoLoadAttempted_ = false;
    std::optional<std::filesystem::path> titleLogoPath_;
    std::unique_ptr<Gdiplus::Bitmap> titleLogoBitmap_;
    std::vector<std::unique_ptr<CachedTexture>> textureCache_;
};

[[nodiscard]] Gdiplus::RectF designRectToCanvas(const Gdiplus::RectF& canvas, int x, int y, int width, int height)
{
    return Gdiplus::RectF(
        canvas.X + (static_cast<float>(x) / static_cast<float>(kDesignWidth) * canvas.Width),
        canvas.Y + (static_cast<float>(y) / static_cast<float>(kDesignHeight) * canvas.Height),
        static_cast<float>(width) / static_cast<float>(kDesignWidth) * canvas.Width,
        static_cast<float>(height) / static_cast<float>(kDesignHeight) * canvas.Height);
}

[[nodiscard]] Gdiplus::RectF canvasRectForClient(const RECT& client)
{
    const float clientWidth = static_cast<float>(std::max(1L, client.right - client.left));
    const float clientHeight = static_cast<float>(std::max(1L, client.bottom - client.top));
    const float scale = std::min(clientWidth / static_cast<float>(kDesignWidth), clientHeight / static_cast<float>(kDesignHeight));
    const float width = static_cast<float>(kDesignWidth) * scale;
    const float height = static_cast<float>(kDesignHeight) * scale;
    return Gdiplus::RectF(
        static_cast<float>(client.left) + ((clientWidth - width) * 0.5F),
        static_cast<float>(client.top) + ((clientHeight - height) * 0.5F),
        width,
        height);
}

[[nodiscard]] Gdiplus::RectF fitBitmapRectToCanvas(const Gdiplus::RectF& canvas, UINT width, UINT height)
{
    if (width == 0 || height == 0)
        return canvas;

    const float scale = std::min(canvas.Width / static_cast<float>(width), canvas.Height / static_cast<float>(height));
    const float fittedWidth = static_cast<float>(width) * scale;
    const float fittedHeight = static_cast<float>(height) * scale;
    return Gdiplus::RectF(
        canvas.X + ((canvas.Width - fittedWidth) * 0.5F),
        canvas.Y + ((canvas.Height - fittedHeight) * 0.5F),
        fittedWidth,
        fittedHeight);
}

[[nodiscard]] std::wstring widenAscii(std::string_view text)
{
    return std::wstring(text.begin(), text.end());
}

void updateRendererStatus(HWND hwnd, const SwardSuUiAssetRenderer& renderer)
{
    const auto status = renderer.selectedScreenIndexText();
    const auto wideStatus = widenAscii(status);
    if (HWND label = GetDlgItem(hwnd, kScreenLabelId))
        SetWindowTextW(label, wideStatus.c_str());

    const std::wstring title = L"SWARD SU UI Asset Renderer - " + wideStatus;
    SetWindowTextW(hwnd, title.c_str());
}

void layoutRendererControls(HWND hwnd)
{
    RECT client{};
    GetClientRect(hwnd, &client);

    const int margin = 10;
    const int buttonWidth = 88;
    const int atlasButtonWidth = 112;
    const int buttonHeight = 28;
    const int top = 8;
    const int nextLeft = margin + buttonWidth + 8;
    const int atlasPrevLeft = nextLeft + buttonWidth + 8;
    const int atlasNextLeft = atlasPrevLeft + atlasButtonWidth + 8;
    const int labelLeft = atlasNextLeft + atlasButtonWidth + 14;
    const int labelWidth = std::max(160L, client.right - labelLeft - margin);

    if (HWND previous = GetDlgItem(hwnd, kPrevButtonId))
        MoveWindow(previous, margin, top, buttonWidth, buttonHeight, TRUE);
    if (HWND next = GetDlgItem(hwnd, kNextButtonId))
        MoveWindow(next, nextLeft, top, buttonWidth, buttonHeight, TRUE);
    if (HWND atlasPrevious = GetDlgItem(hwnd, kAtlasPrevButtonId))
        MoveWindow(atlasPrevious, atlasPrevLeft, top, atlasButtonWidth, buttonHeight, TRUE);
    if (HWND atlasNext = GetDlgItem(hwnd, kAtlasNextButtonId))
        MoveWindow(atlasNext, atlasNextLeft, top, atlasButtonWidth, buttonHeight, TRUE);
    if (HWND label = GetDlgItem(hwnd, kScreenLabelId))
        MoveWindow(label, labelLeft, top, labelWidth, buttonHeight, TRUE);
}

void createRendererControls(HWND hwnd)
{
    const auto instance = reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hwnd, GWLP_HINSTANCE));

    CreateWindowExW(0, L"BUTTON", L"Prev",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        0, 0, 0, 0,
        hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kPrevButtonId)),
        instance,
        nullptr);

    CreateWindowExW(0, L"BUTTON", L"Next",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        0, 0, 0, 0,
        hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kNextButtonId)),
        instance,
        nullptr);

    CreateWindowExW(0, L"BUTTON", L"Atlas Prev",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        0, 0, 0, 0,
        hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kAtlasPrevButtonId)),
        instance,
        nullptr);

    CreateWindowExW(0, L"BUTTON", L"Atlas Next",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        0, 0, 0, 0,
        hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kAtlasNextButtonId)),
        instance,
        nullptr);

    CreateWindowExW(0, L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
        0, 0, 0, 0,
        hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kScreenLabelId)),
        instance,
        nullptr);

    layoutRendererControls(hwnd);
}

void drawMissingCast(Gdiplus::Graphics& graphics, const Gdiplus::RectF& destination)
{
    Gdiplus::SolidBrush fill(Gdiplus::Color(180, 28, 42, 28));
    Gdiplus::Pen outline(Gdiplus::Color(220, 98, 205, 98), 1.0F);
    graphics.FillRectangle(&fill, destination);
    graphics.DrawRectangle(&outline, destination);
}

[[nodiscard]] Gdiplus::PointF designPointToCanvas(const Gdiplus::RectF& canvas, float x, float y);

void drawOutlinedText(
    Gdiplus::Graphics& graphics,
    const Gdiplus::RectF& canvas,
    std::string_view text,
    float x,
    float y,
    float size,
    Gdiplus::Color fill,
    Gdiplus::Color outline,
    INT style);

[[nodiscard]] bool drawRenderCastTexture(
    Gdiplus::Graphics& graphics,
    const Gdiplus::RectF& canvas,
    SwardSuUiAssetRenderer& renderer,
    const SuUiRenderCast& cast)
{
    const auto destination = designRectToCanvas(canvas, cast.destinationX, cast.destinationY, cast.destinationWidth, cast.destinationHeight);
    const auto* texture = renderer.textureFor(cast.textureName);
    if (!texture || !texture->image || !texture->bitmap || !castSourceFits(cast, *texture->image))
    {
        drawMissingCast(graphics, destination);
        return false;
    }

    graphics.DrawImage(
        texture->bitmap.get(),
        destination,
        static_cast<float>(cast.sourceX),
        static_cast<float>(cast.sourceY),
        static_cast<float>(cast.sourceWidth),
        static_cast<float>(cast.sourceHeight),
        Gdiplus::UnitPixel);
    return true;
}

[[nodiscard]] std::uint8_t clampCsdByte(double value)
{
    return static_cast<std::uint8_t>(std::clamp(static_cast<int>(std::llround(value)), 0, 255));
}

[[nodiscard]] CsdColorRgba unpackArgbPixel(std::uint32_t argb)
{
    return CsdColorRgba{
        static_cast<std::uint8_t>((argb >> 16) & 0xFF),
        static_cast<std::uint8_t>((argb >> 8) & 0xFF),
        static_cast<std::uint8_t>(argb & 0xFF),
        static_cast<std::uint8_t>((argb >> 24) & 0xFF),
    };
}

[[nodiscard]] std::uint32_t packArgbPixel(const CsdColorRgba& color)
{
    return (static_cast<std::uint32_t>(color.a) << 24)
        | (static_cast<std::uint32_t>(color.r) << 16)
        | (static_cast<std::uint32_t>(color.g) << 8)
        | static_cast<std::uint32_t>(color.b);
}

[[nodiscard]] CsdColorRgba multiplyCsdRgba(const CsdColorRgba& left, const CsdColorRgba& right)
{
    auto multiply = [](std::uint8_t a, std::uint8_t b)
    {
        return static_cast<std::uint8_t>((static_cast<int>(a) * static_cast<int>(b) + 127) / 255);
    };

    return CsdColorRgba{
        multiply(left.r, right.r),
        multiply(left.g, right.g),
        multiply(left.b, right.b),
        multiply(left.a, right.a),
    };
}

[[nodiscard]] CsdColorRgba sampleTextureArgbNearest(const DdsTextureImage& image, double x, double y)
{
    const int ix = std::clamp(static_cast<int>(std::floor(x)), 0, image.width - 1);
    const int iy = std::clamp(static_cast<int>(std::floor(y)), 0, image.height - 1);
    return unpackArgbPixel(image.argbPixels[static_cast<std::size_t>(iy) * static_cast<std::size_t>(image.width) + static_cast<std::size_t>(ix)]);
}

[[nodiscard]] CsdColorRgba sampleTextureArgbBilinear(const DdsTextureImage& image, double x, double y)
{
    x = std::clamp(x, 0.0, static_cast<double>(image.width - 1));
    y = std::clamp(y, 0.0, static_cast<double>(image.height - 1));
    const int x0 = std::clamp(static_cast<int>(std::floor(x)), 0, image.width - 1);
    const int y0 = std::clamp(static_cast<int>(std::floor(y)), 0, image.height - 1);
    const int x1 = std::min(x0 + 1, image.width - 1);
    const int y1 = std::min(y0 + 1, image.height - 1);
    const double tx = x - static_cast<double>(x0);
    const double ty = y - static_cast<double>(y0);

    auto pixelAt = [&image](int px, int py)
    {
        return unpackArgbPixel(image.argbPixels[static_cast<std::size_t>(py) * static_cast<std::size_t>(image.width) + static_cast<std::size_t>(px)]);
    };

    const auto c00 = pixelAt(x0, y0);
    const auto c10 = pixelAt(x1, y0);
    const auto c01 = pixelAt(x0, y1);
    const auto c11 = pixelAt(x1, y1);
    auto channel = [tx, ty](std::uint8_t v00, std::uint8_t v10, std::uint8_t v01, std::uint8_t v11)
    {
        const double top = static_cast<double>(v00) + ((static_cast<double>(v10) - static_cast<double>(v00)) * tx);
        const double bottom = static_cast<double>(v01) + ((static_cast<double>(v11) - static_cast<double>(v01)) * tx);
        return clampCsdByte(top + ((bottom - top) * ty));
    };

    return CsdColorRgba{
        channel(c00.r, c10.r, c01.r, c11.r),
        channel(c00.g, c10.g, c01.g, c11.g),
        channel(c00.b, c10.b, c01.b, c11.b),
        channel(c00.a, c10.a, c01.a, c11.a),
    };
}

[[nodiscard]] double csdFilterCoordinate(double coordinate, double footprint)
{
    const double safeFootprint = std::max(std::abs(footprint), 0.000001);
    const double seam = std::floor(coordinate + 0.5);
    const double filtered = ((coordinate - seam) / safeFootprint) + seam;
    return std::clamp(filtered, seam - 0.5, seam + 0.5);
}

[[nodiscard]] CsdColorRgba sampleTextureArgbCsdFilter(
    const DdsTextureImage& image,
    double x,
    double y,
    double footprintX,
    double footprintY)
{
    const double filteredX = csdFilterCoordinate(x, footprintX);
    const double filteredY = csdFilterCoordinate(y, footprintY);
    return sampleTextureArgbBilinear(image, filteredX, filteredY);
}

[[nodiscard]] CsdColorRgba gradientVertexColor(const CsdDrawableCommand& command, double u, double v)
{
    if (!command.gradientKnown)
        return CsdColorRgba{};

    const auto top = interpolateCsdRgba(command.gradientTopLeftRgba, command.gradientTopRightRgba, std::clamp(u, 0.0, 1.0));
    const auto bottom = interpolateCsdRgba(command.gradientBottomLeftRgba, command.gradientBottomRightRgba, std::clamp(u, 0.0, 1.0));
    return interpolateCsdRgba(top, bottom, std::clamp(v, 0.0, 1.0));
}

void blendCsdPixelSrcAlphaOver(std::uint32_t& destinationArgb, const CsdColorRgba& source)
{
    const auto destination = unpackArgbPixel(destinationArgb);
    const double sourceAlpha = static_cast<double>(source.a) / 255.0;
    const double invAlpha = 1.0 - sourceAlpha;
    const CsdColorRgba blended{
        clampCsdByte((static_cast<double>(source.r) * sourceAlpha) + (static_cast<double>(destination.r) * invAlpha)),
        clampCsdByte((static_cast<double>(source.g) * sourceAlpha) + (static_cast<double>(destination.g) * invAlpha)),
        clampCsdByte((static_cast<double>(source.b) * sourceAlpha) + (static_cast<double>(destination.b) * invAlpha)),
        clampCsdByte(static_cast<double>(source.a) + (static_cast<double>(destination.a) * invAlpha)),
    };
    destinationArgb = packArgbPixel(blended);
}

void blendCsdPixelSrcAlphaOne(std::uint32_t& destinationArgb, const CsdColorRgba& source)
{
    const auto destination = unpackArgbPixel(destinationArgb);
    const double sourceAlpha = static_cast<double>(source.a) / 255.0;
    const CsdColorRgba blended{
        clampCsdByte((static_cast<double>(source.r) * sourceAlpha) + static_cast<double>(destination.r)),
        clampCsdByte((static_cast<double>(source.g) * sourceAlpha) + static_cast<double>(destination.g)),
        clampCsdByte((static_cast<double>(source.b) * sourceAlpha) + static_cast<double>(destination.b)),
        clampCsdByte(static_cast<double>(source.a) + static_cast<double>(destination.a)),
    };
    destinationArgb = packArgbPixel(blended);
}

[[nodiscard]] bool drawCsdDrawableCommandSoftware(
    std::vector<std::uint32_t>& canvasPixels,
    std::vector<std::uint8_t>* coverageMask,
    int canvasWidth,
    int canvasHeight,
    SwardSuUiAssetRenderer& renderer,
    const CsdDrawableCommand& command,
    CsdSoftwareRenderStats& stats)
{
    if (command.hidden)
        return true;

    const auto* texture = renderer.textureFor(command.textureName);
    if ((!command.sourceFreeStructural && (!texture || !texture->image || !command.sourceFits)) || canvasWidth <= 0 || canvasHeight <= 0)
        return false;

    const double dstX = static_cast<double>(command.destinationX);
    const double dstY = static_cast<double>(command.destinationY);
    const double dstW = static_cast<double>(std::max(1, command.destinationWidth));
    const double dstH = static_cast<double>(std::max(1, command.destinationHeight));
    const double centerX = dstX + (dstW * 0.5);
    const double centerY = dstY + (dstH * 0.5);
    const double radians = command.rotation * kPi / 180.0;
    const double cosTheta = std::cos(radians);
    const double sinTheta = std::sin(radians);

    std::array<std::pair<double, double>, 4> corners{{
        { -dstW * 0.5, -dstH * 0.5 },
        { dstW * 0.5, -dstH * 0.5 },
        { -dstW * 0.5, dstH * 0.5 },
        { dstW * 0.5, dstH * 0.5 },
    }};

    double minX = static_cast<double>(canvasWidth);
    double minY = static_cast<double>(canvasHeight);
    double maxX = 0.0;
    double maxY = 0.0;
    for (const auto& [cornerX, cornerY] : corners)
    {
        const double rotatedX = centerX + (cornerX * cosTheta) - (cornerY * sinTheta);
        const double rotatedY = centerY + (cornerX * sinTheta) + (cornerY * cosTheta);
        minX = std::min(minX, rotatedX);
        minY = std::min(minY, rotatedY);
        maxX = std::max(maxX, rotatedX);
        maxY = std::max(maxY, rotatedY);
    }

    const int startX = std::clamp(static_cast<int>(std::floor(minX)), 0, canvasWidth - 1);
    const int startY = std::clamp(static_cast<int>(std::floor(minY)), 0, canvasHeight - 1);
    const int endX = std::clamp(static_cast<int>(std::ceil(maxX)), 0, canvasWidth - 1);
    const int endY = std::clamp(static_cast<int>(std::ceil(maxY)), 0, canvasHeight - 1);
    if (endX < startX || endY < startY)
        return true;

    ++stats.softwareQuadCommandCount;
    if (command.gradientKnown)
        ++stats.gradientVertexColorCommandCount;
    if (command.additiveBlend)
        ++stats.additiveSoftwareCommandCount;

    for (int y = startY; y <= endY; ++y)
    {
        for (int x = startX; x <= endX; ++x)
        {
            const double px = static_cast<double>(x) + 0.5;
            const double py = static_cast<double>(y) + 0.5;
            const double dx = px - centerX;
            const double dy = py - centerY;
            double localX = (dx * cosTheta) + (dy * sinTheta) + (dstW * 0.5);
            double localY = (-dx * sinTheta) + (dy * cosTheta) + (dstH * 0.5);
            if (command.flipX)
                localX = dstW - localX;
            if (command.flipY)
                localY = dstH - localY;
            if (localX < 0.0 || localY < 0.0 || localX >= dstW || localY >= dstH)
                continue;

            const double u = localX / dstW;
            const double v = localY / dstH;
            CsdColorRgba textureColor{};
            if (!command.sourceFreeStructural)
            {
                const double sourceX = static_cast<double>(command.sourceX) + (u * static_cast<double>(command.sourceWidth));
                const double sourceY = static_cast<double>(command.sourceY) + (v * static_cast<double>(command.sourceHeight));
                if (command.linearFiltering)
                {
                    textureColor = sampleTextureArgbBilinear(*texture->image, sourceX, sourceY);
                    ++stats.samplerStats.bilinearSampleCount;
                }
                else
                {
                    const double footprintX = static_cast<double>(std::max(1, command.sourceWidth)) / dstW;
                    const double footprintY = static_cast<double>(std::max(1, command.sourceHeight)) / dstH;
                    textureColor = sampleTextureArgbCsdFilter(*texture->image, sourceX, sourceY, footprintX, footprintY);
                    ++stats.samplerStats.csdPointFilterSampleCount;
                }
            }
            const auto vertexColor = multiplyCsdRgba(command.colorRgba, gradientVertexColor(command, u, v));
            const auto shaded = multiplyCsdRgba(textureColor, vertexColor);
            if (shaded.a == 0)
                continue;

            const auto pixelIndex = static_cast<std::size_t>(y) * static_cast<std::size_t>(canvasWidth) + static_cast<std::size_t>(x);
            if (coverageMask && pixelIndex < coverageMask->size())
                (*coverageMask)[pixelIndex] = 255;

            auto& destination = canvasPixels[pixelIndex];
            if (command.additiveBlend)
                blendCsdPixelSrcAlphaOne(destination, shaded);
            else
                blendCsdPixelSrcAlphaOver(destination, shaded);
        }
    }

    return true;
}

[[nodiscard]] bool drawCsdDrawableCommand(
    Gdiplus::Graphics& graphics,
    const Gdiplus::RectF& canvas,
    SwardSuUiAssetRenderer& renderer,
    const CsdDrawableCommand& command)
{
    if (command.hidden)
        return true;

    const auto destination = designRectToCanvas(canvas, command.destinationX, command.destinationY, command.destinationWidth, command.destinationHeight);
    const auto* texture = command.sourceFreeStructural ? nullptr : renderer.textureFor(command.textureName);
    if (!command.sourceFreeStructural && (!texture || !texture->image || !texture->bitmap || !command.sourceFits))
    {
        drawMissingCast(graphics, destination);
        return false;
    }

    // Shuriken's CSD reference path treats packed colors as RGBA and multiplies
    // cast color by per-corner gradients before sampling the texture. GDI+ has
    // no textured-quad vertex color path, so gradients are averaged here and
    // reported as an approximation in the Phase 128 manifest.
    const auto effectiveColor = effectiveCsdDrawRgba(command);
    const auto r = static_cast<float>(effectiveColor.r) / 255.0F;
    const auto g = static_cast<float>(effectiveColor.g) / 255.0F;
    const auto b = static_cast<float>(effectiveColor.b) / 255.0F;
    const auto a = static_cast<float>(effectiveColor.a) / 255.0F;
    Gdiplus::ColorMatrix matrix = {{
        { r, 0.0F, 0.0F, 0.0F, 0.0F },
        { 0.0F, g, 0.0F, 0.0F, 0.0F },
        { 0.0F, 0.0F, b, 0.0F, 0.0F },
        { 0.0F, 0.0F, 0.0F, a, 0.0F },
        { 0.0F, 0.0F, 0.0F, 0.0F, 1.0F },
    }};
    Gdiplus::ImageAttributes attributes;
    attributes.SetColorMatrix(&matrix, Gdiplus::ColorMatrixFlagsDefault, Gdiplus::ColorAdjustTypeBitmap);

    const auto state = graphics.Save();
    if (command.flipX || command.flipY)
    {
        graphics.TranslateTransform(destination.X + (destination.Width * 0.5F), destination.Y + (destination.Height * 0.5F));
        graphics.ScaleTransform(command.flipX ? -1.0F : 1.0F, command.flipY ? -1.0F : 1.0F);
        graphics.TranslateTransform(-(destination.X + (destination.Width * 0.5F)), -(destination.Y + (destination.Height * 0.5F)));
    }
    if (std::fabs(command.rotation) > 0.000001)
    {
        graphics.TranslateTransform(destination.X + (destination.Width * 0.5F), destination.Y + (destination.Height * 0.5F));
        graphics.RotateTransform(static_cast<Gdiplus::REAL>(command.rotation));
        graphics.TranslateTransform(-(destination.X + (destination.Width * 0.5F)), -(destination.Y + (destination.Height * 0.5F)));
    }

    graphics.SetCompositingMode(Gdiplus::CompositingModeSourceOver);
    if (command.sourceFreeStructural)
    {
        Gdiplus::SolidBrush brush(Gdiplus::Color(effectiveColor.a, effectiveColor.r, effectiveColor.g, effectiveColor.b));
        graphics.FillRectangle(&brush, destination);
        graphics.Restore(state);
        return true;
    }

    graphics.DrawImage(
        texture->bitmap.get(),
        destination,
        static_cast<float>(command.sourceX),
        static_cast<float>(command.sourceY),
        static_cast<float>(command.sourceWidth),
        static_cast<float>(command.sourceHeight),
        Gdiplus::UnitPixel,
        &attributes);
    graphics.Restore(state);
    return true;
}

void renderAtlasGalleryScreen(Gdiplus::Graphics& graphics, const Gdiplus::RectF& canvas, SwardSuUiAssetRenderer& renderer)
{
    auto* bitmap = renderer.currentAtlasBitmap();
    if (!bitmap)
    {
        drawMissingCast(graphics, canvas);
        return;
    }

    const auto destination = fitBitmapRectToCanvas(canvas, bitmap->GetWidth(), bitmap->GetHeight());
    graphics.DrawImage(
        bitmap,
        destination,
        0.0F,
        0.0F,
        static_cast<float>(bitmap->GetWidth()),
        static_cast<float>(bitmap->GetHeight()),
            Gdiplus::UnitPixel);
}

void drawTitleWordArt(
    Gdiplus::Graphics& graphics,
    const Gdiplus::RectF& canvas,
    std::string_view text,
    float x,
    float y,
    float size,
    Gdiplus::Color fill,
    Gdiplus::Color outline,
    Gdiplus::Color glow,
    float outlineWidth)
{
    const float scale = canvas.Width / static_cast<float>(kDesignWidth);
    Gdiplus::FontFamily family(L"Arial Black");
    Gdiplus::StringFormat format;
    Gdiplus::GraphicsPath path;
    const auto wideText = widenAscii(text);
    const auto point = designPointToCanvas(canvas, x, y);
    path.AddString(
        wideText.c_str(),
        -1,
        &family,
        Gdiplus::FontStyleBold | Gdiplus::FontStyleItalic,
        std::max(1.0F, size * scale),
        point,
        &format);

    Gdiplus::Pen glowPen(glow, std::max(1.0F, outlineWidth * 1.65F * scale));
    Gdiplus::Pen outlinePen(outline, std::max(1.0F, outlineWidth * scale));
    Gdiplus::SolidBrush fillBrush(fill);
    graphics.DrawPath(&glowPen, &path);
    graphics.DrawPath(&outlinePen, &path);
    graphics.FillPath(&fillBrush, &path);
}

void drawTitlePromptShell(Gdiplus::Graphics& graphics, const Gdiplus::RectF& canvas)
{
    const auto glow = designRectToCanvas(canvas, 445, 510, 390, 76);
    Gdiplus::SolidBrush glowBrush(Gdiplus::Color(120, 86, 160, 22));
    graphics.FillEllipse(&glowBrush, glow);

    const auto shadow = designRectToCanvas(canvas, 488, 520, 304, 56);
    Gdiplus::SolidBrush shadowBrush(Gdiplus::Color(190, 50, 70, 42));
    graphics.FillRectangle(&shadowBrush, shadow);

    const auto frame = designRectToCanvas(canvas, 498, 526, 284, 42);
    Gdiplus::SolidBrush frameBrush(Gdiplus::Color(235, 194, 206, 142));
    Gdiplus::SolidBrush innerBrush(Gdiplus::Color(255, 248, 225, 40));
    Gdiplus::Pen edgePen(Gdiplus::Color(255, 238, 238, 142), std::max(1.0F, 2.0F * (canvas.Width / static_cast<float>(kDesignWidth))));
    graphics.FillRectangle(&frameBrush, frame);
    const auto inner = designRectToCanvas(canvas, 512, 534, 256, 26);
    graphics.FillRectangle(&innerBrush, inner);
    graphics.DrawRectangle(&edgePen, frame);
}

void renderTitleLoopReconstructionScreen(
    Gdiplus::Graphics& graphics,
    const Gdiplus::RectF& canvas,
    SwardSuUiAssetRenderer& renderer)
{
    if (auto* titleFrame = renderer.titleMovieFrameBitmap())
    {
        graphics.DrawImage(
            titleFrame,
            canvas,
            0.0F,
            0.0F,
            static_cast<float>(titleFrame->GetWidth()),
            static_cast<float>(titleFrame->GetHeight()),
            Gdiplus::UnitPixel);
    }
    else
    {
        (void)drawRenderCastTexture(graphics, canvas, renderer, kTitleLoopReconstructionCasts[0]);
        Gdiplus::SolidBrush fallbackDim(Gdiplus::Color(180, 0, 0, 0));
        graphics.FillRectangle(&fallbackDim, canvas);
    }

    Gdiplus::SolidBrush filmDim(Gdiplus::Color(105, 0, 0, 0));
    graphics.FillRectangle(&filmDim, canvas);

    bool drewTitleLogo = false;
    if (auto* titleLogo = renderer.titleLogoBitmap())
    {
        const auto titleLogoDestination = designRectToCanvas(canvas, 280, 175, 720, 320);
        graphics.DrawImage(
            titleLogo,
            titleLogoDestination,
            300.0F,
            210.0F,
            720.0F,
            320.0F,
            Gdiplus::UnitPixel);
        drewTitleLogo = true;
    }
    else
    {
        // Keep the decompressed DDS as smoke-test evidence, but do not use the
        // current hand-rolled DXT path for this logo in the visual renderer; it
        // is not spatially faithful for the XCompress-derived texture yet.
        drewTitleLogo = false;
    }

    if (!drewTitleLogo)
    {
        drawTitleWordArt(
            graphics,
            canvas,
            "SONIC",
            358,
            170,
            132,
            Gdiplus::Color(255, 255, 218, 18),
            Gdiplus::Color(255, 0, 0, 0),
            Gdiplus::Color(230, 255, 255, 255),
            18.0F);
        drawTitleWordArt(
            graphics,
            canvas,
            "UNLEASHED",
            388,
            316,
            76,
            Gdiplus::Color(255, 248, 248, 248),
            Gdiplus::Color(255, 0, 0, 0),
            Gdiplus::Color(230, 255, 255, 255),
            12.0F);
        drawOutlinedText(graphics, canvas, "TM", 915, 318, 18, Gdiplus::Color(255, 240, 240, 240), Gdiplus::Color(255, 0, 0, 0), Gdiplus::FontStyleBold);
    }

    drawTitlePromptShell(graphics, canvas);
    (void)drawRenderCastTexture(graphics, canvas, renderer, kTitleLoopReconstructionCasts[2]);
    (void)drawRenderCastTexture(graphics, canvas, renderer, kTitleLoopReconstructionCasts[3]);
}

[[nodiscard]] Gdiplus::PointF designPointToCanvas(const Gdiplus::RectF& canvas, float x, float y)
{
    const float scale = canvas.Width / static_cast<float>(kDesignWidth);
    return Gdiplus::PointF(canvas.X + (x * scale), canvas.Y + (y * scale));
}

void drawOutlinedText(
    Gdiplus::Graphics& graphics,
    const Gdiplus::RectF& canvas,
    std::string_view text,
    float x,
    float y,
    float size,
    Gdiplus::Color fill,
    Gdiplus::Color outline,
    INT style = Gdiplus::FontStyleBold)
{
    const float scale = canvas.Width / static_cast<float>(kDesignWidth);
    Gdiplus::FontFamily family(L"Arial");
    Gdiplus::Font font(&family, std::max(1.0F, size * scale), style, Gdiplus::UnitPixel);
    const auto wideText = widenAscii(text);
    const auto point = designPointToCanvas(canvas, x, y);
    Gdiplus::SolidBrush outlineBrush(outline);
    Gdiplus::SolidBrush fillBrush(fill);
    const float offset = std::max(1.0F, 2.0F * scale);

    graphics.DrawString(wideText.c_str(), -1, &font, Gdiplus::PointF(point.X - offset, point.Y), &outlineBrush);
    graphics.DrawString(wideText.c_str(), -1, &font, Gdiplus::PointF(point.X + offset, point.Y), &outlineBrush);
    graphics.DrawString(wideText.c_str(), -1, &font, Gdiplus::PointF(point.X, point.Y - offset), &outlineBrush);
    graphics.DrawString(wideText.c_str(), -1, &font, Gdiplus::PointF(point.X, point.Y + offset), &outlineBrush);
    graphics.DrawString(wideText.c_str(), -1, &font, point, &fillBrush);
}

void drawSlantedHudPanel(
    Gdiplus::Graphics& graphics,
    const Gdiplus::RectF& canvas,
    float x,
    float y,
    float width,
    float height,
    Gdiplus::Color fill,
    Gdiplus::Color outline)
{
    const float scale = canvas.Width / static_cast<float>(kDesignWidth);
    const float cut = 28.0F * scale;
    const auto topLeft = designPointToCanvas(canvas, x, y);
    const auto bottomRight = designPointToCanvas(canvas, x + width, y + height);
    Gdiplus::PointF points[] = {
        Gdiplus::PointF(topLeft.X + cut, topLeft.Y),
        Gdiplus::PointF(bottomRight.X, topLeft.Y),
        Gdiplus::PointF(bottomRight.X - cut, bottomRight.Y),
        Gdiplus::PointF(topLeft.X, bottomRight.Y),
    };
    Gdiplus::SolidBrush fillBrush(fill);
    Gdiplus::Pen outlinePen(outline, std::max(1.0F, 2.0F * scale));
    graphics.FillPolygon(&fillBrush, points, 4);
    graphics.DrawPolygon(&outlinePen, points, 4);
}

void drawGaugeSegments(
    Gdiplus::Graphics& graphics,
    const Gdiplus::RectF& canvas,
    float x,
    float y,
    int segmentCount,
    int activeCount,
    Gdiplus::Color active,
    Gdiplus::Color inactive)
{
    const float segmentWidth = 15.0F;
    const float segmentHeight = 18.0F;
    const float gap = 4.0F;
    for (int index = 0; index < segmentCount; ++index)
    {
        const auto rect = designRectToCanvas(
            canvas,
            static_cast<int>(x + (index * (segmentWidth + gap))),
            static_cast<int>(y),
            static_cast<int>(segmentWidth),
            static_cast<int>(segmentHeight));
        Gdiplus::SolidBrush brush(index < activeCount ? active : inactive);
        Gdiplus::Pen outline(Gdiplus::Color(180, 220, 220, 220), 1.0F);
        graphics.FillRectangle(&brush, rect);
        graphics.DrawRectangle(&outline, rect);
    }
}

void drawRingIcon(Gdiplus::Graphics& graphics, const Gdiplus::RectF& canvas, float x, float y, float size)
{
    const auto outer = designRectToCanvas(canvas, static_cast<int>(x), static_cast<int>(y), static_cast<int>(size), static_cast<int>(size));
    const auto inner = designRectToCanvas(canvas, static_cast<int>(x + (size * 0.23F)), static_cast<int>(y + (size * 0.23F)), static_cast<int>(size * 0.54F), static_cast<int>(size * 0.54F));
    Gdiplus::SolidBrush orange(Gdiplus::Color(255, 232, 119, 24));
    Gdiplus::SolidBrush yellow(Gdiplus::Color(255, 255, 232, 74));
    Gdiplus::SolidBrush black(Gdiplus::Color(255, 0, 0, 0));
    Gdiplus::Pen darkEdge(Gdiplus::Color(255, 96, 48, 4), 2.0F);
    graphics.FillEllipse(&orange, outer);
    graphics.FillEllipse(&yellow, designRectToCanvas(canvas, static_cast<int>(x + 6), static_cast<int>(y + 5), static_cast<int>(size - 14), static_cast<int>(size - 14)));
    graphics.FillEllipse(&black, inner);
    graphics.DrawEllipse(&darkEdge, outer);
}

void renderSonicHudReconstructionScreen(Gdiplus::Graphics& graphics, const Gdiplus::RectF& canvas)
{
    Gdiplus::SolidBrush shade(Gdiplus::Color(255, 0, 0, 0));
    graphics.FillRectangle(&shade, canvas);

    drawSlantedHudPanel(graphics, canvas, -18, 82, 280, 44, Gdiplus::Color(210, 58, 68, 102), Gdiplus::Color(230, 206, 214, 228));
    drawOutlinedText(graphics, canvas, "05", 104, 80, 40, Gdiplus::Color(255, 250, 250, 250), Gdiplus::Color(255, 24, 24, 24));
    drawOutlinedText(graphics, canvas, "TIME", 138, 134, 22, Gdiplus::Color(255, 236, 236, 236), Gdiplus::Color(255, 24, 24, 24));
    drawSlantedHudPanel(graphics, canvas, -22, 162, 356, 34, Gdiplus::Color(210, 82, 94, 136), Gdiplus::Color(220, 196, 202, 220));
    drawOutlinedText(graphics, canvas, "00:00:31", 136, 154, 38, Gdiplus::Color(255, 250, 250, 250), Gdiplus::Color(255, 24, 24, 24));
    drawOutlinedText(graphics, canvas, "SCORE", 138, 222, 22, Gdiplus::Color(255, 236, 236, 236), Gdiplus::Color(255, 24, 24, 24));
    drawSlantedHudPanel(graphics, canvas, -22, 250, 356, 34, Gdiplus::Color(210, 82, 94, 136), Gdiplus::Color(220, 196, 202, 220));
    drawOutlinedText(graphics, canvas, "00000000", 136, 242, 38, Gdiplus::Color(255, 250, 250, 250), Gdiplus::Color(255, 24, 24, 24));

    drawOutlinedText(graphics, canvas, "GO!", 520, 286, 92, Gdiplus::Color(255, 238, 238, 238), Gdiplus::Color(255, 24, 24, 24));
    drawGaugeSegments(graphics, canvas, 690, 360, 9, 5, Gdiplus::Color(255, 245, 216, 44), Gdiplus::Color(120, 180, 180, 180));

    drawSlantedHudPanel(graphics, canvas, 34, 596, 334, 36, Gdiplus::Color(225, 36, 41, 48), Gdiplus::Color(240, 200, 205, 210));
    drawOutlinedText(graphics, canvas, "SPEED", 108, 588, 24, Gdiplus::Color(255, 236, 236, 236), Gdiplus::Color(255, 18, 18, 18));
    drawGaugeSegments(graphics, canvas, 178, 604, 12, 7, Gdiplus::Color(255, 255, 219, 36), Gdiplus::Color(120, 174, 176, 178));

    drawRingIcon(graphics, canvas, 34, 634, 70);
    drawSlantedHudPanel(graphics, canvas, 86, 640, 306, 40, Gdiplus::Color(225, 42, 47, 54), Gdiplus::Color(240, 200, 205, 210));
    drawOutlinedText(graphics, canvas, "RINGS", 112, 630, 26, Gdiplus::Color(255, 236, 236, 236), Gdiplus::Color(255, 18, 18, 18));
    drawOutlinedText(graphics, canvas, "000", 112, 654, 38, Gdiplus::Color(255, 248, 248, 248), Gdiplus::Color(255, 18, 18, 18));

    drawSlantedHudPanel(graphics, canvas, 84, 688, 424, 36, Gdiplus::Color(225, 42, 47, 54), Gdiplus::Color(240, 200, 205, 210));
    drawOutlinedText(graphics, canvas, "RING ENERGY", 112, 680, 26, Gdiplus::Color(255, 236, 236, 236), Gdiplus::Color(255, 18, 18, 18));
    drawGaugeSegments(graphics, canvas, 278, 696, 12, 8, Gdiplus::Color(255, 95, 220, 82), Gdiplus::Color(120, 174, 176, 178));
}

void renderSgfxTemplatePlaceholderScreen(
    Gdiplus::Graphics& graphics,
    const Gdiplus::RectF& canvas,
    SwardSuUiAssetRenderer& renderer,
    const SgfxTemplateRenderBinding& binding)
{
    const auto* screenTemplate = findSgfxScreenTemplate(binding.templateId);
    const auto panel = designRectToCanvas(canvas, 24, 24, 650, 190);
    Gdiplus::SolidBrush panelFill(Gdiplus::Color(205, 6, 11, 18));
    Gdiplus::Pen panelEdge(Gdiplus::Color(230, 110, 190, 255), std::max(1.0F, 2.0F * (canvas.Width / static_cast<float>(kDesignWidth))));
    graphics.FillRectangle(&panelFill, panel);
    graphics.DrawRectangle(&panelEdge, panel);

    const std::string title = std::string("SGFX template: ") + std::string(binding.templateId);
    const std::string event = std::string("ready: ")
        + std::string(!binding.requiredEventId.empty()
            ? binding.requiredEventId
            : (screenTemplate && !screenTemplate->evidence.requiredEvents.empty() ? std::string_view(screenTemplate->evidence.requiredEvents.front()) : std::string_view("unknown")));
    const std::string timing = std::string("timeline: ") + std::string(binding.timelineBandId)
        + " -> " + std::string(binding.timelineEventLabel);

    drawOutlinedText(graphics, canvas, title, 42, 36, 24, Gdiplus::Color(255, 250, 252, 255), Gdiplus::Color(255, 0, 0, 0));
    drawOutlinedText(graphics, canvas, event, 42, 68, 17, Gdiplus::Color(255, 210, 232, 255), Gdiplus::Color(255, 0, 0, 0));
    drawOutlinedText(graphics, canvas, timing, 42, 94, 17, Gdiplus::Color(255, 220, 255, 205), Gdiplus::Color(255, 0, 0, 0));

    for (std::size_t index = 0; index < binding.slotCount && index < 4; ++index)
    {
        const auto& slot = binding.slots[index];
        const auto* texture = renderer.textureFor(slot.textureName);
        const bool available = texture && texture->image && texture->bitmap;
        const int y = 122 + static_cast<int>(index) * 18;
        const auto marker = designRectToCanvas(canvas, 46, y + 4, 10, 10);
        Gdiplus::SolidBrush markerBrush(available ? Gdiplus::Color(255, 86, 214, 120) : Gdiplus::Color(255, 220, 80, 72));
        graphics.FillRectangle(&markerBrush, marker);

        const std::string slotText = std::string("placeholder_slot=")
            + std::string(slot.slotName)
            + "->"
            + std::string(slot.textureName);
        drawOutlinedText(graphics, canvas, slotText, 64, static_cast<float>(y), 14, Gdiplus::Color(255, 238, 238, 238), Gdiplus::Color(255, 0, 0, 0));
    }
}

[[nodiscard]] std::string formatCsdNumber(double value);

[[nodiscard]] const CsdPipelineEvidence* cachedCsdPipelineEvidence(std::string_view layoutFileName)
{
    static std::vector<std::pair<std::string, std::optional<CsdPipelineEvidence>>> cache;
    const auto found = std::find_if(
        cache.begin(),
        cache.end(),
        [layoutFileName](const std::pair<std::string, std::optional<CsdPipelineEvidence>>& entry)
        {
            return entry.first == layoutFileName;
        });
    if (found != cache.end())
        return found->second ? &*found->second : nullptr;

    cache.emplace_back(std::string(layoutFileName), loadCsdPipelineEvidence(layoutFileName));
    return cache.back().second ? &*cache.back().second : nullptr;
}

[[nodiscard]] const CsdDrawableScene* cachedCsdDrawableScene(const CsdPipelineTemplateBinding& binding)
{
    static std::vector<std::pair<std::string, std::optional<CsdDrawableScene>>> cache;
    const std::string key = std::string(binding.layoutFileName) + ":" + std::string(binding.primarySceneName);
    const auto found = std::find_if(
        cache.begin(),
        cache.end(),
        [&key](const std::pair<std::string, std::optional<CsdDrawableScene>>& entry)
        {
            return entry.first == key;
        });
    if (found != cache.end())
        return found->second ? &*found->second : nullptr;

    cache.emplace_back(key, loadCsdDrawableScene(binding));
    return cache.back().second ? &*cache.back().second : nullptr;
}

[[nodiscard]] bool renderCsdDrawableScene(
    Gdiplus::Graphics& graphics,
    const Gdiplus::RectF& canvas,
    SwardSuUiAssetRenderer& renderer,
    const SgfxTemplateRenderBinding& sgfxBinding)
{
    const auto* csdBinding = findCsdPipelineTemplateBinding(sgfxBinding.templateId);
    const auto* scene = csdBinding ? cachedCsdDrawableScene(*csdBinding) : nullptr;
    if (!scene || scene->commands.empty())
        return false;

    for (const auto& command : scene->commands)
        (void)drawCsdDrawableCommand(graphics, canvas, renderer, command);

    return true;
}

struct CsdReferenceViewerSceneStats
{
    std::string layoutFileName;
    std::string sceneName;
    std::string timelineName;
    int timelineFrame = 0;
    int timelineFrameCount = 0;
    bool timelineResolved = false;
    std::size_t commandCount = 0;
    std::size_t drawnCommandCount = 0;
    std::size_t sampledTrackCount = 0;
    std::size_t timelineTrackCount = 0;
    std::size_t structuralCommandCount = 0;
    std::size_t sourceFreeStructuralCommandCount = 0;
    std::size_t textureCount = 0;
    std::vector<std::string> textureNames;
};

struct CsdReferenceViewerStats
{
    std::size_t sceneCount = 0;
    std::size_t commandCount = 0;
    std::size_t drawnCommandCount = 0;
    std::size_t sampledTrackCount = 0;
    std::size_t timelineResolvedCount = 0;
    std::size_t timelineTrackCount = 0;
    std::size_t structuralCommandCount = 0;
    std::size_t sourceFreeStructuralCommandCount = 0;
    std::size_t textureCount = 0;
    std::vector<CsdReferenceViewerSceneStats> scenes;
};

void appendUniqueTextureName(std::vector<std::string>& names, std::string_view textureName)
{
    if (textureName.empty())
        return;
    if (std::find(names.begin(), names.end(), textureName) == names.end())
        names.emplace_back(textureName);
}

[[nodiscard]] std::vector<CsdDrawableCommand> renderCsdReferenceViewerCommands(
    const CsdPipelineTemplateBinding& sceneBinding,
    const CsdDrawableScene& scene,
    CsdReferenceViewerSceneStats& sceneStats,
    std::optional<int> sampleFrameOverride)
{
    const CsdPipelineTemplateBinding sceneLocalTimelineBinding{
        sceneBinding.templateId,
        sceneBinding.layoutFileName,
        sceneBinding.primarySceneName,
        sceneBinding.primarySceneName,
        sceneBinding.timelineAnimationName,
    };
    auto playback = loadCsdTimelinePlayback(sceneLocalTimelineBinding, sampleFrameOverride);
    if (!playback)
        playback = loadTimelinePlaybackForScene(
            sceneBinding.templateId,
            sceneBinding.layoutFileName,
            sceneBinding.primarySceneName,
            sampleFrameOverride);
    if (!playback && sceneBinding.timelineSceneName != sceneBinding.primarySceneName)
        playback = loadCsdTimelinePlayback(sceneBinding, sampleFrameOverride);

    const CsdTimelinePlayback* playbackPtr = playback ? &*playback : nullptr;
    if (playbackPtr)
    {
        sceneStats.timelineResolved = true;
        sceneStats.timelineName = playbackPtr->animationName;
        sceneStats.timelineFrame = playbackPtr->sampleFrame;
        sceneStats.timelineFrameCount = static_cast<int>(std::llround(playbackPtr->frameCount));
        sceneStats.timelineTrackCount = static_cast<std::size_t>(playbackPtr->trackCount);
    }

    std::size_t sampledTrackCount = 0;
    auto commands = timelineSampledCommands(scene, playbackPtr, sampledTrackCount);
    sceneStats.sampledTrackCount = sampledTrackCount;
    for (const auto& command : commands)
    {
        if (!command.sourceFreeStructural)
            continue;
        ++sceneStats.structuralCommandCount;
        ++sceneStats.sourceFreeStructuralCommandCount;
    }

    return commands;
}

[[nodiscard]] CsdReferenceViewerStats renderCsdReferenceViewerLane(
    Gdiplus::Graphics& graphics,
    const Gdiplus::RectF& canvas,
    SwardSuUiAssetRenderer& renderer,
    const CsdReferenceViewerLane& lane)
{
    CsdReferenceViewerStats stats;
    stats.sceneCount = lane.scenes.size();
    std::vector<std::string> laneTextures;

    for (std::size_t sceneIndex = 0; sceneIndex < lane.scenes.size(); ++sceneIndex)
    {
        const auto& sceneBinding = lane.scenes[sceneIndex];
        CsdReferenceViewerSceneStats sceneStats;
        sceneStats.layoutFileName = std::string(sceneBinding.layoutFileName);
        sceneStats.sceneName = std::string(sceneBinding.primarySceneName);

        const auto* scene = cachedCsdDrawableScene(sceneBinding);
        if (scene)
        {
            std::vector<std::string> sceneTextures;
            const auto commands = renderCsdReferenceViewerCommands(
                sceneBinding,
                *scene,
                sceneStats,
                lane.uiOraclePlaybackClock == "ui-oracle-runtime-frame"
                    ? std::optional<int>(lane.uiOracleRuntimeFrame)
                    : std::nullopt);
            sceneStats.commandCount = commands.size();
            for (const auto& command : commands)
            {
                appendUniqueTextureName(sceneTextures, command.textureName);
                appendUniqueTextureName(laneTextures, command.textureName);
                if (drawCsdDrawableCommand(graphics, canvas, renderer, command))
                    ++sceneStats.drawnCommandCount;
            }
            sceneStats.textureCount = sceneTextures.size();
            sceneStats.textureNames = std::move(sceneTextures);
        }

        stats.commandCount += sceneStats.commandCount;
        stats.drawnCommandCount += sceneStats.drawnCommandCount;
        stats.sampledTrackCount += sceneStats.sampledTrackCount;
        stats.timelineTrackCount += sceneStats.timelineTrackCount;
        stats.structuralCommandCount += sceneStats.structuralCommandCount;
        stats.sourceFreeStructuralCommandCount += sceneStats.sourceFreeStructuralCommandCount;
        if (sceneStats.timelineResolved)
            ++stats.timelineResolvedCount;
        stats.scenes.push_back(std::move(sceneStats));
    }

    stats.textureCount = laneTextures.size();
    return stats;
}

void renderCsdReferenceViewerOverlay(
    Gdiplus::Graphics& graphics,
    const Gdiplus::RectF& canvas,
    const CsdReferenceViewerLane& lane,
    const CsdReferenceViewerStats& stats)
{
    const auto panel = designRectToCanvas(canvas, 20, 20, 780, 116);
    Gdiplus::SolidBrush panelFill(Gdiplus::Color(172, 4, 8, 12));
    Gdiplus::Pen panelEdge(Gdiplus::Color(210, 120, 230, 140), std::max(1.0F, 2.0F * (canvas.Width / static_cast<float>(kDesignWidth))));
    graphics.FillRectangle(&panelFill, panel);
    graphics.DrawRectangle(&panelEdge, panel);

    const std::string_view firstLayout = lane.scenes.empty() ? std::string_view("none") : lane.scenes[0].layoutFileName;
    std::ostringstream title;
    title
        << "phase142-tracked-policy-playback | lane=" << lane.laneId
        << " | screen=" << lane.rendererScreenId;

    std::ostringstream source;
    source
        << "layout=" << firstLayout
        << ":event=" << lane.requiredEventId
        << ":timeline=" << lane.timelineBandId
        << " -> " << lane.timelineEventLabel
        << ":policy=" << lane.policySource;

    std::ostringstream counts;
    counts
        << "compact-reference-status scenes=" << stats.sceneCount
        << " commands=" << stats.commandCount
        << " drawn=" << stats.drawnCommandCount
        << " sampled=" << stats.sampledTrackCount
        << " structural=" << stats.structuralCommandCount
        << " textures=" << stats.textureCount
        << " no-template-card=1";

    std::ostringstream oracle;
    oracle
        << "playback_clock=" << lane.uiOraclePlaybackClock
        << ":runtime_frame=" << lane.uiOracleRuntimeFrame
        << ":sample_frame=" << lane.uiOraclePlaybackFrame
        << ":timeline_frame_source=" << lane.uiOracleFrameSource
        << ":oracle=" << lane.uiOracleSource;

    drawOutlinedText(graphics, canvas, title.str(), 34, 30, 16, Gdiplus::Color(255, 245, 250, 255), Gdiplus::Color(255, 0, 0, 0));
    drawOutlinedText(graphics, canvas, source.str(), 34, 54, 14, Gdiplus::Color(255, 220, 255, 205), Gdiplus::Color(255, 0, 0, 0));
    drawOutlinedText(graphics, canvas, counts.str(), 34, 78, 14, Gdiplus::Color(255, 224, 236, 255), Gdiplus::Color(255, 0, 0, 0));
    drawOutlinedText(graphics, canvas, oracle.str(), 34, 100, 14, Gdiplus::Color(255, 255, 224, 210), Gdiplus::Color(255, 0, 0, 0));
}

[[nodiscard]] std::unique_ptr<Gdiplus::Bitmap> renderCsdReferenceViewerBitmap(
    const CsdReferenceViewerLane& lane,
    bool includeOperatorOverlay,
    CsdReferenceViewerStats& stats)
{
    auto bitmap = std::make_unique<Gdiplus::Bitmap>(kDesignWidth, kDesignHeight, PixelFormat32bppARGB);
    if (!bitmap || bitmap->GetLastStatus() != Gdiplus::Ok)
        return nullptr;

    Gdiplus::Graphics graphics(bitmap.get());
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
    Gdiplus::SolidBrush clearBrush(Gdiplus::Color(255, 0, 0, 0));
    graphics.FillRectangle(&clearBrush, 0, 0, kDesignWidth, kDesignHeight);

    SwardSuUiAssetRenderer renderer;
    const Gdiplus::RectF canvas(0.0F, 0.0F, static_cast<Gdiplus::REAL>(kDesignWidth), static_cast<Gdiplus::REAL>(kDesignHeight));
    stats = renderCsdReferenceViewerLane(graphics, canvas, renderer, lane);
    if (includeOperatorOverlay)
        renderCsdReferenceViewerOverlay(graphics, canvas, lane, stats);
    return bitmap;
}

[[nodiscard]] bool isSonicHudReferenceViewerBinding(const SgfxTemplateRenderBinding& binding)
{
    return binding.templateId == "sonic-hud" || binding.templateId == "tutorial";
}

[[nodiscard]] std::string sonicHudReferenceLocalSceneName(std::string_view sceneName)
{
    const auto slash = sceneName.find_last_of('/');
    return slash == std::string_view::npos
        ? std::string(sceneName)
        : std::string(sceneName.substr(slash + 1));
}

[[nodiscard]] CsdPipelineTemplateBinding sonicHudReferenceCsdBindingForPolicy(
    const SonicHudScenePolicy& policy,
    const std::string& localSceneName)
{
    return {
        "sonic-hud",
        "ui_playscreen.yncp",
        localSceneName,
        localSceneName,
        policy.timeline.animationName,
    };
}

[[nodiscard]] std::optional<CsdDrawableScene> loadSonicHudReferencePolicyScene(
    const SonicHudScenePolicy& policy,
    std::string& localSceneName)
{
    localSceneName = sonicHudReferenceLocalSceneName(policy.sceneName);
    const auto binding = sonicHudReferenceCsdBindingForPolicy(policy, localSceneName);
    return loadCsdDrawableScene(binding);
}

[[nodiscard]] bool shouldRenderSonicHudReferencePolicy(
    const SonicHudScenePolicy& policy,
    const SgfxTemplateRenderBinding* binding)
{
    if (binding && binding->templateId == "tutorial")
        return true;
    return policy.activationEvent == "stage-hud-ready";
}

[[nodiscard]] std::size_t renderSonicHudReferencePolicyScene(
    Gdiplus::Graphics& graphics,
    const Gdiplus::RectF& canvas,
    SwardSuUiAssetRenderer& renderer,
    const SonicHudScenePolicy& policy)
{
    std::string localSceneName;
    const auto scene = loadSonicHudReferencePolicyScene(policy, localSceneName);
    if (!scene)
        return 0;

    std::size_t drawnCommands = 0;
    const std::size_t commandLimit = policy.drawableLayerCount <= 0
        ? 0
        : std::min<std::size_t>(scene->commands.size(), static_cast<std::size_t>(policy.drawableLayerCount));
    for (std::size_t index = 0; index < commandLimit; ++index)
    {
        if (drawCsdDrawableCommand(graphics, canvas, renderer, scene->commands[index]))
            ++drawnCommands;
    }

    return drawnCommands;
}

struct SonicHudReferenceViewerStats
{
    int sceneCount = 0;
    int activeSceneCount = 0;
    int drawableLayerCount = 0;
    int drawnCommandCount = 0;
};

[[nodiscard]] SonicHudReferenceViewerStats renderSonicHudReferencePolicyStack(
    Gdiplus::Graphics& graphics,
    const Gdiplus::RectF& canvas,
    SwardSuUiAssetRenderer& renderer,
    const SgfxTemplateRenderBinding* binding)
{
    SonicHudReferenceViewerStats stats;
    const auto& policies = sonicHudScenePolicies();
    stats.sceneCount = static_cast<int>(policies.size());

    for (const auto& policy : policies)
    {
        stats.drawableLayerCount += policy.drawableLayerCount;
        if (!shouldRenderSonicHudReferencePolicy(policy, binding))
            continue;

        ++stats.activeSceneCount;
        stats.drawnCommandCount += static_cast<int>(renderSonicHudReferencePolicyScene(graphics, canvas, renderer, policy));
    }

    return stats;
}

void renderSonicHudReferenceViewerOverlay(
    Gdiplus::Graphics& graphics,
    const Gdiplus::RectF& canvas,
    const SgfxTemplateRenderBinding* binding,
    const SonicHudReferenceViewerStats& stats)
{
    const auto& owner = sonicHudOwnerReference();
    const auto panel = designRectToCanvas(canvas, 20, 20, 660, 82);
    Gdiplus::SolidBrush panelFill(Gdiplus::Color(172, 4, 8, 12));
    Gdiplus::Pen panelEdge(Gdiplus::Color(210, 80, 210, 255), std::max(1.0F, 2.0F * (canvas.Width / static_cast<float>(kDesignWidth))));
    graphics.FillRectangle(&panelFill, panel);
    graphics.DrawRectangle(&panelEdge, panel);

    std::ostringstream title;
    title
        << "ui_playscreen HUD policy | phase137-ui_playscreen-policy"
        << " | active=" << (binding ? binding->templateId : std::string_view("stage"));

    std::ostringstream ownerLine;
    ownerLine
        << "owner=" << owner.ownerType
        << ":hook=" << owner.ownerHook
        << ":project=" << owner.projectName
        << ":ready=" << owner.readyEvent;

    std::ostringstream sceneLine;
    sceneLine
        << "compact-reference-status scenes=" << stats.sceneCount
        << " active=" << stats.activeSceneCount
        << " drawable_layers=" << stats.drawableLayerCount
        << " drawn_commands=" << stats.drawnCommandCount;

    drawOutlinedText(graphics, canvas, title.str(), 34, 30, 16, Gdiplus::Color(255, 245, 250, 255), Gdiplus::Color(255, 0, 0, 0));
    drawOutlinedText(graphics, canvas, ownerLine.str(), 34, 54, 14, Gdiplus::Color(255, 210, 236, 255), Gdiplus::Color(255, 0, 0, 0));
    drawOutlinedText(graphics, canvas, sceneLine.str(), 34, 76, 14, Gdiplus::Color(255, 220, 255, 205), Gdiplus::Color(255, 0, 0, 0));
}

void renderCsdPipelineEvidenceOverlay(
    Gdiplus::Graphics& graphics,
    const Gdiplus::RectF& canvas,
    const SgfxTemplateRenderBinding& sgfxBinding)
{
    const auto* csdBinding = findCsdPipelineTemplateBinding(sgfxBinding.templateId);
    if (!csdBinding)
        return;

    const auto* evidence = cachedCsdPipelineEvidence(csdBinding->layoutFileName);
    const auto* scene = evidence ? findCsdPipelineScene(*evidence, csdBinding->primarySceneName) : nullptr;
    const auto* timeline = evidence ? findCsdPipelineTimelineHook(*evidence, csdBinding->timelineSceneName, csdBinding->timelineAnimationName) : nullptr;
    const auto manifest = findRuntimeEvidenceManifestForTarget(csdBinding->templateId);

    const auto panel = designRectToCanvas(canvas, 24, 226, 720, 142);
    Gdiplus::SolidBrush panelFill(Gdiplus::Color(205, 12, 8, 22));
    Gdiplus::Pen panelEdge(Gdiplus::Color(230, 170, 230, 120), std::max(1.0F, 2.0F * (canvas.Width / static_cast<float>(kDesignWidth))));
    graphics.FillRectangle(&panelFill, panel);
    graphics.DrawRectangle(&panelEdge, panel);

    std::ostringstream pipeline;
    pipeline
        << "csd_pipeline="
        << csdBinding->templateId
        << ":layout="
        << csdBinding->layoutFileName
        << ":scene="
        << csdBinding->primarySceneName
        << ":casts="
        << (scene ? scene->castCount : 0)
        << ":subimages="
        << (scene ? scene->subimageCount : 0);

    std::ostringstream timelineText;
    timelineText
        << "timeline="
        << (timeline ? timeline->sceneName : std::string(csdBinding->timelineSceneName))
        << "/"
        << (timeline ? timeline->animationName : std::string(csdBinding->timelineAnimationName))
        << "/"
        << (timeline ? formatCsdNumber(timeline->frameCount) : "0")
        << "/"
        << (timeline ? formatCsdNumber(timeline->timelineSeconds) : "0");

    std::ostringstream map;
    map
        << "sgfx_element_map="
        << csdBinding->templateId
        << ":scene="
        << csdBinding->primarySceneName
        << ":slot="
        << (sgfxBinding.slotCount > 0 ? sgfxBinding.slots[0].slotName : std::string_view("none"))
        << ":texture="
        << (sgfxBinding.slotCount > 0 ? sgfxBinding.slots[0].textureName : std::string_view("none"));

    std::ostringstream comparison;
    comparison
        << "runtime_evidence_compare="
        << csdBinding->templateId
        << ":target="
        << csdBinding->templateId
        << ":event="
        << sgfxBinding.requiredEventId
        << ":manifest="
        << (manifest ? "found" : "missing");

    drawOutlinedText(graphics, canvas, pipeline.str(), 42, 242, 16, Gdiplus::Color(255, 248, 255, 238), Gdiplus::Color(255, 0, 0, 0));
    drawOutlinedText(graphics, canvas, timelineText.str(), 42, 270, 15, Gdiplus::Color(255, 222, 255, 205), Gdiplus::Color(255, 0, 0, 0));
    drawOutlinedText(graphics, canvas, map.str(), 42, 298, 15, Gdiplus::Color(255, 224, 236, 255), Gdiplus::Color(255, 0, 0, 0));
    drawOutlinedText(graphics, canvas, comparison.str(), 42, 326, 15, Gdiplus::Color(255, 255, 224, 210), Gdiplus::Color(255, 0, 0, 0));
}

void renderCleanScreen(HWND hwnd, HDC dc, SwardSuUiAssetRenderer& renderer)
{
    RECT client{};
    GetClientRect(hwnd, &client);
    RECT renderClient = client;
    renderClient.top = std::min(renderClient.bottom, renderClient.top + kRendererChromeHeight);

    Gdiplus::Graphics graphics(dc);
    graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);

    Gdiplus::SolidBrush windowBrush(Gdiplus::Color(255, 0, 0, 0));
    graphics.FillRectangle(&windowBrush, 0, 0, client.right - client.left, client.bottom - client.top);

    Gdiplus::SolidBrush chromeBrush(Gdiplus::Color(255, 236, 236, 236));
    graphics.FillRectangle(&chromeBrush, 0, 0, client.right - client.left, kRendererChromeHeight);

    const auto canvas = canvasRectForClient(renderClient);
    const auto& screen = renderer.selectedScreen();
    Gdiplus::SolidBrush canvasBrush(screen.background);
    graphics.FillRectangle(&canvasBrush, canvas);

    const auto* binding = renderer.selectedSgfxTemplateBinding();
    const bool useSonicHudReferenceViewer =
        screen.kind == RendererScreenKind::SonicHudReferencePipeline
        || (binding && isSonicHudReferenceViewerBinding(*binding));
    std::optional<CsdReferenceViewerLane> referenceLane;
    if (!useSonicHudReferenceViewer)
    {
        referenceLane = binding
            ? findCsdReferenceViewerLaneById(binding->templateId)
            : findCsdReferenceViewerLaneByScreenId(screen.id);
    }
    const auto sonicHudReferenceStats = useSonicHudReferenceViewer
        ? std::optional<SonicHudReferenceViewerStats>(renderSonicHudReferencePolicyStack(graphics, canvas, renderer, binding))
        : std::nullopt;
    const auto referenceLaneStats = referenceLane
        ? std::optional<CsdReferenceViewerStats>(renderCsdReferenceViewerLane(graphics, canvas, renderer, *referenceLane))
        : std::nullopt;
    const bool renderedCsdDrawableScene = !useSonicHudReferenceViewer && !referenceLane && binding
        ? renderCsdDrawableScene(graphics, canvas, renderer, *binding)
        : false;

    if (useSonicHudReferenceViewer)
    {
        // The Sonic HUD lane is now driven by the Phase 137 ui_playscreen scene policy stack.
    }
    else if (referenceLane)
    {
        // Phase 142 viewer lanes are driven by the tracked frontend_screen_reference policy.
    }
    else if (renderedCsdDrawableScene)
    {
        // The template lane is CSD draw-command driven when local scene evidence is available.
    }
    else if (screen.kind == RendererScreenKind::TitleLoopReconstruction)
    {
        renderTitleLoopReconstructionScreen(graphics, canvas, renderer);
    }
    else if (screen.kind == RendererScreenKind::SonicHudReconstruction)
    {
        renderSonicHudReconstructionScreen(graphics, canvas);
    }
    else if (screen.kind == RendererScreenKind::AtlasGallery)
    {
        renderAtlasGalleryScreen(graphics, canvas, renderer);
    }
    else
    {
        for (std::size_t index = 0; index < screen.castCount; ++index)
        {
            (void)drawRenderCastTexture(graphics, canvas, renderer, screen.casts[index]);
        }
    }

    if (useSonicHudReferenceViewer && sonicHudReferenceStats)
    {
        renderSonicHudReferenceViewerOverlay(graphics, canvas, binding, *sonicHudReferenceStats);
    }
    else if (referenceLane && referenceLaneStats)
    {
        renderCsdReferenceViewerOverlay(graphics, canvas, *referenceLane, *referenceLaneStats);
    }
    else if (binding)
    {
        renderSgfxTemplatePlaceholderScreen(graphics, canvas, renderer, *binding);
        renderCsdPipelineEvidenceOverlay(graphics, canvas, *binding);
    }
}

[[nodiscard]] LRESULT CALLBACK rendererWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_NCCREATE)
    {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
    }

    auto* renderer = reinterpret_cast<SwardSuUiAssetRenderer*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (message)
    {
    case WM_CREATE:
        createRendererControls(hwnd);
        if (renderer)
            updateRendererStatus(hwnd, *renderer);
        return 0;
    case WM_SIZE:
        layoutRendererControls(hwnd);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_COMMAND:
        if (!renderer)
            break;
        if (LOWORD(wParam) == kPrevButtonId)
        {
            renderer->selectPrevious();
            updateRendererStatus(hwnd, *renderer);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (LOWORD(wParam) == kNextButtonId)
        {
            renderer->selectNext();
            updateRendererStatus(hwnd, *renderer);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (LOWORD(wParam) == kAtlasPrevButtonId)
        {
            renderer->selectPreviousAtlas();
            updateRendererStatus(hwnd, *renderer);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (LOWORD(wParam) == kAtlasNextButtonId)
        {
            renderer->selectNextAtlas();
            updateRendererStatus(hwnd, *renderer);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        break;
    case WM_KEYDOWN:
        if (!renderer)
            break;
        if (wParam == VK_RIGHT || wParam == VK_SPACE)
        {
            renderer->selectNext();
            updateRendererStatus(hwnd, *renderer);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (wParam == VK_LEFT)
        {
            renderer->selectPrevious();
            updateRendererStatus(hwnd, *renderer);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (wParam == VK_ESCAPE)
        {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    case WM_PAINT:
        if (renderer)
        {
            PAINTSTRUCT paint{};
            HDC dc = BeginPaint(hwnd, &paint);
            renderCleanScreen(hwnd, dc, *renderer);
            EndPaint(hwnd, &paint);
            return 0;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

[[nodiscard]] int runRendererSmoke()
{
    std::size_t castCount = 0;
    std::size_t resolvedTextureCount = 0;
    std::size_t fullScreenCastCount = 0;
    std::vector<std::string> descriptors;

    for (const auto& screen : kRendererScreens)
    {
        castCount += screen.castCount;
        for (std::size_t index = 0; index < screen.castCount; ++index)
        {
            const auto& cast = screen.casts[index];
            const auto image = loadDdsTextureImage(cast.textureName);
            if (screen.id == "LoadingComposite"
                && cast.destinationX == 0
                && cast.destinationY == 0
                && cast.destinationWidth == kDesignWidth
                && cast.destinationHeight == kDesignHeight)
            {
                ++fullScreenCastCount;
            }

            std::ostringstream descriptor;
            descriptor
                << screen.id
                << ":" << cast.sceneName
                << "/" << cast.castName
                << ":" << cast.textureName;

            if (image)
            {
                ++resolvedTextureCount;
                descriptor << ":" << image->format << ":" << image->width << "x" << image->height;
                if (!castSourceFits(cast, *image))
                    descriptor << ":source-out-of-bounds";
            }
            else
            {
                descriptor << ":missing";
            }
            descriptor
                << ":dst=" << cast.destinationX << "," << cast.destinationY << ","
                << cast.destinationWidth << "x" << cast.destinationHeight;
            descriptors.push_back(descriptor.str());
        }
    }

    std::cout
        << "sward_su_ui_asset_renderer smoke ok "
        << "screens=" << kRendererScreens.size()
        << " casts=" << castCount
        << " textures=" << resolvedTextureCount
        << " full_screen_casts=" << fullScreenCastCount
        << '\n';

    for (const auto& descriptor : descriptors)
        std::cout << descriptor << '\n';

    return resolvedTextureCount == castCount ? 0 : 1;
}

[[nodiscard]] int runRendererNavigationSmoke()
{
    SwardSuUiAssetRenderer renderer;
    std::cout
        << "sward_su_ui_asset_renderer navigation smoke ok "
        << "screens=" << renderer.screenCount()
        << " controls=5"
        << " first=" << kRendererScreens.front().id
        << " last=" << kRendererScreens.back().id
        << " label=" << renderer.selectedScreenIndexText()
        << '\n';

    for (const auto& screen : kRendererScreens)
    {
        std::cout
            << "screen=" << screen.id
            << ":casts=" << screen.castCount
            << ":contract=" << screen.contractFileName
            << '\n';
    }

    return 0;
}

[[nodiscard]] std::string findAtlasSheetFileName(const std::vector<std::filesystem::path>& sheets, std::string_view fileName)
{
    const auto found = std::find_if(
        sheets.begin(),
        sheets.end(),
        [fileName](const auto& path)
        {
            return path.filename().string() == fileName;
        });
    return found == sheets.end() ? "missing" : found->filename().string();
}

[[nodiscard]] int runRendererAtlasGallerySmoke()
{
    const auto sheets = discoverAtlasSheetPaths();
    const auto first = sheets.empty() ? std::string("none") : sheets.front().filename().string();
    const auto loading = findAtlasSheetFileName(sheets, "loading__ui_loading.png");
    const auto mainMenu = findAtlasSheetFileName(sheets, "mainmenu__ui_mainmenu.png");
    const auto status = findAtlasSheetFileName(sheets, "systemcommoncore__ui_status.png");

    std::cout
        << "sward_su_ui_asset_renderer atlas gallery smoke ok "
        << "sheets=" << sheets.size()
        << " first=" << first
        << " loading=" << loading
        << " mainmenu=" << mainMenu
        << " status=" << status
        << '\n';

    return sheets.empty() || loading == "missing" || mainMenu == "missing" || status == "missing" ? 1 : 0;
}

[[nodiscard]] int runRendererTitleScreenSmoke()
{
    const auto& screen = kRendererScreens.front();
    const auto movieFramePath = findTitleMoviePreviewFramePath();
    const auto titleLogoPreviewPath = findTitleLogoPreviewPath();
    const bool titleLogoPreviewLoads = titleLogoPreviewPath && gdiplusBitmapLoads(*titleLogoPreviewPath);
    const auto titleLogoPath = textureSourcePathForFileName("OPmovie_titlelogo_EN.decompressed.dds");
    std::size_t resolvedCastCount = 0;
    std::size_t inBoundsCastCount = 0;
    std::vector<std::string> descriptors;

    for (std::size_t index = 0; index < screen.castCount; ++index)
    {
        const auto& cast = screen.casts[index];
        const auto image = loadDdsTextureImage(cast.textureName);
        const bool fits = image && castSourceFits(cast, *image);
        if (image)
            ++resolvedCastCount;
        if (fits)
            ++inBoundsCastCount;

        std::ostringstream descriptor;
        descriptor
            << cast.castName
            << ":" << cast.textureName
            << ":src=" << cast.sourceX << "," << cast.sourceY << ","
            << cast.sourceWidth << "x" << cast.sourceHeight
            << ":dst=" << cast.destinationX << "," << cast.destinationY << ","
            << cast.destinationWidth << "x" << cast.destinationHeight;
        if (image)
            descriptor << ":" << image->format << ":" << image->width << "x" << image->height;
        else
            descriptor << ":missing";
        if (image && !fits)
            descriptor << ":source-out-of-bounds";
        descriptors.push_back(descriptor.str());
    }

    std::cout
        << "sward_su_ui_asset_renderer title screen smoke ok "
        << "first=" << screen.id
        << " source=evmo_title_loop.sfd"
        << " contract=" << screen.contractFileName
        << " movie_frame=" << (movieFramePath ? "exists" : "missing")
        << " title_logo_preview=" << (titleLogoPreviewPath ? "exists" : "missing")
        << " title_logo_preview_bitmap=" << (titleLogoPreviewLoads ? "loads" : "not_loaded")
        << " title_logo=" << (titleLogoPath ? "exists" : "missing")
        << " casts=" << screen.castCount
        << " resolved=" << resolvedCastCount
        << " in_bounds=" << inBoundsCastCount
        << " scenes=ui_title/bg/bg,mm_title_intro,CTitleStateIntro::Update"
        << '\n';

    for (const auto& descriptor : descriptors)
        std::cout << descriptor << '\n';

    return screen.id == "TitleLoopReconstruction"
        && movieFramePath
        && titleLogoPreviewPath
        && titleLogoPreviewLoads
        && resolvedCastCount == screen.castCount
        && inBoundsCastCount == screen.castCount
        ? 0
        : 1;
}

[[nodiscard]] int runRendererReconstructedScreenSmoke()
{
    const auto* foundScreen = rendererScreenById("SonicHudReconstruction");
    if (!foundScreen)
        return 1;
    const auto& screen = *foundScreen;
    std::size_t resolvedCastCount = 0;
    std::size_t inBoundsCastCount = 0;
    std::vector<std::string> descriptors;

    for (std::size_t index = 0; index < screen.castCount; ++index)
    {
        const auto& cast = screen.casts[index];
        const auto image = loadDdsTextureImage(cast.textureName);
        const bool fits = image && castSourceFits(cast, *image);
        if (image)
            ++resolvedCastCount;
        if (fits)
            ++inBoundsCastCount;

        std::ostringstream descriptor;
        descriptor
            << cast.castName
            << ":" << cast.textureName
            << ":src=" << cast.sourceX << "," << cast.sourceY << ","
            << cast.sourceWidth << "x" << cast.sourceHeight
            << ":dst=" << cast.destinationX << "," << cast.destinationY << ","
            << cast.destinationWidth << "x" << cast.destinationHeight;
        if (image)
            descriptor << ":" << image->format << ":" << image->width << "x" << image->height;
        else
            descriptor << ":missing";
        if (image && !fits)
            descriptor << ":source-out-of-bounds";
        descriptors.push_back(descriptor.str());
    }

    const auto source = screen.castCount == 0 ? std::string_view("none") : screen.casts[0].sceneName;
    std::cout
        << "sward_su_ui_asset_renderer reconstructed screen smoke ok "
        << "screen=" << screen.id
        << " source=" << source
        << " contract=" << screen.contractFileName
        << " casts=" << screen.castCount
        << " resolved=" << resolvedCastCount
        << " in_bounds=" << inBoundsCastCount
        << '\n';

    for (const auto& descriptor : descriptors)
        std::cout << descriptor << '\n';

    return screen.id == "SonicHudReconstruction"
        && resolvedCastCount == screen.castCount
        && inBoundsCastCount == screen.castCount
        ? 0
        : 1;
}

[[nodiscard]] int runRendererSonicHudReferencePolicySmoke()
{
    const auto* foundScreen = rendererScreenById("SonicHudReconstruction");
    if (!foundScreen)
        return 1;

    const auto& owner = sonicHudOwnerReference();
    const auto& policies = sonicHudScenePolicies();
    int drawableLayers = 0;
    int resolvedScenes = 0;
    bool failed = false;
    std::vector<std::string> descriptors;

    for (const auto& policy : policies)
    {
        drawableLayers += policy.drawableLayerCount;
        std::string localSceneName;
        const auto scene = loadSonicHudReferencePolicyScene(policy, localSceneName);
        const int commandCount = scene
            ? static_cast<int>(std::min<std::size_t>(
                scene->commands.size(),
                policy.drawableLayerCount <= 0 ? 0 : static_cast<std::size_t>(policy.drawableLayerCount)))
            : 0;
        if (scene)
            ++resolvedScenes;
        if (!scene || commandCount != policy.drawableLayerCount)
            failed = true;

        const auto sample = sampleSonicHudTimeline(policy, policy.timeline.sampleFrame);
        std::ostringstream descriptor;
        descriptor
            << "render_scene=" << policy.sceneName
            << ":local_scene=" << localSceneName
            << ":slot=" << policy.sgfxSlot
            << ":order=" << policy.renderOrder
            << ":commands=" << commandCount
            << ":timeline=" << sample.animationName
            << "@" << sample.frame
            << "/" << sample.frameCount;
        descriptors.push_back(descriptor.str());
    }

    std::cout
        << "sward_su_ui_asset_renderer sonic hud reference viewer smoke ok "
        << "screen=" << foundScreen->id
        << ":mode=phase137-ui_playscreen-policy"
        << " owner=" << owner.ownerType
        << ":hook=" << owner.ownerHook
        << ":project=" << owner.projectName
        << " scenes=" << policies.size()
        << ":drawable_layers=" << drawableLayers
        << ":resolved_scenes=" << resolvedScenes
        << '\n';

    for (const auto& descriptor : descriptors)
        std::cout << descriptor << '\n';

    std::cout << "viewer_overlay=compact-reference-status:no-template-card=1\n";
    return failed || resolvedScenes != static_cast<int>(policies.size()) ? 1 : 0;
}

[[nodiscard]] int runRendererReferenceLanesSmoke()
{
    bool failed = false;
    std::vector<std::pair<std::string, CsdReferenceViewerStats>> laneStats;
    const auto lanes = buildReferenceViewerLanesFromTrackedPolicy();

    Gdiplus::GdiplusStartupInput gdiplusInput{};
    ULONG_PTR gdiplusToken = 0;
    if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusInput, nullptr) != Gdiplus::Ok)
        return 1;

    for (const auto& lane : lanes)
    {
        CsdReferenceViewerStats stats;
        const auto bitmap = renderCsdReferenceViewerBitmap(lane, false, stats);
        if (!bitmap || stats.commandCount == 0)
            failed = true;
        laneStats.emplace_back(std::string(lane.laneId), std::move(stats));
    }

    std::cout
        << "sward_su_ui_asset_renderer reference lanes smoke ok "
        << "mode=phase142-tracked-policy-playback"
        << " reference_policy_source=frontend_screen_reference"
        << " runtime_alignment_source=ui_lab_live_state"
        << " lanes=" << lanes.size()
        << '\n';

    for (std::size_t laneIndex = 0; laneIndex < lanes.size(); ++laneIndex)
    {
        const auto& lane = lanes[laneIndex];
        const auto& stats = laneStats[laneIndex].second;
        const auto layout = lane.scenes.empty() ? std::string_view("none") : lane.scenes[0].layoutFileName;
        std::cout
            << "reference_lane=" << lane.laneId
            << ":screen=" << lane.rendererScreenId
            << ":layout=" << layout
            << ":scenes=" << stats.sceneCount
            << ":commands=" << stats.commandCount
            << ":drawn=" << stats.drawnCommandCount
            << ":textures=" << stats.textureCount
            << ":timeline_resolved=" << stats.timelineResolvedCount
            << ":timeline_tracks=" << stats.timelineTrackCount
            << ":sampled_tracks=" << stats.sampledTrackCount
            << ":structural=" << stats.structuralCommandCount
            << ":source_free=" << stats.sourceFreeStructuralCommandCount
            << ":event=" << lane.requiredEventId
            << ":policy_source=" << lane.policySource
            << ":overlay=compact-reference-status:no-template-card=1"
            << '\n';
        std::cout << "runtime_alignment=" << lane.runtimeAlignment << '\n';
        std::cout << "alignment_lane=" << lane.runtimeAlignmentEvidence << '\n';
        std::cout << "live_state_path=" << lane.laneId << ":" << lane.runtimeAlignmentLiveStatePath << '\n';
        std::cout << "alignment_field_status=" << lane.runtimeAlignmentFieldStatus << '\n';
        std::cout << "material_semantics=" << lane.materialSemantics << '\n';

        for (const auto& scene : stats.scenes)
        {
            std::cout
                << "reference_scene=" << lane.laneId
                << ":" << scene.sceneName
                << ":commands=" << scene.commandCount
                << ":drawn=" << scene.drawnCommandCount
                << ":textures=" << scene.textureCount
                << ":timeline=" << (scene.timelineResolved ? scene.timelineName : std::string("unresolved"))
                << "@" << scene.timelineFrame
                << "/" << scene.timelineFrameCount
                << ":sampled_tracks=" << scene.sampledTrackCount
                << ":structural=" << scene.structuralCommandCount
                << ":source_free=" << scene.sourceFreeStructuralCommandCount
                << ":texture_names=" << joinStrings(scene.textureNames)
                << '\n';
        }
    }

    std::cout << "reference_overlay=compact-reference-status:no-template-card=1\n";
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return failed ? 1 : 0;
}

[[nodiscard]] int runRendererRuntimeAlignmentSmoke()
{
    const auto lanes = buildReferenceViewerLanesFromTrackedPolicy();
    const bool anyLive = std::any_of(
        lanes.begin(),
        lanes.end(),
        [](const CsdReferenceViewerLane& lane)
        {
            return lane.runtimeAlignmentSource == "ui_lab_live_state";
        });

    std::cout
        << "sward_su_ui_asset_renderer runtime alignment smoke ok "
        << "mode=phase143-live-state-alignment"
        << " runtime_alignment_source=" << (anyLive ? "ui_lab_live_state" : "frontend_screen_reference")
        << " lanes=" << lanes.size()
        << '\n';

    for (const auto& lane : lanes)
    {
        std::cout << "runtime_alignment=" << lane.runtimeAlignment << '\n';
        std::cout << "alignment_lane=" << lane.runtimeAlignmentEvidence << '\n';
        std::cout << "live_state_path=" << lane.laneId << ":" << lane.runtimeAlignmentLiveStatePath << '\n';
        std::cout << "alignment_field_status=" << lane.runtimeAlignmentFieldStatus << '\n';
    }

    return lanes.empty() ? 1 : 0;
}

[[nodiscard]] int runRendererLiveBridgeAlignmentSmoke()
{
    constexpr std::string_view directProbeToken = "runtime_alignment_probe=direct-live-bridge";
    constexpr std::string_view fallbackProbeToken = "runtime_alignment_probe=snapshot-fallback";
    const auto lanes = buildReferenceViewerLanesFromTrackedPolicy();
    const bool anyDirect = std::any_of(
        lanes.begin(),
        lanes.end(),
        [](const CsdReferenceViewerLane& lane)
        {
            return lane.runtimeAlignmentProbe == "direct-live-bridge";
        });

    std::cout
        << "sward_su_ui_asset_renderer live bridge alignment smoke ok "
        << "mode=phase144-live-bridge-alignment"
        << " " << (anyDirect ? directProbeToken : fallbackProbeToken)
        << " lanes=" << lanes.size()
        << '\n';

    for (const auto& lane : lanes)
    {
        std::cout
            << "bridge_probe=" << lane.laneId
            << ":pipe=" << (lane.runtimeAlignmentBridgePipe.empty() ? std::string("sward_ui_lab_live") : lane.runtimeAlignmentBridgePipe)
            << ":connected=" << (lane.runtimeAlignmentBridgeConnected ? 1 : 0)
            << ":fallback=" << (lane.runtimeAlignmentBridgeFallback.empty() ? std::string("none") : lane.runtimeAlignmentBridgeFallback);
        if (!lane.runtimeAlignmentBridgeError.empty())
            std::cout << ":error=" << lane.runtimeAlignmentBridgeError;
        std::cout << '\n';

        std::cout << "runtime_alignment=" << lane.runtimeAlignment << '\n';
        std::cout << "alignment_lane=" << lane.runtimeAlignmentEvidence << '\n';
        std::cout << "live_state_path=" << lane.laneId << ":" << lane.runtimeAlignmentLiveStatePath << '\n';
        std::cout << "alignment_field_status=" << lane.runtimeAlignmentFieldStatus << '\n';
    }

    return lanes.empty() ? 1 : 0;
}

[[nodiscard]] int runRendererUiOracleSmoke()
{
    constexpr std::string_view directProbeToken = "ui_oracle_probe=direct-ui-oracle";
    constexpr std::string_view stateProbeToken = "ui_oracle_probe=state-fallback";
    constexpr std::string_view snapshotProbeToken = "ui_oracle_probe=snapshot-fallback";

    std::vector<std::pair<std::string, FrontendUiOracleEvidence>> oracleEvidence;
    for (const auto& policy : frontendScreenPolicies())
        oracleEvidence.emplace_back(policy.screenId, loadFrontendUiOracleEvidence(policy));

    const bool anyDirect = std::any_of(
        oracleEvidence.begin(),
        oracleEvidence.end(),
        [](const auto& entry)
        {
            return entry.second.probe == "direct-ui-oracle";
        });
    const bool anyState = std::any_of(
        oracleEvidence.begin(),
        oracleEvidence.end(),
        [](const auto& entry)
        {
            return entry.second.probe == "state-fallback";
        });

    std::cout
        << "sward_su_ui_asset_renderer ui oracle smoke ok "
        << "mode=phase145-ui-only-oracle"
        << " " << (anyDirect ? directProbeToken : (anyState ? stateProbeToken : snapshotProbeToken))
        << " lanes=" << oracleEvidence.size()
        << '\n';

    for (const auto& [laneId, evidence] : oracleEvidence)
    {
        std::cout
            << "ui_oracle=" << laneId
            << ":source=" << evidence.source
            << ":project=" << evidence.activeProject
            << ":scenes=" << evidence.sceneCount
            << ":layers=" << evidence.layerCount
            << ":draw_list_status=" << evidence.runtimeDrawListStatus
            << '\n';
        std::cout
            << "active_scenes=" << laneId << ":"
            << (evidence.activeScenes.empty() ? std::string("none") : joinStrings(evidence.activeScenes))
            << '\n';
        std::cout
            << "ui_oracle_bridge=" << laneId
            << ":pipe=" << (evidence.bridgeProbe.pipeName.empty() ? std::string("sward_ui_lab_live") : evidence.bridgeProbe.pipeName)
            << ":connected=" << (evidence.bridgeProbe.connected ? 1 : 0)
            << ":command=" << evidence.bridgeProbe.command;
        if (!evidence.bridgeProbe.error.empty())
            std::cout << ":error=" << evidence.bridgeProbe.error;
        std::cout << '\n';
    }

    return oracleEvidence.empty() ? 1 : 0;
}

[[nodiscard]] int uiOracleTimelineFrameForScene(
    const FrontendUiOraclePlaybackClock& clock,
    const sward::ui_runtime::FrontendScreenScenePolicy& scene)
{
    if (scene.timeline.frameCount <= 0)
        return 0;

    const int runtimeFrame = std::max(0, clock.runtimeFrame);
    return runtimeFrame % scene.timeline.frameCount;
}

[[nodiscard]] int runRendererUiOraclePlaybackSmoke()
{
    constexpr std::string_view directProbeToken = "ui_oracle_playback_probe=direct-ui-oracle";
    constexpr std::string_view stateProbeToken = "ui_oracle_playback_probe=state-fallback";
    constexpr std::string_view snapshotProbeToken = "ui_oracle_playback_probe=snapshot-fallback";
    constexpr std::string_view runtimeClockToken = "playback_clock=ui-oracle-runtime-frame";
    constexpr std::string_view frameSourceToken = "timeline_frame_source=ui-oracle-mod-frame";

    struct PlaybackRecord
    {
        const sward::ui_runtime::FrontendScreenPolicy* policy = nullptr;
        FrontendUiOraclePlaybackClock clock;
    };

    std::vector<PlaybackRecord> records;
    for (const auto& policy : frontendScreenPolicies())
        records.push_back({ &policy, loadFrontendUiOraclePlaybackClock(policy) });

    const bool anyDirect = std::any_of(
        records.begin(),
        records.end(),
        [](const PlaybackRecord& record)
        {
            return record.clock.probe == "direct-ui-oracle";
        });
    const bool anyState = std::any_of(
        records.begin(),
        records.end(),
        [](const PlaybackRecord& record)
        {
            return record.clock.probe == "state-fallback";
        });

    std::cout
        << "sward_su_ui_asset_renderer ui oracle playback smoke ok "
        << "mode=phase146-ui-oracle-playback"
        << " " << (anyDirect ? directProbeToken : (anyState ? stateProbeToken : snapshotProbeToken))
        << " " << runtimeClockToken
        << " " << frameSourceToken
        << " lanes=" << records.size()
        << '\n';

    for (const auto& record : records)
    {
        const auto& policy = *record.policy;
        const auto& clock = record.clock;
        const int firstSceneFrame = policy.scenes.empty()
            ? clock.runtimeFrame
            : uiOracleTimelineFrameForScene(clock, policy.scenes.front());

        std::cout
            << "oracle_playback=" << policy.screenId
            << ":source=" << clock.source
            << ":runtime_frame=" << clock.runtimeFrame
            << ":playback_clock=" << clock.playbackClock
            << ":playback_frame=" << firstSceneFrame
            << ":active_motion=" << clock.activeMotionName
            << '\n';

        for (const auto& scene : policy.scenes)
        {
            const int sceneFrame = uiOracleTimelineFrameForScene(clock, scene);
            const auto sample = sward::ui_runtime::sampleFrontendScreenTimeline(policy, scene, sceneFrame);
            std::cout
                << "oracle_scene_playback=" << policy.screenId
                << ":" << scene.sceneName
                << ":animation=" << sample.animationName
                << ":frame=" << sample.frame
                << "/" << sample.frameCount
                << ":timeline_frame_source=" << clock.frameSource
                << '\n';
        }
    }

    return records.empty() ? 1 : 0;
}

[[nodiscard]] std::string frontendRuntimeDrawableProjectName(
    const sward::ui_runtime::FrontendScreenPolicy& policy,
    std::string_view oracleProject)
{
    if (!oracleProject.empty() && oracleProject.find(".yncp") == std::string_view::npos)
        return std::string(oracleProject);
    if (policy.screenId == "title-menu" || policy.screenId == "title-options")
        return "ui_title";
    if (policy.screenId == "loading")
        return "ui_loading";
    if (policy.screenId == "pause")
        return "ui_pause";

    std::string project(policy.layoutName);
    const auto slash = project.find_last_of("/\\");
    if (slash != std::string::npos)
        project = project.substr(slash + 1);
    const auto dot = project.find_last_of('.');
    if (dot != std::string::npos)
        project = project.substr(0, dot);
    return project;
}

[[nodiscard]] std::string frontendRuntimeDrawableScenePath(
    const FrontendUiOracleEvidence& evidence,
    std::string_view activeProject,
    std::string_view localSceneName)
{
    const std::string suffix = "/" + std::string(localSceneName);
    for (const auto& scenePath : evidence.activeScenes)
    {
        if (scenePath == localSceneName || scenePath.ends_with(suffix))
            return scenePath;
    }

    if (!activeProject.empty())
        return std::string(activeProject) + "/" + std::string(localSceneName);
    return std::string(localSceneName);
}

[[nodiscard]] FrontendRuntimeDrawableOracle buildFrontendRuntimeDrawableOracle(
    const sward::ui_runtime::FrontendScreenPolicy& policy)
{
    FrontendRuntimeDrawableOracle oracle;
    oracle.screenId = policy.screenId;

    const auto clock = loadFrontendUiOraclePlaybackClock(policy);
    oracle.found = clock.found;
    oracle.source = clock.source;
    oracle.probe = clock.probe;
    oracle.activeProject = frontendRuntimeDrawableProjectName(policy, clock.oracle.activeProject);
    oracle.runtimeFrame = clock.runtimeFrame;
    oracle.activeSceneCount = !clock.oracle.activeScenes.empty()
        ? clock.oracle.activeScenes.size()
        : static_cast<std::size_t>(std::max(0, clock.oracle.sceneCount));

    for (const auto& scenePolicy : policy.scenes)
    {
        const int sceneFrame = uiOracleTimelineFrameForScene(clock, scenePolicy);
        const CsdPipelineTemplateBinding sceneBinding{
            policy.screenId,
            policy.layoutName,
            scenePolicy.sceneName,
            scenePolicy.sceneName,
            scenePolicy.timeline.animationName,
        };

        const auto* drawableScene = cachedCsdDrawableScene(sceneBinding);
        auto playback = loadCsdTimelinePlayback(sceneBinding, sceneFrame);
        if (!playback)
            playback = loadTimelinePlaybackForScene(policy.screenId, policy.layoutName, scenePolicy.sceneName, sceneFrame);

        FrontendRuntimeDrawableOracleScene scene;
        scene.runtimeScenePath = frontendRuntimeDrawableScenePath(
            clock.oracle,
            oracle.activeProject,
            scenePolicy.sceneName);
        scene.localSceneName = scenePolicy.sceneName;
        scene.animationName = playback
            ? playback->animationName
            : scenePolicy.timeline.animationName;
        scene.timelineFrame = playback
            ? playback->sampleFrame
            : sceneFrame;
        scene.timelineFrameCount = playback
            ? static_cast<int>(std::llround(playback->frameCount))
            : scenePolicy.timeline.frameCount;

        if (drawableScene)
        {
            std::size_t sampledTrackCount = 0;
            const auto commands = timelineSampledCommands(
                *drawableScene,
                playback ? &*playback : nullptr,
                sampledTrackCount);
            scene.commandCount = commands.size();
            scene.sampledTrackCount = sampledTrackCount;
            for (const auto& command : commands)
            {
                if (!command.hidden && (command.textureResolved || command.sourceFreeStructural))
                    ++scene.drawnCommandCount;
                if (!command.textureName.empty()
                    && std::find(scene.textureNames.begin(), scene.textureNames.end(), command.textureName) == scene.textureNames.end())
                {
                    scene.textureNames.push_back(command.textureName);
                }
            }
            scene.textureCount = scene.textureNames.size();
        }

        oracle.scenes.push_back(std::move(scene));
    }

    return oracle;
}

[[nodiscard]] int runRendererUiDrawableOracleSmoke()
{
    constexpr std::string_view directProbeToken = "ui_drawable_oracle_probe=direct-ui-oracle";
    constexpr std::string_view stateProbeToken = "ui_drawable_oracle_probe=state-fallback";
    constexpr std::string_view snapshotProbeToken = "ui_drawable_oracle_probe=snapshot-fallback";
    constexpr std::string_view runtimeStatusToken = "runtime_drawable_oracle_status=runtime-csd-tree-local-material";
    constexpr std::string_view gpuStatusToken = "gpu_draw_list_status=pending";
    constexpr std::string_view drawableSceneSourceToken = "drawable_scene_source=ui-oracle-active-scenes";

    std::vector<FrontendRuntimeDrawableOracle> records;
    for (const auto& policy : frontendScreenPolicies())
        records.push_back(buildFrontendRuntimeDrawableOracle(policy));

    const bool anyDirect = std::any_of(
        records.begin(),
        records.end(),
        [](const FrontendRuntimeDrawableOracle& record)
        {
            return record.probe == "direct-ui-oracle";
        });
    const bool anyState = std::any_of(
        records.begin(),
        records.end(),
        [](const FrontendRuntimeDrawableOracle& record)
        {
            return record.probe == "state-fallback";
        });

    std::cout
        << "sward_su_ui_asset_renderer ui drawable oracle smoke ok "
        << "mode=phase147-ui-drawable-oracle"
        << " " << (anyDirect ? directProbeToken : (anyState ? stateProbeToken : snapshotProbeToken))
        << " " << runtimeStatusToken
        << " " << gpuStatusToken
        << " " << drawableSceneSourceToken
        << " lanes=" << records.size()
        << '\n';

    for (const auto& record : records)
    {
        const std::size_t commandCount = std::accumulate(
            record.scenes.begin(),
            record.scenes.end(),
            static_cast<std::size_t>(0),
            [](std::size_t total, const FrontendRuntimeDrawableOracleScene& scene)
            {
                return total + scene.commandCount;
            });
        const std::size_t drawnCommandCount = std::accumulate(
            record.scenes.begin(),
            record.scenes.end(),
            static_cast<std::size_t>(0),
            [](std::size_t total, const FrontendRuntimeDrawableOracleScene& scene)
            {
                return total + scene.drawnCommandCount;
            });

        std::cout
            << "drawable_oracle=" << record.screenId
            << ":source=" << record.source
            << ":active_project=" << record.activeProject
            << ":active_scenes=" << record.activeSceneCount
            << ":drawable_scenes=" << record.scenes.size()
            << ":commands=" << commandCount
            << ":drawn_commands=" << drawnCommandCount
            << ":runtime_drawable_oracle_status=" << record.runtimeDrawableOracleStatus
            << ":gpu_draw_list_status=" << record.gpuDrawListStatus
            << '\n';

        for (const auto& scene : record.scenes)
        {
            std::cout
                << "drawable_oracle_scene=" << record.screenId
                << ":" << scene.localSceneName
                << ":runtime_path=" << scene.runtimeScenePath
                << ":animation=" << scene.animationName
                << ":frame=" << scene.timelineFrame
                << "/" << scene.timelineFrameCount
                << ":commands=" << scene.commandCount
                << ":sampled_tracks=" << scene.sampledTrackCount
                << ":textures=" << scene.textureCount
                << ":drawable_scene_source=" << record.drawableSceneSource
                << '\n';
        }
    }

    return records.empty() ? 1 : 0;
}

[[nodiscard]] bool runtimeDrawCallHasRectangle(const FrontendRuntimeDrawCall& call)
{
    return call.maxX > call.minX && call.maxY > call.minY;
}

[[nodiscard]] bool csdCommandIsDrawable(const CsdDrawableCommand& command)
{
    return !command.hidden && (command.textureResolved || command.sourceFreeStructural);
}

[[nodiscard]] bool runtimeDrawCallMatchesScene(const FrontendRuntimeDrawCall& call, std::string_view sceneName)
{
    if (call.sceneName == sceneName)
        return true;
    const std::string sceneToken = "/" + std::string(sceneName) + "/";
    return call.layerPath.find(sceneToken) != std::string::npos
        || call.layerPath.ends_with("/" + std::string(sceneName));
}

[[nodiscard]] bool runtimeRectOverlapsCommand(
    const FrontendRuntimeDrawCall& call,
    const CsdDrawableCommand& command)
{
    if (!runtimeDrawCallHasRectangle(call) || !csdCommandIsDrawable(command))
        return false;

    const double commandMinX = static_cast<double>(command.destinationX);
    const double commandMinY = static_cast<double>(command.destinationY);
    const double commandMaxX = commandMinX + static_cast<double>(std::max(0, command.destinationWidth));
    const double commandMaxY = commandMinY + static_cast<double>(std::max(0, command.destinationHeight));
    return call.minX < commandMaxX
        && call.maxX > commandMinX
        && call.minY < commandMaxY
        && call.maxY > commandMinY;
}

[[nodiscard]] FrontendRuntimeDrawListTriage buildFrontendRuntimeDrawListTriage(
    const sward::ui_runtime::FrontendScreenPolicy& policy)
{
    FrontendRuntimeDrawListTriage triage;
    triage.screenId = policy.screenId;

    const auto drawList = loadFrontendRuntimeDrawListEvidence(policy);
    triage.source = drawList.source;
    triage.probe = drawList.probe;
    triage.runtimeCallCount = drawList.calls.size();
    triage.runtimeRectCount = static_cast<std::size_t>(std::count_if(
        drawList.calls.begin(),
        drawList.calls.end(),
        runtimeDrawCallHasRectangle));

    const auto clock = loadFrontendUiOraclePlaybackClock(policy);
    std::vector<std::string> screenTextureNames;

    for (const auto& scenePolicy : policy.scenes)
    {
        FrontendRuntimeDrawListTriageScene scene;
        scene.sceneName = scenePolicy.sceneName;

        const int sceneFrame = uiOracleTimelineFrameForScene(clock, scenePolicy);
        const CsdPipelineTemplateBinding sceneBinding{
            policy.screenId,
            policy.layoutName,
            scenePolicy.sceneName,
            scenePolicy.sceneName,
            scenePolicy.timeline.animationName,
        };

        const auto* drawableScene = cachedCsdDrawableScene(sceneBinding);
        auto playback = loadCsdTimelinePlayback(sceneBinding, sceneFrame);
        if (!playback)
            playback = loadTimelinePlaybackForScene(policy.screenId, policy.layoutName, scenePolicy.sceneName, sceneFrame);

        std::vector<CsdDrawableCommand> commands;
        if (drawableScene)
        {
            std::size_t sampledTrackCount = 0;
            commands = timelineSampledCommands(
                *drawableScene,
                playback ? &*playback : nullptr,
                sampledTrackCount);
        }

        std::vector<std::string> sceneTextureNames;
        for (const auto& command : commands)
        {
            if (!csdCommandIsDrawable(command))
                continue;
            ++scene.localCommandCount;
            appendUniqueTextureName(screenTextureNames, command.textureName);
            appendUniqueTextureName(sceneTextureNames, command.textureName);
        }
        scene.localTextureCount = sceneTextureNames.size();

        for (const auto& call : drawList.calls)
        {
            if (!runtimeDrawCallHasRectangle(call))
                continue;

            const bool sceneMatched = runtimeDrawCallMatchesScene(call, scene.sceneName);
            if (sceneMatched)
                ++scene.runtimeRectCount;

            const bool overlapsLocalCommand = std::any_of(
                commands.begin(),
                commands.end(),
                [&call](const CsdDrawableCommand& command)
                {
                    return runtimeRectOverlapsCommand(call, command);
                });
            if (sceneMatched || overlapsLocalCommand)
                ++scene.rectMatchCandidates;
        }

        triage.localCommandCount += scene.localCommandCount;
        triage.rectMatchCandidates += scene.rectMatchCandidates;
        triage.scenes.push_back(std::move(scene));
    }

    triage.localTextureCount = screenTextureNames.size();
    return triage;
}

[[nodiscard]] int runRendererUiDrawListTriageSmoke()
{
    constexpr std::string_view directProbeToken = "ui_draw_list_probe=direct-ui-draw-list";
    constexpr std::string_view oracleFallbackToken = "ui_draw_list_probe=ui-oracle-fallback";
    constexpr std::string_view missingProbeToken = "ui_draw_list_probe=missing";
    constexpr std::string_view runtimeDrawListSourceToken = "runtime_draw_list_source=ui-draw-list";
    constexpr std::string_view materialTriageToken = "material_triage=runtime-rectangles-vs-local-csd";
    constexpr std::string_view backendSubmitStatusToken = "backend_submit_status=pending";

    std::vector<FrontendRuntimeDrawListTriage> records;
    for (const auto& policy : frontendScreenPolicies())
        records.push_back(buildFrontendRuntimeDrawListTriage(policy));

    const bool anyDirect = std::any_of(
        records.begin(),
        records.end(),
        [](const FrontendRuntimeDrawListTriage& record)
        {
            return record.probe == "direct-ui-draw-list";
        });
    const bool anyOracle = std::any_of(
        records.begin(),
        records.end(),
        [](const FrontendRuntimeDrawListTriage& record)
        {
            return record.probe == "ui-oracle-fallback";
        });

    std::cout
        << "sward_su_ui_asset_renderer ui draw-list triage smoke ok "
        << "mode=phase149-ui-draw-list-triage"
        << " " << (anyDirect ? directProbeToken : (anyOracle ? oracleFallbackToken : missingProbeToken))
        << " " << runtimeDrawListSourceToken
        << " " << materialTriageToken
        << " " << backendSubmitStatusToken
        << " lanes=" << records.size()
        << '\n';

    for (const auto& record : records)
    {
        std::cout
            << "draw_list_triage=" << record.screenId
            << ":source=" << record.source
            << ":runtime_calls=" << record.runtimeCallCount
            << ":runtime_rects=" << record.runtimeRectCount
            << ":local_commands=" << record.localCommandCount
            << ":local_textures=" << record.localTextureCount
            << ":rect_match_candidates=" << record.rectMatchCandidates
            << ":backend_submit_status=" << record.backendSubmitStatus
            << '\n';

        for (const auto& scene : record.scenes)
        {
            std::cout
                << "draw_list_scene=" << record.screenId
                << ":" << scene.sceneName
                << ":local_commands=" << scene.localCommandCount
                << ":runtime_rects=" << scene.runtimeRectCount
                << ":material_triage=" << record.materialTriage
                << '\n';
        }
    }

    return records.empty() ? 1 : 0;
}

[[nodiscard]] FrontendGpuSubmitMaterialTriage buildFrontendGpuSubmitMaterialTriage(
    const sward::ui_runtime::FrontendScreenPolicy& policy)
{
    FrontendGpuSubmitMaterialTriage triage;
    triage.screenId = policy.screenId;

    const auto submit = loadFrontendGpuSubmitEvidence(policy);
    triage.source = submit.source;
    triage.probe = submit.probe;
    triage.backendSubmitCount = submit.calls.size();
    triage.texturedSubmitCount = static_cast<std::size_t>(std::count_if(
        submit.calls.begin(),
        submit.calls.end(),
        [](const FrontendGpuSubmitCall& call)
        {
            return call.texture2DDescriptorIndex != 0;
        }));
    triage.alphaBlendSubmitCount = static_cast<std::size_t>(std::count_if(
        submit.calls.begin(),
        submit.calls.end(),
        [](const FrontendGpuSubmitCall& call)
        {
            return call.alphaBlendEnable;
        }));

    const auto drawList = loadFrontendRuntimeDrawListEvidence(policy);
    triage.drawRectCount = static_cast<std::size_t>(std::count_if(
        drawList.calls.begin(),
        drawList.calls.end(),
        runtimeDrawCallHasRectangle));

    const auto clock = loadFrontendUiOraclePlaybackClock(policy);
    for (const auto& scenePolicy : policy.scenes)
    {
        const int sceneFrame = uiOracleTimelineFrameForScene(clock, scenePolicy);
        const CsdPipelineTemplateBinding sceneBinding{
            policy.screenId,
            policy.layoutName,
            scenePolicy.sceneName,
            scenePolicy.sceneName,
            scenePolicy.timeline.animationName,
        };

        const auto* drawableScene = cachedCsdDrawableScene(sceneBinding);
        if (!drawableScene)
            continue;

        auto playback = loadCsdTimelinePlayback(sceneBinding, sceneFrame);
        if (!playback)
            playback = loadTimelinePlaybackForScene(policy.screenId, policy.layoutName, scenePolicy.sceneName, sceneFrame);

        std::size_t sampledTrackCount = 0;
        const auto commands = timelineSampledCommands(
            *drawableScene,
            playback ? &*playback : nullptr,
            sampledTrackCount);
        triage.localCommandCount += static_cast<std::size_t>(std::count_if(
            commands.begin(),
            commands.end(),
            csdCommandIsDrawable));
    }

    return triage;
}

[[nodiscard]] int runRendererGpuSubmitTriageSmoke()
{
    constexpr std::string_view directProbeToken = "gpu_submit_probe=direct-ui-gpu-submit";
    constexpr std::string_view drawListFallbackToken = "gpu_submit_probe=ui-draw-list-fallback";
    constexpr std::string_view missingProbeToken = "gpu_submit_probe=missing";
    constexpr std::string_view gpuSubmitSourceToken = "gpu_submit_source=ui-gpu-submit";
    constexpr std::string_view materialTriageToken = "material_triage=backend-submit-vs-runtime-rectangles";
    constexpr std::string_view backendSubmitStatusToken = "backend_submit_status=render-thread-material-submit";

    std::vector<FrontendGpuSubmitMaterialTriage> records;
    for (const auto& policy : frontendScreenPolicies())
        records.push_back(buildFrontendGpuSubmitMaterialTriage(policy));

    const bool anyDirect = std::any_of(
        records.begin(),
        records.end(),
        [](const FrontendGpuSubmitMaterialTriage& record)
        {
            return record.probe == "direct-ui-gpu-submit";
        });
    const bool anyFallback = std::any_of(
        records.begin(),
        records.end(),
        [](const FrontendGpuSubmitMaterialTriage& record)
        {
            return record.probe == "ui-draw-list-fallback";
        });

    std::cout
        << "sward_su_ui_asset_renderer gpu submit triage smoke ok "
        << "mode=phase150-backend-submit-material-triage"
        << " " << (anyDirect ? directProbeToken : (anyFallback ? drawListFallbackToken : missingProbeToken))
        << " " << gpuSubmitSourceToken
        << " " << materialTriageToken
        << " " << backendSubmitStatusToken
        << " lanes=" << records.size()
        << '\n';

    for (const auto& record : records)
    {
        std::cout
            << "gpu_submit_triage=" << record.screenId
            << ":source=" << record.source
            << ":backend_submits=" << record.backendSubmitCount
            << ":textured_submits=" << record.texturedSubmitCount
            << ":alpha_blend_submits=" << record.alphaBlendSubmitCount
            << ":draw_rects=" << record.drawRectCount
            << ":local_commands=" << record.localCommandCount
            << ":material_triage=" << record.materialTriage
            << '\n';
    }

    return records.empty() ? 1 : 0;
}

[[nodiscard]] FrontendMaterialCorrelationTriage buildFrontendMaterialCorrelationTriage(
    const sward::ui_runtime::FrontendScreenPolicy& policy)
{
    FrontendMaterialCorrelationTriage triage;
    triage.screenId = policy.screenId;

    const auto material = loadFrontendMaterialCorrelationEvidence(policy);
    triage.source = material.source;
    triage.probe = material.probe;
    triage.rawBackendCommandStatus = material.rawBackendCommandStatus;
    triage.pairCount = material.pairs.size();
    triage.drawCallCount = material.drawCallCount;
    triage.backendSubmitCount = material.backendSubmitCount;
    triage.alphaBlendPairCount = static_cast<std::size_t>(std::count_if(
        material.pairs.begin(),
        material.pairs.end(),
        [](const FrontendMaterialCorrelationPair& pair)
        {
            return pair.alphaBlendEnable;
        }));
    triage.additivePairCount = static_cast<std::size_t>(std::count_if(
        material.pairs.begin(),
        material.pairs.end(),
        [](const FrontendMaterialCorrelationPair& pair)
        {
            return pair.additiveBlend || pair.blendSemantic.find("additive") != std::string::npos;
        }));
    triage.filterLinearPairCount = static_cast<std::size_t>(std::count_if(
        material.pairs.begin(),
        material.pairs.end(),
        [](const FrontendMaterialCorrelationPair& pair)
        {
            return pair.linearFilter || pair.samplerSemantic.find("linear") != std::string::npos;
        }));
    triage.filterPointPairCount = static_cast<std::size_t>(std::count_if(
        material.pairs.begin(),
        material.pairs.end(),
        [](const FrontendMaterialCorrelationPair& pair)
        {
            return pair.pointFilter || pair.samplerSemantic.find("point") != std::string::npos;
        }));

    if (triage.drawCallCount == 0)
    {
        const auto drawList = loadFrontendRuntimeDrawListEvidence(policy);
        triage.drawCallCount = drawList.calls.size();
    }

    if (triage.backendSubmitCount == 0)
    {
        const auto submit = loadFrontendGpuSubmitEvidence(policy);
        triage.backendSubmitCount = submit.calls.size();
    }

    const auto clock = loadFrontendUiOraclePlaybackClock(policy);
    for (const auto& scenePolicy : policy.scenes)
    {
        const int sceneFrame = uiOracleTimelineFrameForScene(clock, scenePolicy);
        const CsdPipelineTemplateBinding sceneBinding{
            policy.screenId,
            policy.layoutName,
            scenePolicy.sceneName,
            scenePolicy.sceneName,
            scenePolicy.timeline.animationName,
        };

        const auto* drawableScene = cachedCsdDrawableScene(sceneBinding);
        if (!drawableScene)
            continue;

        auto playback = loadCsdTimelinePlayback(sceneBinding, sceneFrame);
        if (!playback)
            playback = loadTimelinePlaybackForScene(policy.screenId, policy.layoutName, scenePolicy.sceneName, sceneFrame);

        std::size_t sampledTrackCount = 0;
        const auto commands = timelineSampledCommands(
            *drawableScene,
            playback ? &*playback : nullptr,
            sampledTrackCount);
        triage.localCommandCount += static_cast<std::size_t>(std::count_if(
            commands.begin(),
            commands.end(),
            csdCommandIsDrawable));
    }

    return triage;
}

[[nodiscard]] int runRendererMaterialCorrelationSmoke()
{
    constexpr std::string_view directProbeToken = "material_correlation_probe=direct-ui-material-correlation";
    constexpr std::string_view gpuSubmitFallbackToken = "material_correlation_probe=gpu-submit-fallback";
    constexpr std::string_view missingProbeToken = "material_correlation_probe=missing";
    constexpr std::string_view materialCorrelationSourceToken = "material_correlation_source=ui-material-correlation";
    constexpr std::string_view blendSemanticsToken = "blend_semantics=runtime-submit-named";
    constexpr std::string_view samplerSemanticsToken = "sampler_semantics=runtime-submit-named";

    std::vector<FrontendMaterialCorrelationTriage> records;
    for (const auto& policy : frontendScreenPolicies())
        records.push_back(buildFrontendMaterialCorrelationTriage(policy));

    const bool anyDirect = std::any_of(
        records.begin(),
        records.end(),
        [](const FrontendMaterialCorrelationTriage& record)
        {
            return record.probe == "direct-ui-material-correlation";
        });
    const bool anyFallback = std::any_of(
        records.begin(),
        records.end(),
        [](const FrontendMaterialCorrelationTriage& record)
        {
            return record.probe == "gpu-submit-fallback";
        });

    std::cout
        << "sward_su_ui_asset_renderer material correlation smoke ok "
        << "mode=phase151-material-correlation"
        << " " << (anyDirect ? directProbeToken : (anyFallback ? gpuSubmitFallbackToken : missingProbeToken))
        << " " << materialCorrelationSourceToken
        << " " << blendSemanticsToken
        << " " << samplerSemanticsToken
        << " lanes=" << records.size()
        << '\n';

    for (const auto& record : records)
    {
        std::cout
            << "material_correlation=" << record.screenId
            << ":source=" << record.source
            << ":pairs=" << record.pairCount
            << ":draw_calls=" << record.drawCallCount
            << ":backend_submits=" << record.backendSubmitCount
            << ":alpha_blend_pairs=" << record.alphaBlendPairCount
            << ":additive_pairs=" << record.additivePairCount
            << ":filter_linear_pairs=" << record.filterLinearPairCount
            << ":filter_point_pairs=" << record.filterPointPairCount
            << ":local_commands=" << record.localCommandCount
            << ":raw_backend_command_status=" << record.rawBackendCommandStatus
            << '\n';
    }

    return records.empty() ? 1 : 0;
}

[[nodiscard]] FrontendBackendResolvedTriage buildFrontendBackendResolvedTriage(
    const sward::ui_runtime::FrontendScreenPolicy& policy)
{
    FrontendBackendResolvedTriage triage;
    triage.screenId = policy.screenId;

    const auto resolved = loadFrontendBackendResolvedEvidence(policy);
    triage.source = resolved.source;
    triage.probe = resolved.probe;
    triage.resolvedBackendStatus = resolved.resolvedBackendStatus;
    triage.backendResolvedSubmitCount = resolved.submits.size();
    triage.materialPairCount = resolved.materialPairCount;
    triage.resolvedPipelineSubmitCount = static_cast<std::size_t>(std::count_if(
        resolved.submits.begin(),
        resolved.submits.end(),
        [](const FrontendBackendResolvedSubmit& submit)
        {
            return submit.resolvedPipelineKnown;
        }));
    triage.blendEnabledSubmitCount = static_cast<std::size_t>(std::count_if(
        resolved.submits.begin(),
        resolved.submits.end(),
        [](const FrontendBackendResolvedSubmit& submit)
        {
            return submit.blendEnabled;
        }));
    triage.renderTargetFormatKnownCount = static_cast<std::size_t>(std::count_if(
        resolved.submits.begin(),
        resolved.submits.end(),
        [](const FrontendBackendResolvedSubmit& submit)
        {
            return submit.renderTargetFormat0 != 0;
        }));
    triage.framebufferKnownCount = static_cast<std::size_t>(std::count_if(
        resolved.submits.begin(),
        resolved.submits.end(),
        [](const FrontendBackendResolvedSubmit& submit)
        {
            return submit.activeFramebufferKnown;
        }));

    if (triage.materialPairCount == 0)
    {
        const auto material = loadFrontendMaterialCorrelationEvidence(policy);
        triage.materialPairCount = material.pairs.size();
    }

    const auto clock = loadFrontendUiOraclePlaybackClock(policy);
    for (const auto& scenePolicy : policy.scenes)
    {
        const int sceneFrame = uiOracleTimelineFrameForScene(clock, scenePolicy);
        const CsdPipelineTemplateBinding sceneBinding{
            policy.screenId,
            policy.layoutName,
            scenePolicy.sceneName,
            scenePolicy.sceneName,
            scenePolicy.timeline.animationName,
        };

        const auto* drawableScene = cachedCsdDrawableScene(sceneBinding);
        if (!drawableScene)
            continue;

        auto playback = loadCsdTimelinePlayback(sceneBinding, sceneFrame);
        if (!playback)
            playback = loadTimelinePlaybackForScene(policy.screenId, policy.layoutName, scenePolicy.sceneName, sceneFrame);

        std::size_t sampledTrackCount = 0;
        const auto commands = timelineSampledCommands(
            *drawableScene,
            playback ? &*playback : nullptr,
            sampledTrackCount);
        triage.localCommandCount += static_cast<std::size_t>(std::count_if(
            commands.begin(),
            commands.end(),
            csdCommandIsDrawable));
    }

    return triage;
}

[[nodiscard]] int runRendererBackendResolvedTriageSmoke()
{
    constexpr std::string_view directProbeToken = "backend_resolved_probe=direct-ui-backend-resolved";
    constexpr std::string_view materialFallbackToken = "backend_resolved_probe=material-correlation-fallback";
    constexpr std::string_view missingProbeToken = "backend_resolved_probe=missing";
    constexpr std::string_view backendResolvedSourceToken = "backend_resolved_source=ui-backend-resolved";
    constexpr std::string_view materialCorrelationToken = "material_correlation_backend_resolved=joined";
    constexpr std::string_view resolvedPsoToken = "resolved_pso_blend_framebuffer=runtime-backend";

    std::vector<FrontendBackendResolvedTriage> records;
    for (const auto& policy : frontendScreenPolicies())
        records.push_back(buildFrontendBackendResolvedTriage(policy));

    const bool anyDirect = std::any_of(
        records.begin(),
        records.end(),
        [](const FrontendBackendResolvedTriage& record)
        {
            return record.probe == "direct-ui-backend-resolved";
        });
    const bool anyFallback = std::any_of(
        records.begin(),
        records.end(),
        [](const FrontendBackendResolvedTriage& record)
        {
            return record.probe == "material-correlation-fallback";
        });

    std::cout
        << "sward_su_ui_asset_renderer backend resolved triage smoke ok "
        << "mode=phase152-backend-resolved-submit"
        << " " << (anyDirect ? directProbeToken : (anyFallback ? materialFallbackToken : missingProbeToken))
        << " " << backendResolvedSourceToken
        << " " << materialCorrelationToken
        << " " << resolvedPsoToken
        << " lanes=" << records.size()
        << '\n';

    for (const auto& record : records)
    {
        std::cout
            << "backend_resolved=" << record.screenId
            << ":source=" << record.source
            << ":backend_resolved_submits=" << record.backendResolvedSubmitCount
            << ":resolved_pipeline_submits=" << record.resolvedPipelineSubmitCount
            << ":blend_enabled_submits=" << record.blendEnabledSubmitCount
            << ":rt0_format_known=" << record.renderTargetFormatKnownCount
            << ":framebuffer_known=" << record.framebufferKnownCount
            << ":material_pairs=" << record.materialPairCount
            << ":local_commands=" << record.localCommandCount
            << ":resolved_backend_status=" << record.resolvedBackendStatus
            << '\n';
    }

    return records.empty() ? 1 : 0;
}

[[nodiscard]] FrontendBackendMaterialParityTriage buildFrontendBackendMaterialParityTriage(
    const sward::ui_runtime::FrontendScreenPolicy& policy)
{
    FrontendBackendMaterialParityTriage triage;
    triage.screenId = policy.screenId;

    const auto resolved = loadFrontendBackendResolvedEvidence(policy);
    triage.source = resolved.source;
    triage.probe = resolved.probe;
    triage.materialParityStatus = resolved.materialParityStatus;
    triage.textureViewSamplerGap = resolved.textureViewSamplerGap == "pending-descriptor-view-decode"
        ? "pending"
        : resolved.textureViewSamplerGap;
    triage.textMovieSfxGap = resolved.textMovieSfxGap == "pending-title-loading-media-timing"
        ? "pending"
        : resolved.textMovieSfxGap;
    triage.backendResolvedSubmitCount = resolved.submits.size();
    triage.sourceOverSubmitCount = static_cast<std::size_t>(std::count_if(
        resolved.submits.begin(),
        resolved.submits.end(),
        [](const FrontendBackendResolvedSubmit& submit)
        {
            return submit.materialParityHint == "source-over-alpha";
        }));
    triage.additiveSubmitCount = static_cast<std::size_t>(std::count_if(
        resolved.submits.begin(),
        resolved.submits.end(),
        [](const FrontendBackendResolvedSubmit& submit)
        {
            return submit.materialParityHint == "additive-alpha";
        }));
    triage.opaqueSubmitCount = static_cast<std::size_t>(std::count_if(
        resolved.submits.begin(),
        resolved.submits.end(),
        [](const FrontendBackendResolvedSubmit& submit)
        {
            return submit.materialParityHint == "opaque-no-blend";
        }));
    triage.framebufferRegisteredSubmitCount = static_cast<std::size_t>(std::count_if(
        resolved.submits.begin(),
        resolved.submits.end(),
        [](const FrontendBackendResolvedSubmit& submit)
        {
            return submit.framebufferRegistered;
        }));

    const auto clock = loadFrontendUiOraclePlaybackClock(policy);
    for (const auto& scenePolicy : policy.scenes)
    {
        const int sceneFrame = uiOracleTimelineFrameForScene(clock, scenePolicy);
        const CsdPipelineTemplateBinding sceneBinding{
            policy.screenId,
            policy.layoutName,
            scenePolicy.sceneName,
            scenePolicy.sceneName,
            scenePolicy.timeline.animationName,
        };

        const auto* drawableScene = cachedCsdDrawableScene(sceneBinding);
        if (!drawableScene)
            continue;

        auto playback = loadCsdTimelinePlayback(sceneBinding, sceneFrame);
        if (!playback)
            playback = loadTimelinePlaybackForScene(policy.screenId, policy.layoutName, scenePolicy.sceneName, sceneFrame);

        std::size_t sampledTrackCount = 0;
        const auto commands = timelineSampledCommands(
            *drawableScene,
            playback ? &*playback : nullptr,
            sampledTrackCount);
        triage.localCommandCount += static_cast<std::size_t>(std::count_if(
            commands.begin(),
            commands.end(),
            csdCommandIsDrawable));
    }

    return triage;
}

[[nodiscard]] int runRendererMaterialParityHintsSmoke()
{
    constexpr std::string_view directProbeToken = "material_parity_probe=direct-ui-backend-resolved";
    constexpr std::string_view materialFallbackToken = "material_parity_probe=material-correlation-fallback";
    constexpr std::string_view missingProbeToken = "material_parity_probe=missing";
    constexpr std::string_view policyToken = "material_parity_policy=backend-resolved-pso-blend-framebuffer";
    constexpr std::string_view textureGapToken = "texture_view_sampler_gap=pending";
    constexpr std::string_view textMovieSfxGapToken = "text_movie_sfx_gap=pending";

    std::vector<FrontendBackendMaterialParityTriage> records;
    for (const auto& policy : frontendScreenPolicies())
        records.push_back(buildFrontendBackendMaterialParityTriage(policy));

    const bool anyDirect = std::any_of(
        records.begin(),
        records.end(),
        [](const FrontendBackendMaterialParityTriage& record)
        {
            return record.probe == "direct-ui-backend-resolved";
        });
    const bool anyFallback = std::any_of(
        records.begin(),
        records.end(),
        [](const FrontendBackendMaterialParityTriage& record)
        {
            return record.probe == "material-correlation-fallback";
        });

    std::cout
        << "sward_su_ui_asset_renderer material parity hints smoke ok "
        << "mode=phase153-backend-material-parity-hints"
        << " " << (anyDirect ? directProbeToken : (anyFallback ? materialFallbackToken : missingProbeToken))
        << " " << policyToken
        << " " << textureGapToken
        << " " << textMovieSfxGapToken
        << " lanes=" << records.size()
        << '\n';

    for (const auto& record : records)
    {
        std::cout
            << "material_parity=" << record.screenId
            << ":source=" << record.source
            << ":backend_resolved_submits=" << record.backendResolvedSubmitCount
            << ":source_over=" << record.sourceOverSubmitCount
            << ":additive=" << record.additiveSubmitCount
            << ":opaque=" << record.opaqueSubmitCount
            << ":framebuffer_registered=" << record.framebufferRegisteredSubmitCount
            << ":local_commands=" << record.localCommandCount
            << ":material_parity_status=" << record.materialParityStatus
            << '\n';
    }

    return records.empty() ? 1 : 0;
}

[[nodiscard]] FrontendDescriptorSemanticsTriage buildFrontendDescriptorSemanticsTriage(
    const sward::ui_runtime::FrontendScreenPolicy& policy)
{
    FrontendDescriptorSemanticsTriage triage;
    triage.screenId = policy.screenId;

    const auto resolved = loadFrontendBackendResolvedEvidence(policy);
    triage.source = resolved.source;
    triage.probe = resolved.probe;
    triage.textureViewSamplerStatus = resolved.textureViewSamplerStatus;
    triage.vendorDescriptorGap = resolved.vendorDescriptorCaptureGap;
    triage.textMovieSfxGap = resolved.textMovieSfxGap == "pending-title-loading-media-timing"
        ? "pending"
        : resolved.textMovieSfxGap;
    triage.textureDescriptorKnownCount = resolved.textureDescriptorKnownCount;
    triage.samplerDescriptorKnownCount = resolved.samplerDescriptorKnownCount;
    triage.linearSamplerDescriptorCount = resolved.linearSamplerDescriptorCount;
    triage.pointSamplerDescriptorCount = resolved.pointSamplerDescriptorCount;
    triage.wrapSamplerDescriptorCount = resolved.wrapSamplerDescriptorCount;
    triage.clampSamplerDescriptorCount = resolved.clampSamplerDescriptorCount;

    const auto clock = loadFrontendUiOraclePlaybackClock(policy);
    for (const auto& scenePolicy : policy.scenes)
    {
        const int sceneFrame = uiOracleTimelineFrameForScene(clock, scenePolicy);
        const CsdPipelineTemplateBinding sceneBinding{
            policy.screenId,
            policy.layoutName,
            scenePolicy.sceneName,
            scenePolicy.sceneName,
            scenePolicy.timeline.animationName,
        };

        const auto* drawableScene = cachedCsdDrawableScene(sceneBinding);
        if (!drawableScene)
            continue;

        auto playback = loadCsdTimelinePlayback(sceneBinding, sceneFrame);
        if (!playback)
            playback = loadTimelinePlaybackForScene(policy.screenId, policy.layoutName, scenePolicy.sceneName, sceneFrame);

        std::size_t sampledTrackCount = 0;
        const auto commands = timelineSampledCommands(
            *drawableScene,
            playback ? &*playback : nullptr,
            sampledTrackCount);
        triage.localCommandCount += static_cast<std::size_t>(std::count_if(
            commands.begin(),
            commands.end(),
            csdCommandIsDrawable));
    }

    return triage;
}

[[nodiscard]] int runRendererDescriptorSemanticsSmoke()
{
    constexpr std::string_view directProbeToken = "descriptor_semantics_probe=direct-ui-backend-resolved";
    constexpr std::string_view materialFallbackToken = "descriptor_semantics_probe=material-correlation-fallback";
    constexpr std::string_view missingProbeToken = "descriptor_semantics_probe=missing";
    constexpr std::string_view textureSamplerPolicyToken = "texture_sampler_policy=runtime-descriptor-state";
    constexpr std::string_view vendorDescriptorGapToken = "vendor_descriptor_gap=pending-native-descriptor-dump";
    constexpr std::string_view textMovieSfxGapToken = "text_movie_sfx_gap=pending";

    std::vector<FrontendDescriptorSemanticsTriage> records;
    for (const auto& policy : frontendScreenPolicies())
        records.push_back(buildFrontendDescriptorSemanticsTriage(policy));

    const bool anyDirect = std::any_of(
        records.begin(),
        records.end(),
        [](const FrontendDescriptorSemanticsTriage& record)
        {
            return record.probe == "direct-ui-backend-resolved";
        });
    const bool anyFallback = std::any_of(
        records.begin(),
        records.end(),
        [](const FrontendDescriptorSemanticsTriage& record)
        {
            return record.probe == "material-correlation-fallback";
        });

    std::cout
        << "sward_su_ui_asset_renderer descriptor semantics smoke ok "
        << "mode=phase154-texture-sampler-descriptor-semantics"
        << " " << (anyDirect ? directProbeToken : (anyFallback ? materialFallbackToken : missingProbeToken))
        << " " << textureSamplerPolicyToken
        << " " << vendorDescriptorGapToken
        << " " << textMovieSfxGapToken
        << " lanes=" << records.size()
        << '\n';

    for (const auto& record : records)
    {
        std::cout
            << "descriptor_semantics=" << record.screenId
            << ":source=" << record.source
            << ":texture_descriptor_known=" << record.textureDescriptorKnownCount
            << ":sampler_descriptor_known=" << record.samplerDescriptorKnownCount
            << ":linear=" << record.linearSamplerDescriptorCount
            << ":point=" << record.pointSamplerDescriptorCount
            << ":wrap=" << record.wrapSamplerDescriptorCount
            << ":clamp=" << record.clampSamplerDescriptorCount
            << ":local_commands=" << record.localCommandCount
            << ":texture_view_sampler_status=" << record.textureViewSamplerStatus
            << '\n';
    }

    return records.empty() ? 1 : 0;
}

[[nodiscard]] FrontendVendorResourceCaptureTriage buildFrontendVendorResourceCaptureTriage(
    const sward::ui_runtime::FrontendScreenPolicy& policy)
{
    FrontendVendorResourceCaptureTriage triage;
    triage.screenId = policy.screenId;

    const auto resolved = loadFrontendBackendResolvedEvidence(policy);
    triage.source = resolved.source;
    triage.probe = resolved.probe;
    triage.vendorResourceCaptureStatus = resolved.vendorResourceCaptureStatus;
    triage.uiOnlyLayerStatus = resolved.uiOnlyLayerCaptureStatus;
    triage.nativeCommandGap = resolved.nativeCommandCaptureGap;
    triage.textureResourceViewKnownCount = resolved.textureResourceViewKnownCount;
    triage.samplerResourceViewKnownCount = resolved.samplerResourceViewKnownCount;
    triage.resourceViewPairCount = resolved.resourceViewPairCount;

    const auto clock = loadFrontendUiOraclePlaybackClock(policy);
    for (const auto& scenePolicy : policy.scenes)
    {
        const int sceneFrame = uiOracleTimelineFrameForScene(clock, scenePolicy);
        const CsdPipelineTemplateBinding sceneBinding{
            policy.screenId,
            policy.layoutName,
            scenePolicy.sceneName,
            scenePolicy.sceneName,
            scenePolicy.timeline.animationName,
        };

        const auto* drawableScene = cachedCsdDrawableScene(sceneBinding);
        if (!drawableScene)
            continue;

        auto playback = loadCsdTimelinePlayback(sceneBinding, sceneFrame);
        if (!playback)
            playback = loadTimelinePlaybackForScene(policy.screenId, policy.layoutName, scenePolicy.sceneName, sceneFrame);

        std::size_t sampledTrackCount = 0;
        const auto commands = timelineSampledCommands(
            *drawableScene,
            playback ? &*playback : nullptr,
            sampledTrackCount);
        triage.localCommandCount += static_cast<std::size_t>(std::count_if(
            commands.begin(),
            commands.end(),
            csdCommandIsDrawable));
    }

    return triage;
}

[[nodiscard]] int runRendererVendorResourceCaptureSmoke()
{
    constexpr std::string_view directProbeToken = "vendor_resource_probe=direct-ui-backend-resolved";
    constexpr std::string_view materialFallbackToken = "vendor_resource_probe=material-correlation-fallback";
    constexpr std::string_view missingProbeToken = "vendor_resource_probe=missing";
    constexpr std::string_view policyToken = "vendor_resource_policy=native-rhi-resource-view-sampler";
    constexpr std::string_view uiOnlyLayerToken = "ui_only_layer_status=pending-runtime-ui-render-target-copy";
    constexpr std::string_view nativeCommandGapToken = "native_command_gap=pending-full-vendor-command-buffer-dump";

    std::vector<FrontendVendorResourceCaptureTriage> records;
    for (const auto& policy : frontendScreenPolicies())
        records.push_back(buildFrontendVendorResourceCaptureTriage(policy));

    const bool anyDirect = std::any_of(
        records.begin(),
        records.end(),
        [](const FrontendVendorResourceCaptureTriage& record)
        {
            return record.probe == "direct-ui-backend-resolved";
        });
    const bool anyFallback = std::any_of(
        records.begin(),
        records.end(),
        [](const FrontendVendorResourceCaptureTriage& record)
        {
            return record.probe == "material-correlation-fallback";
        });

    std::cout
        << "sward_su_ui_asset_renderer vendor resource capture smoke ok "
        << "mode=phase155-vendor-resource-capture"
        << " " << (anyDirect ? directProbeToken : (anyFallback ? materialFallbackToken : missingProbeToken))
        << " " << policyToken
        << " " << uiOnlyLayerToken
        << " " << nativeCommandGapToken
        << " lanes=" << records.size()
        << '\n';

    for (const auto& record : records)
    {
        std::cout
            << "vendor_resource=" << record.screenId
            << ":source=" << record.source
            << ":texture_views=" << record.textureResourceViewKnownCount
            << ":sampler_views=" << record.samplerResourceViewKnownCount
            << ":resource_pairs=" << record.resourceViewPairCount
            << ":local_commands=" << record.localCommandCount
            << ":vendor_resource_status=" << record.vendorResourceCaptureStatus
            << '\n';
    }

    return records.empty() ? 1 : 0;
}

struct CsdReusableReferenceSceneModel
{
    std::string sceneName;
    std::string timelineName;
    int timelineFrame = 0;
    int timelineFrameCount = 0;
    std::size_t commandCount = 0;
    std::size_t drawnCommandCount = 0;
    std::size_t sampledTrackCount = 0;
    std::size_t structuralCommandCount = 0;
};

struct CsdReusableReferenceScreenModel
{
    std::string laneId;
    std::string rendererScreenId;
    std::string contractFileName;
    std::string layoutFileName;
    std::string activationEvent;
    std::string transitionBandId;
    std::string transitionReadyLabel;
    std::size_t materialSlotCount = 0;
    std::size_t sgfxSlotCount = 0;
    std::size_t renderOrderSceneCount = 0;
    std::size_t commandCount = 0;
    std::size_t sampledTrackCount = 0;
    std::size_t structuralCommandCount = 0;
    std::vector<CsdReusableReferenceSceneModel> scenes;
};

[[nodiscard]] CsdReusableReferenceScreenModel buildReusableReferenceScreenModel(
    const CsdReferenceViewerLane& lane,
    const CsdReferenceViewerStats& stats)
{
    CsdReusableReferenceScreenModel model;
    model.laneId = std::string(lane.laneId);
    model.rendererScreenId = std::string(lane.rendererScreenId);
    model.contractFileName = std::string(lane.contractFileName);
    model.layoutFileName = lane.scenes.empty() ? std::string("none") : std::string(lane.scenes[0].layoutFileName);
    model.activationEvent = std::string(lane.requiredEventId);
    model.transitionBandId = std::string(lane.timelineBandId);
    model.transitionReadyLabel = std::string(lane.timelineEventLabel);
    model.materialSlotCount = lane.slots.size();
    model.sgfxSlotCount = lane.slots.size();
    model.renderOrderSceneCount = stats.sceneCount;
    model.commandCount = stats.commandCount;
    model.sampledTrackCount = stats.sampledTrackCount;
    model.structuralCommandCount = stats.structuralCommandCount;

    for (const auto& sceneStats : stats.scenes)
    {
        CsdReusableReferenceSceneModel scene;
        scene.sceneName = sceneStats.sceneName;
        scene.timelineName = sceneStats.timelineResolved ? sceneStats.timelineName : std::string("unresolved");
        scene.timelineFrame = sceneStats.timelineFrame;
        scene.timelineFrameCount = sceneStats.timelineFrameCount;
        scene.commandCount = sceneStats.commandCount;
        scene.drawnCommandCount = sceneStats.drawnCommandCount;
        scene.sampledTrackCount = sceneStats.sampledTrackCount;
        scene.structuralCommandCount = sceneStats.structuralCommandCount;
        model.scenes.push_back(std::move(scene));
    }

    return model;
}

[[nodiscard]] bool writeReusableScreenReferenceCode(
    const std::filesystem::path& exportPath,
    const std::vector<CsdReusableReferenceScreenModel>& models)
{
    std::error_code error;
    std::filesystem::create_directories(exportPath.parent_path(), error);
    if (error)
        return false;

    std::ofstream out(exportPath, std::ios::binary);
    if (!out)
        return false;

    out << "#pragma once\n";
    out << "// Generated local-only by SWARD Phase 140 from recovered CSD evidence.\n";
    out << "// Readable reference architecture only; Sonic assets remain local placeholders.\n\n";
    out << "namespace sward::recovered::frontend_screens\n{\n";
    out << "struct ScreenScenePolicy\n";
    out << "{\n";
    out << "    const char* scene;\n";
    out << "    const char* timeline;\n";
    out << "    int frame;\n";
    out << "    int frameCount;\n";
    out << "    int commands;\n";
    out << "    int structuralCommands;\n";
    out << "};\n\n";
    out << "struct ScreenPolicy\n";
    out << "{\n";
    out << "    const char* id;\n";
    out << "    const char* screen;\n";
    out << "    const char* layout;\n";
    out << "    const char* activation;\n";
    out << "    const char* transition;\n";
    out << "    const char* inputLock;\n";
    out << "    int materialSlots;\n";
    out << "    int sgfxSlots;\n";
    out << "    int sceneCount;\n";
    out << "};\n\n";
    out << "inline constexpr ScreenPolicy kScreenPolicies[] = {\n";
    for (const auto& model : models)
    {
        out << "    { \"" << jsonEscape(model.laneId)
            << "\", \"" << jsonEscape(model.rendererScreenId)
            << "\", \"" << jsonEscape(model.layoutFileName)
            << "\", \"" << jsonEscape(model.activationEvent)
            << "\", \"" << jsonEscape(model.transitionBandId + "->" + model.transitionReadyLabel)
            << "\", \"" << jsonEscape(std::string("until:") + model.activationEvent)
            << "\", " << model.materialSlotCount
            << ", " << model.sgfxSlotCount
            << ", " << model.renderOrderSceneCount
            << " },\n";
    }
    out << "};\n\n";

    for (const auto& model : models)
    {
        out << "inline constexpr ScreenScenePolicy k" << model.rendererScreenId << "Scenes[] = {\n";
        for (const auto& scene : model.scenes)
        {
            out << "    { \"" << jsonEscape(scene.sceneName)
                << "\", \"" << jsonEscape(scene.timelineName)
                << "\", " << scene.timelineFrame
                << ", " << scene.timelineFrameCount
                << ", " << scene.commandCount
                << ", " << scene.structuralCommandCount
                << " },\n";
        }
        out << "};\n\n";
    }
    out << "} // namespace sward::recovered::frontend_screens\n";
    return true;
}

[[nodiscard]] int runReferencePolicyExportSmoke()
{
    bool failed = false;
    std::vector<CsdReusableReferenceScreenModel> models;
    const auto lanes = buildReferenceViewerLanesFromTrackedPolicy();
    const auto outputRoot = repoRootForOutput() / "out" / "csd_runtime_exports" / "phase140";
    const auto exportPath = outputRoot / "title_loading_options_pause_reference.hpp";

    Gdiplus::GdiplusStartupInput gdiplusInput{};
    ULONG_PTR gdiplusToken = 0;
    if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusInput, nullptr) != Gdiplus::Ok)
        return 1;

    for (const auto& lane : lanes)
    {
        CsdReferenceViewerStats stats;
        const auto bitmap = renderCsdReferenceViewerBitmap(lane, false, stats);
        if (!bitmap || stats.commandCount == 0)
            failed = true;
        models.push_back(buildReusableReferenceScreenModel(lane, stats));
    }

    const bool wrote = writeReusableScreenReferenceCode(exportPath, models);
    if (!wrote)
        failed = true;

    std::cout
        << "sward_su_ui_asset_renderer reference policy export smoke ok "
        << "mode=phase140-reusable-screen-policy"
        << " screens=" << models.size()
        << " output=" << portablePath(exportPath)
        << '\n';
    std::cout << "reference_policy_export_path=" << portablePath(exportPath) << '\n';

    for (const auto& model : models)
    {
        std::cout
            << "reference_policy=" << model.laneId
            << ":screen=" << model.rendererScreenId
            << ":layout=" << model.layoutFileName
            << ":activation=" << model.activationEvent
            << ":transition=" << model.transitionBandId << "->" << model.transitionReadyLabel
            << ":input_lock=until:" << model.activationEvent
            << ":render_order=scene-stack"
            << ":material_slots=" << model.materialSlotCount
            << ":sgfx_slots=" << model.sgfxSlotCount
            << ":scenes=" << model.renderOrderSceneCount
            << ":commands=" << model.commandCount
            << ":sampled_tracks=" << model.sampledTrackCount
            << ":structural=" << model.structuralCommandCount
            << '\n';

        for (const auto& scene : model.scenes)
        {
            std::cout
                << "reference_policy_scene=" << model.laneId
                << ":" << scene.sceneName
                << ":timeline=" << scene.timelineName
                << "@" << scene.timelineFrame
                << "/" << scene.timelineFrameCount
                << ":commands=" << scene.commandCount
                << ":structural=" << scene.structuralCommandCount
                << ":sampled_tracks=" << scene.sampledTrackCount
                << '\n';
        }
    }

    std::cout << "reference_policy_source_status=clean-readable-title-loading-options-pause-exported\n";
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return failed ? 1 : 0;
}

[[nodiscard]] const sward::ui_runtime::SgfxTimelineBand* findTimelineBand(
    const SgfxScreenTemplate& screenTemplate,
    std::string_view bandId)
{
    const auto found = std::find_if(
        screenTemplate.timelineBands.begin(),
        screenTemplate.timelineBands.end(),
        [bandId](const sward::ui_runtime::SgfxTimelineBand& band)
        {
            return band.id == bandId;
        });
    return found == screenTemplate.timelineBands.end() ? nullptr : &*found;
}

[[nodiscard]] std::string firstRequiredEvent(const SgfxScreenTemplate& screenTemplate)
{
    return screenTemplate.evidence.requiredEvents.empty() ? std::string("none") : screenTemplate.evidence.requiredEvents.front();
}

[[nodiscard]] int runSgfxTemplateSmoke(const std::optional<std::string>& templateFilter)
{
    std::size_t templateCount = 0;
    std::size_t bindingCount = 0;
    bool failed = false;
    std::vector<std::string> descriptors;

    for (const auto& screenTemplate : sgfxScreenTemplates())
    {
        if (templateFilter && screenTemplate.id != *templateFilter)
            continue;

        ++templateCount;
        const auto* binding = findSgfxTemplateRenderBinding(screenTemplate.id);
        const auto* screen = binding ? rendererScreenById(binding->rendererScreenId) : nullptr;
        if (!binding || !screen)
        {
            failed = true;
            continue;
        }

        ++bindingCount;
        std::ostringstream descriptor;
        descriptor
            << "template=" << screenTemplate.id
            << ":screen=" << screen->id
            << ":contract=" << screenTemplate.contractFileName
            << ":event=" << (!binding->requiredEventId.empty() ? std::string(binding->requiredEventId) : firstRequiredEvent(screenTemplate));
        descriptors.push_back(descriptor.str());

        for (std::size_t index = 0; index < binding->slotCount; ++index)
        {
            const auto& slot = binding->slots[index];
            std::ostringstream slotDescriptor;
            slotDescriptor
                << "placeholder_slot="
                << screenTemplate.id
                << ":"
                << slot.slotName
                << "->"
                << slot.textureName;
            descriptors.push_back(slotDescriptor.str());
        }

        const auto* band = findTimelineBand(screenTemplate, binding->timelineBandId);
        if (!band)
        {
            failed = true;
            continue;
        }

        std::ostringstream timingDescriptor;
        timingDescriptor
            << "timeline_hook="
            << screenTemplate.id
            << ":"
            << band->id
            << "="
            << band->seconds
            << ":"
            << binding->timelineEventLabel;
        descriptors.push_back(timingDescriptor.str());
    }

    if (templateFilter && templateCount == 0)
        failed = true;

    std::cout
        << "sward_su_ui_asset_renderer sgfx template smoke ok "
        << "templates=" << templateCount
        << " bindings=" << bindingCount
        << '\n';

    for (const auto& descriptor : descriptors)
        std::cout << descriptor << '\n';

    return failed || templateCount == 0 || templateCount != bindingCount ? 1 : 0;
}

[[nodiscard]] std::string formatCsdNumber(double value)
{
    if (std::fabs(value - std::round(value)) < 0.000001)
    {
        std::ostringstream integer;
        integer << static_cast<long long>(std::llround(value));
        return integer.str();
    }

    std::ostringstream formatted;
    formatted << std::fixed << std::setprecision(6) << value;
    return formatted.str();
}

[[nodiscard]] std::string_view sgfxSlotNameForDrawableCommand(
    const SgfxTemplateRenderBinding& binding,
    const CsdDrawableCommand& command,
    std::size_t commandIndex)
{
    for (std::size_t index = 0; index < binding.slotCount; ++index)
    {
        if (binding.slots[index].textureName == std::string_view(command.textureName))
            return binding.slots[index].slotName;
    }

    if (binding.slotCount == 0)
        return "none";

    return binding.slots[std::min(commandIndex, binding.slotCount - 1)].slotName;
}

[[nodiscard]] std::string csdDrawableCommandDescriptor(
    std::string_view templateId,
    const CsdDrawableCommand& command)
{
    std::ostringstream descriptor;
    descriptor
        << "csd_draw_command="
        << templateId
        << ":"
        << command.sceneName
        << "/"
        << command.castName
        << "->"
        << command.castName
        << ":texture="
        << command.textureName
        << ":subimage="
        << command.subimageIndex
        << ":src="
        << command.sourceX
        << ","
        << command.sourceY
        << ","
        << command.sourceWidth
        << "x"
        << command.sourceHeight
        << ":dst="
        << command.destinationX
        << ","
        << command.destinationY
        << ","
        << command.destinationWidth
        << "x"
        << command.destinationHeight;
    if (command.flipX)
        descriptor << ":flipX=1";
    if (command.flipY)
        descriptor << ":flipY=1";
    if (command.sourceFreeStructural)
        descriptor << ":source-free-structural";
    else if (!command.sourceFits)
        descriptor << ":source-out-of-bounds";
    return descriptor.str();
}

[[nodiscard]] std::string csdDrawableTransformDescriptor(
    std::string_view templateId,
    const CsdDrawableCommand& command)
{
    std::ostringstream descriptor;
    descriptor
        << "sampled_transform="
        << templateId
        << ":"
        << command.sceneName
        << "/"
        << command.castName
        << ":translation="
        << formatCsdNumber(command.translationX)
        << ","
        << formatCsdNumber(command.translationY)
        << ":scale="
        << formatCsdNumber(command.scaleX)
        << ","
        << formatCsdNumber(command.scaleY)
        << ":rotation="
        << formatCsdNumber(command.rotation);
    return descriptor.str();
}

[[nodiscard]] int runCsdDrawableSmoke(const std::optional<std::string>& templateFilter)
{
    std::size_t templateCount = 0;
    bool failed = false;
    std::vector<std::string> uniquePackages;
    std::vector<std::pair<std::string, std::optional<CsdDrawableScene>>> loadedScenes;
    std::vector<std::string> descriptors;

    auto drawableFor = [&loadedScenes](const CsdPipelineTemplateBinding& binding) -> const CsdDrawableScene*
    {
        const std::string key = std::string(binding.layoutFileName) + ":" + std::string(binding.primarySceneName);
        const auto found = std::find_if(
            loadedScenes.begin(),
            loadedScenes.end(),
            [&key](const std::pair<std::string, std::optional<CsdDrawableScene>>& entry)
            {
                return entry.first == key;
            });
        if (found != loadedScenes.end())
            return found->second ? &*found->second : nullptr;

        loadedScenes.emplace_back(key, loadCsdDrawableScene(binding));
        return loadedScenes.back().second ? &*loadedScenes.back().second : nullptr;
    };

    for (const auto& csdBinding : kCsdPipelineTemplateBindings)
    {
        if (templateFilter && csdBinding.templateId != *templateFilter)
            continue;

        ++templateCount;
        if (std::find(uniquePackages.begin(), uniquePackages.end(), csdBinding.layoutFileName) == uniquePackages.end())
            uniquePackages.emplace_back(csdBinding.layoutFileName);

        const auto* scene = drawableFor(csdBinding);
        const auto* sgfxBinding = findSgfxTemplateRenderBinding(csdBinding.templateId);
        if (!scene || !sgfxBinding || scene->commands.empty())
        {
            failed = true;
            continue;
        }

        std::ostringstream sceneDescriptor;
        sceneDescriptor
            << "csd_drawable="
            << csdBinding.templateId
            << ":layout="
            << csdBinding.layoutFileName
            << ":scene="
            << scene->sceneName
            << ":commands="
            << scene->commands.size()
            << ":casts="
            << scene->castCount
            << ":subimages="
            << scene->subimageCount;
        descriptors.push_back(sceneDescriptor.str());

        std::vector<std::string> emittedTextureBindings;
        for (std::size_t index = 0; index < scene->commands.size(); ++index)
        {
            const auto& command = scene->commands[index];
            descriptors.push_back(csdDrawableCommandDescriptor(csdBinding.templateId, command));
            descriptors.push_back(csdDrawableTransformDescriptor(csdBinding.templateId, command));

            std::ostringstream slotDescriptor;
            slotDescriptor
                << "sgfx_element_map="
                << csdBinding.templateId
                << ":scene="
                << scene->sceneName
                << ":cast="
                << command.castName
                << ":slot="
                << sgfxSlotNameForDrawableCommand(*sgfxBinding, command, index)
                << ":texture="
                << command.textureName;
            descriptors.push_back(slotDescriptor.str());

            if (std::find(emittedTextureBindings.begin(), emittedTextureBindings.end(), command.textureName) == emittedTextureBindings.end())
            {
                emittedTextureBindings.push_back(command.textureName);
                std::ostringstream textureDescriptor;
                textureDescriptor
                    << "texture_binding="
                    << csdBinding.templateId
                    << ":"
                    << command.textureName
                    << ":resolved="
                    << (command.textureResolved ? "1" : "0")
                    << ":size="
                    << command.textureWidth
                    << "x"
                    << command.textureHeight;
                descriptors.push_back(textureDescriptor.str());
            }
        }

        const auto manifest = findRuntimeEvidenceManifestForTarget(csdBinding.templateId);
        std::ostringstream evidenceDescriptor;
        evidenceDescriptor
            << "native_bmp_compare="
            << csdBinding.templateId
            << ":target="
            << csdBinding.templateId
            << ":event="
            << sgfxBinding->requiredEventId
            << ":manifest="
            << (manifest ? "found" : "missing");
        descriptors.push_back(evidenceDescriptor.str());
    }

    if (templateFilter && templateCount == 0)
        failed = true;

    std::cout
        << "sward_su_ui_asset_renderer csd drawable smoke ok "
        << "layout_source=research_uiux/data/layout_deep_analysis.json "
        << "templates=" << templateCount
        << " packages=" << uniquePackages.size()
        << '\n';

    for (const auto& descriptor : descriptors)
        std::cout << descriptor << '\n';

    return failed || templateCount == 0 ? 1 : 0;
}

[[nodiscard]] std::string csdTimelineSampleDescriptor(
    std::string_view templateId,
    const CsdTimelineTrackSample& sample)
{
    std::ostringstream descriptor;
    descriptor
        << "timeline_sample="
        << templateId
        << ":"
        << sample.sceneName
        << "/"
        << sample.castName
        << ":track="
        << sample.trackType
        << ":frame="
        << sample.sampleFrame
        << ":value="
        << formatCsdNumber(sample.value);
    return descriptor.str();
}

[[nodiscard]] std::string formatPackedRgbaHex(std::uint32_t packed)
{
    std::ostringstream descriptor;
    descriptor
        << "0x"
        << std::uppercase
        << std::hex
        << std::setw(8)
        << std::setfill('0')
        << packed;
    return descriptor.str();
}

[[nodiscard]] std::string csdPackedRgbaTimelineSampleDescriptor(
    std::string_view templateId,
    const CsdTimelinePackedRgbaTrackSample& sample)
{
    std::ostringstream descriptor;
    descriptor
        << "timeline_rgba_sample="
        << templateId
        << ":"
        << sample.sceneName
        << "/"
        << sample.castName
        << ":track="
        << sample.trackType
        << ":frame="
        << sample.sampleFrame
        << ":rgba="
        << formatPackedRgbaHex(sample.packedRgba)
        << ":components="
        << static_cast<int>(sample.color.r)
        << ","
        << static_cast<int>(sample.color.g)
        << ","
        << static_cast<int>(sample.color.b)
        << ","
        << static_cast<int>(sample.color.a);
    return descriptor.str();
}

[[nodiscard]] std::string csdTimelineDrawCommandDescriptor(
    std::string_view templateId,
    const CsdTimelineTrackSample& sample,
    const CsdDrawableCommand& sampledCommand)
{
    std::ostringstream descriptor;
    descriptor
        << "timeline_draw_command="
        << templateId
        << ":"
        << sample.sceneName
        << "/"
        << sample.castName
        << ":frame="
        << sample.sampleFrame
        << ":track="
        << sample.trackType
        << ":value="
        << formatCsdNumber(sample.value)
        << ":dst="
        << sampledCommand.destinationX
        << ","
        << sampledCommand.destinationY
        << ","
        << sampledCommand.destinationWidth
        << "x"
        << sampledCommand.destinationHeight;
    return descriptor.str();
}

[[nodiscard]] int runCsdTimelineSmoke(const std::optional<std::string>& templateFilter)
{
    std::size_t templateCount = 0;
    bool failed = false;
    std::vector<std::string> uniquePackages;
    std::vector<std::pair<std::string, std::optional<CsdTimelinePlayback>>> loadedTimelines;
    std::vector<std::pair<std::string, std::optional<CsdDrawableScene>>> loadedScenes;
    std::vector<std::string> descriptors;

    auto timelineFor = [&loadedTimelines](const CsdPipelineTemplateBinding& binding) -> const CsdTimelinePlayback*
    {
        const std::string key = std::string(binding.layoutFileName) + ":" + std::string(binding.timelineSceneName) + ":" + std::string(binding.timelineAnimationName);
        const auto found = std::find_if(
            loadedTimelines.begin(),
            loadedTimelines.end(),
            [&key](const std::pair<std::string, std::optional<CsdTimelinePlayback>>& entry)
            {
                return entry.first == key;
            });
        if (found != loadedTimelines.end())
            return found->second ? &*found->second : nullptr;

        loadedTimelines.emplace_back(key, loadCsdTimelinePlayback(binding));
        return loadedTimelines.back().second ? &*loadedTimelines.back().second : nullptr;
    };

    auto drawableFor = [&loadedScenes](const CsdPipelineTemplateBinding& binding) -> const CsdDrawableScene*
    {
        const std::string key = std::string(binding.layoutFileName) + ":" + std::string(binding.primarySceneName);
        const auto found = std::find_if(
            loadedScenes.begin(),
            loadedScenes.end(),
            [&key](const std::pair<std::string, std::optional<CsdDrawableScene>>& entry)
            {
                return entry.first == key;
            });
        if (found != loadedScenes.end())
            return found->second ? &*found->second : nullptr;

        loadedScenes.emplace_back(key, loadCsdDrawableScene(binding));
        return loadedScenes.back().second ? &*loadedScenes.back().second : nullptr;
    };

    for (const auto& csdBinding : kCsdPipelineTemplateBindings)
    {
        if (templateFilter && csdBinding.templateId != *templateFilter)
            continue;

        ++templateCount;
        if (std::find(uniquePackages.begin(), uniquePackages.end(), csdBinding.layoutFileName) == uniquePackages.end())
            uniquePackages.emplace_back(csdBinding.layoutFileName);

        const auto* playback = timelineFor(csdBinding);
        const auto* sgfxBinding = findSgfxTemplateRenderBinding(csdBinding.templateId);
        if (!playback || !sgfxBinding)
        {
            failed = true;
            continue;
        }

        std::ostringstream timelineDescriptor;
        timelineDescriptor
            << "csd_timeline="
            << csdBinding.templateId
            << ":layout="
            << csdBinding.layoutFileName
            << ":scene="
            << playback->sceneName
            << ":animation="
            << playback->animationName
            << ":frame="
            << playback->sampleFrame
            << "/"
            << formatCsdNumber(playback->frameCount)
            << ":tracks="
            << playback->trackCount
            << ":numeric="
            << playback->numericTrackCount
            << ":keyframes="
            << playback->keyframeCount;
        descriptors.push_back(timelineDescriptor.str());

        for (const auto& sample : playback->samples)
            descriptors.push_back(csdTimelineSampleDescriptor(csdBinding.templateId, sample));
        for (const auto& sample : playback->packedRgbaSamples)
            descriptors.push_back(csdPackedRgbaTimelineSampleDescriptor(csdBinding.templateId, sample));

        if (const auto* drawable = drawableFor(csdBinding))
        {
            for (const auto& sample : playback->samples)
            {
                for (const auto& command : drawable->commands)
                {
                    const auto sampled = applyCsdTimelineToDrawableCommand(command, sample);
                    if (!sampled)
                        continue;

                    descriptors.push_back(csdTimelineDrawCommandDescriptor(csdBinding.templateId, sample, *sampled));
                }
            }
        }

        const auto manifest = findRuntimeEvidenceManifestForTarget(csdBinding.templateId);
        std::ostringstream comparisonDescriptor;
        comparisonDescriptor
            << "rendered_frame_compare="
            << csdBinding.templateId
            << ":target="
            << csdBinding.templateId
            << ":event="
            << sgfxBinding->requiredEventId
            << ":frame="
            << playback->sampleFrame
            << ":native="
            << (manifest ? "found" : "missing");
        descriptors.push_back(comparisonDescriptor.str());
    }

    if (templateFilter && templateCount == 0)
        failed = true;

    std::cout
        << "sward_su_ui_asset_renderer csd timeline smoke ok "
        << "layout_source=research_uiux/data/layout_deep_analysis.json "
        << "templates=" << templateCount
        << " packages=" << uniquePackages.size()
        << '\n';

    for (const auto& descriptor : descriptors)
        std::cout << descriptor << '\n';

    return failed || templateCount == 0 ? 1 : 0;
}

[[nodiscard]] std::filesystem::path repoRootForOutput()
{
    for (const auto& root : repoRootCandidates())
    {
        std::error_code error;
        if (std::filesystem::is_regular_file(root / "research_uiux" / "runtime_reference" / "examples" / "su_ui_asset_renderer.cpp", error))
            return root;
    }

    return std::filesystem::current_path();
}

[[nodiscard]] std::string portablePath(const std::filesystem::path& path)
{
    std::error_code error;
    auto relative = std::filesystem::relative(path, repoRootForOutput(), error);
    std::string text = (error ? path : relative).generic_string();
    return text;
}

[[nodiscard]] std::string jsonEscape(std::string_view text)
{
    std::ostringstream escaped;
    for (const char ch : text)
    {
        switch (ch)
        {
        case '\\': escaped << "\\\\"; break;
        case '"': escaped << "\\\""; break;
        case '\n': escaped << "\\n"; break;
        case '\r': escaped << "\\r"; break;
        case '\t': escaped << "\\t"; break;
        default: escaped << ch; break;
        }
    }
    return escaped.str();
}

[[nodiscard]] bool layoutEvidenceContainsFile(std::string_view layoutFileName);
[[nodiscard]] std::string sonicHudSlotLabelForScene(std::string_view sceneName);
[[nodiscard]] int runtimeLayerCountForScene(
    const CsdHudRuntimeSceneEvidence& evidence,
    std::string_view sceneName);

[[nodiscard]] bool writeSonicHudRuntimeCsdTreeExport(
    const CsdHudRuntimeSceneEvidence& evidence,
    const std::filesystem::path& exportPath)
{
    std::error_code error;
    std::filesystem::create_directories(exportPath.parent_path(), error);
    if (error)
        return false;

    std::ofstream out(exportPath, std::ios::binary);
    if (!out)
        return false;

    out << "{\n";
    out << "  \"phase\": 134,\n";
    out << "  \"target\": \"sonic-hud\",\n";
    out << "  \"source\": \"live-bridge CCsdProject::Make runtime tree\",\n";
    out << "  \"drawableStatus\": \"runtime-scene-layer-tree-exported-no-material-rects\",\n";
    out << "  \"runtimeProject\": \"" << jsonEscape(evidence.runtimeProject) << "\",\n";
    out << "  \"localLayout\": \"" << jsonEscape(evidence.localLayoutFileName) << "\",\n";
    out << "  \"localProject\": \"" << jsonEscape(evidence.localProject) << "\",\n";
    out << "  \"layoutStatus\": \"" << jsonEscape(evidence.layoutStatus) << "\",\n";
    out << "  \"exactLayoutFound\": " << (layoutEvidenceContainsFile(evidence.runtimeProject + ".yncp") ? "true" : "false") << ",\n";
    out << "  \"liveStatePath\": \"" << jsonEscape(portablePath(evidence.liveStatePath)) << "\",\n";
    out << "  \"stageReadyFrame\": " << evidence.stageReadyFrame << ",\n";
    out << "  \"ownerPathStatus\": \"" << jsonEscape(evidence.ownerPathStatus) << "\",\n";
    out << "  \"ownerFieldMaturationStatus\": \"" << jsonEscape(evidence.ownerFieldMaturationStatus) << "\",\n";
    out << "  \"counts\": { \"scenes\": " << evidence.runtimeSceneCount
        << ", \"nodes\": " << evidence.runtimeNodeCount
        << ", \"layers\": " << evidence.runtimeLayerCount
        << ", \"exportedScenes\": " << evidence.runtimeScenes.size()
        << ", \"exportedNodes\": " << evidence.runtimeNodes.size()
        << ", \"exportedLayers\": " << evidence.runtimeLayers.size() << " },\n";

    out << "  \"scenes\": [\n";
    for (std::size_t index = 0; index < evidence.runtimeScenes.size(); ++index)
    {
        const auto& scene = evidence.runtimeScenes[index];
        out << "    { \"path\": \"" << jsonEscape(scene.path)
            << "\", \"scene\": \"" << jsonEscape(scene.sceneName)
            << "\", \"castCount\": " << scene.castCount
            << ", \"layerCount\": " << runtimeLayerCountForScene(evidence, scene.sceneName)
            << ", \"frame\": " << scene.frame
            << ", \"sgfxSlot\": \"" << jsonEscape(sonicHudSlotLabelForScene(scene.sceneName))
            << "\", \"drawableStatus\": \"runtime-scene-layer-tree-exported-no-material-rects\" }"
            << (index + 1 == evidence.runtimeScenes.size() ? "\n" : ",\n");
    }
    out << "  ],\n";

    out << "  \"nodes\": [\n";
    for (std::size_t index = 0; index < evidence.runtimeNodes.size(); ++index)
    {
        const auto& node = evidence.runtimeNodes[index];
        out << "    { \"path\": \"" << jsonEscape(node.path)
            << "\", \"nodeAddress\": \"" << jsonEscape(node.nodeAddress)
            << "\", \"projectAddress\": \"" << jsonEscape(node.projectAddress)
            << "\", \"sceneCount\": " << node.sceneCount
            << ", \"childNodeCount\": " << node.childNodeCount
            << ", \"frame\": " << node.frame << " }"
            << (index + 1 == evidence.runtimeNodes.size() ? "\n" : ",\n");
    }
    out << "  ],\n";

    out << "  \"layers\": [\n";
    for (std::size_t index = 0; index < evidence.runtimeLayers.size(); ++index)
    {
        const auto& layer = evidence.runtimeLayers[index];
        out << "    { \"path\": \"" << jsonEscape(layer.path)
            << "\", \"scene\": \"" << jsonEscape(layer.sceneName)
            << "\", \"layer\": \"" << jsonEscape(layer.layerName)
            << "\", \"layerAddress\": \"" << jsonEscape(layer.layerAddress)
            << "\", \"castNodeAddress\": \"" << jsonEscape(layer.castNodeAddress)
            << "\", \"castNodeIndex\": " << layer.castNodeIndex
            << ", \"castIndex\": " << layer.castIndex
            << ", \"frame\": " << layer.frame << " }"
            << (index + 1 == evidence.runtimeLayers.size() ? "\n" : ",\n");
    }
    out << "  ]\n";
    out << "}\n";
    return true;
}

[[nodiscard]] std::string localCsdSceneNameForRuntimeScene(std::string_view runtimeSceneName)
{
    const std::string scene(runtimeSceneName);
    const auto slash = scene.find_last_of('/');
    return slash == std::string::npos ? scene : scene.substr(slash + 1);
}

[[nodiscard]] std::string timelineAnimationNameForLocalScene(
    std::string_view layoutFileName,
    std::string_view localSceneName)
{
    const auto evidence = loadCsdPipelineEvidence(layoutFileName);
    if (!evidence)
        return "DefaultAnim";

    if (localSceneName == "u_info")
    {
        const auto intro = std::find_if(
            evidence->timelines.begin(),
            evidence->timelines.end(),
            [localSceneName](const CsdPipelineTimelineHook& hook)
            {
                return hook.sceneName == localSceneName && hook.animationName == "Intro_Anim";
            });
        if (intro != evidence->timelines.end())
            return intro->animationName;
    }

    for (const auto& hook : evidence->timelines)
    {
        if (hook.sceneName == localSceneName)
            return hook.animationName;
    }

    return "DefaultAnim";
}

[[nodiscard]] const CsdDrawableCommand* findDrawableCommandForLayer(
    const CsdDrawableScene& scene,
    std::string_view layerName)
{
    const auto found = std::find_if(
        scene.commands.begin(),
        scene.commands.end(),
        [layerName](const CsdDrawableCommand& command)
        {
            return command.castName == layerName;
        });
    return found == scene.commands.end() ? nullptr : &*found;
}

[[nodiscard]] std::optional<CsdTimelinePlayback> loadTimelinePlaybackForScene(
    std::string_view templateId,
    std::string_view layoutFileName,
    std::string_view localSceneName,
    std::optional<int> sampleFrameOverride)
{
    const std::string animationName = timelineAnimationNameForLocalScene(layoutFileName, localSceneName);
    const CsdPipelineTemplateBinding binding{
        templateId,
        layoutFileName,
        localSceneName,
        localSceneName,
        animationName,
    };
    return loadCsdTimelinePlayback(binding, sampleFrameOverride);
}

[[nodiscard]] std::vector<CsdHudRuntimeMaterialEntry> buildSonicHudRuntimeMaterialEntries(
    const CsdHudRuntimeSceneEvidence& evidence)
{
    std::vector<CsdHudRuntimeMaterialEntry> entries;
    constexpr std::string_view kTemplateId = "sonic-hud";
    constexpr std::string_view kLayoutFileName = "ui_playscreen.yncp";

    struct LocalSceneCacheEntry
    {
        std::string runtimeSceneName;
        std::string localSceneName;
        std::optional<CsdDrawableScene> drawableScene;
        std::optional<CsdTimelinePlayback> timeline;
    };

    std::vector<LocalSceneCacheEntry> sceneCache;
    auto sceneCacheFor = [&sceneCache](std::string_view runtimeSceneName) -> LocalSceneCacheEntry*
    {
        const auto found = std::find_if(
            sceneCache.begin(),
            sceneCache.end(),
            [runtimeSceneName](const LocalSceneCacheEntry& entry)
            {
                return entry.runtimeSceneName == runtimeSceneName;
            });
        return found == sceneCache.end() ? nullptr : &*found;
    };

    for (const auto& runtimeScene : evidence.runtimeScenes)
    {
        const std::string localSceneName = localCsdSceneNameForRuntimeScene(runtimeScene.sceneName);
        const std::string timelineAnimationName = timelineAnimationNameForLocalScene(kLayoutFileName, localSceneName);
        const CsdPipelineTemplateBinding binding{
            kTemplateId,
            kLayoutFileName,
            localSceneName,
            localSceneName,
            timelineAnimationName,
        };

        sceneCache.push_back({
            runtimeScene.sceneName,
            localSceneName,
            loadCsdDrawableScene(binding),
            loadCsdTimelinePlayback(binding),
        });
    }

    for (const auto& layer : evidence.runtimeLayers)
    {
        auto* cache = sceneCacheFor(layer.sceneName);
        if (!cache || !cache->drawableScene)
            continue;

        const auto* command = findDrawableCommandForLayer(*cache->drawableScene, layer.layerName);
        if (!command)
            continue;

        CsdHudRuntimeMaterialEntry entry;
        entry.runtimePath = layer.path;
        entry.runtimeSceneName = layer.sceneName;
        entry.localSceneName = cache->localSceneName;
        entry.layerName = layer.layerName;
        entry.castName = command->castName;
        entry.textureName = command->textureName;
        entry.sgfxSlotLabel = sonicHudSlotLabelForScene(layer.sceneName);
        entry.materialSourceStatus = "exact-local-layout";
        entry.timelineName = cache->timeline ? cache->timeline->animationName : "";
        entry.timelineFrame = cache->timeline ? cache->timeline->sampleFrame : 0;
        entry.timelineFrameCount = cache->timeline ? static_cast<int>(std::llround(cache->timeline->frameCount)) : 0;
        entry.sourceX = command->sourceX;
        entry.sourceY = command->sourceY;
        entry.sourceWidth = command->sourceWidth;
        entry.sourceHeight = command->sourceHeight;
        entry.destinationX = command->destinationX;
        entry.destinationY = command->destinationY;
        entry.destinationWidth = command->destinationWidth;
        entry.destinationHeight = command->destinationHeight;
        entry.textureWidth = command->textureWidth;
        entry.textureHeight = command->textureHeight;
        entry.subimageIndex = command->subimageIndex;
        entry.textureIndex = command->textureIndex;
        entry.castIndex = layer.castIndex;
        entry.textureResolved = command->textureResolved;
        entry.sourceFits = command->sourceFits;
        entry.timelineResolved = cache->timeline.has_value();
        entries.push_back(std::move(entry));
    }

    return entries;
}

[[nodiscard]] int materialResolvedCountForRuntimeScene(
    const std::vector<CsdHudRuntimeMaterialEntry>& entries,
    std::string_view runtimeSceneName)
{
    return static_cast<int>(std::count_if(
        entries.begin(),
        entries.end(),
        [runtimeSceneName](const CsdHudRuntimeMaterialEntry& entry)
        {
            return entry.runtimeSceneName == runtimeSceneName
                && entry.subimageIndex >= 0
                && entry.textureResolved
                && entry.sourceFits;
        }));
}

[[nodiscard]] bool writeSonicHudRuntimeMaterialExport(
    const CsdHudRuntimeSceneEvidence& evidence,
    const std::vector<CsdHudRuntimeMaterialEntry>& entries,
    const std::filesystem::path& exportPath)
{
    std::error_code error;
    std::filesystem::create_directories(exportPath.parent_path(), error);
    if (error)
        return false;

    std::ofstream out(exportPath, std::ios::binary);
    if (!out)
        return false;

    auto isResolvedMaterialEntry = [](const CsdHudRuntimeMaterialEntry& entry)
    {
        return entry.subimageIndex >= 0 && entry.textureResolved && entry.sourceFits;
    };

    const int materialResolved = static_cast<int>(std::count_if(
        entries.begin(),
        entries.end(),
        isResolvedMaterialEntry));
    const int subimageResolved = materialResolved;
    const int timelineResolved = static_cast<int>(std::count_if(
        entries.begin(),
        entries.end(),
        [isResolvedMaterialEntry](const CsdHudRuntimeMaterialEntry& entry)
        {
            return isResolvedMaterialEntry(entry) && entry.timelineResolved;
        }));

    out << "{\n";
    out << "  \"phase\": 135,\n";
    out << "  \"target\": \"sonic-hud\",\n";
    out << "  \"source\": \"runtime-tree+exact-local-layout\",\n";
    out << "  \"drawableStatus\": \"runtime-material-exact-local-layout\",\n";
    out << "  \"runtimeProject\": \"" << jsonEscape(evidence.runtimeProject) << "\",\n";
    out << "  \"localLayout\": \"" << jsonEscape(evidence.localLayoutFileName) << "\",\n";
    out << "  \"localProject\": \"" << jsonEscape(evidence.localProject) << "\",\n";
    out << "  \"layoutStatus\": \"" << jsonEscape(evidence.layoutStatus) << "\",\n";
    out << "  \"liveStatePath\": \"" << jsonEscape(portablePath(evidence.liveStatePath)) << "\",\n";
    out << "  \"counts\": { \"runtimeLayers\": " << evidence.runtimeLayerCount
        << ", \"exportedLayers\": " << evidence.runtimeLayers.size()
        << ", \"materialResolved\": " << materialResolved
        << ", \"subimageResolved\": " << subimageResolved
        << ", \"timelineResolved\": " << timelineResolved
        << ", \"materialUnresolved\": " << (static_cast<int>(evidence.runtimeLayers.size()) - materialResolved)
        << " },\n";

    out << "  \"materials\": [\n";
    for (std::size_t index = 0; index < entries.size(); ++index)
    {
        const auto& entry = entries[index];
        out << "    { \"runtimePath\": \"" << jsonEscape(entry.runtimePath)
            << "\", \"runtimeScene\": \"" << jsonEscape(entry.runtimeSceneName)
            << "\", \"localScene\": \"" << jsonEscape(entry.localSceneName)
            << "\", \"layer\": \"" << jsonEscape(entry.layerName)
            << "\", \"cast\": \"" << jsonEscape(entry.castName)
            << "\", \"texture\": \"" << jsonEscape(entry.textureName)
            << "\", \"subimageIndex\": " << entry.subimageIndex
            << ", \"textureIndex\": " << entry.textureIndex
            << ", \"textureSize\": { \"width\": " << entry.textureWidth << ", \"height\": " << entry.textureHeight << " }"
            << ", \"source\": { \"x\": " << entry.sourceX << ", \"y\": " << entry.sourceY
            << ", \"width\": " << entry.sourceWidth << ", \"height\": " << entry.sourceHeight << " }"
            << ", \"destination\": { \"x\": " << entry.destinationX << ", \"y\": " << entry.destinationY
            << ", \"width\": " << entry.destinationWidth << ", \"height\": " << entry.destinationHeight << " }"
            << ", \"timeline\": \"" << jsonEscape(entry.timelineName)
            << "\", \"timelineFrame\": " << entry.timelineFrame
            << ", \"timelineFrameCount\": " << entry.timelineFrameCount
            << ", \"sgfxSlot\": \"" << jsonEscape(entry.sgfxSlotLabel)
            << "\", \"materialSourceStatus\": \"" << jsonEscape(entry.materialSourceStatus) << "\" }"
            << (index + 1 == entries.size() ? "\n" : ",\n");
    }
    out << "  ]\n";
    out << "}\n";
    return true;
}

void appendUniqueString(std::vector<std::string>& values, std::string_view value)
{
    if (value.empty())
        return;
    if (std::find(values.begin(), values.end(), value) == values.end())
        values.emplace_back(value);
}

[[nodiscard]] std::string sonicHudActivationEventForScene(std::string_view sceneName)
{
    if (sceneName.find("u_info") != std::string_view::npos)
        return "tutorial-hud-owner-path-ready";
    return "stage-hud-ready";
}

[[nodiscard]] SonicHudCompositorModel buildSonicHudCompositorModel(
    const CsdHudRuntimeSceneEvidence& evidence,
    const std::vector<CsdHudRuntimeMaterialEntry>& entries)
{
    SonicHudCompositorModel model;
    model.project = evidence.runtimeProject;
    model.liveStatePath = evidence.liveStatePath;
    model.sceneCount = evidence.runtimeSceneCount;
    model.runtimeLayerCount = evidence.runtimeLayerCount;
    model.exportedLayerCount = static_cast<int>(evidence.runtimeLayers.size());
    model.drawableLayerCount = static_cast<int>(std::count_if(
        entries.begin(),
        entries.end(),
        [](const CsdHudRuntimeMaterialEntry& entry)
        {
            return entry.subimageIndex >= 0 && entry.textureResolved && entry.sourceFits;
        }));
    model.structuralLayerCount = model.exportedLayerCount - model.drawableLayerCount;

    for (const auto& runtimeScene : evidence.runtimeScenes)
    {
        SonicHudCompositorScene scene;
        scene.runtimePath = runtimeScene.path;
        scene.sceneName = runtimeScene.sceneName;
        scene.localSceneName = localCsdSceneNameForRuntimeScene(runtimeScene.sceneName);
        scene.sgfxSlotLabel = sonicHudSlotLabelForScene(runtimeScene.sceneName);
        scene.activationEvent = sonicHudActivationEventForScene(runtimeScene.sceneName);
        scene.runtimeLayerCount = runtimeLayerCountForScene(evidence, runtimeScene.sceneName);
        scene.drawableLayerCount = materialResolvedCountForRuntimeScene(entries, runtimeScene.sceneName);
        scene.structuralLayerCount = scene.runtimeLayerCount - scene.drawableLayerCount;
        scene.castCount = runtimeScene.castCount;

        const auto firstEntry = std::find_if(
            entries.begin(),
            entries.end(),
            [&runtimeScene](const CsdHudRuntimeMaterialEntry& entry)
            {
                return entry.runtimeSceneName == runtimeScene.sceneName;
            });
        if (firstEntry != entries.end())
        {
            scene.timelineName = firstEntry->timelineName;
            scene.timelineFrame = firstEntry->timelineFrame;
            scene.timelineFrameCount = firstEntry->timelineFrameCount;
        }
        else
        {
            scene.timelineName = timelineAnimationNameForLocalScene(evidence.localLayoutFileName, scene.localSceneName);
        }

        for (const auto& entry : entries)
        {
            if (entry.runtimeSceneName == runtimeScene.sceneName && entry.textureResolved && entry.sourceFits)
                appendUniqueString(scene.textureNames, entry.textureName);
        }

        model.scenes.push_back(std::move(scene));
    }

    return model;
}

[[nodiscard]] bool writeSonicHudCompositorManifest(
    const SonicHudCompositorModel& model,
    const std::filesystem::path& exportPath)
{
    std::error_code error;
    std::filesystem::create_directories(exportPath.parent_path(), error);
    if (error)
        return false;

    std::ofstream out(exportPath, std::ios::binary);
    if (!out)
        return false;

    out << "{\n";
    out << "  \"phase\": 136,\n";
    out << "  \"target\": \"" << jsonEscape(model.target) << "\",\n";
    out << "  \"source\": \"" << jsonEscape(model.source) << "\",\n";
    out << "  \"project\": \"" << jsonEscape(model.project) << "\",\n";
    out << "  \"owner\": \"" << jsonEscape(model.owner) << "\",\n";
    out << "  \"ownerHook\": \"" << jsonEscape(model.ownerHook) << "\",\n";
    out << "  \"stateEvent\": \"" << jsonEscape(model.stateEvent) << "\",\n";
    out << "  \"referenceStatus\": \"" << jsonEscape(model.referenceStatus) << "\",\n";
    out << "  \"liveStatePath\": \"" << jsonEscape(portablePath(model.liveStatePath)) << "\",\n";
    out << "  \"counts\": { \"scenes\": " << model.sceneCount
        << ", \"runtimeLayers\": " << model.runtimeLayerCount
        << ", \"exportedLayers\": " << model.exportedLayerCount
        << ", \"drawableLayers\": " << model.drawableLayerCount
        << ", \"structuralLayers\": " << model.structuralLayerCount << " },\n";

    out << "  \"scenes\": [\n";
    for (std::size_t index = 0; index < model.scenes.size(); ++index)
    {
        const auto& scene = model.scenes[index];
        out << "    { \"path\": \"" << jsonEscape(scene.runtimePath)
            << "\", \"scene\": \"" << jsonEscape(scene.sceneName)
            << "\", \"localScene\": \"" << jsonEscape(scene.localSceneName)
            << "\", \"slot\": \"" << jsonEscape(scene.sgfxSlotLabel)
            << "\", \"activation\": \"" << jsonEscape(scene.activationEvent)
            << "\", \"runtimeLayers\": " << scene.runtimeLayerCount
            << ", \"drawableLayers\": " << scene.drawableLayerCount
            << ", \"structuralLayers\": " << scene.structuralLayerCount
            << ", \"timeline\": \"" << jsonEscape(scene.timelineName)
            << "\", \"timelineFrame\": " << scene.timelineFrame
            << ", \"timelineFrameCount\": " << scene.timelineFrameCount
            << ", \"textureCount\": " << scene.textureNames.size()
            << ", \"textures\": [";
        for (std::size_t textureIndex = 0; textureIndex < scene.textureNames.size(); ++textureIndex)
        {
            if (textureIndex != 0)
                out << ", ";
            out << "\"" << jsonEscape(scene.textureNames[textureIndex]) << "\"";
        }
        out << "] }" << (index + 1 == model.scenes.size() ? "\n" : ",\n");
    }
    out << "  ]\n";
    out << "}\n";
    return true;
}

[[nodiscard]] bool writeSonicHudReferenceCode(
    const SonicHudCompositorModel& model,
    const std::filesystem::path& exportPath)
{
    std::error_code error;
    std::filesystem::create_directories(exportPath.parent_path(), error);
    if (error)
        return false;

    std::ofstream out(exportPath, std::ios::binary);
    if (!out)
        return false;

    out << "#pragma once\n";
    out << "// Generated local-only by SWARD Phase 136 from live bridge + exact ui_playscreen CSD evidence.\n";
    out << "// This is readable reference architecture, not original SEGA source.\n\n";
    out << "namespace sward::recovered::sonic_hud\n{\n";
    out << "struct SonicHudSceneReference\n";
    out << "{\n";
    out << "    const char* scenePath;\n";
    out << "    const char* sgfxSlot;\n";
    out << "    const char* activationEvent;\n";
    out << "    const char* timeline;\n";
    out << "    int runtimeLayers;\n";
    out << "    int drawableLayers;\n";
    out << "};\n\n";
    out << "struct CHudSonicStageOwnerReference\n";
    out << "{\n";
    out << "    const char* ownerType;\n";
    out << "    const char* ownerHook;\n";
    out << "    const char* project;\n";
    out << "    int sceneCount;\n";
    out << "};\n\n";
    out << "inline constexpr CHudSonicStageOwnerReference kOwnerReference{\n";
    out << "    \"" << jsonEscape(model.owner) << "\",\n";
    out << "    \"" << jsonEscape(model.ownerHook) << "\",\n";
    out << "    \"" << jsonEscape(model.project) << "\",\n";
    out << "    " << model.sceneCount << ",\n";
    out << "};\n\n";
    out << "inline constexpr SonicHudSceneReference kNormalSonicHudScenes[] = {\n";
    for (const auto& scene : model.scenes)
    {
        out << "    { \"" << jsonEscape(scene.runtimePath)
            << "\", \"" << jsonEscape(scene.sgfxSlotLabel)
            << "\", \"" << jsonEscape(scene.activationEvent)
            << "\", \"" << jsonEscape(scene.timelineName)
            << "\", " << scene.runtimeLayerCount
            << ", " << scene.drawableLayerCount
            << " },\n";
    }
    out << "};\n";
    out << "} // namespace sward::recovered::sonic_hud\n";
    return true;
}

[[nodiscard]] std::optional<CLSID> imageEncoderClsid(const wchar_t* mimeType)
{
    UINT encoderCount = 0;
    UINT encoderBytes = 0;
    if (Gdiplus::GetImageEncodersSize(&encoderCount, &encoderBytes) != Gdiplus::Ok || encoderBytes == 0)
        return std::nullopt;

    std::vector<std::uint8_t> buffer(encoderBytes);
    auto* encoders = reinterpret_cast<Gdiplus::ImageCodecInfo*>(buffer.data());
    if (Gdiplus::GetImageEncoders(encoderCount, encoderBytes, encoders) != Gdiplus::Ok)
        return std::nullopt;

    for (UINT index = 0; index < encoderCount; ++index)
    {
        if (std::wcscmp(encoders[index].MimeType, mimeType) == 0)
            return encoders[index].Clsid;
    }

    return std::nullopt;
}

[[nodiscard]] bool saveBitmapAsBmp(Gdiplus::Bitmap& bitmap, const std::filesystem::path& path)
{
    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);
    if (error)
        return false;

    const auto encoder = imageEncoderClsid(L"image/bmp");
    if (!encoder)
        return false;

    return bitmap.Save(path.wstring().c_str(), &*encoder, nullptr) == Gdiplus::Ok;
}

[[nodiscard]] std::optional<std::string> findRuntimeEvidenceRecordForTarget(
    const std::string& manifestText,
    std::string_view target)
{
    for (const auto objectSpan : jsonObjectSpansInArray(manifestText))
    {
        if (jsonStringField(objectSpan, "target").value_or("") == target)
            return std::string(objectSpan);
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<std::filesystem::path> extractNativeBestBmpPathFromManifestText(
    const std::string& manifestText,
    std::string_view target)
{
    if (manifestText.empty())
        return std::nullopt;

    const auto targetRecord = findRuntimeEvidenceRecordForTarget(manifestText, target);
    if (targetRecord)
    {
        const auto summary = jsonObjectFieldSpan(*targetRecord, "nativeFrameSignalSummary");
        const auto bestPath = summary ? jsonStringField(*summary, "bestPath") : std::optional<std::string>{};
        if (bestPath && !bestPath->empty())
            return std::filesystem::path(*bestPath);
    }

    const std::string fieldNeedle = "\"target\"";
    const std::string valueNeedle = "\"" + std::string(target) + "\"";
    std::size_t offset = 0;
    while ((offset = manifestText.find(fieldNeedle, offset)) != std::string::npos)
    {
        const auto colonOffset = manifestText.find(':', offset + fieldNeedle.size());
        if (colonOffset == std::string::npos)
            break;

        const auto valueOffset = skipJsonWhitespace(manifestText, colonOffset + 1);
        const auto value = parseJsonStringAt(manifestText, valueOffset);
        if (value && *value == valueNeedle.substr(1, valueNeedle.size() - 2))
        {
            const auto summaryOffset = manifestText.find("\"nativeFrameSignalSummary\"", valueOffset);
            const auto bestPathOffset = summaryOffset == std::string::npos
                ? std::string::npos
                : manifestText.find("\"bestPath\"", summaryOffset);
            const auto bestPathColon = bestPathOffset == std::string::npos
                ? std::string::npos
                : manifestText.find(':', bestPathOffset);
            if (bestPathColon != std::string::npos)
            {
                const auto bestPathValueOffset = skipJsonWhitespace(manifestText, bestPathColon + 1);
                if (const auto fallbackBestPath = parseJsonStringAt(manifestText, bestPathValueOffset))
                    return std::filesystem::path(*fallbackBestPath);
            }
        }
        offset = colonOffset + 1;
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<std::filesystem::path> findNativeBestBmpPathForTarget(std::string_view target)
{
    std::optional<std::filesystem::path> bestPath;
    std::filesystem::file_time_type bestWriteTime{};
    bool hasBestWriteTime = false;

    for (const auto& root : repoRootCandidates())
    {
        std::error_code error;
        const auto evidenceRoot = root / "out" / "ui_lab_runtime_evidence";
        if (!std::filesystem::is_directory(evidenceRoot, error))
            continue;

        for (const auto& entry : std::filesystem::recursive_directory_iterator(evidenceRoot, error))
        {
            if (error)
                break;
            if (!entry.is_regular_file(error) || entry.path().filename() != "capture_manifest.json")
                continue;

            const auto manifestText = readTextFile(entry.path());
            const auto candidate = extractNativeBestBmpPathFromManifestText(manifestText, target);
            if (!candidate)
                continue;

            const auto writeTime = std::filesystem::last_write_time(entry.path(), error);
            if (!bestPath || !hasBestWriteTime || (!error && writeTime > bestWriteTime))
            {
                bestPath = *candidate;
                if (!error)
                {
                    bestWriteTime = writeTime;
                    hasBestWriteTime = true;
                }
            }
        }
    }

    return bestPath;
}

[[nodiscard]] Gdiplus::Color bitmapPixelNearest(Gdiplus::Bitmap& bitmap, int sampleX, int sampleY, int gridWidth, int gridHeight)
{
    const int x = std::clamp(
        static_cast<int>(std::llround((static_cast<double>(sampleX) + 0.5) * static_cast<double>(bitmap.GetWidth()) / static_cast<double>(gridWidth))),
        0,
        std::max(0, static_cast<int>(bitmap.GetWidth()) - 1));
    const int y = std::clamp(
        static_cast<int>(std::llround((static_cast<double>(sampleY) + 0.5) * static_cast<double>(bitmap.GetHeight()) / static_cast<double>(gridHeight))),
        0,
        std::max(0, static_cast<int>(bitmap.GetHeight()) - 1));

    Gdiplus::Color color;
    bitmap.GetPixel(x, y, &color);
    return color;
}

[[nodiscard]] Gdiplus::Color bitmapPixelNearest(
    Gdiplus::Bitmap& bitmap,
    int sampleX,
    int sampleY,
    int gridWidth,
    int gridHeight,
    int cropX,
    int cropY,
    int cropWidth,
    int cropHeight)
{
    const int safeCropWidth = std::max(1, cropWidth);
    const int safeCropHeight = std::max(1, cropHeight);
    const int x = std::clamp(
        cropX + static_cast<int>(std::llround((static_cast<double>(sampleX) + 0.5) * static_cast<double>(safeCropWidth) / static_cast<double>(gridWidth))),
        0,
        std::max(0, static_cast<int>(bitmap.GetWidth()) - 1));
    const int y = std::clamp(
        cropY + static_cast<int>(std::llround((static_cast<double>(sampleY) + 0.5) * static_cast<double>(safeCropHeight) / static_cast<double>(gridHeight))),
        0,
        std::max(0, static_cast<int>(bitmap.GetHeight()) - 1));

    Gdiplus::Color color;
    bitmap.GetPixel(x, y, &color);
    return color;
}

[[nodiscard]] Gdiplus::Color bitmapPixelAtClamped(Gdiplus::Bitmap& bitmap, int x, int y)
{
    const int safeX = std::clamp(x, 0, std::max(0, static_cast<int>(bitmap.GetWidth()) - 1));
    const int safeY = std::clamp(y, 0, std::max(0, static_cast<int>(bitmap.GetHeight()) - 1));
    Gdiplus::Color color;
    bitmap.GetPixel(safeX, safeY, &color);
    return color;
}

void nativeAlignmentCrop(Gdiplus::Bitmap& native, BitmapComparisonStats& stats)
{
    const int nativeWidth = static_cast<int>(native.GetWidth());
    const int nativeHeight = static_cast<int>(native.GetHeight());
    stats.nativeAlignmentCropX = 0;
    stats.nativeAlignmentCropY = 0;
    stats.nativeAlignmentCropWidth = nativeWidth;
    stats.nativeAlignmentCropHeight = nativeHeight;

    if (nativeWidth <= 0 || nativeHeight <= 0)
        return;

    constexpr double kDesignAspect = static_cast<double>(kDesignWidth) / static_cast<double>(kDesignHeight);
    const double nativeAspect = static_cast<double>(nativeWidth) / static_cast<double>(nativeHeight);
    if (nativeAspect > kDesignAspect)
    {
        stats.nativeAlignmentCropWidth = std::max(1, static_cast<int>(std::llround(static_cast<double>(nativeHeight) * kDesignAspect)));
        stats.nativeAlignmentCropX = std::max(0, (nativeWidth - stats.nativeAlignmentCropWidth) / 2);
    }
    else if (nativeAspect < kDesignAspect)
    {
        stats.nativeAlignmentCropHeight = std::max(1, static_cast<int>(std::llround(static_cast<double>(nativeWidth) / kDesignAspect)));
        stats.nativeAlignmentCropY = std::max(0, (nativeHeight - stats.nativeAlignmentCropHeight) / 2);
    }
}

[[nodiscard]] std::pair<double, int> computeNativeCropDelta(
    Gdiplus::Bitmap& rendered,
    Gdiplus::Bitmap& native,
    const BitmapComparisonStats& stats,
    int cropX,
    int cropY,
    int cropWidth,
    int cropHeight)
{
    std::uint64_t totalAbs = 0;
    int maxAbs = 0;
    int sampleCount = 0;
    for (int y = 0; y < stats.sampleGridHeight; ++y)
    {
        for (int x = 0; x < stats.sampleGridWidth; ++x)
        {
            const auto left = bitmapPixelNearest(rendered, x, y, stats.sampleGridWidth, stats.sampleGridHeight);
            const auto right = bitmapPixelNearest(
                native,
                x,
                y,
                stats.sampleGridWidth,
                stats.sampleGridHeight,
                cropX,
                cropY,
                cropWidth,
                cropHeight);
            const int dr = std::abs(static_cast<int>(left.GetR()) - static_cast<int>(right.GetR()));
            const int dg = std::abs(static_cast<int>(left.GetG()) - static_cast<int>(right.GetG()));
            const int db = std::abs(static_cast<int>(left.GetB()) - static_cast<int>(right.GetB()));
            totalAbs += static_cast<std::uint64_t>(dr + dg + db);
            maxAbs = std::max(maxAbs, std::max({ dr, dg, db }));
            ++sampleCount;
        }
    }

    const double meanAbsRgb = sampleCount == 0 ? 0.0 : static_cast<double>(totalAbs) / static_cast<double>(sampleCount * 3);
    return { meanAbsRgb, maxAbs };
}

[[nodiscard]] CsdNativeFrameRegistration findBestNativeFrameRegistration(
    Gdiplus::Bitmap& rendered,
    Gdiplus::Bitmap& native,
    const BitmapComparisonStats& stats)
{
    CsdNativeFrameRegistration registration;
    registration.cropX = stats.nativeAlignmentCropX;
    registration.cropY = stats.nativeAlignmentCropY;
    registration.cropWidth = stats.nativeAlignmentCropWidth;
    registration.cropHeight = stats.nativeAlignmentCropHeight;

    const auto baseDelta = computeNativeCropDelta(
        rendered,
        native,
        stats,
        registration.cropX,
        registration.cropY,
        registration.cropWidth,
        registration.cropHeight);
    registration.baseMeanAbsRgb = baseDelta.first;
    registration.bestMeanAbsRgb = baseDelta.first;
    registration.bestMaxAbsRgb = baseDelta.second;

    const int nativeWidth = static_cast<int>(native.GetWidth());
    const int nativeHeight = static_cast<int>(native.GetHeight());
    const int maxShiftX = std::min(32, std::max(0, (nativeWidth - registration.cropWidth) / 2));
    const int maxShiftY = std::min(32, std::max(0, (nativeHeight - registration.cropHeight) / 2));
    const int stepX = std::max(1, maxShiftX / 2);
    const int stepY = std::max(1, maxShiftY / 2);
    const int baseCropX = registration.cropX;
    const int baseCropY = registration.cropY;

    for (int yOffset = -maxShiftY; yOffset <= maxShiftY; yOffset += stepY)
    {
        for (int xOffset = -maxShiftX; xOffset <= maxShiftX; xOffset += stepX)
        {
            const int candidateX = std::clamp(baseCropX + xOffset, 0, std::max(0, nativeWidth - registration.cropWidth));
            const int candidateY = std::clamp(baseCropY + yOffset, 0, std::max(0, nativeHeight - registration.cropHeight));
            ++registration.candidateCount;
            const auto delta = computeNativeCropDelta(
                rendered,
                native,
                stats,
                candidateX,
                candidateY,
                registration.cropWidth,
                registration.cropHeight);
            if (delta.first < registration.bestMeanAbsRgb)
            {
                registration.bestMeanAbsRgb = delta.first;
                registration.bestMaxAbsRgb = delta.second;
                registration.offsetX = candidateX - baseCropX;
                registration.offsetY = candidateY - baseCropY;
                registration.cropX = candidateX;
                registration.cropY = candidateY;
            }
        }
    }

    return registration;
}

[[nodiscard]] CsdFullFrameDeltaStats computeFullFrameDeltaStats(
    Gdiplus::Bitmap& rendered,
    Gdiplus::Bitmap& native,
    const BitmapComparisonStats& alignmentStats,
    const std::filesystem::path& diffFramePath)
{
    CsdFullFrameDeltaStats stats;
    std::uint64_t totalAbs = 0;
    std::vector<std::uint32_t> diffPixels(static_cast<std::size_t>(kDesignWidth) * static_cast<std::size_t>(kDesignHeight), 0xFF000000);

    for (int y = 0; y < kDesignHeight; ++y)
    {
        for (int x = 0; x < kDesignWidth; ++x)
        {
            const auto left = bitmapPixelNearest(rendered, x, y, kDesignWidth, kDesignHeight);
            const auto right = bitmapPixelNearest(
                native,
                x,
                y,
                kDesignWidth,
                kDesignHeight,
                alignmentStats.nativeAlignmentCropX,
                alignmentStats.nativeAlignmentCropY,
                alignmentStats.nativeAlignmentCropWidth,
                alignmentStats.nativeAlignmentCropHeight);

            const int dr = std::abs(static_cast<int>(left.GetR()) - static_cast<int>(right.GetR()));
            const int dg = std::abs(static_cast<int>(left.GetG()) - static_cast<int>(right.GetG()));
            const int db = std::abs(static_cast<int>(left.GetB()) - static_cast<int>(right.GetB()));
            const int channelDelta = dr + dg + db;
            totalAbs += static_cast<std::uint64_t>(channelDelta);
            stats.maxAbsRgb = std::max(stats.maxAbsRgb, std::max({ dr, dg, db }));
            if (channelDelta == 0)
                ++stats.exactMatchPixels;
            if (channelDelta > 24)
                ++stats.significantDeltaPixels;
            if (left.GetR() != 0 || left.GetG() != 0 || left.GetB() != 0)
                ++stats.renderNonBlackPixels;
            if (right.GetR() != 0 || right.GetG() != 0 || right.GetB() != 0)
                ++stats.nativeNonBlackPixels;

            diffPixels[static_cast<std::size_t>(y) * static_cast<std::size_t>(kDesignWidth) + static_cast<std::size_t>(x)] =
                packArgbPixel(CsdColorRgba{
                    static_cast<std::uint8_t>(dr),
                    static_cast<std::uint8_t>(dg),
                    static_cast<std::uint8_t>(db),
                    255,
                });
        }
    }

    stats.pixelCount = kDesignWidth * kDesignHeight;
    stats.meanAbsRgb = stats.pixelCount == 0 ? 0.0 : static_cast<double>(totalAbs) / static_cast<double>(stats.pixelCount * 3);
    stats.renderNonBlackRatio = stats.pixelCount == 0 ? 0.0 : static_cast<double>(stats.renderNonBlackPixels) / static_cast<double>(stats.pixelCount);
    stats.nativeNonBlackRatio = stats.pixelCount == 0 ? 0.0 : static_cast<double>(stats.nativeNonBlackPixels) / static_cast<double>(stats.pixelCount);

    auto diffBitmap = bitmapFromArgbPixels(kDesignWidth, kDesignHeight, diffPixels);
    stats.computed = diffBitmap && saveBitmapAsBmp(*diffBitmap, diffFramePath);
    return stats;
}

[[nodiscard]] CsdUiLayerMaskStats computeUiLayerDeltaStats(
    Gdiplus::Bitmap& rendered,
    Gdiplus::Bitmap& native,
    const BitmapComparisonStats& alignmentStats,
    const std::vector<std::uint8_t>& coverageMask,
    const std::filesystem::path& uiLayerDiffFramePath)
{
    CsdUiLayerMaskStats stats;
    std::uint64_t totalAbs = 0;
    std::vector<std::uint32_t> diffPixels(static_cast<std::size_t>(kDesignWidth) * static_cast<std::size_t>(kDesignHeight), 0xFF000000);

    for (int y = 0; y < kDesignHeight; ++y)
    {
        for (int x = 0; x < kDesignWidth; ++x)
        {
            const auto pixelIndex = static_cast<std::size_t>(y) * static_cast<std::size_t>(kDesignWidth) + static_cast<std::size_t>(x);
            if (pixelIndex >= coverageMask.size() || coverageMask[pixelIndex] == 0)
                continue;

            const auto left = bitmapPixelAtClamped(rendered, x, y);
            const auto right = bitmapPixelNearest(
                native,
                x,
                y,
                kDesignWidth,
                kDesignHeight,
                alignmentStats.nativeAlignmentCropX,
                alignmentStats.nativeAlignmentCropY,
                alignmentStats.nativeAlignmentCropWidth,
                alignmentStats.nativeAlignmentCropHeight);

            const int dr = std::abs(static_cast<int>(left.GetR()) - static_cast<int>(right.GetR()));
            const int dg = std::abs(static_cast<int>(left.GetG()) - static_cast<int>(right.GetG()));
            const int db = std::abs(static_cast<int>(left.GetB()) - static_cast<int>(right.GetB()));
            const int channelDelta = dr + dg + db;
            totalAbs += static_cast<std::uint64_t>(channelDelta);
            stats.maxAbsRgb = std::max(stats.maxAbsRgb, std::max({ dr, dg, db }));
            if (channelDelta == 0)
                ++stats.exactMatchPixels;
            if (channelDelta > 24)
                ++stats.significantDeltaPixels;
            ++stats.maskedPixelCount;

            diffPixels[pixelIndex] = packArgbPixel(CsdColorRgba{
                static_cast<std::uint8_t>(dr),
                static_cast<std::uint8_t>(dg),
                static_cast<std::uint8_t>(db),
                255,
            });
        }
    }

    stats.pixelCount = kDesignWidth * kDesignHeight;
    stats.meanAbsRgb = stats.maskedPixelCount == 0 ? 0.0 : static_cast<double>(totalAbs) / static_cast<double>(stats.maskedPixelCount * 3);
    stats.maskCoverageRatio = stats.pixelCount == 0 ? 0.0 : static_cast<double>(stats.maskedPixelCount) / static_cast<double>(stats.pixelCount);
    stats.fullFrameMeanAbsRgb = alignmentStats.fullFrame.meanAbsRgb;
    stats.fullFrameDeltaReduction = alignmentStats.fullFrame.meanAbsRgb - stats.meanAbsRgb;

    auto diffBitmap = bitmapFromArgbPixels(kDesignWidth, kDesignHeight, diffPixels);
    stats.computed = stats.maskedPixelCount != 0 && diffBitmap && saveBitmapAsBmp(*diffBitmap, uiLayerDiffFramePath);
    return stats;
}

[[nodiscard]] BitmapSignalStats computeBitmapSignalStats(Gdiplus::Bitmap& bitmap)
{
    BitmapSignalStats stats;
    stats.loaded = bitmap.GetLastStatus() == Gdiplus::Ok && bitmap.GetWidth() > 0 && bitmap.GetHeight() > 0;
    if (!stats.loaded)
        return stats;

    stats.width = static_cast<int>(bitmap.GetWidth());
    stats.height = static_cast<int>(bitmap.GetHeight());
    constexpr int kGridWidth = 64;
    constexpr int kGridHeight = 36;
    for (int y = 0; y < kGridHeight; ++y)
    {
        for (int x = 0; x < kGridWidth; ++x)
        {
            const auto color = bitmapPixelNearest(bitmap, x, y, kGridWidth, kGridHeight);
            stats.rgbSum += static_cast<std::uint64_t>(color.GetR()) + color.GetG() + color.GetB();
            stats.alphaSum += color.GetA();
            if (color.GetR() != 0 || color.GetG() != 0 || color.GetB() != 0)
                ++stats.rgbNonBlack;
        }
    }

    return stats;
}

[[nodiscard]] BitmapComparisonStats computeBitmapComparisonStats(
    Gdiplus::Bitmap& rendered,
    const std::optional<std::filesystem::path>& nativePath,
    const std::filesystem::path& diffFramePath,
    const std::vector<std::uint8_t>& coverageMask,
    const std::filesystem::path& uiLayerDiffFramePath)
{
    BitmapComparisonStats stats;
    stats.rendered = computeBitmapSignalStats(rendered);
    if (!nativePath)
        return stats;

    auto native = std::unique_ptr<Gdiplus::Bitmap>(Gdiplus::Bitmap::FromFile(nativePath->wstring().c_str(), FALSE));
    if (!native || native->GetLastStatus() != Gdiplus::Ok)
        return stats;

    stats.nativeFound = true;
    stats.native = computeBitmapSignalStats(*native);
    nativeAlignmentCrop(*native, stats);
    const auto registration = findBestNativeFrameRegistration(rendered, *native, stats);
    stats.nativeAlignmentCropX = registration.cropX;
    stats.nativeAlignmentCropY = registration.cropY;
    stats.nativeAlignmentCropWidth = registration.cropWidth;
    stats.nativeAlignmentCropHeight = registration.cropHeight;
    stats.registrationOffsetX = registration.offsetX;
    stats.registrationOffsetY = registration.offsetY;
    stats.registrationCandidateCount = registration.candidateCount;
    stats.registrationBaseMeanAbsRgb = registration.baseMeanAbsRgb;
    stats.meanAbsRgb = registration.bestMeanAbsRgb;
    stats.maxAbsRgb = registration.bestMaxAbsRgb;
    stats.sampleCount = stats.sampleGridWidth * stats.sampleGridHeight;
    stats.fullFrame = computeFullFrameDeltaStats(rendered, *native, stats, diffFramePath);
    stats.uiLayerDelta = computeUiLayerDeltaStats(rendered, *native, stats, coverageMask, uiLayerDiffFramePath);
    return stats;
}

[[nodiscard]] std::vector<CsdDrawableCommand> timelineSampledCommands(
    const CsdDrawableScene& scene,
    const CsdTimelinePlayback* playback,
    std::size_t& sampledCommandCount)
{
    std::vector<CsdDrawableCommand> commands = scene.commands;
    sampledCommandCount = 0;
    if (!playback)
        return commands;

    for (auto& command : commands)
    {
        for (const auto& sample : playback->samples)
        {
            const auto sampled = applyCsdTimelineToDrawableCommand(command, sample);
            if (!sampled)
                continue;

            command = *sampled;
            ++sampledCommandCount;
        }
        for (const auto& sample : playback->packedRgbaSamples)
        {
            const auto sampled = applyCsdPackedRgbaTimelineToDrawableCommand(command, sample);
            if (!sampled)
                continue;

            command = *sampled;
            ++sampledCommandCount;
        }
    }

    return commands;
}

[[nodiscard]] std::unique_ptr<Gdiplus::Bitmap> renderCsdOffscreenFrameWithCoverageMask(
    const CsdDrawableScene& scene,
    const CsdTimelinePlayback* playback,
    SwardSuUiAssetRenderer& renderer,
    std::size_t& sampledCommandCount,
    CsdSoftwareRenderStats& renderStats,
    std::vector<std::uint8_t>& coverageMask)
{
    std::vector<std::uint32_t> canvasPixels(static_cast<std::size_t>(kDesignWidth) * static_cast<std::size_t>(kDesignHeight), 0xFF000000);
    coverageMask.assign(static_cast<std::size_t>(kDesignWidth) * static_cast<std::size_t>(kDesignHeight), 0);
    const auto commands = timelineSampledCommands(scene, playback, sampledCommandCount);
    for (const auto& command : commands)
        (void)drawCsdDrawableCommandSoftware(canvasPixels, &coverageMask, kDesignWidth, kDesignHeight, renderer, command, renderStats);

    return bitmapFromArgbPixels(kDesignWidth, kDesignHeight, canvasPixels);
}

[[nodiscard]] std::unique_ptr<Gdiplus::Bitmap> renderCsdOffscreenFrame(
    const CsdDrawableScene& scene,
    const CsdTimelinePlayback* playback,
    SwardSuUiAssetRenderer& renderer,
    std::size_t& sampledCommandCount,
    CsdSoftwareRenderStats& renderStats)
{
    std::vector<std::uint8_t> coverageMask;
    return renderCsdOffscreenFrameWithCoverageMask(scene, playback, renderer, sampledCommandCount, renderStats, coverageMask);
}

[[nodiscard]] std::string projectNameFromLayoutFileName(std::string_view layoutFileName)
{
    std::string project(layoutFileName);
    const auto slash = project.find_last_of("/\\");
    if (slash != std::string::npos)
        project = project.substr(slash + 1);
    const auto dot = project.find_last_of('.');
    if (dot != std::string::npos)
        project = project.substr(0, dot);
    return project;
}

[[nodiscard]] std::string sceneLeafNameFromRuntimePath(std::string_view runtimePath, std::string_view runtimeProject)
{
    const std::string path(runtimePath);
    const std::string projectPrefix = std::string(runtimeProject) + "/";
    if (path.starts_with(projectPrefix))
        return path.substr(projectPrefix.size());
    const auto slash = path.find('/');
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

[[nodiscard]] std::string layerLeafNameFromRuntimePath(std::string_view runtimePath)
{
    const std::string path(runtimePath);
    const auto slash = path.find_last_of('/');
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

[[nodiscard]] std::string runtimeSceneNameForLayerPath(
    std::string_view layerPath,
    const std::vector<CsdHudRuntimeSceneEntry>& runtimeScenes)
{
    std::string bestScene;
    std::size_t bestLength = 0;
    const std::string path(layerPath);
    for (const auto& scene : runtimeScenes)
    {
        const std::string prefix = scene.path + "/";
        if ((path == scene.path || path.starts_with(prefix)) && scene.path.size() > bestLength)
        {
            bestScene = scene.sceneName;
            bestLength = scene.path.size();
        }
    }
    return bestScene;
}

[[nodiscard]] int runtimeLayerCountForScene(
    const CsdHudRuntimeSceneEvidence& evidence,
    std::string_view sceneName)
{
    int count = 0;
    for (const auto& layer : evidence.runtimeLayers)
    {
        if (layer.sceneName == sceneName)
            ++count;
    }
    return count;
}

[[nodiscard]] bool layoutEvidenceContainsFile(std::string_view layoutFileName)
{
    return loadCsdPipelineEvidence(layoutFileName).has_value();
}

[[nodiscard]] std::string sonicHudSlotLabelForScene(std::string_view sceneName)
{
    if (sceneName.find("so_speed_gauge") != std::string_view::npos)
        return "speed_gauge";
    if (sceneName.find("so_ringenagy_gauge") != std::string_view::npos)
        return "energy_gauge";
    if (sceneName.find("ring_count") != std::string_view::npos || sceneName.find("ring_get") != std::string_view::npos)
        return "ring_counter";
    if (sceneName.find("gauge_frame") != std::string_view::npos)
        return "side_panel";
    if (sceneName.find("u_info") != std::string_view::npos)
        return "prompt_strip";
    if (sceneName.find("player_count") != std::string_view::npos)
        return "life_icon";
    if (sceneName.find("time_count") != std::string_view::npos)
        return "timer";
    if (sceneName.find("score_count") != std::string_view::npos)
        return "score_counter";
    if (sceneName.find("exp_count") != std::string_view::npos)
        return "experience_counter";
    if (sceneName.find("medal_get") != std::string_view::npos)
        return "medal_counter";
    if (sceneName.find("speed_count") != std::string_view::npos)
        return "speed_readout";
    return "unmapped_runtime_scene";
}

[[nodiscard]] std::optional<std::filesystem::path> latestLiveStatePathForTarget(std::string_view target)
{
    std::optional<std::filesystem::path> bestPath;
    std::filesystem::file_time_type bestWriteTime{};
    bool hasBestWriteTime = false;

    for (const auto& root : repoRootCandidates())
    {
        std::error_code error;
        const auto evidenceRoot = root / "out" / "ui_lab_runtime_evidence";
        if (!std::filesystem::is_directory(evidenceRoot, error))
            continue;

        for (const auto& entry : std::filesystem::recursive_directory_iterator(evidenceRoot, error))
        {
            if (error)
                break;
            if (!entry.is_regular_file(error) || entry.path().filename() != "ui_lab_live_state.json")
                continue;
            if (entry.path().parent_path().filename().string() != target)
                continue;

            const auto manifestPath = entry.path().parent_path().parent_path() / "capture_manifest.json";
            if (std::filesystem::is_regular_file(manifestPath, error))
            {
                const std::string manifestText = readTextFile(manifestPath);
                if (const auto evidenceReady = jsonBoolField(manifestText, "evidenceReady"); evidenceReady && !*evidenceReady)
                    continue;
            }

            const auto writeTime = std::filesystem::last_write_time(entry.path(), error);
            if (!bestPath || !hasBestWriteTime || (!error && writeTime > bestWriteTime))
            {
                bestPath = entry.path();
                if (!error)
                {
                    bestWriteTime = writeTime;
                    hasBestWriteTime = true;
                }
            }
        }
    }

    return bestPath;
}

[[nodiscard]] std::optional<std::filesystem::path> findLatestFrontendLiveStatePath(std::string_view target)
{
    return latestLiveStatePathForTarget(target);
}

[[nodiscard]] std::string discoverFrontendLiveBridgeName(std::string_view target)
{
    constexpr std::string_view defaultPipeName = "sward_ui_lab_live";
    const auto liveStatePath = findLatestFrontendLiveStatePath(target);
    if (!liveStatePath)
        return std::string(defaultPipeName);

    const std::string text = readTextFile(*liveStatePath);
    if (text.empty())
        return std::string(defaultPipeName);

    if (const auto liveBridge = jsonObjectFieldSpan(text, "liveBridge"))
    {
        const auto name = jsonStringField(*liveBridge, "name");
        if (name && !name->empty())
            return *name;
    }

    return std::string(defaultPipeName);
}

[[nodiscard]] FrontendLiveBridgeProbeResult queryUiLabLiveBridgeCommand(
    std::string_view pipeName,
    std::string_view commandText,
    DWORD timeoutMilliseconds)
{
    FrontendLiveBridgeProbeResult result;
    result.attempted = true;
    result.pipeName = pipeName.empty() ? "sward_ui_lab_live" : std::string(pipeName);
    result.command = commandText.empty() ? "state" : std::string(commandText);

    const std::string pipePath = "\\\\.\\pipe\\" + result.pipeName;
    std::vector<char> response(1024 * 1024, '\0');
    DWORD bytesRead = 0;
    const BOOL ok = CallNamedPipeA(
        pipePath.c_str(),
        result.command.data(),
        static_cast<DWORD>(result.command.size()),
        response.data(),
        static_cast<DWORD>(response.size() - 1),
        &bytesRead,
        timeoutMilliseconds);

    if (!ok)
    {
        const DWORD errorCode = GetLastError();
        std::ostringstream error;
        error << "call-failed-" << errorCode;
        result.error = error.str();
        return result;
    }

    result.connected = true;
    response[std::min<std::size_t>(bytesRead, response.size() - 1)] = '\0';
    result.responseJson.assign(response.data(), bytesRead);
    if (result.responseJson.empty())
        result.error = "empty-response";

    return result;
}

[[nodiscard]] FrontendLiveBridgeProbeResult queryUiLabLiveBridgeState(
    std::string_view pipeName,
    DWORD timeoutMilliseconds)
{
    return queryUiLabLiveBridgeCommand(pipeName, "state", timeoutMilliseconds);
}

[[nodiscard]] FrontendLiveBridgeProbeResult queryUiLabLiveBridgeUiOracle(
    std::string_view pipeName,
    DWORD timeoutMilliseconds)
{
    return queryUiLabLiveBridgeCommand(pipeName, "ui-oracle", timeoutMilliseconds);
}

[[nodiscard]] FrontendLiveBridgeProbeResult queryUiLabLiveBridgeUiDrawList(
    std::string_view pipeName,
    DWORD timeoutMilliseconds)
{
    return queryUiLabLiveBridgeCommand(pipeName, "ui-draw-list", timeoutMilliseconds);
}

[[nodiscard]] FrontendLiveBridgeProbeResult queryUiLabLiveBridgeGpuSubmit(
    std::string_view pipeName,
    DWORD timeoutMilliseconds)
{
    return queryUiLabLiveBridgeCommand(pipeName, "ui-gpu-submit", timeoutMilliseconds);
}

[[nodiscard]] FrontendLiveBridgeProbeResult queryUiLabLiveBridgeMaterialCorrelation(
    std::string_view pipeName,
    DWORD timeoutMilliseconds)
{
    return queryUiLabLiveBridgeCommand(pipeName, "ui-material-correlation", timeoutMilliseconds);
}

[[nodiscard]] FrontendLiveBridgeProbeResult queryUiLabLiveBridgeBackendResolved(
    std::string_view pipeName,
    DWORD timeoutMilliseconds)
{
    return queryUiLabLiveBridgeCommand(pipeName, "ui-backend-resolved", timeoutMilliseconds);
}

[[nodiscard]] std::string frontendAlignmentCursorOwnerFromLiveState(
    const sward::ui_runtime::FrontendScreenPolicy& policy,
    std::string_view text)
{
    if (policy.screenId == "title-menu" || policy.screenId == "title-options")
    {
        const auto title = jsonObjectFieldSpan(text, "title");
        const int cursor = static_cast<int>(title ? jsonNumberField(*title, "menuCursor").value_or(0.0) : 0.0);
        std::ostringstream out;
        out << (policy.screenId == "title-options" ? "CTitleStateMenu/options" : "CTitleStateMenu")
            << "/menu_cursor=" << cursor;
        return out.str();
    }

    if (policy.screenId == "loading")
    {
        std::string label;
        int displayType = static_cast<int>(jsonNumberField(text, "loadingDisplayType").value_or(0.0));
        if (const auto typedInspectors = jsonObjectFieldSpan(text, "typedInspectors"))
        {
            if (const auto loading = jsonObjectFieldSpan(*typedInspectors, "loading"))
            {
                displayType = static_cast<int>(jsonNumberField(*loading, "loadingDisplayType").value_or(displayType));
                label = jsonStringField(*loading, "loadingDisplayTypeLabel").value_or("");
            }
        }

        std::ostringstream out;
        out << "LoadingDisplay/display_type=" << displayType;
        if (!label.empty())
            out << "/label=" << label;
        return out.str();
    }

    if (policy.screenId == "pause")
    {
        std::string menu = "unknown";
        std::string status = "unknown";
        std::string transition = "unknown";
        bool visible = false;
        if (const auto typedInspectors = jsonObjectFieldSpan(text, "typedInspectors"))
        {
            if (const auto pauseGeneralSave = jsonObjectFieldSpan(*typedInspectors, "pauseGeneralSave"))
            {
                if (const auto pause = jsonObjectFieldSpan(*pauseGeneralSave, "pause"))
                {
                    menu = jsonStringField(*pause, "pauseMenuLabel").value_or(menu);
                    status = jsonStringField(*pause, "pauseStatusLabel").value_or(status);
                    transition = jsonStringField(*pause, "pauseTransitionLabel").value_or(transition);
                    visible = jsonBoolField(*pause, "pauseVisible").value_or(false);
                }
            }
        }

        std::ostringstream out;
        out << "CHudPause/menu=" << menu
            << "/status=" << status
            << "/transition=" << transition
            << "/visible=" << (visible ? 1 : 0);
        return out.str();
    }

    return defaultFrontendRuntimeAlignment(policy).cursorOwner;
}

[[nodiscard]] bool frontendAlignmentReadyFromLiveState(
    const sward::ui_runtime::FrontendScreenPolicy& policy,
    std::string_view text)
{
    const std::string nativeStatus = jsonStringField(text, "nativeCaptureStatus").value_or("");
    const bool nativeComplete = nativeStatus == "complete";
    const bool targetObserved = jsonBoolField(text, "targetCsdObserved").value_or(false);
    const std::string route = jsonStringField(text, "route").value_or("");
    const std::string stageReadyEvent = jsonStringField(text, "stageReadyEvent").value_or("");

    if (policy.screenId == "title-menu")
        return jsonBoolField(text, "titleMenuVisualReady").value_or(false) && nativeComplete;
    if (policy.screenId == "loading")
        return jsonBoolField(text, "loadingDisplayActive").value_or(false) && nativeComplete;
    if (policy.screenId == "title-options")
        return targetObserved && nativeComplete;
    if (policy.screenId == "pause")
        return nativeComplete && (stageReadyEvent == "pause-ready" || route.find("pause target ready") != std::string::npos);

    return targetObserved && nativeComplete;
}

[[nodiscard]] FrontendLiveStateAlignmentEvidence makeDefaultFrontendRuntimeAlignmentEvidence(
    const sward::ui_runtime::FrontendScreenPolicy& policy)
{
    FrontendLiveStateAlignmentEvidence evidence;
    evidence.alignment = defaultFrontendRuntimeAlignment(policy);
    evidence.alignment.source = "frontend_screen_reference";
    evidence.fieldStatus =
        policy.screenId
        + ":active_screen=policy:active_scenes=policy:motion=policy:frame=policy:cursor_owner=policy:transition=policy:input_lock=policy";
    return evidence;
}

void applyFrontendRuntimeAlignmentFromLiveStateJson(
    FrontendLiveStateAlignmentEvidence& evidence,
    const sward::ui_runtime::FrontendScreenPolicy& policy,
    std::string_view text,
    std::string_view source)
{
    evidence.found = true;
    evidence.alignment.screenId = jsonStringField(text, "target").value_or(policy.screenId);
    evidence.alignment.activeScenes.clear();
    for (const auto& scene : policy.scenes)
        evidence.alignment.activeScenes.push_back(scene.sceneName);
    evidence.route = jsonStringField(text, "route").value_or(evidence.alignment.activeMotionName);
    evidence.nativeCaptureStatus = jsonStringField(text, "nativeCaptureStatus").value_or("");
    evidence.alignment.activeMotionName = evidence.route.empty() ? evidence.alignment.activeMotionName : evidence.route;
    evidence.alignment.activeFrame = static_cast<int>(jsonNumberField(text, "frame").value_or(evidence.alignment.activeFrame));
    evidence.alignment.cursorOwner = frontendAlignmentCursorOwnerFromLiveState(policy, text);
    evidence.alignment.transitionBand = policy.transitionBand;
    evidence.alignment.inputLockState = frontendAlignmentReadyFromLiveState(policy, text)
        ? ("released:" + policy.activationEvent)
        : policy.inputLockTiming;
    evidence.alignment.source = std::string(source);
    evidence.fieldStatus =
        policy.screenId
        + ":active_screen=live:active_scenes=policy:motion=live-route:frame=live-frame:cursor_owner=live-"
        + (policy.screenId == "title-menu"
            ? "title-menu"
            : (policy.screenId == "title-options"
                ? "title-options"
                : (policy.screenId == "loading" ? "loading" : (policy.screenId == "pause" ? "pause" : "generic"))))
        + ":transition=policy:input_lock=live-readiness";
}

[[nodiscard]] FrontendLiveStateAlignmentEvidence loadFrontendRuntimeAlignmentFromLiveState(
    const sward::ui_runtime::FrontendScreenPolicy& policy)
{
    FrontendLiveStateAlignmentEvidence evidence = makeDefaultFrontendRuntimeAlignmentEvidence(policy);

    const auto liveStatePath = findLatestFrontendLiveStatePath(policy.screenId);
    if (!liveStatePath)
        return evidence;

    const std::string text = readTextFile(*liveStatePath);
    if (text.empty())
        return evidence;

    evidence.found = true;
    evidence.liveStatePath = *liveStatePath;
    applyFrontendRuntimeAlignmentFromLiveStateJson(evidence, policy, text, "ui_lab_live_state");

    return evidence;
}

[[nodiscard]] FrontendLiveStateAlignmentEvidence loadFrontendRuntimeAlignmentFromLiveBridge(
    const sward::ui_runtime::FrontendScreenPolicy& policy)
{
    constexpr std::string_view defaultPipeName = "sward_ui_lab_live";
    const auto discoveredPipeName = discoverFrontendLiveBridgeName(policy.screenId);
    auto probe = queryUiLabLiveBridgeState(discoveredPipeName);
    if (!probe.connected && discoveredPipeName != defaultPipeName)
        probe = queryUiLabLiveBridgeState(defaultPipeName);

    const auto responseTarget = probe.connected && !probe.responseJson.empty()
        ? jsonStringField(probe.responseJson, "target")
        : std::optional<std::string>{};

    if (probe.connected && !probe.responseJson.empty() && responseTarget && *responseTarget == policy.screenId)
    {
        FrontendLiveStateAlignmentEvidence evidence = makeDefaultFrontendRuntimeAlignmentEvidence(policy);
        applyFrontendRuntimeAlignmentFromLiveStateJson(
            evidence,
            policy,
            probe.responseJson,
            "ui_lab_live_bridge");
        evidence.fallbackSource = "none";
        evidence.bridgeProbe = probe;
        evidence.bridgeProbe.fallbackSource = "none";
        return evidence;
    }

    auto fallback = loadFrontendRuntimeAlignmentFromLiveState(policy);
    fallback.fallbackSource = fallback.alignment.source == "ui_lab_live_state" ? "ui_lab_live_state" : "none";
    if (probe.connected && responseTarget && *responseTarget != policy.screenId)
        probe.error = "target-mismatch-" + *responseTarget;
    probe.fallbackSource = fallback.fallbackSource;
    fallback.bridgeProbe = probe;
    return fallback;
}

void applyFrontendUiOracleFromUiOracleJson(
    FrontendUiOracleEvidence& evidence,
    const sward::ui_runtime::FrontendScreenPolicy& policy,
    std::string_view text,
    std::string_view source)
{
    evidence.found = true;
    evidence.source = std::string(source);
    evidence.target = jsonStringField(text, "target").value_or(policy.screenId);
    evidence.activeMotionName = jsonStringField(text, "activeMotionName")
        .value_or(jsonStringField(text, "route").value_or(""));
    evidence.cursorOwner = jsonStringField(text, "cursorOwner").value_or("");
    evidence.transitionBand = jsonStringField(text, "transitionBand").value_or(policy.transitionBand);
    evidence.inputLockState = jsonStringField(text, "inputLockState").value_or(policy.inputLockTiming);
    evidence.runtimeFrame = static_cast<int>(jsonNumberField(text, "frame").value_or(0.0));
    evidence.runtimeDrawListStatus = jsonStringField(text, "runtimeDrawListStatus")
        .value_or("runtime CSD tree; GPU draw-list pending");

    if (const auto activeScenes = jsonArrayFieldSpan(text, "activeScenes"))
        evidence.activeScenes = parseJsonStringArray(*activeScenes);

    if (const auto oracle = jsonObjectFieldSpan(text, "uiLayerOracle"))
    {
        evidence.activeProject = jsonStringField(*oracle, "activeProject")
            .value_or(jsonStringField(*oracle, "targetProject").value_or(""));
        evidence.sceneCount = static_cast<int>(jsonNumberField(*oracle, "sceneCount").value_or(0.0));
        evidence.layerCount = static_cast<int>(jsonNumberField(*oracle, "layerCount").value_or(0.0));
        evidence.runtimeSceneMotionFrame = static_cast<int>(jsonNumberField(*oracle, "runtimeSceneMotionFrame").value_or(-1.0));
        evidence.runtimeDrawListStatus = jsonStringField(*oracle, "runtimeDrawListStatus")
            .value_or(evidence.runtimeDrawListStatus);

        if (evidence.activeScenes.empty())
        {
            if (const auto scenes = jsonArrayFieldSpan(*oracle, "scenes"))
            {
                for (const auto sceneObject : jsonObjectSpansInArray(*scenes))
                {
                    const auto path = jsonStringField(sceneObject, "path");
                    if (path && !path->empty())
                        evidence.activeScenes.push_back(*path);
                }
            }
        }
    }

    if (evidence.activeProject.empty())
        evidence.activeProject = policy.layoutName;
}

void applyFrontendUiOracleFromLiveStateJson(
    FrontendUiOracleEvidence& evidence,
    const sward::ui_runtime::FrontendScreenPolicy& policy,
    std::string_view text,
    std::string_view source)
{
    evidence.found = true;
    evidence.source = std::string(source);
    evidence.target = jsonStringField(text, "target").value_or(policy.screenId);
    evidence.activeMotionName = jsonStringField(text, "route").value_or(defaultFrontendRuntimeAlignment(policy).activeMotionName);
    evidence.cursorOwner = frontendAlignmentCursorOwnerFromLiveState(policy, text);
    evidence.transitionBand = policy.transitionBand;
    evidence.inputLockState = frontendAlignmentReadyFromLiveState(policy, text)
        ? "released:" + policy.activationEvent
        : policy.inputLockTiming;
    evidence.runtimeFrame = static_cast<int>(jsonNumberField(text, "frame").value_or(0.0));
    evidence.runtimeDrawListStatus = "runtime CSD tree; GPU draw-list pending";

    if (const auto typedInspectors = jsonObjectFieldSpan(text, "typedInspectors"))
    {
        if (const auto tree = jsonObjectFieldSpan(*typedInspectors, "csdProjectTree"))
        {
            evidence.activeProject = jsonStringField(*tree, "activeProject")
                .value_or(jsonStringField(*tree, "targetProject").value_or(""));
            evidence.sceneCount = static_cast<int>(jsonNumberField(*tree, "sceneCount").value_or(0.0));
            evidence.layerCount = static_cast<int>(jsonNumberField(*tree, "layerCount").value_or(0.0));
            evidence.runtimeSceneMotionFrame = static_cast<int>(jsonNumberField(*tree, "runtimeSceneMotionFrame").value_or(-1.0));

            if (const auto scenes = jsonArrayFieldSpan(*tree, "scenes"))
            {
                for (const auto sceneObject : jsonObjectSpansInArray(*scenes))
                {
                    const auto path = jsonStringField(sceneObject, "path");
                    if (path && !path->empty())
                        evidence.activeScenes.push_back(*path);
                }
            }
        }
    }

    if (evidence.activeProject.empty())
        evidence.activeProject = policy.layoutName;
}

[[nodiscard]] FrontendUiOracleEvidence loadFrontendUiOracleEvidence(
    const sward::ui_runtime::FrontendScreenPolicy& policy)
{
    constexpr std::string_view defaultPipeName = "sward_ui_lab_live";
    const auto discoveredPipeName = discoverFrontendLiveBridgeName(policy.screenId);
    auto oracleProbe = queryUiLabLiveBridgeUiOracle(discoveredPipeName);
    if (!oracleProbe.connected && discoveredPipeName != defaultPipeName)
        oracleProbe = queryUiLabLiveBridgeUiOracle(defaultPipeName);

    const auto oracleTarget = oracleProbe.connected && !oracleProbe.responseJson.empty()
        ? jsonStringField(oracleProbe.responseJson, "target")
        : std::optional<std::string>{};
    if (oracleProbe.connected && oracleTarget && *oracleTarget == policy.screenId)
    {
        FrontendUiOracleEvidence evidence;
        evidence.probe = "direct-ui-oracle";
        evidence.bridgeProbe = oracleProbe;
        applyFrontendUiOracleFromUiOracleJson(
            evidence,
            policy,
            oracleProbe.responseJson,
            "ui_lab_live_bridge_ui_oracle");
        return evidence;
    }

    auto stateProbe = queryUiLabLiveBridgeState(discoveredPipeName);
    if (!stateProbe.connected && discoveredPipeName != defaultPipeName)
        stateProbe = queryUiLabLiveBridgeState(defaultPipeName);

    const auto stateTarget = stateProbe.connected && !stateProbe.responseJson.empty()
        ? jsonStringField(stateProbe.responseJson, "target")
        : std::optional<std::string>{};
    if (stateProbe.connected && stateTarget && *stateTarget == policy.screenId)
    {
        FrontendUiOracleEvidence evidence;
        evidence.probe = "state-fallback";
        evidence.bridgeProbe = stateProbe;
        applyFrontendUiOracleFromLiveStateJson(
            evidence,
            policy,
            stateProbe.responseJson,
            "ui_lab_live_bridge_state");
        return evidence;
    }

    FrontendUiOracleEvidence fallback;
    fallback.probe = "snapshot-fallback";
    fallback.bridgeProbe = oracleProbe;
    const auto liveStatePath = findLatestFrontendLiveStatePath(policy.screenId);
    if (!liveStatePath)
        return fallback;

    const std::string text = readTextFile(*liveStatePath);
    if (text.empty())
        return fallback;

    fallback.liveStatePath = *liveStatePath;
    applyFrontendUiOracleFromLiveStateJson(
        fallback,
        policy,
        text,
        "ui_lab_live_state");
    return fallback;
}

[[nodiscard]] std::string runtimeDrawListSceneFromLayerPath(std::string_view activeProject, std::string_view layerPath)
{
    std::string remaining(layerPath);
    const std::string projectPrefix = std::string(activeProject) + "/";
    if (!activeProject.empty() && remaining.starts_with(projectPrefix))
        remaining = remaining.substr(projectPrefix.size());

    const auto slash = remaining.find('/');
    if (slash != std::string::npos)
        return remaining.substr(0, slash);
    return remaining;
}

[[nodiscard]] FrontendRuntimeDrawCall parseFrontendRuntimeDrawCall(
    std::string_view objectSpan,
    std::string_view activeProject)
{
    FrontendRuntimeDrawCall call;
    call.project = jsonStringField(objectSpan, "project").value_or(std::string(activeProject));
    call.layerPath = jsonStringField(objectSpan, "layerPath").value_or("");
    call.sceneName = runtimeDrawListSceneFromLayerPath(
        call.project.empty() ? activeProject : std::string_view(call.project),
        call.layerPath);
    call.primitive = jsonStringField(objectSpan, "primitive").value_or("quad");
    call.colorSample = jsonStringField(objectSpan, "colorSample").value_or("");
    call.textured = jsonBoolField(objectSpan, "textured").value_or(false);
    call.vertexCount = static_cast<int>(jsonNumberField(objectSpan, "vertexCount").value_or(0.0));
    call.vertexStride = static_cast<int>(jsonNumberField(objectSpan, "vertexStride").value_or(0.0));

    if (const auto screenRect = jsonObjectFieldSpan(objectSpan, "screenRect"))
    {
        call.minX = jsonNumberField(*screenRect, "minX").value_or(0.0);
        call.minY = jsonNumberField(*screenRect, "minY").value_or(0.0);
        call.maxX = jsonNumberField(*screenRect, "maxX").value_or(0.0);
        call.maxY = jsonNumberField(*screenRect, "maxY").value_or(0.0);
    }

    return call;
}

void applyFrontendRuntimeDrawListFromJson(
    FrontendRuntimeDrawListEvidence& evidence,
    const sward::ui_runtime::FrontendScreenPolicy& policy,
    std::string_view text,
    std::string_view source,
    std::string_view probe)
{
    evidence.found = true;
    evidence.screenId = jsonStringField(text, "target").value_or(policy.screenId);
    evidence.source = std::string(source);
    evidence.probe = std::string(probe);
    evidence.activeProject = jsonStringField(text, "targetProject").value_or("");
    if (evidence.activeProject.empty())
        evidence.activeProject = jsonStringField(text, "activeProject").value_or("");
    if (evidence.activeProject.empty())
        evidence.activeProject = frontendRuntimeDrawableProjectName(policy, "");
    evidence.runtimeFrame = static_cast<int>(jsonNumberField(text, "frame").value_or(0.0));
    evidence.runtimeDrawListStatus = jsonStringField(text, "runtimeDrawListStatus").value_or("missing");
    evidence.backendSubmitStatus = jsonStringField(text, "gpuDrawListStatus").value_or("GPU backend submit pending");

    const auto drawListOracle = jsonObjectFieldSpan(text, "uiDrawListOracle");
    const std::string_view drawListSpan = drawListOracle ? *drawListOracle : text;
    if (const auto calls = jsonArrayFieldSpan(drawListSpan, "drawCalls"))
    {
        for (const auto callSpan : jsonObjectSpansInArray(*calls))
        {
            auto call = parseFrontendRuntimeDrawCall(callSpan, evidence.activeProject);
            if (!call.layerPath.empty())
                evidence.calls.push_back(std::move(call));
        }
    }
}

[[nodiscard]] FrontendRuntimeDrawListEvidence loadFrontendRuntimeDrawListEvidence(
    const sward::ui_runtime::FrontendScreenPolicy& policy)
{
    constexpr std::string_view defaultPipeName = "sward_ui_lab_live";
    const auto discoveredPipeName = discoverFrontendLiveBridgeName(policy.screenId);

    auto drawProbe = queryUiLabLiveBridgeUiDrawList(discoveredPipeName);
    if (!drawProbe.connected && discoveredPipeName != defaultPipeName)
        drawProbe = queryUiLabLiveBridgeUiDrawList(defaultPipeName);

    const auto drawTarget = drawProbe.connected && !drawProbe.responseJson.empty()
        ? jsonStringField(drawProbe.responseJson, "target")
        : std::optional<std::string>{};
    if (drawProbe.connected && drawTarget && *drawTarget == policy.screenId)
    {
        FrontendRuntimeDrawListEvidence evidence;
        evidence.bridgeProbe = drawProbe;
        applyFrontendRuntimeDrawListFromJson(
            evidence,
            policy,
            drawProbe.responseJson,
            "ui_lab_live_bridge_ui_draw_list",
            "direct-ui-draw-list");
        return evidence;
    }

    auto oracleProbe = queryUiLabLiveBridgeUiOracle(discoveredPipeName);
    if (!oracleProbe.connected && discoveredPipeName != defaultPipeName)
        oracleProbe = queryUiLabLiveBridgeUiOracle(defaultPipeName);

    const auto oracleTarget = oracleProbe.connected && !oracleProbe.responseJson.empty()
        ? jsonStringField(oracleProbe.responseJson, "target")
        : std::optional<std::string>{};
    if (oracleProbe.connected && oracleTarget && *oracleTarget == policy.screenId)
    {
        FrontendRuntimeDrawListEvidence evidence;
        evidence.bridgeProbe = oracleProbe;
        applyFrontendRuntimeDrawListFromJson(
            evidence,
            policy,
            oracleProbe.responseJson,
            "ui_lab_live_bridge_ui_oracle",
            "ui-oracle-fallback");
        return evidence;
    }

    FrontendRuntimeDrawListEvidence missing;
    missing.screenId = policy.screenId;
    missing.activeProject = frontendRuntimeDrawableProjectName(policy, "");
    missing.bridgeProbe = drawProbe;
    return missing;
}

[[nodiscard]] FrontendGpuSubmitCall parseFrontendGpuSubmitCall(std::string_view objectSpan)
{
    FrontendGpuSubmitCall call;
    call.source = jsonStringField(objectSpan, "source").value_or("");
    call.indexed = jsonBoolField(objectSpan, "indexed").value_or(false);
    call.inlineVertexStream = jsonBoolField(objectSpan, "inlineVertexStream").value_or(false);
    call.vertexCount = static_cast<int>(jsonNumberField(objectSpan, "vertexCount").value_or(0.0));
    call.indexCount = static_cast<int>(jsonNumberField(objectSpan, "indexCount").value_or(0.0));
    call.instanceCount = static_cast<int>(jsonNumberField(objectSpan, "instanceCount").value_or(0.0));
    call.texture2DDescriptorIndex = static_cast<int>(jsonNumberField(objectSpan, "texture2DDescriptorIndex").value_or(0.0));
    call.samplerDescriptorIndex = static_cast<int>(jsonNumberField(objectSpan, "samplerDescriptorIndex").value_or(0.0));
    if (const auto pipelineState = jsonObjectFieldSpan(objectSpan, "pipelineState"))
        call.alphaBlendEnable = jsonBoolField(*pipelineState, "alphaBlendEnable").value_or(false);
    return call;
}

void applyFrontendGpuSubmitFromJson(
    FrontendGpuSubmitEvidence& evidence,
    const sward::ui_runtime::FrontendScreenPolicy& policy,
    std::string_view text,
    std::string_view source,
    std::string_view probe)
{
    evidence.found = true;
    evidence.screenId = jsonStringField(text, "target").value_or(policy.screenId);
    evidence.source = std::string(source);
    evidence.probe = std::string(probe);
    evidence.runtimeFrame = static_cast<int>(jsonNumberField(text, "frame").value_or(0.0));
    evidence.backendSubmitStatus = jsonStringField(text, "backendSubmitStatus").value_or("missing");

    const auto gpuSubmitOracle = jsonObjectFieldSpan(text, "gpuSubmitOracle");
    const std::string_view submitSpan = gpuSubmitOracle ? *gpuSubmitOracle : text;
    if (const auto calls = jsonArrayFieldSpan(submitSpan, "submitCalls"))
    {
        for (const auto callSpan : jsonObjectSpansInArray(*calls))
            evidence.calls.push_back(parseFrontendGpuSubmitCall(callSpan));
    }
}

[[nodiscard]] FrontendGpuSubmitEvidence loadFrontendGpuSubmitEvidence(
    const sward::ui_runtime::FrontendScreenPolicy& policy)
{
    constexpr std::string_view defaultPipeName = "sward_ui_lab_live";
    const auto discoveredPipeName = discoverFrontendLiveBridgeName(policy.screenId);

    auto gpuProbe = queryUiLabLiveBridgeGpuSubmit(discoveredPipeName);
    if (!gpuProbe.connected && discoveredPipeName != defaultPipeName)
        gpuProbe = queryUiLabLiveBridgeGpuSubmit(defaultPipeName);

    const auto gpuTarget = gpuProbe.connected && !gpuProbe.responseJson.empty()
        ? jsonStringField(gpuProbe.responseJson, "target")
        : std::optional<std::string>{};
    if (gpuProbe.connected && gpuTarget && *gpuTarget == policy.screenId)
    {
        FrontendGpuSubmitEvidence evidence;
        evidence.bridgeProbe = gpuProbe;
        applyFrontendGpuSubmitFromJson(
            evidence,
            policy,
            gpuProbe.responseJson,
            "ui_lab_live_bridge_gpu_submit",
            "direct-ui-gpu-submit");
        return evidence;
    }

    const auto drawList = loadFrontendRuntimeDrawListEvidence(policy);
    FrontendGpuSubmitEvidence fallback;
    fallback.screenId = policy.screenId;
    fallback.source = drawList.found ? "ui_lab_live_bridge_ui_draw_list" : "missing";
    fallback.probe = drawList.found ? "ui-draw-list-fallback" : "missing";
    fallback.backendSubmitStatus = drawList.backendSubmitStatus;
    return fallback;
}

[[nodiscard]] FrontendMaterialCorrelationPair parseFrontendMaterialCorrelationPair(std::string_view objectSpan)
{
    FrontendMaterialCorrelationPair pair;
    pair.uiDrawSequence = static_cast<int>(jsonNumberField(objectSpan, "uiDrawSequence").value_or(0.0));
    pair.gpuSubmitSequence = static_cast<int>(jsonNumberField(objectSpan, "gpuSubmitSequence").value_or(0.0));
    pair.blendSemantic = jsonStringField(objectSpan, "blendSemantic").value_or("");
    pair.samplerSemantic = jsonStringField(objectSpan, "samplerSemantic").value_or("");
    pair.addressSemantic = jsonStringField(objectSpan, "addressSemantic").value_or("");
    pair.alphaBlendEnable = jsonBoolField(objectSpan, "alphaBlendEnable").value_or(false);
    pair.additiveBlend = jsonBoolField(objectSpan, "additiveBlend").value_or(false);
    pair.linearFilter = jsonBoolField(objectSpan, "linearFilter").value_or(false);
    pair.pointFilter = jsonBoolField(objectSpan, "pointFilter").value_or(false);
    return pair;
}

void applyFrontendMaterialCorrelationFromJson(
    FrontendMaterialCorrelationEvidence& evidence,
    const sward::ui_runtime::FrontendScreenPolicy& policy,
    std::string_view text,
    std::string_view source,
    std::string_view probe)
{
    evidence.found = true;
    evidence.screenId = jsonStringField(text, "target").value_or(policy.screenId);
    evidence.source = std::string(source);
    evidence.probe = std::string(probe);
    evidence.runtimeFrame = static_cast<int>(jsonNumberField(text, "frame").value_or(0.0));
    evidence.correlationStatus = jsonStringField(text, "correlationStatus").value_or("missing");
    evidence.rawBackendCommandStatus = jsonStringField(text, "rawBackendCommandStatus").value_or("missing");
    evidence.resolvedBackendStatus = jsonStringField(text, "resolvedBackendStatus").value_or("missing");
    evidence.drawCallCount = static_cast<std::size_t>(jsonNumberField(text, "uiDrawCallCount").value_or(0.0));
    evidence.backendSubmitCount = static_cast<std::size_t>(jsonNumberField(text, "backendSubmitCallCount").value_or(0.0));
    evidence.backendResolvedSubmitCount = static_cast<std::size_t>(jsonNumberField(text, "backendResolvedSubmitCount").value_or(0.0));

    const auto oracle = jsonObjectFieldSpan(text, "materialCorrelationOracle");
    const std::string_view oracleSpan = oracle ? *oracle : text;
    if (evidence.rawBackendCommandStatus == "missing")
        evidence.rawBackendCommandStatus = jsonStringField(oracleSpan, "rawBackendCommandStatus").value_or("missing");
    if (evidence.resolvedBackendStatus == "missing")
        evidence.resolvedBackendStatus = jsonStringField(oracleSpan, "resolvedBackendStatus").value_or("missing");
    if (evidence.drawCallCount == 0)
        evidence.drawCallCount = static_cast<std::size_t>(jsonNumberField(oracleSpan, "uiDrawCallCount").value_or(0.0));
    if (evidence.backendSubmitCount == 0)
        evidence.backendSubmitCount = static_cast<std::size_t>(jsonNumberField(oracleSpan, "backendSubmitCallCount").value_or(0.0));
    if (evidence.backendResolvedSubmitCount == 0)
        evidence.backendResolvedSubmitCount = static_cast<std::size_t>(jsonNumberField(oracleSpan, "backendResolvedSubmitCount").value_or(0.0));

    if (const auto pairs = jsonArrayFieldSpan(oracleSpan, "pairs"))
    {
        for (const auto pairSpan : jsonObjectSpansInArray(*pairs))
            evidence.pairs.push_back(parseFrontendMaterialCorrelationPair(pairSpan));
    }
}

[[nodiscard]] FrontendMaterialCorrelationEvidence loadFrontendMaterialCorrelationEvidence(
    const sward::ui_runtime::FrontendScreenPolicy& policy)
{
    constexpr std::string_view defaultPipeName = "sward_ui_lab_live";
    const auto discoveredPipeName = discoverFrontendLiveBridgeName(policy.screenId);

    auto materialProbe = queryUiLabLiveBridgeMaterialCorrelation(discoveredPipeName);
    if (!materialProbe.connected && discoveredPipeName != defaultPipeName)
        materialProbe = queryUiLabLiveBridgeMaterialCorrelation(defaultPipeName);

    const auto materialTarget = materialProbe.connected && !materialProbe.responseJson.empty()
        ? jsonStringField(materialProbe.responseJson, "target")
        : std::optional<std::string>{};
    if (materialProbe.connected && materialTarget && *materialTarget == policy.screenId)
    {
        FrontendMaterialCorrelationEvidence evidence;
        evidence.bridgeProbe = materialProbe;
        applyFrontendMaterialCorrelationFromJson(
            evidence,
            policy,
            materialProbe.responseJson,
            "ui_lab_live_bridge_material_correlation",
            "direct-ui-material-correlation");
        return evidence;
    }

    const auto submit = loadFrontendGpuSubmitEvidence(policy);
    FrontendMaterialCorrelationEvidence fallback;
    fallback.screenId = policy.screenId;
    fallback.source = submit.found ? "ui_lab_live_bridge_gpu_submit" : "missing";
    fallback.probe = submit.found ? "gpu-submit-fallback" : "missing";
    fallback.rawBackendCommandStatus = submit.backendSubmitStatus;
    fallback.backendSubmitCount = submit.calls.size();
    fallback.bridgeProbe = materialProbe;
    return fallback;
}

[[nodiscard]] FrontendBackendResolvedSubmit parseFrontendBackendResolvedSubmit(std::string_view objectSpan)
{
    FrontendBackendResolvedSubmit submit;
    submit.backend = jsonStringField(objectSpan, "backend").value_or("");
    submit.nativeCommand = jsonStringField(objectSpan, "nativeCommand").value_or("");
    submit.materialParityHint = jsonStringField(objectSpan, "materialParityHint").value_or("missing");
    submit.indexed = jsonBoolField(objectSpan, "indexed").value_or(false);
    submit.resolvedPipelineKnown = jsonBoolField(objectSpan, "resolvedPipelineKnown").value_or(false);
    submit.activeFramebufferKnown = jsonBoolField(objectSpan, "activeFramebufferKnown").value_or(false);
    submit.framebufferRegistered = jsonBoolField(objectSpan, "framebufferRegistered").value_or(false);
    submit.vertexCount = static_cast<int>(jsonNumberField(objectSpan, "vertexCount").value_or(0.0));
    submit.indexCount = static_cast<int>(jsonNumberField(objectSpan, "indexCount").value_or(0.0));
    submit.instanceCount = static_cast<int>(jsonNumberField(objectSpan, "instanceCount").value_or(0.0));
    submit.renderTargetFormat0 = static_cast<int>(jsonNumberField(objectSpan, "renderTargetFormat0").value_or(0.0));
    submit.depthTargetFormat = static_cast<int>(jsonNumberField(objectSpan, "depthTargetFormat").value_or(0.0));
    submit.primitiveTopology = static_cast<int>(jsonNumberField(objectSpan, "primitiveTopology").value_or(0.0));
    submit.renderTargetCount = static_cast<int>(jsonNumberField(objectSpan, "renderTargetCount").value_or(0.0));
    if (const auto blendState = jsonObjectFieldSpan(objectSpan, "pipelineBlendState"))
        submit.blendEnabled = jsonBoolField(*blendState, "blendEnabled").value_or(false);
    return submit;
}

void applyFrontendBackendResolvedFromJson(
    FrontendBackendResolvedEvidence& evidence,
    const sward::ui_runtime::FrontendScreenPolicy& policy,
    std::string_view text,
    std::string_view source,
    std::string_view probe)
{
    evidence.found = true;
    evidence.screenId = jsonStringField(text, "target").value_or(policy.screenId);
    evidence.source = std::string(source);
    evidence.probe = std::string(probe);
    evidence.runtimeFrame = static_cast<int>(jsonNumberField(text, "frame").value_or(0.0));
    evidence.resolvedBackendStatus = jsonStringField(text, "resolvedBackendStatus").value_or("missing");
    evidence.materialParityStatus = jsonStringField(text, "materialParityStatus").value_or("missing");
    evidence.materialPairCount = static_cast<std::size_t>(jsonNumberField(text, "materialPairCount").value_or(0.0));

    const auto oracle = jsonObjectFieldSpan(text, "backendResolvedSubmitOracle");
    const std::string_view oracleSpan = oracle ? *oracle : text;
    const auto parityHints = jsonObjectFieldSpan(text, "backendMaterialParityHints");
    const std::string_view paritySpan = parityHints ? *parityHints : text;
    if (evidence.resolvedBackendStatus == "missing")
        evidence.resolvedBackendStatus = jsonStringField(oracleSpan, "resolvedBackendStatus").value_or("missing");
    if (evidence.materialParityStatus == "missing")
        evidence.materialParityStatus = jsonStringField(paritySpan, "materialParityStatus").value_or("missing");
    evidence.blendParityPolicy = jsonStringField(paritySpan, "blendParityPolicy").value_or("missing");
    evidence.framebufferParityPolicy = jsonStringField(paritySpan, "framebufferParityPolicy").value_or("missing");
    evidence.textureViewSamplerGap = jsonStringField(paritySpan, "textureViewSamplerGap").value_or("pending");
    evidence.textMovieSfxGap = jsonStringField(paritySpan, "textMovieSfxGap").value_or("pending");
    const auto descriptorSemantics = jsonObjectFieldSpan(text, "backendDescriptorSemantics");
    const std::string_view descriptorSpan = descriptorSemantics ? *descriptorSemantics : text;
    evidence.textureViewSamplerStatus = jsonStringField(descriptorSpan, "textureViewSamplerStatus").value_or("missing");
    evidence.textureDescriptorPolicy = jsonStringField(descriptorSpan, "textureDescriptorPolicy").value_or("missing");
    evidence.samplerDescriptorPolicy = jsonStringField(descriptorSpan, "samplerDescriptorPolicy").value_or("missing");
    evidence.vendorDescriptorCaptureGap = jsonStringField(descriptorSpan, "vendorDescriptorCaptureGap").value_or("pending-native-descriptor-dump");
    evidence.textureDescriptorKnownCount = static_cast<std::size_t>(jsonNumberField(descriptorSpan, "textureDescriptorKnownCount").value_or(0.0));
    evidence.samplerDescriptorKnownCount = static_cast<std::size_t>(jsonNumberField(descriptorSpan, "samplerDescriptorKnownCount").value_or(0.0));
    evidence.linearSamplerDescriptorCount = static_cast<std::size_t>(jsonNumberField(descriptorSpan, "linearSamplerDescriptorCount").value_or(0.0));
    evidence.pointSamplerDescriptorCount = static_cast<std::size_t>(jsonNumberField(descriptorSpan, "pointSamplerDescriptorCount").value_or(0.0));
    evidence.wrapSamplerDescriptorCount = static_cast<std::size_t>(jsonNumberField(descriptorSpan, "wrapSamplerDescriptorCount").value_or(0.0));
    evidence.clampSamplerDescriptorCount = static_cast<std::size_t>(jsonNumberField(descriptorSpan, "clampSamplerDescriptorCount").value_or(0.0));
    const auto vendorResourceCapture = jsonObjectFieldSpan(text, "backendVendorResourceCapture");
    const std::string_view vendorResourceSpan = vendorResourceCapture ? *vendorResourceCapture : text;
    evidence.vendorResourceCaptureStatus = jsonStringField(vendorResourceSpan, "vendorResourceCaptureStatus")
        .value_or(jsonStringField(text, "vendorResourceCaptureStatus").value_or("missing"));
    evidence.vendorResourceCapturePolicy = jsonStringField(vendorResourceSpan, "vendorResourceCapturePolicy").value_or("missing");
    evidence.uiOnlyLayerCaptureStatus = jsonStringField(vendorResourceSpan, "uiOnlyLayerCaptureStatus")
        .value_or(jsonStringField(text, "uiOnlyLayerCaptureStatus").value_or("pending-runtime-ui-render-target-copy"));
    evidence.nativeCommandCaptureGap = jsonStringField(vendorResourceSpan, "nativeCommandCaptureGap")
        .value_or(jsonStringField(text, "nativeCommandCaptureGap").value_or("pending-full-vendor-command-buffer-dump"));
    evidence.textureResourceViewKnownCount = static_cast<std::size_t>(jsonNumberField(vendorResourceSpan, "textureResourceViewKnownCount").value_or(0.0));
    evidence.samplerResourceViewKnownCount = static_cast<std::size_t>(jsonNumberField(vendorResourceSpan, "samplerResourceViewKnownCount").value_or(0.0));
    evidence.resourceViewPairCount = static_cast<std::size_t>(jsonNumberField(vendorResourceSpan, "resourceViewPairCount").value_or(0.0));
    if (evidence.materialPairCount == 0)
        evidence.materialPairCount = static_cast<std::size_t>(jsonNumberField(oracleSpan, "materialPairCount").value_or(0.0));

    if (const auto submits = jsonArrayFieldSpan(oracleSpan, "submits"))
    {
        for (const auto submitSpan : jsonObjectSpansInArray(*submits))
            evidence.submits.push_back(parseFrontendBackendResolvedSubmit(submitSpan));
    }
}

[[nodiscard]] FrontendBackendResolvedEvidence loadFrontendBackendResolvedEvidence(
    const sward::ui_runtime::FrontendScreenPolicy& policy)
{
    constexpr std::string_view defaultPipeName = "sward_ui_lab_live";
    const auto discoveredPipeName = discoverFrontendLiveBridgeName(policy.screenId);

    auto backendProbe = queryUiLabLiveBridgeBackendResolved(discoveredPipeName);
    if (!backendProbe.connected && discoveredPipeName != defaultPipeName)
        backendProbe = queryUiLabLiveBridgeBackendResolved(defaultPipeName);

    const auto backendTarget = backendProbe.connected && !backendProbe.responseJson.empty()
        ? jsonStringField(backendProbe.responseJson, "target")
        : std::optional<std::string>{};
    if (backendProbe.connected && backendTarget && *backendTarget == policy.screenId)
    {
        FrontendBackendResolvedEvidence evidence;
        evidence.bridgeProbe = backendProbe;
        applyFrontendBackendResolvedFromJson(
            evidence,
            policy,
            backendProbe.responseJson,
            "ui_lab_live_bridge_backend_resolved",
            "direct-ui-backend-resolved");
        return evidence;
    }

    const auto material = loadFrontendMaterialCorrelationEvidence(policy);
    FrontendBackendResolvedEvidence fallback;
    fallback.screenId = policy.screenId;
    fallback.source = material.found ? "ui_lab_live_bridge_material_correlation" : "missing";
    fallback.probe = material.found ? "material-correlation-fallback" : "missing";
    fallback.resolvedBackendStatus = material.resolvedBackendStatus;
    fallback.materialParityStatus = material.found ? "material correlation fallback; backend material parity hints unavailable" : "missing";
    fallback.textureViewSamplerStatus = material.found ? "material correlation fallback; descriptor semantics unavailable" : "missing";
    fallback.vendorDescriptorCaptureGap = "pending-native-descriptor-dump";
    fallback.vendorResourceCaptureStatus = material.found ? "material correlation fallback; vendor resource capture unavailable" : "missing";
    fallback.vendorResourceCapturePolicy = "native-rhi-resource-view-and-sampler-handles";
    fallback.uiOnlyLayerCaptureStatus = "pending-runtime-ui-render-target-copy";
    fallback.nativeCommandCaptureGap = "pending-full-vendor-command-buffer-dump";
    fallback.linearSamplerDescriptorCount = static_cast<std::size_t>(std::count_if(
        material.pairs.begin(),
        material.pairs.end(),
        [](const FrontendMaterialCorrelationPair& pair)
        {
            return pair.linearFilter || pair.samplerSemantic.find("linear") != std::string::npos;
        }));
    fallback.pointSamplerDescriptorCount = static_cast<std::size_t>(std::count_if(
        material.pairs.begin(),
        material.pairs.end(),
        [](const FrontendMaterialCorrelationPair& pair)
        {
            return pair.pointFilter || pair.samplerSemantic.find("point") != std::string::npos;
        }));
    fallback.wrapSamplerDescriptorCount = static_cast<std::size_t>(std::count_if(
        material.pairs.begin(),
        material.pairs.end(),
        [](const FrontendMaterialCorrelationPair& pair)
        {
            return pair.addressSemantic.find("WRAP") != std::string::npos;
        }));
    fallback.clampSamplerDescriptorCount = static_cast<std::size_t>(std::count_if(
        material.pairs.begin(),
        material.pairs.end(),
        [](const FrontendMaterialCorrelationPair& pair)
        {
            return pair.addressSemantic.find("CLAMP") != std::string::npos;
        }));
    fallback.materialPairCount = material.pairs.size();
    fallback.bridgeProbe = backendProbe;
    return fallback;
}

[[nodiscard]] FrontendUiOraclePlaybackClock loadFrontendUiOraclePlaybackClock(
    const sward::ui_runtime::FrontendScreenPolicy& policy)
{
    const auto oracle = loadFrontendUiOracleEvidence(policy);
    const auto fallback = defaultFrontendRuntimeAlignment(policy);

    FrontendUiOraclePlaybackClock clock;
    clock.found = oracle.found;
    clock.source = oracle.source;
    clock.probe = oracle.probe;
    clock.playbackClock = oracle.found ? "ui-oracle-runtime-frame" : "frontend-policy";
    clock.frameSource = oracle.found ? "ui-oracle-mod-frame" : "policy-sample-frame";
    clock.activeMotionName = !oracle.activeMotionName.empty() ? oracle.activeMotionName : fallback.activeMotionName;
    clock.runtimeFrame = oracle.runtimeFrame > 0
        ? oracle.runtimeFrame
        : (oracle.runtimeSceneMotionFrame > 0 ? oracle.runtimeSceneMotionFrame : fallback.activeFrame);
    clock.playbackFrame = clock.runtimeFrame;
    clock.oracle = oracle;
    return clock;
}

[[nodiscard]] std::string formatFrontendRuntimeAlignmentEvidence(const FrontendLiveStateAlignmentEvidence& evidence)
{
    const std::string liveStateLabel = !evidence.liveStatePath.empty()
        ? portablePath(evidence.liveStatePath)
        : (evidence.alignment.source == "ui_lab_live_bridge" ? std::string("direct-live-bridge") : std::string("missing"));

    std::ostringstream out;
    out << evidence.alignment.screenId
        << ":source=" << evidence.alignment.source
        << ":live_state=" << liveStateLabel
        << ":native_capture=" << (evidence.nativeCaptureStatus.empty() ? std::string("unknown") : evidence.nativeCaptureStatus)
        << ":field_status=" << evidence.fieldStatus;
    return out.str();
}

[[nodiscard]] std::string sonicHudLayoutStatus(std::string_view runtimeProject, std::string_view localProject)
{
    const bool exactLayoutAvailable = layoutEvidenceContainsFile(std::string(runtimeProject) + ".yncp");
    if (runtimeProject == localProject && exactLayoutAvailable)
        return "exact-runtime-layout";
    if (runtimeProject == "ui_playscreen" && !exactLayoutAvailable)
        return "exact-ui-playscreen-layout-unrecovered;local-proxy-layout-ui_prov_playscreen";
    if (runtimeProject != localProject)
        return "runtime-local-layout-mismatch";
    return "layout-evidence-present";
}

[[nodiscard]] CsdHudRuntimeSceneEvidence loadLatestSonicHudLiveStateEvidence(const CsdPipelineTemplateBinding& csdBinding)
{
    CsdHudRuntimeSceneEvidence evidence;
    evidence.localLayoutFileName = std::string(csdBinding.layoutFileName);
    evidence.localProject = projectNameFromLayoutFileName(csdBinding.layoutFileName);
    evidence.localSceneName = std::string(csdBinding.primarySceneName);

    const auto liveStatePath = latestLiveStatePathForTarget("sonic-hud");
    if (!liveStatePath)
    {
        evidence.layoutStatus = "missing-live-state";
        return evidence;
    }

    const std::string text = readTextFile(*liveStatePath);
    if (text.empty())
        return evidence;

    evidence.found = true;
    evidence.liveStatePath = *liveStatePath;
    evidence.runtimeProject = jsonStringField(text, "targetCsd").value_or("");
    evidence.stageReadyFrame = static_cast<int>(jsonNumberField(text, "stageReadyFrame").value_or(0.0));

    if (const auto tree = jsonObjectFieldSpan(text, "csdProjectTree"))
    {
        const auto activeProject = jsonStringField(*tree, "activeProject");
        if (activeProject && !activeProject->empty())
            evidence.runtimeProject = *activeProject;
        evidence.runtimeSceneCount = static_cast<int>(jsonNumberField(*tree, "sceneCount").value_or(0.0));
        evidence.runtimeNodeCount = static_cast<int>(jsonNumberField(*tree, "nodeCount").value_or(0.0));
        evidence.runtimeLayerCount = static_cast<int>(jsonNumberField(*tree, "layerCount").value_or(0.0));

        if (const auto scenes = jsonArrayFieldSpan(*tree, "scenes"))
        {
            for (const auto sceneObject : jsonObjectSpansInArray(*scenes))
            {
                CsdHudRuntimeSceneEntry scene;
                scene.path = jsonStringField(sceneObject, "path").value_or("");
                scene.sceneName = sceneLeafNameFromRuntimePath(scene.path, evidence.runtimeProject);
                scene.castCount = static_cast<int>(jsonNumberField(sceneObject, "castCount").value_or(0.0));
                scene.frame = static_cast<int>(jsonNumberField(sceneObject, "frame").value_or(0.0));
                if (!scene.path.empty())
                    evidence.runtimeScenes.push_back(std::move(scene));
            }
        }

        if (const auto nodes = jsonArrayFieldSpan(*tree, "nodes"))
        {
            for (const auto nodeObject : jsonObjectSpansInArray(*nodes))
            {
                CsdHudRuntimeNodeEntry node;
                node.path = jsonStringField(nodeObject, "path").value_or("");
                node.nodeAddress = jsonStringField(nodeObject, "nodeAddress").value_or("");
                node.projectAddress = jsonStringField(nodeObject, "projectAddress").value_or("");
                node.sceneCount = static_cast<int>(jsonNumberField(nodeObject, "sceneCount").value_or(0.0));
                node.childNodeCount = static_cast<int>(jsonNumberField(nodeObject, "childNodeCount").value_or(0.0));
                node.frame = static_cast<int>(jsonNumberField(nodeObject, "frame").value_or(0.0));
                if (!node.path.empty())
                    evidence.runtimeNodes.push_back(std::move(node));
            }
        }

        if (const auto layers = jsonArrayFieldSpan(*tree, "layers"))
        {
            for (const auto layerObject : jsonObjectSpansInArray(*layers))
            {
                CsdHudRuntimeLayerEntry layer;
                layer.path = jsonStringField(layerObject, "path").value_or("");
                layer.sceneName = runtimeSceneNameForLayerPath(layer.path, evidence.runtimeScenes);
                layer.layerName = layerLeafNameFromRuntimePath(layer.path);
                layer.layerAddress = jsonStringField(layerObject, "layerAddress").value_or("");
                layer.castNodeAddress = jsonStringField(layerObject, "castNodeAddress").value_or("");
                layer.castNodeIndex = static_cast<int>(jsonNumberField(layerObject, "castNodeIndex").value_or(0.0));
                layer.castIndex = static_cast<int>(jsonNumberField(layerObject, "castIndex").value_or(0.0));
                layer.frame = static_cast<int>(jsonNumberField(layerObject, "frame").value_or(0.0));
                if (!layer.path.empty())
                    evidence.runtimeLayers.push_back(std::move(layer));
            }
        }
    }

    if (evidence.runtimeProject.empty())
        evidence.runtimeProject = "unknown";

    if (const auto sonicHud = jsonObjectFieldSpan(text, "sonicHud"))
    {
        evidence.rawOwnerKnown = jsonBoolField(*sonicHud, "rawOwnerKnown").value_or(false);
        evidence.rawOwnerFieldsReady = jsonBoolField(*sonicHud, "rawOwnerFieldsReady").value_or(false);
        if (const auto ownerPath = jsonObjectFieldSpan(*sonicHud, "ownerPath"))
        {
            evidence.ownerPathStatus = jsonStringField(*ownerPath, "ownerPointerStatus").value_or(evidence.ownerPathStatus);
            evidence.ownerFieldMaturationStatus = jsonStringField(*ownerPath, "ownerFieldMaturationStatus").value_or(evidence.ownerFieldMaturationStatus);
            evidence.resolvedFromCsdProjectTree = jsonBoolField(*ownerPath, "resolvedFromCsdProjectTree").value_or(false);
        }
    }

    evidence.layoutStatus = sonicHudLayoutStatus(evidence.runtimeProject, evidence.localProject);
    return evidence;
}

[[nodiscard]] std::vector<CsdHudSceneCoverageDiagnostic> buildSonicHudSceneCoverageDiagnostics(
    const CsdHudRuntimeSceneEvidence& evidence,
    const CsdDrawableScene& localScene,
    const std::vector<std::uint8_t>& coverageMask)
{
    std::vector<CsdHudSceneCoverageDiagnostic> diagnostics;
    int localCoveredPixels = 0;
    for (const auto value : coverageMask)
    {
        if (value != 0)
            ++localCoveredPixels;
    }

    for (const auto& runtimeScene : evidence.runtimeScenes)
    {
        CsdHudSceneCoverageDiagnostic diagnostic;
        diagnostic.sceneName = runtimeScene.sceneName;
        diagnostic.runtimePath = runtimeScene.path;
        diagnostic.sgfxSlotLabel = sonicHudSlotLabelForScene(runtimeScene.sceneName);
        diagnostic.runtimeCastCount = runtimeScene.castCount;
        diagnostic.runtimeSceneMatched = runtimeScene.sceneName == localScene.sceneName;
        diagnostic.locallyRendered = diagnostic.runtimeSceneMatched;
        if (diagnostic.locallyRendered)
        {
            diagnostic.localCommandCount = static_cast<int>(localScene.commands.size());
            diagnostic.localCoveredPixels = localCoveredPixels;
            diagnostic.localCoverageRatio = static_cast<double>(localCoveredPixels)
                / static_cast<double>(std::max(1, kDesignWidth * kDesignHeight));
        }
        diagnostics.push_back(std::move(diagnostic));
    }

    if (diagnostics.empty())
    {
        CsdHudSceneCoverageDiagnostic diagnostic;
        diagnostic.sceneName = localScene.sceneName;
        diagnostic.sgfxSlotLabel = sonicHudSlotLabelForScene(localScene.sceneName);
        diagnostic.localCommandCount = static_cast<int>(localScene.commands.size());
        diagnostic.localCoveredPixels = localCoveredPixels;
        diagnostic.localCoverageRatio = static_cast<double>(localCoveredPixels)
            / static_cast<double>(std::max(1, kDesignWidth * kDesignHeight));
        diagnostic.locallyRendered = true;
        diagnostics.push_back(std::move(diagnostic));
    }

    return diagnostics;
}

[[nodiscard]] std::pair<int, int> commandCoverageBounds(const CsdDrawableCommand& command, int& startX, int& startY, int& endX, int& endY)
{
    if (command.hidden || command.destinationWidth <= 0 || command.destinationHeight <= 0)
    {
        startX = startY = 0;
        endX = endY = -1;
        return { 0, 0 };
    }

    const double dstX = static_cast<double>(command.destinationX);
    const double dstY = static_cast<double>(command.destinationY);
    const double dstW = static_cast<double>(std::max(1, command.destinationWidth));
    const double dstH = static_cast<double>(std::max(1, command.destinationHeight));
    const double centerX = dstX + (dstW * 0.5);
    const double centerY = dstY + (dstH * 0.5);
    const double radians = command.rotation * kPi / 180.0;
    const double cosTheta = std::cos(radians);
    const double sinTheta = std::sin(radians);

    std::array<std::pair<double, double>, 4> corners{{
        { -dstW * 0.5, -dstH * 0.5 },
        { dstW * 0.5, -dstH * 0.5 },
        { -dstW * 0.5, dstH * 0.5 },
        { dstW * 0.5, dstH * 0.5 },
    }};

    double minX = static_cast<double>(kDesignWidth);
    double minY = static_cast<double>(kDesignHeight);
    double maxX = 0.0;
    double maxY = 0.0;
    for (const auto& [cornerX, cornerY] : corners)
    {
        const double rotatedX = centerX + (cornerX * cosTheta) - (cornerY * sinTheta);
        const double rotatedY = centerY + (cornerX * sinTheta) + (cornerY * cosTheta);
        minX = std::min(minX, rotatedX);
        minY = std::min(minY, rotatedY);
        maxX = std::max(maxX, rotatedX);
        maxY = std::max(maxY, rotatedY);
    }

    startX = std::clamp(static_cast<int>(std::floor(minX)), 0, kDesignWidth - 1);
    startY = std::clamp(static_cast<int>(std::floor(minY)), 0, kDesignHeight - 1);
    endX = std::clamp(static_cast<int>(std::ceil(maxX)), 0, kDesignWidth - 1);
    endY = std::clamp(static_cast<int>(std::ceil(maxY)), 0, kDesignHeight - 1);
    if (endX < startX || endY < startY)
        return { 0, 0 };
    return { (endX - startX) + 1, (endY - startY) + 1 };
}

[[nodiscard]] int countNativeNonBlackPixelsInCommandBounds(
    Gdiplus::Bitmap& native,
    const BitmapComparisonStats& alignmentStats,
    const CsdDrawableCommand& command,
    int& localCoveredPixels)
{
    int startX = 0;
    int startY = 0;
    int endX = -1;
    int endY = -1;
    const auto [width, height] = commandCoverageBounds(command, startX, startY, endX, endY);
    localCoveredPixels = width * height;
    if (localCoveredPixels == 0)
        return 0;

    int nativeNonBlack = 0;
    for (int y = startY; y <= endY; ++y)
    {
        for (int x = startX; x <= endX; ++x)
        {
            const auto nativePixel = bitmapPixelNearest(
                native,
                x,
                y,
                kDesignWidth,
                kDesignHeight,
                alignmentStats.nativeAlignmentCropX,
                alignmentStats.nativeAlignmentCropY,
                alignmentStats.nativeAlignmentCropWidth,
                alignmentStats.nativeAlignmentCropHeight);
            if (nativePixel.GetR() != 0 || nativePixel.GetG() != 0 || nativePixel.GetB() != 0)
                ++nativeNonBlack;
        }
    }
    return nativeNonBlack;
}

[[nodiscard]] std::vector<CsdHudCastCoverageDiagnostic> buildSonicHudCastCoverageDiagnostics(
    const std::vector<CsdDrawableCommand>& commands,
    const std::optional<std::filesystem::path>& nativeBestPath,
    const BitmapComparisonStats& alignmentStats)
{
    std::vector<CsdHudCastCoverageDiagnostic> diagnostics;
    std::unique_ptr<Gdiplus::Bitmap> native;
    if (nativeBestPath)
        native = std::unique_ptr<Gdiplus::Bitmap>(Gdiplus::Bitmap::FromFile(nativeBestPath->wstring().c_str(), FALSE));
    const bool nativeReady = native && native->GetLastStatus() == Gdiplus::Ok;

    for (const auto& command : commands)
    {
        CsdHudCastCoverageDiagnostic diagnostic;
        diagnostic.sceneName = command.sceneName;
        diagnostic.castName = command.castName;
        diagnostic.textureName = command.textureName;
        diagnostic.sgfxSlotLabel = sonicHudSlotLabelForScene(command.sceneName);
        int startX = 0;
        int startY = 0;
        int endX = -1;
        int endY = -1;
        const auto [width, height] = commandCoverageBounds(command, startX, startY, endX, endY);
        diagnostic.destinationX = startX;
        diagnostic.destinationY = startY;
        diagnostic.destinationWidth = width;
        diagnostic.destinationHeight = height;
        diagnostic.localCoveredPixels = width * height;
        if (nativeReady)
        {
            int coveredPixels = 0;
            diagnostic.nativeNonBlackPixels = countNativeNonBlackPixelsInCommandBounds(*native, alignmentStats, command, coveredPixels);
            diagnostic.localCoveredPixels = coveredPixels;
            diagnostic.nativeOverlapRatio = coveredPixels == 0 ? 0.0 : static_cast<double>(diagnostic.nativeNonBlackPixels) / static_cast<double>(coveredPixels);
        }
        diagnostics.push_back(std::move(diagnostic));
    }

    std::sort(
        diagnostics.begin(),
        diagnostics.end(),
        [](const CsdHudCastCoverageDiagnostic& left, const CsdHudCastCoverageDiagnostic& right)
        {
            if (left.nativeNonBlackPixels != right.nativeNonBlackPixels)
                return left.nativeNonBlackPixels > right.nativeNonBlackPixels;
            return left.localCoveredPixels > right.localCoveredPixels;
        });

    return diagnostics;
}

[[nodiscard]] std::size_t countUniqueTextures(const std::vector<CsdDrawableCommand>& commands)
{
    std::vector<std::string> textures;
    for (const auto& command : commands)
    {
        if (std::find(textures.begin(), textures.end(), command.textureName) == textures.end())
            textures.push_back(command.textureName);
    }
    return textures.size();
}

[[nodiscard]] CsdMaterialParityTriage materialParityTriageForComparison(const CsdRenderedFrameComparison& comparison)
{
    CsdMaterialParityTriage triage;
    if (!comparison.visualDelta.nativeFound)
    {
        triage.primaryBlocker = "missing-native-oracle";
        triage.riskFlags.push_back("native-bmp-missing");
        return triage;
    }

    if (!comparison.visualDelta.fullFrame.computed)
    {
        triage.primaryBlocker = "full-frame-diff-missing";
        triage.riskFlags.push_back("diff-bmp-not-written");
        return triage;
    }

    triage.coverageGap = comparison.visualDelta.fullFrame.nativeNonBlackRatio - comparison.visualDelta.fullFrame.renderNonBlackRatio;
    triage.sampledVsFullFrameGap = comparison.visualDelta.meanAbsRgb - comparison.visualDelta.fullFrame.meanAbsRgb;

    if (triage.coverageGap > 0.35 && comparison.visualDelta.fullFrame.renderNonBlackRatio < 0.25)
    {
        const bool stageUiTarget = comparison.templateId == "sonic-hud" || comparison.templateId == "tutorial";
        triage.primaryBlocker = stageUiTarget ? "stage-background-not-rendered" : "native-background-not-rendered";
        triage.riskFlags.push_back(triage.primaryBlocker);
        triage.riskFlags.push_back("native-composite-includes-world-backbuffer");
    }
    else if (comparison.visualDelta.fullFrame.meanAbsRgb <= 12.0)
    {
        triage.primaryBlocker = "low-full-frame-delta";
    }
    else if (comparison.gradientCommandCount != 0 || comparison.gradientTrackSampleCount != 0)
    {
        triage.primaryBlocker = "gradient-material-delta";
        triage.riskFlags.push_back("gradient-material-risk");
    }
    else
    {
        triage.primaryBlocker = "shader-material-delta";
        triage.riskFlags.push_back("shader-material-risk");
    }

    if (comparison.csdPointFilterSampleCount != 0)
        triage.riskFlags.push_back("csd-point-seam-sampler-risk");
    if (comparison.linearFilteringCommandCount != 0 || comparison.bilinearSampleCount != 0)
        triage.riskFlags.push_back("linear-filtering-risk");
    if (comparison.additiveCommandCount != 0)
        triage.riskFlags.push_back("additive-blend-risk");
    if (comparison.decodedPackedKeyframeCount != 0)
        triage.riskFlags.push_back("packed-rgba-timeline-active");
    if (comparison.visualDelta.registrationOffsetX != 0 || comparison.visualDelta.registrationOffsetY != 0)
        triage.riskFlags.push_back("native-frame-registration-shift");
    if (comparison.visualDelta.uiLayerDelta.computed)
        triage.riskFlags.push_back("ui-layer-aware-diff-active");

    return triage;
}

[[nodiscard]] CsdRenderedFrameComparison renderCsdFrameComparison(
    const CsdPipelineTemplateBinding& csdBinding,
    const SgfxTemplateRenderBinding& sgfxBinding,
    const std::filesystem::path& outputRoot)
{
    CsdRenderedFrameComparison comparison;
    comparison.templateId = std::string(csdBinding.templateId);
    comparison.layoutFileName = std::string(csdBinding.layoutFileName);
    comparison.sceneName = std::string(csdBinding.primarySceneName);
    comparison.timelineSceneName = std::string(csdBinding.timelineSceneName);
    comparison.timelineAnimationName = std::string(csdBinding.timelineAnimationName);
    comparison.frame = csdTimelineSampleFrameForTemplate(csdBinding.templateId);
    comparison.renderedFramePath = outputRoot / (std::string(csdBinding.templateId) + "_frame" + std::to_string(comparison.frame) + ".bmp");
    comparison.diffFramePath = outputRoot / (std::string(csdBinding.templateId) + "_frame" + std::to_string(comparison.frame) + "_diff.bmp");
    comparison.uiLayerDiffFramePath = outputRoot / (std::string(csdBinding.templateId) + "_frame" + std::to_string(comparison.frame) + "_ui_layer_diff.bmp");

    for (std::size_t index = 0; index < sgfxBinding.slotCount; ++index)
        comparison.sgfxSlots.emplace_back(sgfxBinding.slots[index].slotName);

    const auto scene = loadCsdDrawableScene(csdBinding);
    const auto playback = loadCsdTimelinePlayback(csdBinding);
    if (!scene)
        return comparison;

    comparison.drawCommandCount = scene->commands.size();
    comparison.textureBindingCount = countUniqueTextures(scene->commands);
    for (const auto& command : scene->commands)
    {
        if (command.colorKnown)
            ++comparison.colorCommandCount;
        if (command.colorRgba.a != 0xFF)
            ++comparison.alphaModulatedCommandCount;
        if (command.gradientKnown)
            ++comparison.gradientCommandCount;
        if (command.additiveBlend)
            ++comparison.additiveCommandCount;
        else
            ++comparison.normalBlendCommandCount;
        if (command.linearFiltering)
            ++comparison.linearFilteringCommandCount;
    }
    if (playback)
    {
        comparison.packedColorTrackCount = static_cast<std::size_t>(std::max(0, playback->packedColorTrackCount));
        comparison.packedGradientTrackCount = static_cast<std::size_t>(std::max(0, playback->packedGradientTrackCount));
        comparison.decodedPackedColorTrackCount = static_cast<std::size_t>(std::max(0, playback->decodedPackedColorTrackCount));
        comparison.decodedPackedGradientTrackCount = static_cast<std::size_t>(std::max(0, playback->decodedPackedGradientTrackCount));
        comparison.decodedPackedKeyframeCount = static_cast<std::size_t>(std::max(0, playback->decodedPackedKeyframeCount));
        comparison.unresolvedPackedKeyframeCount = static_cast<std::size_t>(std::max(0, playback->unresolvedPackedKeyframeCount));
        for (const auto& sample : playback->samples)
        {
            if (isCsdGradientTrack(sample.trackType))
                ++comparison.gradientTrackSampleCount;
        }
        for (const auto& sample : playback->packedRgbaSamples)
        {
            if (isCsdGradientTrack(sample.trackType))
                ++comparison.gradientTrackSampleCount;
        }
    }

    SwardSuUiAssetRenderer renderer;
    std::size_t sampledCommandCount = 0;
    CsdSoftwareRenderStats renderStats;
    std::vector<std::uint8_t> coverageMask;
    auto bitmap = renderCsdOffscreenFrameWithCoverageMask(*scene, playback ? &*playback : nullptr, renderer, sampledCommandCount, renderStats, coverageMask);
    comparison.sampledCommandCount = sampledCommandCount;
    comparison.softwareQuadCommandCount = renderStats.softwareQuadCommandCount;
    comparison.gradientVertexColorCommandCount = renderStats.gradientVertexColorCommandCount;
    comparison.additiveSoftwareCommandCount = renderStats.additiveSoftwareCommandCount;
    comparison.csdPointFilterSampleCount = renderStats.samplerStats.csdPointFilterSampleCount;
    comparison.bilinearSampleCount = renderStats.samplerStats.bilinearSampleCount;
    comparison.nearestSampleCount = renderStats.samplerStats.nearestSampleCount;
    const bool saved = bitmap && saveBitmapAsBmp(*bitmap, comparison.renderedFramePath);
    comparison.nativeBestPath = findNativeBestBmpPathForTarget(csdBinding.templateId);
    comparison.visualDelta = bitmap
        ? computeBitmapComparisonStats(*bitmap, comparison.nativeBestPath, comparison.diffFramePath, coverageMask, comparison.uiLayerDiffFramePath)
        : BitmapComparisonStats{};
    comparison.materialTriage = materialParityTriageForComparison(comparison);
    if (comparison.templateId == "sonic-hud")
    {
        comparison.sonicHudRuntimeScene = loadLatestSonicHudLiveStateEvidence(csdBinding);
        comparison.sonicHudSceneCoverage = buildSonicHudSceneCoverageDiagnostics(comparison.sonicHudRuntimeScene, *scene, coverageMask);
        std::size_t diagnosticSampledCommandCount = 0;
        const auto sampledCommands = timelineSampledCommands(*scene, playback ? &*playback : nullptr, diagnosticSampledCommandCount);
        comparison.sonicHudCastCoverage = buildSonicHudCastCoverageDiagnostics(sampledCommands, comparison.nativeBestPath, comparison.visualDelta);
    }
    if (!saved)
        comparison.renderedFramePath.clear();
    return comparison;
}

void writeCsdRenderCompareManifest(
    const std::filesystem::path& manifestPath,
    const std::vector<CsdRenderedFrameComparison>& comparisons)
{
    std::error_code error;
    std::filesystem::create_directories(manifestPath.parent_path(), error);
    std::ofstream out(manifestPath, std::ios::binary);
    if (!out)
        return;

    out << "{\n";
    out << "  \"phase\": 133,\n";
    out << "  \"canvas\": { \"width\": " << kDesignWidth << ", \"height\": " << kDesignHeight << " },\n";
    out << "  \"records\": [\n";
    for (std::size_t index = 0; index < comparisons.size(); ++index)
    {
        const auto& comparison = comparisons[index];
        out << "    {\n";
        out << "      \"template\": \"" << jsonEscape(comparison.templateId) << "\",\n";
        out << "      \"layout\": \"" << jsonEscape(comparison.layoutFileName) << "\",\n";
        out << "      \"scene\": \"" << jsonEscape(comparison.sceneName) << "\",\n";
        out << "      \"timelineScene\": \"" << jsonEscape(comparison.timelineSceneName) << "\",\n";
        out << "      \"animation\": \"" << jsonEscape(comparison.timelineAnimationName) << "\",\n";
        out << "      \"frame\": " << comparison.frame << ",\n";
        out << "      \"renderedFramePath\": \"" << jsonEscape(portablePath(comparison.renderedFramePath)) << "\",\n";
        out << "      \"diffFramePath\": \"" << jsonEscape(portablePath(comparison.diffFramePath)) << "\",\n";
        out << "      \"uiLayerDiffFramePath\": \"" << jsonEscape(portablePath(comparison.uiLayerDiffFramePath)) << "\",\n";
        out << "      \"nativeBestPath\": \"" << jsonEscape(comparison.nativeBestPath ? portablePath(*comparison.nativeBestPath) : std::string("")) << "\",\n";
        out << "      \"drawCommandCount\": " << comparison.drawCommandCount << ",\n";
        out << "      \"sampledCommandCount\": " << comparison.sampledCommandCount << ",\n";
        out << "      \"textureBindingCount\": " << comparison.textureBindingCount << ",\n";
        out << "      \"materialSemantics\": { \"quadRenderer\": \"software-argb\", \"samplerFilter\": \"csd-point-seam\", \"colorOrder\": \"rgba\", \"blend\": \"src-alpha/inv-src-alpha\", \"additiveBlend\": \"src-alpha/one\", \"colorCommands\": " << comparison.colorCommandCount
            << ", \"alphaModulatedCommands\": " << comparison.alphaModulatedCommandCount
            << ", \"gradientCommands\": " << comparison.gradientCommandCount
            << ", \"gradientApproxCommands\": " << comparison.gradientApproxCommandCount
            << ", \"gradientVertexColorCommands\": " << comparison.gradientVertexColorCommandCount
            << ", \"normalBlendCommands\": " << comparison.normalBlendCommandCount
            << ", \"additiveCommands\": " << comparison.additiveCommandCount
            << ", \"additiveSoftwareCommands\": " << comparison.additiveSoftwareCommandCount
            << ", \"linearFilteringCommands\": " << comparison.linearFilteringCommandCount
            << ", \"softwareQuadCommands\": " << comparison.softwareQuadCommandCount
            << ", \"csdPointFilterSamples\": " << comparison.csdPointFilterSampleCount
            << ", \"bilinearSamples\": " << comparison.bilinearSampleCount
            << ", \"nearestSamples\": " << comparison.nearestSampleCount
            << ", \"gradientTrackSamples\": " << comparison.gradientTrackSampleCount << " },\n";
        out << "      \"channelSemantics\": { \"packedColorTracks\": " << comparison.packedColorTrackCount
            << ", \"packedGradientTracks\": " << comparison.packedGradientTrackCount
            << ", \"decodedPackedColorTracks\": " << comparison.decodedPackedColorTrackCount
            << ", \"decodedPackedGradientTracks\": " << comparison.decodedPackedGradientTrackCount
            << ", \"decodedPackedKeyframes\": " << comparison.decodedPackedKeyframeCount
            << ", \"unresolvedPackedKeyframes\": " << comparison.unresolvedPackedKeyframeCount
            << ", \"status\": \"packed color/gradient keyframes decoded when raw RGBA payload fields are present\" },\n";
        out << "      \"nativeAlignment\": { \"mode\": \"" << jsonEscape(comparison.visualDelta.alignmentMode)
            << "\", \"crop\": { \"x\": " << comparison.visualDelta.nativeAlignmentCropX
            << ", \"y\": " << comparison.visualDelta.nativeAlignmentCropY
            << ", \"width\": " << comparison.visualDelta.nativeAlignmentCropWidth
            << ", \"height\": " << comparison.visualDelta.nativeAlignmentCropHeight
            << " }, \"registration\": { \"offsetX\": " << comparison.visualDelta.registrationOffsetX
            << ", \"offsetY\": " << comparison.visualDelta.registrationOffsetY
            << ", \"candidateCount\": " << comparison.visualDelta.registrationCandidateCount
            << ", \"baseMeanAbsRgb\": " << std::fixed << std::setprecision(3) << comparison.visualDelta.registrationBaseMeanAbsRgb << " } },\n";
        out << "      \"visualDelta\": { \"nativeFound\": " << (comparison.visualDelta.nativeFound ? "true" : "false")
            << ", \"sampleGrid\": \"64x36\", \"alignment\": \"" << jsonEscape(comparison.visualDelta.alignmentMode)
            << "\", \"sampleCount\": " << comparison.visualDelta.sampleCount
            << ", \"meanAbsRgb\": " << std::fixed << std::setprecision(3) << comparison.visualDelta.meanAbsRgb
            << ", \"maxAbsRgb\": " << comparison.visualDelta.maxAbsRgb
            << ", \"renderRgbSum\": " << comparison.visualDelta.rendered.rgbSum
            << ", \"nativeRgbSum\": " << comparison.visualDelta.native.rgbSum
            << ", \"fullFrame\": { \"mode\": \"" << jsonEscape(comparison.visualDelta.fullFrame.mode)
            << "\", \"computed\": " << (comparison.visualDelta.fullFrame.computed ? "true" : "false")
            << ", \"pixels\": " << comparison.visualDelta.fullFrame.pixelCount
            << ", \"exactMatchPixels\": " << comparison.visualDelta.fullFrame.exactMatchPixels
            << ", \"significantDeltaPixels\": " << comparison.visualDelta.fullFrame.significantDeltaPixels
            << ", \"meanAbsRgb\": " << std::fixed << std::setprecision(3) << comparison.visualDelta.fullFrame.meanAbsRgb
            << ", \"maxAbsRgb\": " << comparison.visualDelta.fullFrame.maxAbsRgb
            << ", \"renderNonBlackRatio\": " << std::fixed << std::setprecision(6) << comparison.visualDelta.fullFrame.renderNonBlackRatio
            << ", \"nativeNonBlackRatio\": " << comparison.visualDelta.fullFrame.nativeNonBlackRatio
            << " }, \"uiLayerDelta\": { \"mode\": \"" << jsonEscape(comparison.visualDelta.uiLayerDelta.mode)
            << "\", \"computed\": " << (comparison.visualDelta.uiLayerDelta.computed ? "true" : "false")
            << ", \"maskedPixels\": " << comparison.visualDelta.uiLayerDelta.maskedPixelCount
            << ", \"maskCoverageRatio\": " << std::fixed << std::setprecision(6) << comparison.visualDelta.uiLayerDelta.maskCoverageRatio
            << ", \"exactMatchPixels\": " << comparison.visualDelta.uiLayerDelta.exactMatchPixels
            << ", \"significantDeltaPixels\": " << comparison.visualDelta.uiLayerDelta.significantDeltaPixels
            << ", \"meanAbsRgb\": " << std::fixed << std::setprecision(3) << comparison.visualDelta.uiLayerDelta.meanAbsRgb
            << ", \"maxAbsRgb\": " << comparison.visualDelta.uiLayerDelta.maxAbsRgb
            << ", \"fullFrameMeanAbsRgb\": " << comparison.visualDelta.uiLayerDelta.fullFrameMeanAbsRgb
            << ", \"fullFrameDeltaReduction\": " << comparison.visualDelta.uiLayerDelta.fullFrameDeltaReduction << " } },\n";
        out << "      \"materialParityTriage\": { \"primaryBlocker\": \"" << jsonEscape(comparison.materialTriage.primaryBlocker)
            << "\", \"coverageGap\": " << std::fixed << std::setprecision(6) << comparison.materialTriage.coverageGap
            << ", \"sampledVsFullFrameGap\": " << std::fixed << std::setprecision(3) << comparison.materialTriage.sampledVsFullFrameGap
            << ", \"riskFlags\": [";
        for (std::size_t flagIndex = 0; flagIndex < comparison.materialTriage.riskFlags.size(); ++flagIndex)
        {
            if (flagIndex != 0)
                out << ", ";
            out << "\"" << jsonEscape(comparison.materialTriage.riskFlags[flagIndex]) << "\"";
        }
        out << "] },\n";
        if (comparison.templateId == "sonic-hud")
        {
            const auto& hud = comparison.sonicHudRuntimeScene;
            out << "      \"sonicHudRuntime\": { \"found\": " << (hud.found ? "true" : "false")
                << ", \"liveStatePath\": \"" << jsonEscape(portablePath(hud.liveStatePath)) << "\""
                << ", \"runtimeProject\": \"" << jsonEscape(hud.runtimeProject) << "\""
                << ", \"localLayout\": \"" << jsonEscape(hud.localLayoutFileName) << "\""
                << ", \"localProject\": \"" << jsonEscape(hud.localProject) << "\""
                << ", \"localScene\": \"" << jsonEscape(hud.localSceneName) << "\""
                << ", \"layoutStatus\": \"" << jsonEscape(hud.layoutStatus) << "\""
                << ", \"ownerPathStatus\": \"" << jsonEscape(hud.ownerPathStatus) << "\""
                << ", \"ownerFieldMaturationStatus\": \"" << jsonEscape(hud.ownerFieldMaturationStatus) << "\""
                << ", \"rawOwnerKnown\": " << (hud.rawOwnerKnown ? "true" : "false")
                << ", \"rawOwnerFieldsReady\": " << (hud.rawOwnerFieldsReady ? "true" : "false")
                << ", \"resolvedFromCsdProjectTree\": " << (hud.resolvedFromCsdProjectTree ? "true" : "false")
                << ", \"stageReadyFrame\": " << hud.stageReadyFrame
                << ", \"runtimeSceneCount\": " << hud.runtimeSceneCount
                << ", \"runtimeNodeCount\": " << hud.runtimeNodeCount
                << ", \"runtimeLayerCount\": " << hud.runtimeLayerCount
                << " },\n";
            out << "      \"sonicHudSceneCoverage\": [";
            for (std::size_t sceneIndex = 0; sceneIndex < comparison.sonicHudSceneCoverage.size(); ++sceneIndex)
            {
                const auto& diagnostic = comparison.sonicHudSceneCoverage[sceneIndex];
                if (sceneIndex != 0)
                    out << ", ";
                out << "{ \"scene\": \"" << jsonEscape(diagnostic.sceneName)
                    << "\", \"runtimePath\": \"" << jsonEscape(diagnostic.runtimePath)
                    << "\", \"sgfxSlot\": \"" << jsonEscape(diagnostic.sgfxSlotLabel)
                    << "\", \"runtimeCastCount\": " << diagnostic.runtimeCastCount
                    << ", \"localCommandCount\": " << diagnostic.localCommandCount
                    << ", \"localCoveredPixels\": " << diagnostic.localCoveredPixels
                    << ", \"localCoverageRatio\": " << std::fixed << std::setprecision(6) << diagnostic.localCoverageRatio
                    << ", \"runtimeSceneMatched\": " << (diagnostic.runtimeSceneMatched ? "true" : "false")
                    << ", \"locallyRendered\": " << (diagnostic.locallyRendered ? "true" : "false")
                    << " }";
            }
            out << "],\n";
            out << "      \"sonicHudCastCoverage\": [";
            const std::size_t castLimit = std::min<std::size_t>(comparison.sonicHudCastCoverage.size(), 16);
            for (std::size_t castIndex = 0; castIndex < castLimit; ++castIndex)
            {
                const auto& diagnostic = comparison.sonicHudCastCoverage[castIndex];
                if (castIndex != 0)
                    out << ", ";
                out << "{ \"scene\": \"" << jsonEscape(diagnostic.sceneName)
                    << "\", \"cast\": \"" << jsonEscape(diagnostic.castName)
                    << "\", \"texture\": \"" << jsonEscape(diagnostic.textureName)
                    << "\", \"sgfxSlot\": \"" << jsonEscape(diagnostic.sgfxSlotLabel)
                    << "\", \"destination\": { \"x\": " << diagnostic.destinationX
                    << ", \"y\": " << diagnostic.destinationY
                    << ", \"width\": " << diagnostic.destinationWidth
                    << ", \"height\": " << diagnostic.destinationHeight
                    << " }, \"localCoveredPixels\": " << diagnostic.localCoveredPixels
                    << ", \"nativeNonBlackPixels\": " << diagnostic.nativeNonBlackPixels
                    << ", \"nativeOverlapRatio\": " << std::fixed << std::setprecision(6) << diagnostic.nativeOverlapRatio
                    << " }";
            }
            out << "],\n";
        }
        out << "      \"sgfxSlots\": [";
        for (std::size_t slotIndex = 0; slotIndex < comparison.sgfxSlots.size(); ++slotIndex)
        {
            if (slotIndex != 0)
                out << ", ";
            out << "\"" << jsonEscape(comparison.sgfxSlots[slotIndex]) << "\"";
        }
        out << "]\n";
        out << "    }" << (index + 1 == comparisons.size() ? "\n" : ",\n");
    }
    out << "  ]\n";
    out << "}\n";
}

[[nodiscard]] std::string formatVisualDeltaLine(const CsdRenderedFrameComparison& comparison)
{
    std::ostringstream descriptor;
    descriptor
        << "visual_delta="
        << comparison.templateId
        << ":native="
        << (comparison.visualDelta.nativeFound ? "found" : "missing")
        << ":sample_grid="
        << comparison.visualDelta.sampleGridWidth
        << "x"
        << comparison.visualDelta.sampleGridHeight
        << ":alignment="
        << comparison.visualDelta.alignmentMode
        << ":mean_abs_rgb="
        << std::fixed
        << std::setprecision(3)
        << comparison.visualDelta.meanAbsRgb
        << ":max_abs_rgb="
        << comparison.visualDelta.maxAbsRgb
        << ":render_rgb_sum="
        << comparison.visualDelta.rendered.rgbSum
        << ":native_rgb_sum="
        << comparison.visualDelta.native.rgbSum;
    return descriptor.str();
}

struct CsdReferenceViewerFrameComparison
{
    std::string laneId;
    std::string rendererScreenId;
    std::string contractFileName;
    std::filesystem::path viewerFramePath;
    std::filesystem::path diffFramePath;
    std::optional<std::filesystem::path> nativeBestPath;
    CsdReferenceViewerStats stats;
    BitmapComparisonStats visualDelta;
};

[[nodiscard]] CsdReferenceViewerFrameComparison renderCsdReferenceViewerFrameComparison(
    const CsdReferenceViewerLane& lane,
    const std::filesystem::path& outputRoot)
{
    CsdReferenceViewerFrameComparison comparison;
    comparison.laneId = std::string(lane.laneId);
    comparison.rendererScreenId = std::string(lane.rendererScreenId);
    comparison.contractFileName = std::string(lane.contractFileName);
    comparison.viewerFramePath = outputRoot / (std::string(lane.laneId) + "_viewer.bmp");
    comparison.diffFramePath = outputRoot / (std::string(lane.laneId) + "_viewer_diff.bmp");

    CsdReferenceViewerStats stats;
    auto bitmap = renderCsdReferenceViewerBitmap(lane, false, stats);
    comparison.stats = std::move(stats);
    comparison.nativeBestPath = findNativeBestBmpPathForTarget(lane.nativeTargetId);
    if (bitmap)
    {
        const std::vector<std::uint8_t> emptyCoverageMask;
        const auto uiLayerDiffPath = outputRoot / (std::string(lane.laneId) + "_viewer_ui_layer_diff.bmp");
        comparison.visualDelta = computeBitmapComparisonStats(
            *bitmap,
            comparison.nativeBestPath,
            comparison.diffFramePath,
            emptyCoverageMask,
            uiLayerDiffPath);
        if (!saveBitmapAsBmp(*bitmap, comparison.viewerFramePath))
            comparison.viewerFramePath.clear();
    }

    return comparison;
}

void writeViewerRenderCompareManifest(
    const std::filesystem::path& manifestPath,
    const std::vector<CsdReferenceViewerFrameComparison>& comparisons)
{
    std::error_code error;
    std::filesystem::create_directories(manifestPath.parent_path(), error);
    std::ofstream out(manifestPath, std::ios::binary);
    if (!out)
        return;

    out << "{\n";
    out << "  \"phase\": 139,\n";
    out << "  \"mode\": \"phase139-reference-viewer\",\n";
    out << "  \"operatorOverlay\": { \"mode\": \"compact-reference-status\", \"excludedFromNativeCompare\": true },\n";
    out << "  \"canvas\": { \"width\": " << kDesignWidth << ", \"height\": " << kDesignHeight << " },\n";
    out << "  \"records\": [\n";
    for (std::size_t index = 0; index < comparisons.size(); ++index)
    {
        const auto& comparison = comparisons[index];
        out << "    {\n";
        out << "      \"lane\": \"" << jsonEscape(comparison.laneId) << "\",\n";
        out << "      \"screen\": \"" << jsonEscape(comparison.rendererScreenId) << "\",\n";
        out << "      \"contract\": \"" << jsonEscape(comparison.contractFileName) << "\",\n";
        out << "      \"viewerFramePath\": \"" << jsonEscape(portablePath(comparison.viewerFramePath)) << "\",\n";
        out << "      \"diffFramePath\": \"" << jsonEscape(portablePath(comparison.diffFramePath)) << "\",\n";
        out << "      \"nativeBestPath\": \"" << jsonEscape(comparison.nativeBestPath ? portablePath(*comparison.nativeBestPath) : std::string("")) << "\",\n";
        out << "      \"counts\": { \"scenes\": " << comparison.stats.sceneCount
            << ", \"commands\": " << comparison.stats.commandCount
            << ", \"drawn\": " << comparison.stats.drawnCommandCount
            << ", \"textures\": " << comparison.stats.textureCount << " },\n";
        out << "      \"visualDelta\": { \"nativeFound\": " << (comparison.visualDelta.nativeFound ? "true" : "false")
            << ", \"sampleGrid\": \"64x36\", \"meanAbsRgb\": " << std::fixed << std::setprecision(3) << comparison.visualDelta.meanAbsRgb
            << ", \"maxAbsRgb\": " << comparison.visualDelta.maxAbsRgb
            << ", \"renderRgbSum\": " << comparison.visualDelta.rendered.rgbSum
            << ", \"nativeRgbSum\": " << comparison.visualDelta.native.rgbSum << " },\n";
        out << "      \"scenes\": [";
        for (std::size_t sceneIndex = 0; sceneIndex < comparison.stats.scenes.size(); ++sceneIndex)
        {
            const auto& scene = comparison.stats.scenes[sceneIndex];
            if (sceneIndex != 0)
                out << ", ";
            out << "{ \"layout\": \"" << jsonEscape(scene.layoutFileName)
                << "\", \"scene\": \"" << jsonEscape(scene.sceneName)
                << "\", \"commands\": " << scene.commandCount
                << ", \"drawn\": " << scene.drawnCommandCount
                << ", \"textures\": " << scene.textureCount << " }";
        }
        out << "]\n";
        out << "    }" << (index + 1 == comparisons.size() ? "\n" : ",\n");
    }
    out << "  ]\n";
    out << "}\n";
}

[[nodiscard]] std::string formatViewerVisualDeltaLine(const CsdReferenceViewerFrameComparison& comparison)
{
    std::ostringstream descriptor;
    descriptor
        << "viewer_visual_delta="
        << comparison.laneId
        << ":native="
        << (comparison.visualDelta.nativeFound ? "found" : "missing")
        << ":sample_grid="
        << comparison.visualDelta.sampleGridWidth
        << "x"
        << comparison.visualDelta.sampleGridHeight
        << ":mean_abs_rgb="
        << std::fixed
        << std::setprecision(3)
        << comparison.visualDelta.meanAbsRgb
        << ":max_abs_rgb="
        << comparison.visualDelta.maxAbsRgb
        << ":render_rgb_sum="
        << comparison.visualDelta.rendered.rgbSum
        << ":native_rgb_sum="
        << comparison.visualDelta.native.rgbSum;
    return descriptor.str();
}

[[nodiscard]] int runViewerRenderCompareSmoke()
{
    bool failed = false;
    std::vector<CsdReferenceViewerFrameComparison> comparisons;
    const auto lanes = buildReferenceViewerLanesFromTrackedPolicy();
    const auto outputRoot = repoRootForOutput() / "out" / "viewer_render_compare" / "phase139";
    const auto manifestPath = outputRoot / "viewer_render_compare_manifest.json";

    Gdiplus::GdiplusStartupInput gdiplusInput{};
    ULONG_PTR gdiplusToken = 0;
    if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusInput, nullptr) != Gdiplus::Ok)
        return 1;

    for (const auto& lane : lanes)
    {
        auto comparison = renderCsdReferenceViewerFrameComparison(lane, outputRoot);
        if (comparison.viewerFramePath.empty() || comparison.stats.commandCount == 0)
            failed = true;
        comparisons.push_back(std::move(comparison));
    }

    writeViewerRenderCompareManifest(manifestPath, comparisons);

    std::cout
        << "sward_su_ui_asset_renderer viewer render compare smoke ok "
        << "mode=phase139-reference-viewer"
        << " lanes=" << comparisons.size()
        << " manifest=" << portablePath(manifestPath)
        << '\n';
    std::cout << "viewer_compare_manifest=" << portablePath(manifestPath) << '\n';

    for (const auto& comparison : comparisons)
    {
        std::cout
            << "viewer_frame_path=" << comparison.laneId
            << ":" << portablePath(comparison.viewerFramePath)
            << '\n';
        std::cout
            << "viewer_diff_frame_path=" << comparison.laneId
            << ":" << portablePath(comparison.diffFramePath)
            << '\n';
        std::cout << formatViewerVisualDeltaLine(comparison) << '\n';
        std::cout
            << "viewer_render_source=" << comparison.laneId
            << ":screen=" << comparison.rendererScreenId
            << ":scenes=" << comparison.stats.sceneCount
            << ":commands=" << comparison.stats.commandCount
            << ":drawn=" << comparison.stats.drawnCommandCount
            << ":textures=" << comparison.stats.textureCount
            << ":operator_overlay=compact-reference-status:excluded_from_native_compare=1"
            << '\n';
    }

    std::cout << "operator_overlay=compact-reference-status:excluded_from_native_compare=1\n";
    Gdiplus::GdiplusShutdown(gdiplusToken);
    return failed ? 1 : 0;
}

[[nodiscard]] int runRuntimeCsdTreeExportSmoke(const std::optional<std::string>& templateFilter)
{
    const std::string target = templateFilter.value_or("sonic-hud");
    if (target != "sonic-hud")
    {
        std::cerr << "Runtime CSD tree export currently supports sonic-hud only.\n";
        return 2;
    }

    const auto* csdBinding = findCsdPipelineTemplateBinding("sonic-hud");
    if (!csdBinding)
        return 1;

    const auto evidence = loadLatestSonicHudLiveStateEvidence(*csdBinding);
    if (!evidence.found || evidence.runtimeProject.empty())
        return 1;

    const auto outputRoot = repoRootForOutput() / "out" / "csd_runtime_exports" / "phase134";
    const auto outputPath = outputRoot / (evidence.runtimeProject + "_runtime_tree.json");
    const bool wrote = writeSonicHudRuntimeCsdTreeExport(evidence, outputPath);
    if (!wrote)
        return 1;

    constexpr std::string_view kDrawableStatus = "runtime-scene-layer-tree-exported-no-material-rects";
    std::cout
        << "sward_su_ui_asset_renderer runtime csd export ok "
        << "target=sonic-hud"
        << " project=" << evidence.runtimeProject
        << " scenes=" << evidence.runtimeSceneCount
        << " nodes=" << evidence.runtimeNodeCount
        << " layers=" << evidence.runtimeLayerCount
        << " output=" << portablePath(outputPath)
        << '\n';
    std::cout
        << "runtime_csd_export=sonic-hud"
        << ":source=live-bridge"
        << ":project=" << evidence.runtimeProject
        << ":scenes=" << evidence.runtimeSceneCount
        << ":nodes=" << evidence.runtimeNodeCount
        << ":layers=" << evidence.runtimeLayerCount
        << ":layout_status=" << evidence.layoutStatus
        << ":owner_path_status=" << evidence.ownerPathStatus
        << ":drawable_status=" << kDrawableStatus
        << '\n';

    for (const auto& scene : evidence.runtimeScenes)
    {
        std::cout
            << "runtime_csd_scene=sonic-hud:"
            << scene.path
            << ":casts="
            << scene.castCount
            << ":layers="
            << runtimeLayerCountForScene(evidence, scene.sceneName)
            << ":sgfx_slot="
            << sonicHudSlotLabelForScene(scene.sceneName)
            << ":drawable_status="
            << kDrawableStatus
            << '\n';

        const auto layer = std::find_if(
            evidence.runtimeLayers.begin(),
            evidence.runtimeLayers.end(),
            [&scene](const CsdHudRuntimeLayerEntry& candidate)
            {
                return candidate.sceneName == scene.sceneName;
            });
        if (layer != evidence.runtimeLayers.end())
        {
            std::cout
                << "runtime_csd_layer_sample=sonic-hud:"
                << layer->path
                << ":scene="
                << layer->sceneName
                << ":layer="
                << layer->layerName
                << ":cast_index="
                << layer->castIndex
                << ":layer_address="
                << layer->layerAddress
                << '\n';
        }
    }

    std::cout << "runtime_csd_export_path=" << portablePath(outputPath) << '\n';
    return 0;
}

[[nodiscard]] int runRuntimeCsdMaterialExportSmoke(const std::optional<std::string>& templateFilter)
{
    const std::string target = templateFilter.value_or("sonic-hud");
    if (target != "sonic-hud")
    {
        std::cerr << "Runtime CSD material export currently supports sonic-hud only.\n";
        return 2;
    }

    const auto* csdBinding = findCsdPipelineTemplateBinding("sonic-hud");
    if (!csdBinding)
        return 1;

    const auto evidence = loadLatestSonicHudLiveStateEvidence(*csdBinding);
    if (!evidence.found || evidence.runtimeProject.empty() || evidence.layoutStatus != "exact-runtime-layout")
        return 1;

    const auto entries = buildSonicHudRuntimeMaterialEntries(evidence);
    if (entries.empty())
        return 1;

    auto isResolvedMaterialEntry = [](const CsdHudRuntimeMaterialEntry& entry)
    {
        return entry.subimageIndex >= 0 && entry.textureResolved && entry.sourceFits;
    };

    const int materialResolved = static_cast<int>(std::count_if(
        entries.begin(),
        entries.end(),
        isResolvedMaterialEntry));
    const int subimageResolved = materialResolved;
    const int timelineResolved = static_cast<int>(std::count_if(
        entries.begin(),
        entries.end(),
        [isResolvedMaterialEntry](const CsdHudRuntimeMaterialEntry& entry)
        {
            return isResolvedMaterialEntry(entry) && entry.timelineResolved;
        }));

    const auto outputRoot = repoRootForOutput() / "out" / "csd_runtime_exports" / "phase135";
    const auto outputPath = outputRoot / (evidence.runtimeProject + "_runtime_materials.json");
    const bool wrote = writeSonicHudRuntimeMaterialExport(evidence, entries, outputPath);
    if (!wrote)
        return 1;

    constexpr std::string_view kDrawableStatus = "runtime-material-exact-local-layout";
    std::cout
        << "sward_su_ui_asset_renderer runtime csd material export ok "
        << "target=sonic-hud"
        << " project=" << evidence.runtimeProject
        << " materials=" << materialResolved
        << " output=" << portablePath(outputPath)
        << '\n';
    std::cout
        << "runtime_csd_material_export=sonic-hud"
        << ":source=runtime-tree+exact-local-layout"
        << ":project=" << evidence.runtimeProject
        << ":local_layout=" << evidence.localLayoutFileName
        << ":local_project=" << evidence.localProject
        << ":runtime_layers=" << evidence.runtimeLayerCount
        << ":exported_layers=" << evidence.runtimeLayers.size()
        << ":material_resolved=" << materialResolved
        << ":subimage_resolved=" << subimageResolved
        << ":timeline_resolved=" << timelineResolved
        << ":material_unresolved=" << (static_cast<int>(evidence.runtimeLayers.size()) - materialResolved)
        << ":drawable_status=" << kDrawableStatus
        << '\n';

    for (const auto& scene : evidence.runtimeScenes)
    {
        const std::string localSceneName = localCsdSceneNameForRuntimeScene(scene.sceneName);
        const auto timeline = loadTimelinePlaybackForScene("sonic-hud", evidence.localLayoutFileName, localSceneName);
        const int sceneResolved = materialResolvedCountForRuntimeScene(entries, scene.sceneName);
        const int sceneSubimageResolved = static_cast<int>(std::count_if(
            entries.begin(),
            entries.end(),
            [&scene](const CsdHudRuntimeMaterialEntry& entry)
            {
                return entry.runtimeSceneName == scene.sceneName && entry.subimageIndex >= 0 && entry.textureResolved && entry.sourceFits;
            }));

        std::cout
            << "runtime_csd_material_scene=sonic-hud:"
            << scene.path
            << ":layers=" << runtimeLayerCountForScene(evidence, scene.sceneName)
            << ":material_resolved=" << sceneResolved
            << ":subimage_resolved=" << sceneSubimageResolved;
        if (timeline)
        {
            std::cout
                << ":timeline=" << timeline->animationName
                << "@" << timeline->sampleFrame
                << "/" << static_cast<int>(std::llround(timeline->frameCount));
        }
        else
        {
            std::cout << ":timeline=unresolved";
        }
        std::cout
            << ":sgfx_slot=" << sonicHudSlotLabelForScene(scene.sceneName)
            << '\n';
    }

    const auto sample = std::find_if(
        entries.begin(),
        entries.end(),
        [](const CsdHudRuntimeMaterialEntry& entry)
        {
            return entry.castName == "Cast_0506_bg";
        });
    const auto& sampleEntry = sample != entries.end() ? *sample : entries.front();
    std::cout
        << "runtime_csd_material_sample=sonic-hud:"
        << sampleEntry.runtimePath
        << ":cast=" << sampleEntry.castName
        << ":texture=" << sampleEntry.textureName
        << ":subimage=" << sampleEntry.subimageIndex
        << ":src=" << sampleEntry.sourceX << "," << sampleEntry.sourceY << ","
        << sampleEntry.sourceWidth << "x" << sampleEntry.sourceHeight
        << ":dst=" << sampleEntry.destinationX << "," << sampleEntry.destinationY << ","
        << sampleEntry.destinationWidth << "x" << sampleEntry.destinationHeight
        << ":timeline=" << (sampleEntry.timelineName.empty() ? "unresolved" : sampleEntry.timelineName)
        << "@" << sampleEntry.timelineFrame
        << '\n';

    std::cout << "runtime_csd_material_export_path=" << portablePath(outputPath) << '\n';
    return 0;
}

[[nodiscard]] int runSonicHudCompositorExportSmoke(const std::optional<std::string>& templateFilter)
{
    const std::string target = templateFilter.value_or("sonic-hud");
    if (target != "sonic-hud")
    {
        std::cerr << "Sonic HUD compositor export currently supports sonic-hud only.\n";
        return 2;
    }

    const auto* csdBinding = findCsdPipelineTemplateBinding("sonic-hud");
    if (!csdBinding)
        return 1;

    const auto evidence = loadLatestSonicHudLiveStateEvidence(*csdBinding);
    if (!evidence.found || evidence.runtimeProject.empty() || evidence.layoutStatus != "exact-runtime-layout")
        return 1;

    const auto entries = buildSonicHudRuntimeMaterialEntries(evidence);
    if (entries.empty())
        return 1;

    const auto model = buildSonicHudCompositorModel(evidence, entries);
    const auto outputRoot = repoRootForOutput() / "out" / "csd_runtime_exports" / "phase136";
    const auto manifestPath = outputRoot / "ui_playscreen_hud_compositor.json";
    const auto referenceCodePath = outputRoot / "ui_playscreen_hud_reference.hpp";
    if (!writeSonicHudCompositorManifest(model, manifestPath))
        return 1;
    if (!writeSonicHudReferenceCode(model, referenceCodePath))
        return 1;

    std::cout
        << "sward_su_ui_asset_renderer sonic hud compositor export ok "
        << "target=sonic-hud"
        << " project=" << model.project
        << " scenes=" << model.sceneCount
        << " output=" << portablePath(manifestPath)
        << '\n';
    std::cout
        << "sonic_hud_compositor=sonic-hud"
        << ":source=" << model.source
        << ":project=" << model.project
        << ":scenes=" << model.sceneCount
        << ":runtime_layers=" << model.runtimeLayerCount
        << ":exported_layers=" << model.exportedLayerCount
        << ":drawable_layers=" << model.drawableLayerCount
        << ":structural_layers=" << model.structuralLayerCount
        << ":owner=" << model.owner
        << ":state=" << model.stateEvent
        << ":reference_status=" << model.referenceStatus
        << '\n';

    for (const auto& scene : model.scenes)
    {
        std::cout
            << "sonic_hud_compositor_scene=sonic-hud:"
            << scene.runtimePath
            << ":activation=" << scene.activationEvent
            << ":slot=" << scene.sgfxSlotLabel
            << ":runtime_layers=" << scene.runtimeLayerCount
            << ":drawable_layers=" << scene.drawableLayerCount
            << ":timeline=" << (scene.timelineName.empty() ? "unresolved" : scene.timelineName)
            << "@" << scene.timelineFrame
            << "/" << scene.timelineFrameCount
            << ":textures=" << scene.textureNames.size()
            << '\n';
    }

    std::cout
        << "sonic_hud_reference_owner=" << model.owner
        << ":ownerHook=" << model.ownerHook
        << ":project=" << model.project
        << ":sceneCount=" << model.sceneCount
        << '\n';
    std::cout << "sonic_hud_reference_code_path=" << portablePath(referenceCodePath) << '\n';
    std::cout << "sonic_hud_compositor_manifest_path=" << portablePath(manifestPath) << '\n';
    return 0;
}

[[nodiscard]] int runCsdRenderCompareSmoke(const std::optional<std::string>& templateFilter)
{
    std::size_t templateCount = 0;
    bool failed = false;
    std::vector<CsdRenderedFrameComparison> comparisons;
    const auto outputRoot = repoRootForOutput() / "out" / "csd_render_compare" / "phase133";
    const auto manifestPath = outputRoot / "csd_render_compare_manifest.json";

    Gdiplus::GdiplusStartupInput gdiplusInput{};
    ULONG_PTR gdiplusToken = 0;
    if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusInput, nullptr) != Gdiplus::Ok)
        return 1;

    for (const auto& csdBinding : kCsdPipelineTemplateBindings)
    {
        if (templateFilter && csdBinding.templateId != *templateFilter)
            continue;

        ++templateCount;
        const auto* sgfxBinding = findSgfxTemplateRenderBinding(csdBinding.templateId);
        if (!sgfxBinding)
        {
            failed = true;
            continue;
        }

        auto comparison = renderCsdFrameComparison(csdBinding, *sgfxBinding, outputRoot);
        if (comparison.renderedFramePath.empty() || comparison.drawCommandCount == 0)
            failed = true;
        comparisons.push_back(std::move(comparison));
    }

    if (templateFilter && templateCount == 0)
        failed = true;

    writeCsdRenderCompareManifest(manifestPath, comparisons);

    std::cout
        << "sward_su_ui_asset_renderer csd render compare smoke ok "
        << "layout_source=research_uiux/data/layout_deep_analysis.json "
        << "templates=" << templateCount
        << " manifest=" << portablePath(manifestPath)
        << '\n';
    std::cout << "render_compare_manifest=" << portablePath(manifestPath) << '\n';

    for (const auto& comparison : comparisons)
    {
        std::cout
            << "rendered_frame_path="
            << comparison.templateId
            << ":"
            << portablePath(comparison.renderedFramePath)
            << '\n';
        std::cout
            << "diff_frame_path="
            << comparison.templateId
            << ":"
            << portablePath(comparison.diffFramePath)
            << '\n';
        std::cout
            << "ui_layer_diff_frame_path="
            << comparison.templateId
            << ":"
            << portablePath(comparison.uiLayerDiffFramePath)
            << '\n';
        std::cout
            << "material_semantics="
            << comparison.templateId
            << ":quad_renderer=software-argb"
            << ":sampler_filter=csd-point-seam"
            << ":color_order=rgba"
            << ":blend=src-alpha/inv-src-alpha"
            << ":additive_blend=src-alpha/one"
            << ":color_commands="
            << comparison.colorCommandCount
            << ":alpha_modulated="
            << comparison.alphaModulatedCommandCount
            << ":gradients="
            << comparison.gradientCommandCount
            << ":gradient_average_approx="
            << comparison.gradientApproxCommandCount
            << ":gradient_vertex_color="
            << comparison.gradientVertexColorCommandCount
            << ":additive_commands="
            << comparison.additiveCommandCount
            << ":additive_software="
            << comparison.additiveSoftwareCommandCount
            << ":linear_filter_commands="
            << comparison.linearFilteringCommandCount
            << ":software_quads="
            << comparison.softwareQuadCommandCount
            << ":csd_point_samples="
            << comparison.csdPointFilterSampleCount
            << ":bilinear_samples="
            << comparison.bilinearSampleCount
            << ":nearest_samples="
            << comparison.nearestSampleCount
            << ":gradient_samples="
            << comparison.gradientTrackSampleCount
            << '\n';
        std::cout
            << "channel_semantics="
            << comparison.templateId
            << ":packed_color_tracks="
            << comparison.packedColorTrackCount
            << ":packed_gradient_tracks="
            << comparison.packedGradientTrackCount
            << ":decoded_packed_color_tracks="
            << comparison.decodedPackedColorTrackCount
            << ":decoded_packed_gradient_tracks="
            << comparison.decodedPackedGradientTrackCount
            << ":decoded_packed_keyframes="
            << comparison.decodedPackedKeyframeCount
            << ":unresolved_packed_keyframes="
            << comparison.unresolvedPackedKeyframeCount
            << '\n';
        std::cout
            << "native_alignment="
            << comparison.templateId
            << ":mode="
            << comparison.visualDelta.alignmentMode
            << ":crop="
            << comparison.visualDelta.nativeAlignmentCropX
            << ","
            << comparison.visualDelta.nativeAlignmentCropY
            << ","
            << comparison.visualDelta.nativeAlignmentCropWidth
            << "x"
            << comparison.visualDelta.nativeAlignmentCropHeight
            << '\n';
        std::cout
            << "native_frame_registration="
            << comparison.templateId
            << ":mode="
            << comparison.visualDelta.alignmentMode
            << ":registration_offset="
            << comparison.visualDelta.registrationOffsetX
            << ","
            << comparison.visualDelta.registrationOffsetY
            << ":registration_candidates="
            << comparison.visualDelta.registrationCandidateCount
            << ":base_mean_abs_rgb="
            << std::fixed
            << std::setprecision(3)
            << comparison.visualDelta.registrationBaseMeanAbsRgb
            << ":best_mean_abs_rgb="
            << comparison.visualDelta.meanAbsRgb
            << '\n';
        std::cout << formatVisualDeltaLine(comparison) << '\n';
        std::cout
            << "full_frame_delta="
            << comparison.templateId
            << ":mode="
            << comparison.visualDelta.fullFrame.mode
            << ":pixels="
            << comparison.visualDelta.fullFrame.pixelCount
            << ":exact_match="
            << comparison.visualDelta.fullFrame.exactMatchPixels
            << ":significant="
            << comparison.visualDelta.fullFrame.significantDeltaPixels
            << ":mean_abs_rgb="
            << std::fixed
            << std::setprecision(3)
            << comparison.visualDelta.fullFrame.meanAbsRgb
            << ":max_abs_rgb="
            << comparison.visualDelta.fullFrame.maxAbsRgb
            << ":render_nonblack_ratio="
            << std::fixed
            << std::setprecision(6)
            << comparison.visualDelta.fullFrame.renderNonBlackRatio
            << ":native_nonblack_ratio="
            << comparison.visualDelta.fullFrame.nativeNonBlackRatio
            << '\n';
        std::cout
            << "material_parity_triage="
            << comparison.templateId
            << ":primary="
            << comparison.materialTriage.primaryBlocker
            << ":flags="
            << joinStrings(comparison.materialTriage.riskFlags)
            << ":coverage_gap="
            << std::fixed
            << std::setprecision(6)
            << comparison.materialTriage.coverageGap
            << ":sampled_vs_full_frame_gap="
            << std::fixed
            << std::setprecision(3)
            << comparison.materialTriage.sampledVsFullFrameGap
            << '\n';
        std::cout
            << "ui_layer_delta="
            << comparison.templateId
            << ":mode="
            << comparison.visualDelta.uiLayerDelta.mode
            << ":masked_pixels="
            << comparison.visualDelta.uiLayerDelta.maskedPixelCount
            << ":coverage="
            << std::fixed
            << std::setprecision(6)
            << comparison.visualDelta.uiLayerDelta.maskCoverageRatio
            << ":mean_abs_rgb="
            << std::fixed
            << std::setprecision(3)
            << comparison.visualDelta.uiLayerDelta.meanAbsRgb
            << ":max_abs_rgb="
            << comparison.visualDelta.uiLayerDelta.maxAbsRgb
            << ":significant="
            << comparison.visualDelta.uiLayerDelta.significantDeltaPixels
            << ":full_frame_mean_abs_rgb="
            << comparison.visualDelta.uiLayerDelta.fullFrameMeanAbsRgb
            << ":full_frame_delta_reduction="
            << comparison.visualDelta.uiLayerDelta.fullFrameDeltaReduction
            << '\n';
        std::cout
            << "native_best_path="
            << comparison.templateId
            << ":"
            << (comparison.nativeBestPath ? portablePath(*comparison.nativeBestPath) : std::string("missing"))
            << '\n';
        if (comparison.templateId == "sonic-hud")
        {
            const auto& hud = comparison.sonicHudRuntimeScene;
            std::cout
                << "sonic_hud_runtime_scene=sonic-hud"
                << ":runtime_project="
                << hud.runtimeProject
                << ":local_layout="
                << hud.localLayoutFileName
                << ":local_project="
                << hud.localProject
                << ":local_scene="
                << hud.localSceneName
                << ":layout_status="
                << hud.layoutStatus
                << ":stage_ready_frame="
                << hud.stageReadyFrame
                << ":runtime_scenes="
                << hud.runtimeSceneCount
                << ":runtime_layers="
                << hud.runtimeLayerCount
                << ":owner_path_status="
                << hud.ownerPathStatus
                << ":owner_fields_ready="
                << (hud.rawOwnerFieldsReady ? 1 : 0)
                << ":resolved_from_csd_tree="
                << (hud.resolvedFromCsdProjectTree ? 1 : 0)
                << ":live_state="
                << (hud.found ? portablePath(hud.liveStatePath) : std::string("missing"))
                << '\n';
            for (const auto& diagnostic : comparison.sonicHudSceneCoverage)
            {
                std::cout
                    << "sonic_hud_scene_coverage=sonic-hud"
                    << ":scene="
                    << diagnostic.sceneName
                    << ":runtime_casts="
                    << diagnostic.runtimeCastCount
                    << ":local_commands="
                    << diagnostic.localCommandCount
                    << ":covered_pixels="
                    << diagnostic.localCoveredPixels
                    << ":coverage="
                    << std::fixed
                    << std::setprecision(6)
                    << diagnostic.localCoverageRatio
                    << ":sgfx_slot="
                    << diagnostic.sgfxSlotLabel
                    << ":runtime_scene_matched="
                    << (diagnostic.runtimeSceneMatched ? 1 : 0)
                    << ":locally_rendered="
                    << (diagnostic.locallyRendered ? 1 : 0)
                    << '\n';
            }
            const std::size_t castLimit = std::min<std::size_t>(comparison.sonicHudCastCoverage.size(), 12);
            for (std::size_t castIndex = 0; castIndex < castLimit; ++castIndex)
            {
                const auto& diagnostic = comparison.sonicHudCastCoverage[castIndex];
                std::cout
                    << "sonic_hud_cast_coverage=sonic-hud"
                    << ":scene="
                    << diagnostic.sceneName
                    << ":cast="
                    << diagnostic.castName
                    << ":texture="
                    << diagnostic.textureName
                    << ":sgfx_slot="
                    << diagnostic.sgfxSlotLabel
                    << ":dst="
                    << diagnostic.destinationX
                    << ","
                    << diagnostic.destinationY
                    << ","
                    << diagnostic.destinationWidth
                    << "x"
                    << diagnostic.destinationHeight
                    << ":local_pixels="
                    << diagnostic.localCoveredPixels
                    << ":native_nonblack_pixels="
                    << diagnostic.nativeNonBlackPixels
                    << ":native_overlap="
                    << std::fixed
                    << std::setprecision(6)
                    << diagnostic.nativeOverlapRatio
                    << '\n';
            }
        }
        std::cout
            << "render_frame_source="
            << comparison.templateId
            << ":layout="
            << comparison.layoutFileName
            << ":scene="
            << comparison.sceneName
            << ":timeline="
            << comparison.timelineSceneName
            << "/"
            << comparison.timelineAnimationName
            << ":frame="
            << comparison.frame
            << ":commands="
            << comparison.drawCommandCount
            << ":sampled_commands="
            << comparison.sampledCommandCount
            << ":textures="
            << comparison.textureBindingCount
            << '\n';
    }

    Gdiplus::GdiplusShutdown(gdiplusToken);
    return failed || templateCount == 0 ? 1 : 0;
}

[[nodiscard]] int runCsdPipelineSmoke(const std::optional<std::string>& templateFilter)
{
    std::size_t templateCount = 0;
    bool failed = false;
    std::vector<std::string> uniquePackages;
    std::vector<std::pair<std::string, CsdPipelineEvidence>> loadedEvidence;
    std::vector<std::string> descriptors;

    auto evidenceFor = [&loadedEvidence](std::string_view layoutFileName) -> const CsdPipelineEvidence*
    {
        const auto found = std::find_if(
            loadedEvidence.begin(),
            loadedEvidence.end(),
            [layoutFileName](const std::pair<std::string, CsdPipelineEvidence>& entry)
            {
                return entry.first == layoutFileName;
            });
        if (found != loadedEvidence.end())
            return &found->second;

        const auto evidence = loadCsdPipelineEvidence(layoutFileName);
        if (!evidence)
            return nullptr;

        loadedEvidence.emplace_back(std::string(layoutFileName), *evidence);
        return &loadedEvidence.back().second;
    };

    for (const auto& csdBinding : kCsdPipelineTemplateBindings)
    {
        if (templateFilter && csdBinding.templateId != *templateFilter)
            continue;

        ++templateCount;
        if (std::find(uniquePackages.begin(), uniquePackages.end(), csdBinding.layoutFileName) == uniquePackages.end())
            uniquePackages.emplace_back(csdBinding.layoutFileName);

        const auto* evidence = evidenceFor(csdBinding.layoutFileName);
        const auto* sgfxBinding = findSgfxTemplateRenderBinding(csdBinding.templateId);
        if (!evidence || !sgfxBinding)
        {
            failed = true;
            continue;
        }

        const auto* scene = findCsdPipelineScene(*evidence, csdBinding.primarySceneName);
        const auto* timeline = findCsdPipelineTimelineHook(*evidence, csdBinding.timelineSceneName, csdBinding.timelineAnimationName);
        if (!scene || !timeline)
            failed = true;

        std::ostringstream pipelineDescriptor;
        pipelineDescriptor
            << "csd_pipeline="
            << csdBinding.templateId
            << ":layout="
            << csdBinding.layoutFileName
            << ":scene="
            << csdBinding.primarySceneName
            << ":casts="
            << (scene ? scene->castCount : 0)
            << ":subimages="
            << (scene ? scene->subimageCount : 0)
            << ":textures="
            << joinStrings(evidence->textureNames);
        descriptors.push_back(pipelineDescriptor.str());

        if (timeline)
        {
            std::ostringstream timelineDescriptor;
            timelineDescriptor
                << "timeline="
                << timeline->sceneName
                << "/"
                << timeline->animationName
                << "/"
                << formatCsdNumber(timeline->frameCount)
                << "/"
                << formatCsdNumber(timeline->timelineSeconds)
                << ":keyframes="
                << timeline->totalKeyframes;
            descriptors.push_back(timelineDescriptor.str());
        }

        for (std::size_t index = 0; index < sgfxBinding->slotCount; ++index)
        {
            const auto& slot = sgfxBinding->slots[index];
            std::ostringstream mapDescriptor;
            mapDescriptor
                << "sgfx_element_map="
                << csdBinding.templateId
                << ":scene="
                << csdBinding.primarySceneName
                << ":slot="
                << slot.slotName
                << ":texture="
                << slot.textureName;
            descriptors.push_back(mapDescriptor.str());
        }

        const auto manifest = findRuntimeEvidenceManifestForTarget(csdBinding.templateId);
        std::ostringstream evidenceDescriptor;
        evidenceDescriptor
            << "runtime_evidence_compare="
            << csdBinding.templateId
            << ":target="
            << csdBinding.templateId
            << ":event="
            << sgfxBinding->requiredEventId
            << ":manifest="
            << (manifest ? "found" : "missing");
        descriptors.push_back(evidenceDescriptor.str());
    }

    if (templateFilter && templateCount == 0)
        failed = true;

    std::cout
        << "sward_su_ui_asset_renderer csd pipeline smoke ok "
        << "layout_source=research_uiux/data/layout_deep_analysis.json "
        << "templates=" << templateCount
        << " packages=" << uniquePackages.size()
        << '\n';

    for (const auto& descriptor : descriptors)
        std::cout << descriptor << '\n';

    return failed || templateCount == 0 ? 1 : 0;
}

[[nodiscard]] std::vector<std::string> commandLineTokens()
{
    std::vector<std::string> tokens;
    std::istringstream stream(GetCommandLineA());
    std::string token;
    while (stream >> token)
        tokens.push_back(token);
    return tokens;
}

[[nodiscard]] bool commandLineHasFlag(std::string_view flag)
{
    const auto tokens = commandLineTokens();
    const std::string flagText(flag);
    return std::find(tokens.begin(), tokens.end(), flagText) != tokens.end();
}

[[nodiscard]] std::optional<std::string> commandLineValueAfter(std::string_view flag)
{
    const auto tokens = commandLineTokens();
    const std::string flagText(flag);
    const std::string prefix = flagText + "=";
    for (std::size_t index = 0; index < tokens.size(); ++index)
    {
        if (tokens[index] == flagText && index + 1 < tokens.size())
            return tokens[index + 1];
        if (tokens[index].starts_with(prefix))
            return tokens[index].substr(prefix.size());
    }

    return std::nullopt;
}

[[nodiscard]] int runRendererWindow(HINSTANCE instance, int showCommand, const std::optional<std::string>& initialTemplate)
{
    Gdiplus::GdiplusStartupInput gdiplusInput{};
    ULONG_PTR gdiplusToken = 0;
    if (Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusInput, nullptr) != Gdiplus::Ok)
        return 1;

    const wchar_t* className = L"SwardSuUiAssetRendererWindow";
    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = rendererWindowProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    windowClass.lpszClassName = className;
    RegisterClassW(&windowClass);

    SwardSuUiAssetRenderer renderer;
    if (initialTemplate && !renderer.selectSgfxTemplate(*initialTemplate))
    {
        std::cerr << "Unknown SGFX template: " << *initialTemplate << '\n';
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return 2;
    }

    HWND window = CreateWindowExW(
        0,
        className,
        L"SWARD SU UI Asset Renderer",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1380,
        820,
        nullptr,
        nullptr,
        instance,
        &renderer);

    if (!window)
    {
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return 1;
    }

    ShowWindow(window, showCommand);
    UpdateWindow(window);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    Gdiplus::GdiplusShutdown(gdiplusToken);
    return static_cast<int>(message.wParam);
}
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int showCommand)
{
    const auto templateFilter = commandLineValueAfter("--template");
    if (commandLineHasFlag("--export-runtime-csd-tree"))
        return runRuntimeCsdTreeExportSmoke(templateFilter);
    if (commandLineHasFlag("--export-runtime-csd-materials"))
        return runRuntimeCsdMaterialExportSmoke(templateFilter);
    if (commandLineHasFlag("--export-sonic-hud-compositor"))
        return runSonicHudCompositorExportSmoke(templateFilter);
    if (commandLineHasFlag("--csd-render-compare-smoke"))
        return runCsdRenderCompareSmoke(templateFilter);
    if (commandLineHasFlag("--viewer-render-compare-smoke"))
        return runViewerRenderCompareSmoke();
    if (commandLineHasFlag("--csd-timeline-smoke"))
        return runCsdTimelineSmoke(templateFilter);
    if (commandLineHasFlag("--csd-drawable-smoke"))
        return runCsdDrawableSmoke(templateFilter);
    if (commandLineHasFlag("--csd-pipeline-smoke"))
        return runCsdPipelineSmoke(templateFilter);
    if (commandLineHasFlag("--sgfx-template-smoke"))
        return runSgfxTemplateSmoke(templateFilter);
    if (commandLineHasFlag("--renderer-smoke"))
        return runRendererSmoke();
    if (commandLineHasFlag("--renderer-navigation-smoke"))
        return runRendererNavigationSmoke();
    if (commandLineHasFlag("--renderer-atlas-gallery-smoke"))
        return runRendererAtlasGallerySmoke();
    if (commandLineHasFlag("--renderer-title-screen-smoke"))
        return runRendererTitleScreenSmoke();
    if (commandLineHasFlag("--renderer-reconstructed-screen-smoke"))
        return runRendererReconstructedScreenSmoke();
    if (commandLineHasFlag("--renderer-sonic-hud-reference-smoke"))
        return runRendererSonicHudReferencePolicySmoke();
    if (commandLineHasFlag("--renderer-reference-lanes-smoke"))
        return runRendererReferenceLanesSmoke();
    if (commandLineHasFlag("--renderer-runtime-alignment-smoke"))
        return runRendererRuntimeAlignmentSmoke();
    if (commandLineHasFlag("--renderer-live-bridge-alignment-smoke"))
        return runRendererLiveBridgeAlignmentSmoke();
    if (commandLineHasFlag("--renderer-ui-oracle-smoke"))
        return runRendererUiOracleSmoke();
    if (commandLineHasFlag("--renderer-ui-oracle-playback-smoke"))
        return runRendererUiOraclePlaybackSmoke();
    if (commandLineHasFlag("--renderer-ui-drawable-oracle-smoke"))
        return runRendererUiDrawableOracleSmoke();
    if (commandLineHasFlag("--renderer-ui-draw-list-triage-smoke"))
        return runRendererUiDrawListTriageSmoke();
    if (commandLineHasFlag("--renderer-gpu-submit-triage-smoke"))
        return runRendererGpuSubmitTriageSmoke();
    if (commandLineHasFlag("--renderer-material-correlation-smoke"))
        return runRendererMaterialCorrelationSmoke();
    if (commandLineHasFlag("--renderer-backend-resolved-triage-smoke"))
        return runRendererBackendResolvedTriageSmoke();
    if (commandLineHasFlag("--renderer-material-parity-hints-smoke"))
        return runRendererMaterialParityHintsSmoke();
    if (commandLineHasFlag("--renderer-descriptor-semantics-smoke"))
        return runRendererDescriptorSemanticsSmoke();
    if (commandLineHasFlag("--renderer-vendor-resource-capture-smoke"))
        return runRendererVendorResourceCaptureSmoke();
    if (commandLineHasFlag("--renderer-reference-policy-export-smoke"))
        return runReferencePolicyExportSmoke();

    return runRendererWindow(instance, showCommand, templateFilter);
}
