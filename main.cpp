#include <mod/amlmod.h>
#include <mod/logger.h>
#include <mod/config.h>

#include <stdint.h>
#include <math.h>
#include <time.h>

#ifdef GTA3_TARGET
#include <android/log.h>
#define GTA3SKIES_LOG(...) __android_log_print(ANDROID_LOG_INFO, "GTA3Skies", __VA_ARGS__)
#else
#define GTA3SKIES_LOG(...)
#endif

#define STARRY_SKIES

#ifndef GTA3_TARGET
#define IMPROVED_MOON
#endif

#define IMPROVED_MOON_HEIGHT    (50.0f)
#define STARS_STARTHOUR         22
#define START_LASTHOUR          5



uintptr_t pGame;
void *hGame;
bool hasJPatch15;

#ifdef GTA3_TARGET
static bool CanSeeOutsideFallback() { return true; }
static float ExtraSunnynessFallback = 0.0f;
static float AspectRatioFallback = 4.0f / 3.0f;
static float ScreenWidthFallback = 2340.0f;
static float ScreenHeightFallback = 1080.0f;
static uint32_t MoonSizeFallback = 0;
static bool ForceVisibleClouds = false;
static bool LogRenderHook = true;
static bool LogCameraCandidates = false;
static bool RenderFluffyClouds = true;
static bool DebugSprite = false;
static bool DebugSpriteScreenSpace = false;
static bool GTA3ScreenSpaceLowClouds = false;
static bool GTA3LowCloudUseCoronaTexture = false;
static bool GTA3DrawLowCloudsInBackground = false;
static bool GTA3DrawCloudsAfterScene = false;
static bool GTA3DrawCloudsInHorizon = false;
static bool GTA3DrawSkyBeforeWorld = true;
static bool GTA3MoonMovesWithTime = false;
static bool GTA3FixRainbowUpdate = true;
static bool GTA3DebugForceRainbow = false;
static uint32_t LastRainbowSkyLog = 0;
static uint32_t LastRainbowCloudLog = 0;
static uint32_t CameraPosOffset = 0x34;
static float DebugSpriteDistance = 120.0f;
static float DebugSpriteSize = 80.0f;
static float GTA3DebugForceRainbowValue = 1.0f;
static float GTA3RainbowWorldX = 35.0f;
static float GTA3RainbowWorldY = -100.0f;
static float GTA3RainbowWorldZ = 22.0f;
static float GTA3RainbowWorldSpacing = 1.5f;
static float GTA3RainbowWorldWidthScale = 2.0f;
static float GTA3RainbowWorldHeightScale = 50.0f;
static float GTA3LowCloudScreenY = 0.18f;
static float GTA3LowCloudWidth = 650.0f;
static float GTA3LowCloudHeight = 110.0f;
static float GTA3LowCloudDriftSpeed = 0.012f;
static float GTA3CelestialCloudFadeScale = 0.65f;
static float GTA3MovingMoonDistance = 150.0f;
static float GTA3MovingMoonHeightScale = 50.0f;
static bool GTA3UseRe3LowClouds = true;
static float GTA3Re3LowCloudDistanceScale = 800.0f;
static float GTA3Re3LowCloudBaseZ = 40.0f;
static float GTA3Re3LowCloudZScale = 60.0f;
static float GTA3Re3LowCloudWidthScale = 320.0f;
static float GTA3Re3LowCloudHeightScale = 40.0f;
static int32_t GTA3LowCloudAlpha = 255;
static uint32_t RenderHookCounter = 0;
static uint32_t BackgroundHookCounter = 0;
static uint32_t HorizonHookCounter = 0;
static uint32_t BeforeWorldHookCounter = 0;
#endif

#include "SimpleGTA.h"
#include "vars.inl"
#ifdef STARRY_SKIES
    #include "starryskies.inl"
#else
    float StarCoorsX[9] = { 0.0f, 0.05f, 0.13f, 0.4f, 0.7f, 0.6f, 0.27f, 0.55f, 0.75f };
    float StarCoorsY[9] = { 0.0f, 0.45f, 0.9f, 1.0f, 0.85f, 0.52f, 0.48f, 0.35f, 0.2f };
    float StarSizes[9] = { 1.0f, 1.4f, 0.9f, 1.0f, 0.6f, 1.5f, 1.3f, 1.0f, 0.8f };
#endif



#ifdef GTA3_TARGET
extern float LowCloudsX[12];
extern float LowCloudsY[12];
extern float LowCloudsZ[12];
extern float CoorsOffsetX[37];
extern float CoorsOffsetY[37];
extern float CoorsOffsetZ[37];

static void GTA3RenderCloudLayer(int& lowCloudSprites, int& fluffyCloudSprites);
static void GTA3RenderSkyLayer(int& lowCloudSprites, int& fluffyCloudSprites);

uint8_t BowRed[6]   = { 30, 30, 30, 10,  0, 15 };
uint8_t BowGreen[6] = {  0, 15, 30, 30,  0,  0 };
uint8_t BowBlue[6]  = {  0,  0,  0, 10, 30, 30 };

static float GTA3GetScreenWidth()
{
    return (RsGlobal && RsGlobal->x > 1) ? (float)RsGlobal->x : ScreenWidthFallback;
}

static float GTA3GetScreenHeight()
{
    return (RsGlobal && RsGlobal->y > 1) ? (float)RsGlobal->y : ScreenHeightFallback;
}

static float GTA3GetWideScreenCorrection()
{
    const float screenH = GTA3GetScreenHeight();
    if(screenH <= 1.0f) return 1.0f;
    const float screenAspect = GTA3GetScreenWidth() / screenH;
    const float correction = screenAspect / (4.0f / 3.0f);
    return correction > 0.01f ? correction : 1.0f;
}

static void GTA3FixSpriteAspect(float& szx)
{
    szx /= GTA3GetWideScreenCorrection();
}

static uint8_t ClampToByte(int32_t value)
{
    if(value < 0) return 0;
    if(value > 255) return 255;
    return (uint8_t)value;
}

static void GTA3RenderRainbowLayer(const char* tag, uint32_t& lastLog, CVector* camPos)
{
    if(!Rainbow || *Rainbow == 0.0f || !gpCoronaTexture) return;

    RwRenderStateSet(1, *(gpCoronaTexture[0]));
    RwRenderStateSet(10, (void*)2);
    RwRenderStateSet(11, (void*)2);
    InitSpriteBuffer();

    int rainbowSprites = 0;
    RwV3d firstScreen = { 0.0f, 0.0f, 0.0f };

    if(camPos)
    {
        for(int i = 0; i < 6; ++i)
        {
            RwV3d pos = {
                GTA3RainbowWorldX + (float)i * GTA3RainbowWorldSpacing,
                GTA3RainbowWorldY,
                GTA3RainbowWorldZ
            };
            RwV3d worldpos = pos + *camPos;
            RwV3d screenpos;
            float szx, szy;
            if(CalcScreenCoors(&worldpos, &screenpos, &szx, &szy, false))
            {
                if(rainbowSprites == 0) firstScreen = screenpos;
                ++rainbowSprites;
                GTA3FixSpriteAspect(szx);
                RenderBufferedOneXLUSprite(screenpos, GTA3RainbowWorldWidthScale * szx, GTA3RainbowWorldHeightScale * szy,
                                           BowRed[i] * *Rainbow,
                                           BowGreen[i] * *Rainbow,
                                           BowBlue[i] * *Rainbow,
                                           255, 1.0f / screenpos.z, 255);
            }
        }
    }

    FlushSpriteBuffer();

    uint32_t now = m_snTimeInMilliseconds ? *m_snTimeInMilliseconds : 0;
    if(LogRenderHook && now - lastLog > 2500)
    {
        lastLog = now;
        GTA3SKIES_LOG("%s sprites=%d first=(%.1f %.1f %.2f) value=%.3f world=(%.1f %.1f %.1f)",
                      tag, rainbowSprites, firstScreen.x, firstScreen.y, firstScreen.z, *Rainbow,
                      GTA3RainbowWorldX, GTA3RainbowWorldY, GTA3RainbowWorldZ);
    }
}

static void GTA3ForceLowCloudColour(int& r, int& g, int& b, float& lowcintens)
{
    if(lowcintens < 0.65f) lowcintens = 0.65f;
    if(r + g + b < 60)
    {
        r = 190;
        g = 190;
        b = 190;
    }
}

static float GTA3Fract(float value)
{
    return value - floorf(value);
}

static void GTA3RenderScreenSpaceLowClouds(int r, int g, int b, int& lowCloudSprites)
{
    if(!GTA3ScreenSpaceLowClouds || !gpCloudTex || !gpCoronaTexture) return;

    RwRenderStateSet(8, (void*)0);
    RwRenderStateSet(6, (void*)GTA3DrawCloudsAfterScene);
    RwRenderStateSet(12, (void*)1);
    RwRenderStateSet(10, (void*)2);
    RwRenderStateSet(11, (void*)2);
    InitSpriteBuffer();

    const float screenW = GTA3GetScreenWidth();
    const float screenH = GTA3GetScreenHeight();
    const float xNorms[12] = { 0.04f, 0.16f, 0.29f, 0.41f, 0.54f, 0.67f, 0.80f, 0.93f, 0.10f, 0.35f, 0.61f, 0.86f };
    const float yOffsets[12] = { 0.00f, 0.03f, -0.02f, 0.02f, -0.01f, 0.04f, 0.00f, 0.03f, 0.09f, 0.08f, 0.10f, 0.07f };
    const float sizeScales[12] = { 1.08f, 0.88f, 1.18f, 0.96f, 1.12f, 0.84f, 1.06f, 0.94f, 0.78f, 0.90f, 0.82f, 0.86f };
    const float drift = m_snTimeInMilliseconds ? fmodf((float)(*m_snTimeInMilliseconds) * GTA3LowCloudDriftSpeed, screenW) : 0.0f;
    const float baseY = screenH * GTA3LowCloudScreenY;
    const uint8_t cr = ClampToByte(r);
    const uint8_t cg = ClampToByte(g);
    const uint8_t cb = ClampToByte(b);
    const short alpha = (short)ClampToByte(GTA3LowCloudAlpha);

    RwRenderStateSet(1, GTA3LowCloudUseCoronaTexture ? *(gpCoronaTexture[0]) : *(gpCloudTex[0]));
    for(int i = 0; i < 12; ++i)
    {
        float x = fmodf(xNorms[i] * screenW + drift, screenW);
        if(x < 0.0f) x += screenW;
        RwV3d screenpos = { x, baseY + yOffsets[i] * screenH, 1.0f };

        const float width = GTA3LowCloudWidth * sizeScales[i];
        const float height = GTA3LowCloudHeight * (0.80f + 0.06f * (float)(i % 4));
        const float rotation = ((i & 1) ? 0.045f : -0.035f) + *ms_cameraRoll;
        RenderBufferedOneXLUSprite_Rotate_Dimension(screenpos, width, height, cr, cg, cb, alpha, 1.0f, rotation, 255);
        ++lowCloudSprites;

        if(x < width * 0.5f || x > screenW - width * 0.5f)
        {
            screenpos.x = (x < screenW * 0.5f) ? x + screenW : x - screenW;
            RenderBufferedOneXLUSprite_Rotate_Dimension(screenpos, width, height, cr, cg, cb, alpha, 1.0f, rotation, 255);
            ++lowCloudSprites;
        }
    }
    FlushSpriteBuffer();
}

static void GTA3RenderRe3LowClouds(int r, int g, int b, int& lowCloudSprites)
{
    if(!gpCloudTex || !CamPos) return;

    RwRenderStateSet(8, (void*)0);
    RwRenderStateSet(6, (void*)0);
    RwRenderStateSet(12, (void*)1);
    RwRenderStateSet(10, (void*)2);
    RwRenderStateSet(11, (void*)2);
    InitSpriteBuffer();

    const uint8_t cr = ClampToByte(r);
    const uint8_t cg = ClampToByte(g);
    const uint8_t cb = ClampToByte(b);
    const short alpha = (short)ClampToByte(GTA3LowCloudAlpha);

    for(int cloudtype = 0; cloudtype < 3; ++cloudtype)
    {
        RwRenderStateSet(1, *(gpCloudTex[cloudtype]));
        for(int i = cloudtype; i < 12; i += 3)
        {
            RwV3d worldpos = {
                CamPos->x + GTA3Re3LowCloudDistanceScale * LowCloudsX[i],
                CamPos->y + GTA3Re3LowCloudDistanceScale * LowCloudsY[i],
                GTA3Re3LowCloudBaseZ + GTA3Re3LowCloudZScale * LowCloudsZ[i]
            };

            RwV3d screenpos;
            float szx, szy;
            if(CalcScreenCoors(&worldpos, &screenpos, &szx, &szy, false))
            {
                GTA3FixSpriteAspect(szx);
                RenderBufferedOneXLUSprite_Rotate_Dimension(screenpos,
                                                            szx * GTA3Re3LowCloudWidthScale,
                                                            szy * GTA3Re3LowCloudHeightScale,
                                                            cr, cg, cb, alpha,
                                                            1.0f / screenpos.z,
                                                            *ms_cameraRoll,
                                                            255);
                ++lowCloudSprites;
            }
        }
        FlushSpriteBuffer();
    }
}

static void GTA3RenderRe3FluffyClouds(float coverage, float decoverage, int& fluffyCloudSprites)
{
    if(!RenderFluffyClouds || !gpCloudTex || !CamPos) return;

    float screenW = GTA3GetScreenWidth();
    float distLimit = screenW * 0.5f;
    float sundistBlocked = screenW * 0.1f;
    float sundistHilit = screenW * 0.333333f;

    static bool bCloudOnScreen[37];
    static float fSunDist[37];
    static float fCloudHighlight[37];

    int fluffyalpha = 160 * (1.0f - *Foggyness);
    if(fluffyalpha <= 0) return;

    float rot_sin = sinf(*CloudRotation);
    float rot_cos = cosf(*CloudRotation);
    float rotationValue = ((2.0f * M_PI) * (uint16_t)*IndividualRotation) / 65536.0f + *ms_cameraRoll;

    RwRenderStateSet(8, (void*)0);
    RwRenderStateSet(6, (void*)0);
    RwRenderStateSet(12, (void*)1);
    RwRenderStateSet(10, (void*)5);
    RwRenderStateSet(11, (void*)6);
    RwRenderStateSet(1, *(gpCloudTex[4]));
    InitSpriteBuffer();

    for(int i = 0; i < 37; ++i)
    {
        RwV3d pos = { 2.0f * CoorsOffsetX[i], 2.0f * CoorsOffsetY[i], 40.0f * CoorsOffsetZ[i] + 40.0f };
        RwV3d worldpos = {
            pos.x * rot_cos + pos.y * rot_sin + CamPos->x,
            pos.x * rot_sin - pos.y * rot_cos + CamPos->y,
            pos.z
        };

        RwV3d screenpos;
        float szx, szy;
        if(CalcScreenCoors(&worldpos, &screenpos, &szx, &szy, false))
        {
            float sunDx = screenpos.x - *SunScreenX;
            float sunDy = screenpos.y - *SunScreenY;
            fSunDist[i] = sqrtf(sunDx * sunDx + sunDy * sunDy);
            fCloudHighlight[i] = 0.0f;

            int tr = *m_nCurrentFluffyCloudsTopRed;
            int tg = *m_nCurrentFluffyCloudsTopGreen;
            int tb = *m_nCurrentFluffyCloudsTopBlue;
            int br = *m_nCurrentFluffyCloudsBottomRed;
            int bg = *m_nCurrentFluffyCloudsBottomGreen;
            int bb = *m_nCurrentFluffyCloudsBottomBlue;

            if(fSunDist[i] < distLimit)
            {
                fCloudHighlight[i] = decoverage * (1.0f - fSunDist[i] / distLimit);
                tr = tr * (1.0f - fCloudHighlight[i]) + 255 * fCloudHighlight[i];
                tg = tg * (1.0f - fCloudHighlight[i]) + 190 * fCloudHighlight[i];
                tb = tb * (1.0f - fCloudHighlight[i]) + 190 * fCloudHighlight[i];
                br = br * (1.0f - fCloudHighlight[i]) + 255 * fCloudHighlight[i];
                bg = bg * (1.0f - fCloudHighlight[i]) + 190 * fCloudHighlight[i];
                bb = bb * (1.0f - fCloudHighlight[i]) + 190 * fCloudHighlight[i];
                if(fSunDist[i] < sundistBlocked) *SunBlockedByClouds = (fluffyalpha > 50);
            }

            GTA3FixSpriteAspect(szx);
            RenderBufferedOneXLUSprite_Rotate_2Colours(screenpos, szx * 55.0f, szy * 55.0f, tr, tg, tb, br, bg, bb, 0.0f, -1.0f, 1.0f / screenpos.z, rotationValue, fluffyalpha);
            ++fluffyCloudSprites;
            bCloudOnScreen[i] = true;
        }
        else
        {
            bCloudOnScreen[i] = false;
        }
    }
    FlushSpriteBuffer();

    RwRenderStateSet(10, (void*)2);
    RwRenderStateSet(11, (void*)2);
    RwRenderStateSet(1, *(gpCloudTex[3]));
    InitSpriteBuffer();
    for(int i = 0; i < 37; ++i)
    {
        RwV3d pos = { 2.0f * CoorsOffsetX[i], 2.0f * CoorsOffsetY[i], 40.0f * CoorsOffsetZ[i] + 40.0f };
        RwV3d worldpos = {
            pos.x * rot_cos + pos.y * rot_sin + CamPos->x,
            pos.x * rot_sin - pos.y * rot_cos + CamPos->y,
            pos.z
        };

        RwV3d screenpos;
        float szx, szy;
        if(bCloudOnScreen[i] && fSunDist[i] < sundistHilit && CalcScreenCoors(&worldpos, &screenpos, &szx, &szy, false))
        {
            GTA3FixSpriteAspect(szx);
            RenderBufferedOneXLUSprite_Rotate_Aspect(screenpos, szx * 30.0f, szy * 30.0f, 200 * fCloudHighlight[i], 0, 0, 255, 1.0f / screenpos.z,
                                                     1.7f - GetATanOfXY(screenpos.x - *SunScreenX, screenpos.y - *SunScreenY) + *ms_cameraRoll, 255);
        }
    }
    FlushSpriteBuffer();
}

#endif

#ifdef GTA3_TARGET
MYMOD(net.icepie.gta3skies, GTA3Skies, 0.1, icepie)
NEEDGAME(com.rockstar.gta3)
#else
MYMOD(net.rusjj.viceskies, ViceSkies, 1.3, RusJJ)
NEEDGAME(com.rockstargames.gtavc)
#endif

float LowCloudsX[12] = { 1.0f,  0.7f,  0.0f, -0.7f, -1.0f, -0.7f, 0.0f, 0.7f, 0.8f, -0.8f,  0.4f, -0.4f };
float LowCloudsY[12] = { 0.0f, -0.7f, -1.0f, -0.7f,  0.0f,  0.7f, 1.0f, 0.7f, 0.4f,  0.4f, -0.8f, -0.8f };
float LowCloudsZ[12] = { 0.0f,  1.0f,  0.5f,  0.0f,  1.0f,  0.3f, 0.9f, 0.4f, 1.3f,  1.4f,  1.2f,  1.7f };

float CoorsOffsetX[37] = {
    0.0f, 60.0f, 72.0f, 48.0f, 21.0f, 12.0f,
    9.0f, -3.0f, -8.4f, -18.0f, -15.0f, -36.0f,
    -40.0f, -48.0f, -60.0f, -24.0f, 100.0f, 100.0f,
    100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f,
    100.0f, 100.0f, -30.0f, -20.0f, 10.0f, 30.0f,
    0.0f, -100.0f, -100.0f, -100.0f, -100.0f, -100.0f, -100.0f
};
float CoorsOffsetY[37] = {
    100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f,
    100.0f, 100.0f, 100.0f, 100.0f, 100.0f, 100.0f,
    100.0f, 100.0f, 100.0f, 100.0f, -30.0f, 10.0f,
    -25.0f, -5.0f, 28.0f, -10.0f, 10.0f, 0.0f,
    15.0f, 40.0f, -100.0f, -100.0f, -100.0f, -100.0f,
    -100.0f, -40.0f, -20.0f, 0.0f, 10.0f, 30.0f, 35.0f
};
float CoorsOffsetZ[37] = {
    2.0f, 1.0f, 0.0f, 0.3f, 0.7f, 1.4f,
    1.7f, 0.24f, 0.7f, 1.3f, 1.6f, 1.0f,
    1.2f, 0.3f, 0.7f, 1.4f, 0.0f, 0.1f,
    0.5f, 0.4f, 0.55f, 0.75f, 1.0f, 1.4f,
    1.7f, 2.0f, 2.0f, 2.3f, 1.9f, 2.4f,
    2.0f, 2.0f, 1.5f, 1.2f, 1.7f, 1.5f, 2.1f
};

RwIm3DVertex Skies_TempBufferRenderVertices[32];
uint16_t pShootingStarIndices[] = { 0, 1 };

CVector MoonVector;
inline float SQR(float v) { return v*v; }

#ifdef GTA3_TARGET
DECL_HOOKv(RenderBackground, short topred, short topgreen, short topblue, short botred, short botgreen, short botblue, short alpha)
{
    RenderBackground(topred, topgreen, topblue, botred, botgreen, botblue, alpha);
    ++BackgroundHookCounter;

    if(LogRenderHook && (BackgroundHookCounter == 1 || (BackgroundHookCounter % 300) == 0))
    {
        GTA3SKIES_LOG("background=%u fog=%.2f cloud=%.2f",
                      BackgroundHookCounter,
                      Foggyness ? *Foggyness : 0.0f,
                      CloudCoverage ? *CloudCoverage : 0.0f);
    }
}

static void GTA3RenderCloudLayer(int& lowCloudSprites, int& fluffyCloudSprites)
{
    if(!CanSeeOutSideFromCurrArea()) return;

    float coverage = fmaxf(*Foggyness, *CloudCoverage);
    float decoverage = 1.0f - coverage;

    float lowcintens = 1.0f - fmaxf(coverage, *ExtraSunnyness);
    int r = *m_nCurrentLowCloudsRed * lowcintens;
    int g = *m_nCurrentLowCloudsGreen * lowcintens;
    int b = *m_nCurrentLowCloudsBlue * lowcintens;

    if(GTA3ScreenSpaceLowClouds) GTA3RenderScreenSpaceLowClouds(r, g, b, lowCloudSprites);
    else GTA3RenderRe3LowClouds(r, g, b, lowCloudSprites);
    GTA3RenderRe3FluffyClouds(coverage, decoverage, fluffyCloudSprites);
}

static void GTA3RenderSkyLayer(int& lowCloudSprites, int& fluffyCloudSprites)
{
    if(!CanSeeOutSideFromCurrArea() || !CamPos) return;

    float szx, szy;
    RwV3d screenpos;
    RwV3d worldpos;

    RwRenderStateSet(8, (void*)0);
    RwRenderStateSet(6, (void*)0);
    RwRenderStateSet(12, (void*)1);
    RwRenderStateSet(10, (void*)2);
    RwRenderStateSet(11, (void*)2);
    InitSpriteBuffer();

    *SunBlockedByClouds = false;
    float coverage = fmaxf(*Foggyness, *CloudCoverage);
    float decoverage = 1.0f - coverage;
    float celestialCoverage = fmaxf(*Foggyness, *CloudCoverage * GTA3CelestialCloudFadeScale);
    if(celestialCoverage > 1.0f) celestialCoverage = 1.0f;
    float celestialVisibility = 1.0f - celestialCoverage;

    if(celestialVisibility > 0.0f && (*ms_nGameClockHours >= STARS_STARTHOUR || *ms_nGameClockHours <= START_LASTHOUR))
    {
        float brightness = 255.0f * celestialVisibility;
        if(*ms_nGameClockHours == STARS_STARTHOUR) brightness *= (0.0166667f * *ms_nGameClockMinutes);
        else if(*ms_nGameClockHours == START_LASTHOUR) brightness *= (0.0166667f * (60 - *ms_nGameClockMinutes));

        RwRenderStateSet(1, *(gpCoronaTexture[0]));
      #ifdef STARRY_SKIES
        StarrySkies_Patch(brightness);
      #else
        for(int i = 0; i < 11; ++i)
        {
            RwV3d pos = { 100.0f, 0.0f, 10.0f };
            if(i >= 9) pos.x = -pos.x;
            worldpos = pos + *CamPos;
            worldpos.y -= 90.0f * StarCoorsX[i % 9];
            worldpos.z += 80.0f * StarCoorsY[i % 9];

            if(CalcScreenCoors(&worldpos, &screenpos, &szx, &szy, false))
            {
                float sz = 0.8f * StarSizes[i % 9];
                GTA3FixSpriteAspect(szx);
                RenderBufferedOneXLUSprite(screenpos, szx * sz, szy * sz, brightness, brightness, brightness, 255, 1.0f / screenpos.z, 255);
            }
        }
      #endif
        FlushSpriteBuffer();
    }

    float minute = 60.0f * *ms_nGameClockHours + *ms_nGameClockMinutes;
    int moonfadeout = (int)(fabsf(minute - 180.0f));
    if(moonfadeout < 180)
    {
        RwV3d pos = { 0.0f, -100.0f, 15.0f };
        if(GTA3MoonMovesWithTime)
        {
            const float t = minute / 1440.0f;
            const float angle = (t * 2.0f * M_PI) + M_PI;
            const float height = sinf((t - 0.125f) * 2.0f * M_PI);
            pos.x = cosf(angle) * GTA3MovingMoonDistance;
            pos.y = sinf(angle) * GTA3MovingMoonDistance;
            pos.z = 15.0f + height * GTA3MovingMoonHeightScale;
        }
        worldpos = pos + *CamPos;
        if(CalcScreenCoors(&worldpos, &screenpos, &szx, &szy, false))
        {
            RwRenderStateSet(1, *(gpCoronaTexture[2]));
            int brightness = celestialVisibility * (180 - moonfadeout);
            float sz = *MoonSize * 2.0f + 4.0f;
            GTA3FixSpriteAspect(szx);
            RenderBufferedOneXLUSprite(screenpos, szx * sz, szy * sz, brightness, brightness, brightness, 255, 1.0f / screenpos.z, 255);
            FlushSpriteBuffer();
        }
    }

    GTA3RenderCloudLayer(lowCloudSprites, fluffyCloudSprites);

    GTA3RenderRainbowLayer("rainbowSky", LastRainbowSkyLog, CamPos);

    if(*NewWeatherType == 0 || *NewWeatherType == 4)
    {
        uint32_t time = *m_snTimeInMilliseconds;
        uint32_t phase = (time & 0x1FFF);
        if(phase < 800)
        {
            float tail = (400 - phase) + (400 - phase);
            Skies_TempBufferRenderVertices[0].color = 0xE1FFFFFF;
            Skies_TempBufferRenderVertices[1].color = 0x00FFFFFF;

            uint32_t seed = (time >> 13) & 0x3F;
            CVector starScale{ 0.1f * (seed % 7 - 3), 0.1f * ((time >> 13) - 4), 1.0f };
            starScale.Normalise();
            CVector starDir{ (float)(seed % 9 - 5), (float)(seed % 10 - 5), 0.1f };
            starDir.Normalise();

            worldpos = *CamPos + starDir * 1000.0f;
            Skies_TempBufferRenderVertices[0].pos = worldpos + starScale * tail;
            Skies_TempBufferRenderVertices[1].pos = worldpos + starScale * (tail + 50.0f);

            RwRenderStateSet(1, (void*)0);
            RwRenderStateSet(10, (void*)5);
            RwRenderStateSet(11, (void*)6);
            if(RwIm3DTransform(Skies_TempBufferRenderVertices, 2, NULL, 0x10 | 0x8))
            {
                RwIm3DRenderIndexedPrimitive(2, pShootingStarIndices, 2);
                RwIm3DEnd();
            }
        }
    }

    RwRenderStateSet(8, (void*)1);
    RwRenderStateSet(6, (void*)1);
    RwRenderStateSet(12, (void*)0);
    RwRenderStateSet(10, (void*)5);
    RwRenderStateSet(11, (void*)6);
}

DECL_HOOKv(RenderHorizon)
{
    ++HorizonHookCounter;
    RenderHorizon();

    if(LogRenderHook && (HorizonHookCounter == 1 || (HorizonHookCounter % 300) == 0))
    {
        GTA3SKIES_LOG("horizon=%u time=%02u:%02u weather=%d/%d interp=%.3f rain=%.3f wet=%.3f fog=%.2f cloud=%.2f rainbow=%.3f lightning=%d/%d cam=(%.1f %.1f %.1f)",
                      HorizonHookCounter,
                      ms_nGameClockHours ? *ms_nGameClockHours : 255,
                      ms_nGameClockMinutes ? *ms_nGameClockMinutes : 255,
                      NewWeatherType ? *NewWeatherType : -1,
                      OldWeatherType ? *OldWeatherType : -1,
                      InterpolationValue ? *InterpolationValue : 0.0f,
                      Rain ? *Rain : 0.0f,
                      WetRoads ? *WetRoads : 0.0f,
                      Foggyness ? *Foggyness : 0.0f,
                      CloudCoverage ? *CloudCoverage : 0.0f,
                      Rainbow ? *Rainbow : 0.0f,
                      LightningFlash ? (int)*LightningFlash : -1,
                      LightningBurst ? (int)*LightningBurst : -1,
                      CamPos ? CamPos->x : 0.0f,
                      CamPos ? CamPos->y : 0.0f,
                      CamPos ? CamPos->z : 0.0f);
    }
}

DECL_HOOKv(RenderEverythingBarRoads)
{
    ++BeforeWorldHookCounter;

    int lowCloudSprites = 0;
    int fluffyCloudSprites = 0;
    if(GTA3DrawSkyBeforeWorld)
    {
        GTA3RenderSkyLayer(lowCloudSprites, fluffyCloudSprites);
    }

    if(LogRenderHook && (BeforeWorldHookCounter == 1 || (BeforeWorldHookCounter % 300) == 0))
    {
        GTA3SKIES_LOG("beforeWorld=%u low=%d fluffy=%d draw=%d time=%02u:%02u weather=%d/%d interp=%.3f rain=%.3f wet=%.3f fog=%.2f cloud=%.2f rainbow=%.3f lightning=%d/%d lowRGB=(%d,%d,%d) fluffyTop=(%d,%d,%d) cam=(%.1f %.1f %.1f)",
                      BeforeWorldHookCounter,
                      lowCloudSprites,
                      fluffyCloudSprites,
                      GTA3DrawSkyBeforeWorld,
                      ms_nGameClockHours ? *ms_nGameClockHours : 255,
                      ms_nGameClockMinutes ? *ms_nGameClockMinutes : 255,
                      NewWeatherType ? *NewWeatherType : -1,
                      OldWeatherType ? *OldWeatherType : -1,
                      InterpolationValue ? *InterpolationValue : 0.0f,
                      Rain ? *Rain : 0.0f,
                      WetRoads ? *WetRoads : 0.0f,
                      Foggyness ? *Foggyness : 0.0f,
                      CloudCoverage ? *CloudCoverage : 0.0f,
                      Rainbow ? *Rainbow : 0.0f,
                      LightningFlash ? (int)*LightningFlash : -1,
                      LightningBurst ? (int)*LightningBurst : -1,
                      m_nCurrentLowCloudsRed ? *m_nCurrentLowCloudsRed : 0,
                      m_nCurrentLowCloudsGreen ? *m_nCurrentLowCloudsGreen : 0,
                      m_nCurrentLowCloudsBlue ? *m_nCurrentLowCloudsBlue : 0,
                      m_nCurrentFluffyCloudsTopRed ? *m_nCurrentFluffyCloudsTopRed : 0,
                      m_nCurrentFluffyCloudsTopGreen ? *m_nCurrentFluffyCloudsTopGreen : 0,
                      m_nCurrentFluffyCloudsTopBlue ? *m_nCurrentFluffyCloudsTopBlue : 0,
                      CamPos ? CamPos->x : 0.0f,
                      CamPos ? CamPos->y : 0.0f,
                      CamPos ? CamPos->z : 0.0f);
    }

    RenderEverythingBarRoads();
}

#endif

DECL_HOOKv(RenderClouds)
{
  #ifdef GTA3_TARGET
    RenderClouds();
    ++RenderHookCounter;
    if(LogRenderHook && RenderHookCounter == 1)
    {
        GTA3SKIES_LOG("Cloud render hook hit. RsGlobal=%p TheCamera=%p CamPos=%p gpCloudTex=%p gpCoronaTexture=%p",
                      RsGlobal, TheCamera, CamPos, gpCloudTex, gpCoronaTexture);
    }
    if(LogCameraCandidates && RenderHookCounter == 1 && TheCamera)
    {
        const uint32_t offsets[] = { 0x30, 0x34, 0x40, 0x44, 0x50, 0x54, 0x60, 0x64, 0x70, 0x74, 0x80, 0x84, 0x90, 0x94, 0xA0, 0xA4, 0xB0, 0xB4, 0xC0, 0xC4 };
        for(uint32_t offset : offsets)
        {
            CVector* candidate = (CVector*)(TheCamera + offset);
            GTA3SKIES_LOG("TheCamera+0x%X = (%.2f %.2f %.2f)", offset, candidate->x, candidate->y, candidate->z);
        }
    }
    if(GTA3DrawCloudsInHorizon || GTA3DrawSkyBeforeWorld)
    {
        if(LogRenderHook && (RenderHookCounter == 1 || (RenderHookCounter % 300) == 0))
        {
            GTA3SKIES_LOG("frame=%u skyMovedBeforeWorld=%d skyMovedToHorizon=%d cam=(%.1f %.1f %.1f)",
                          RenderHookCounter,
                          GTA3DrawSkyBeforeWorld,
                          GTA3DrawCloudsInHorizon,
                          CamPos ? CamPos->x : 0.0f,
                          CamPos ? CamPos->y : 0.0f,
                          CamPos ? CamPos->z : 0.0f);
        }
        return;
    }
  #endif

    float szx, szy;
    RwV3d screenpos;
    RwV3d worldpos;
    int lowCloudSprites = 0;
    int fluffyCloudSprites = 0;
    
    if(!CanSeeOutSideFromCurrArea())
    {
        return;
    }
    
    RwRenderStateSet(8, (void*)0);
    RwRenderStateSet(6, (void*)0);
    RwRenderStateSet(12, (void*)1);
    RwRenderStateSet(10, (void*)2);
    RwRenderStateSet(11, (void*)2);
    InitSpriteBuffer();
    
    *SunBlockedByClouds = false;
    float coverage = fmaxf(*Foggyness, *CloudCoverage);
  #ifdef GTA3_TARGET
    if(ForceVisibleClouds && coverage > 0.35f) coverage = 0.35f;
  #endif
    float decoverage = 1.0f - coverage;
    
    // Stars
    if(coverage < 1 && (*ms_nGameClockHours >= STARS_STARTHOUR || *ms_nGameClockHours <= START_LASTHOUR))
    {
        float brightness = 255.0f * decoverage;
        if(*ms_nGameClockHours == STARS_STARTHOUR) brightness *= (0.0166667f * *ms_nGameClockMinutes);
        else if(*ms_nGameClockHours == START_LASTHOUR) brightness *= (0.0166667f * (60 - *ms_nGameClockMinutes));
        
        RwRenderStateSet(1, *(gpCoronaTexture[0]));
      #ifdef STARRY_SKIES
        StarrySkies_Patch(brightness);
      #else
        for(int i = 0; i < 11; ++i)
        {
            RwV3d pos = { 100.0f, 0.0f, 10.0f };
            if(i >= 9) pos.x = -pos.x;
            worldpos = pos + *CamPos;
            worldpos.y -= 90.0f * StarCoorsX[i % 9];
			worldpos.z += 80.0f * StarCoorsY[i % 9];
            
            if(CalcScreenCoors(&worldpos, &screenpos, &szx, &szy, false))
            {
                float sz = 0.8f * StarSizes[i % 9];
                GTA3FixSpriteAspect(szx);
                RenderBufferedOneXLUSprite(screenpos, szx * sz, szy * sz, brightness, brightness, brightness, 255, 1.0f / screenpos.z, 255);
            }
        }
      #endif
        FlushSpriteBuffer();
    }
    
    // Moon
    float minute = 60.0f * *ms_nGameClockHours + *ms_nGameClockMinutes;// + 0.0166667f * *ms_nGameClockSeconds; // useless part
  #ifdef IMPROVED_MOON
    int moonfadeout;
    float smoothBrightnessAdjust = 1.9f;
    if(minute > 1100)
    {
        moonfadeout = (int)(fabsf(minute - 1100.0f) / smoothBrightnessAdjust);
    }
    else if(minute < 240)
    {
        moonfadeout = 180;
    }
    else
    {
        moonfadeout = (int)(180.0f - fabsf(minute - 240.0f) * smoothBrightnessAdjust);
    }
    
    if (moonfadeout > 0 && moonfadeout < 340)
    {
        CVector& vecsun = m_VectorToSun[*m_CurrentStoredValue];
        MoonVector = { -vecsun.x, -vecsun.y, -(IMPROVED_MOON_HEIGHT / 150.0f) * vecsun.z }; // normalized vector (important for DotProd)
        RwV3d pos = { 150.0f * MoonVector.x, 150.0f * MoonVector.y, 150.0f * MoonVector.z };
  #else
    int moonfadeout = (int)(fabsf(minute - 180.0f));
    if(moonfadeout < 180)
    {
        RwV3d pos = { 0.0f, -100.0f, 15.0f };
  #endif
        worldpos = pos + *CamPos;
        if(CalcScreenCoors(&worldpos, &screenpos, &szx, &szy, false))
        {
            RwRenderStateSet(1, *(gpCoronaTexture[2]));
          #ifdef IMPROVED_MOON
            float sz = *MoonSize * 2.7f + 4.0f;
            int brightness = decoverage * moonfadeout;
          #else
            float sz = *MoonSize * 2.0f + 4.0f;
            int brightness = decoverage * (180 - moonfadeout);
          #endif
            GTA3FixSpriteAspect(szx);
            RenderBufferedOneXLUSprite(screenpos, szx * sz, szy * sz, brightness, brightness, brightness, 255, 1.0f / screenpos.z, 255);
            FlushSpriteBuffer();
        }
    }
    
    // Low Clouds
    float lowcintens = 1.0f - fmaxf(coverage, *ExtraSunnyness);
    int r = *m_nCurrentLowCloudsRed   * lowcintens;
    int g = *m_nCurrentLowCloudsGreen * lowcintens;
    int b = *m_nCurrentLowCloudsBlue  * lowcintens;
  #ifdef GTA3_TARGET
    if(ForceVisibleClouds)
    {
        GTA3ForceLowCloudColour(r, g, b, lowcintens);
    }
  #endif
  #ifdef GTA3_TARGET
    if(!GTA3DrawLowCloudsInBackground && !GTA3DrawCloudsInHorizon)
    {
        if(GTA3ScreenSpaceLowClouds) GTA3RenderScreenSpaceLowClouds(r, g, b, lowCloudSprites);
        else GTA3RenderRe3LowClouds(r, g, b, lowCloudSprites);
    }
    else if(!GTA3DrawCloudsInHorizon)
  #endif
    for(int cloudtype = 0; cloudtype < 3; ++cloudtype)
    {
        RwRenderStateSet(1, *(gpCloudTex[cloudtype]));
        for(int i = cloudtype; i < 12; i += 3)
        {
            RwV3d pos = { 800.0f * LowCloudsX[i], 800.0f * LowCloudsY[i], 60.0f * LowCloudsZ[i] };
            worldpos.x = CamPos->x + pos.x;
            worldpos.y = CamPos->y + pos.y;
            worldpos.z = 40.0f + pos.z;
            if(CalcScreenCoors(&worldpos, &screenpos, &szx, &szy, false))
            {
                GTA3FixSpriteAspect(szx);
                RenderBufferedOneXLUSprite_Rotate_Dimension(screenpos, szx * 320.0f, szy * 40.0f, r, g, b, 255, 1.0f / screenpos.z, *ms_cameraRoll, 255);
                ++lowCloudSprites;
            }
        }
        FlushSpriteBuffer();
    }
    
    // Fluffy Clouds
    if(coverage < 1
      #ifdef GTA3_TARGET
        && RenderFluffyClouds && !GTA3DrawCloudsInHorizon
      #endif
    )
    {
        float ARdiff = (3.0f * *ms_fAspectRatio) / 4.0f; // is it necessary? gonna check later!
        float distLimit = ((3.0f * (float)(RsGlobal->x)) / 4.0f) * ARdiff;
        float sundistBlocked = ((float)(RsGlobal->x) / 10.0f) / ARdiff;
        float sundistHilit = ((float)(RsGlobal->x) / 3.0) / ARdiff;
        
        static bool bCloudOnScreen[37];
        static float fSunDist[37];
        static float fCloudHighlight[37];
        
        int fluffyalpha = 160 * decoverage;
      #ifdef GTA3_TARGET
        if(ForceVisibleClouds && fluffyalpha < 160) fluffyalpha = 160;
      #endif
        float rot_sin = sinf(*CloudRotation);
        float rot_cos = cosf(*CloudRotation);

        float rotationValue = ((2.0f * M_PI) * (uint16_t)*IndividualRotation) / 65536.0f + *ms_cameraRoll;
        
        RwRenderStateSet(10, (void*)5);
        RwRenderStateSet(11, (void*)6);
        RwRenderStateSet(1, *(gpCloudTex[4]));
        for(int i = 0; i < 37; ++i)
        {
            RwV3d pos = { 2.0f*CoorsOffsetX[i], 2.0f*CoorsOffsetY[i], 40.0f*CoorsOffsetZ[i] + 40.0f };
            worldpos.x = pos.x*rot_cos + pos.y*rot_sin + CamPos->x;
            worldpos.y = pos.x*rot_sin - pos.y*rot_cos + CamPos->y;
            worldpos.z = pos.z;
            
            if(CalcScreenCoors(&worldpos, &screenpos, &szx, &szy, false))
            {
                fSunDist[i] = sqrtf(SQR(screenpos.x - *SunScreenX) + SQR(screenpos.y - *SunScreenY));
                fCloudHighlight[i] = decoverage * (1.0f - fSunDist[i] / distLimit);
                
                int tr = *m_nCurrentFluffyCloudsTopRed;
                int tg = *m_nCurrentFluffyCloudsTopGreen;
                int tb = *m_nCurrentFluffyCloudsTopBlue;
                
                int br = *m_nCurrentFluffyCloudsBottomRed;
                int bg = *m_nCurrentFluffyCloudsBottomGreen;
                int bb = *m_nCurrentFluffyCloudsBottomBlue;
              #ifdef GTA3_TARGET
                if(ForceVisibleClouds && tr + tg + tb + br + bg + bb < 180)
                {
                    tr = 220;
                    tg = 220;
                    tb = 220;
                    br = 150;
                    bg = 150;
                    bb = 150;
                }
              #endif
                
                if(fSunDist[i] < distLimit)
                {
                    tr = tr * (1.0f - fCloudHighlight[i]) + 235 * fCloudHighlight[i];
                    tg = tg * (1.0f - fCloudHighlight[i]) + 190 * fCloudHighlight[i];
                    tb = tb * (1.0f - fCloudHighlight[i]) + 190 * fCloudHighlight[i];
                    br = br * (1.0f - fCloudHighlight[i]) + 235 * fCloudHighlight[i];
                    bg = bg * (1.0f - fCloudHighlight[i]) + 190 * fCloudHighlight[i];
                    bb = bb * (1.0f - fCloudHighlight[i]) + 190 * fCloudHighlight[i];
                    if(fSunDist[i] < sundistBlocked) *SunBlockedByClouds = (fluffyalpha > 50);
                }
                else
                {
                    fCloudHighlight[i] = 0.0f;
                }
                GTA3FixSpriteAspect(szx);
                RenderBufferedOneXLUSprite_Rotate_2Colours(screenpos, szx * 55.0f, szy * 55.0f, tr, tg, tb, br, bg, bb, 0.0f, -1.0f, 1.0f / screenpos.z, rotationValue, fluffyalpha);
                ++fluffyCloudSprites;
                bCloudOnScreen[i] = true;
            }
            else
            {
                bCloudOnScreen[i] = false;
            }
        }
        FlushSpriteBuffer();
        
        RwRenderStateSet(10, (void*)2);
        RwRenderStateSet(11, (void*)2);
        RwRenderStateSet(1, *(gpCloudTex[3]));
        for(int i = 0; i < 37; ++i)
        {
            RwV3d pos = { 2.0f * CoorsOffsetX[i], 2.0f * CoorsOffsetY[i], 40.0f * CoorsOffsetZ[i] + 40.0f };
            worldpos.x = pos.x*rot_cos + pos.y*rot_sin + CamPos->x;
            worldpos.y = pos.x*rot_sin - pos.y*rot_cos + CamPos->y;
            worldpos.z = pos.z;
            
            if(bCloudOnScreen[i] && CalcScreenCoors(&worldpos, &screenpos, &szx, &szy, false) && fSunDist[i] < sundistHilit)
            {
                GTA3FixSpriteAspect(szx);
                RenderBufferedOneXLUSprite_Rotate_Aspect(screenpos, szx * 30.0f, szy * 30.0f, 200 * fCloudHighlight[i], 0, 0, 255, 1.0f / screenpos.z,
                                                         1.7f - GetATanOfXY(screenpos.x - *SunScreenX, screenpos.y - *SunScreenY) + *ms_cameraRoll, 255);
            }
        }
        FlushSpriteBuffer();
    }
    
    // Rainbow
    GTA3RenderRainbowLayer("rainbowCloud", LastRainbowCloudLog, CamPos);

    // Falling Star (backported from San Andreas)
    if ( (*NewWeatherType == 0 || *NewWeatherType == 4) )
    {
        uint32_t v36 = (*m_snTimeInMilliseconds & 0x1FFF);
        if (v36 < 800)
        {
            float v43 = (400 - v36) + (400 - v36);
            Skies_TempBufferRenderVertices[0].color = 0xE1FFFFFF;
            Skies_TempBufferRenderVertices[1].color = 0x00FFFFFF;

            uint32_t v37 = (*m_snTimeInMilliseconds >> 13) & 0x3F;
            CVector starScale{ 0.1f * (v37 % 7 - 3), 0.1f * ((*m_snTimeInMilliseconds >> 13) - 4), 1.0f };
            starScale.Normalise();
            CVector starDir{ (float)(v37 % 9 - 5), (float)(v37 % 10 - 5), 0.1f };
            starDir.Normalise();

            worldpos = *CamPos + starDir * 1000.0f;

            Skies_TempBufferRenderVertices[0].pos = worldpos + starScale * v43;
            Skies_TempBufferRenderVertices[1].pos = worldpos + starScale * (v43 + 50.0f);

            RwRenderStateSet(1, (void*)0); // FIX_BUGS
            RwRenderStateSet(10, (void*)5);
            RwRenderStateSet(11, (void*)6);

            if (RwIm3DTransform(Skies_TempBufferRenderVertices, 2, NULL, 0x10 | 0x8))
            {
                RwIm3DRenderIndexedPrimitive(2, pShootingStarIndices, 2);
                RwIm3DEnd();
            }
        }
    }

  #ifdef GTA3_TARGET
    if(DebugSprite)
    {
        RwRenderStateSet(1, *(gpCoronaTexture[0]));
        RwRenderStateSet(8, (void*)0);
        RwRenderStateSet(6, (void*)0);
        RwRenderStateSet(12, (void*)1);
        RwRenderStateSet(10, (void*)2);
        RwRenderStateSet(11, (void*)2);
        InitSpriteBuffer();

        bool debugDrawn = false;
        if(DebugSpriteScreenSpace)
        {
            screenpos = { ScreenWidthFallback * 0.5f, ScreenHeightFallback * 0.5f, 1.0f };
            RenderBufferedOneXLUSprite(screenpos, DebugSpriteSize, DebugSpriteSize, 255, 32, 32, 255, 1.0f, 255);
            debugDrawn = true;
        }
        else
        {
            worldpos = *CamPos;
            worldpos.y += DebugSpriteDistance;
            worldpos.z += 20.0f;
            if(CalcScreenCoors(&worldpos, &screenpos, &szx, &szy, false))
            {
                GTA3FixSpriteAspect(szx);
                RenderBufferedOneXLUSprite(screenpos, szx * DebugSpriteSize, szy * DebugSpriteSize, 255, 32, 32, 255, 1.0f / screenpos.z, 255);
                debugDrawn = true;
            }
        }

        FlushSpriteBuffer();
        if(LogRenderHook && (RenderHookCounter == 1 || (RenderHookCounter % 300) == 0))
        {
            GTA3SKIES_LOG("debugSprite=%d screenSpace=%d screen=(%.1f %.1f %.3f) size=%.1f",
                          debugDrawn,
                          DebugSpriteScreenSpace,
                          screenpos.x,
                          screenpos.y,
                          screenpos.z,
                          DebugSpriteSize);
        }
    }
  #endif
    
    RwRenderStateSet(8, (void*)1);
    RwRenderStateSet(6, (void*)1);
    RwRenderStateSet(12, (void*)0);
    RwRenderStateSet(10, (void*)5);
    RwRenderStateSet(11, (void*)6);

  #ifdef GTA3_TARGET
    if(LogRenderHook && (RenderHookCounter == 1 || (RenderHookCounter % 300) == 0))
    {
        GTA3SKIES_LOG("frame=%u hour=%u coverage=%.2f aspect=%.2f low=%d fluffy=%d cam=(%.1f %.1f %.1f)",
                      RenderHookCounter,
                      ms_nGameClockHours ? *ms_nGameClockHours : 255,
                      coverage,
                      ms_fAspectRatio ? *ms_fAspectRatio : 0.0f,
                      lowCloudSprites,
                      fluffyCloudSprites,
                      CamPos ? CamPos->x : 0.0f,
                      CamPos ? CamPos->y : 0.0f,
                      CamPos ? CamPos->z : 0.0f);
    }
  #endif
}

#ifdef IMPROVED_MOON
uintptr_t FireSniper_BackTo;
#define DotProduct(v1, v2) (v1.z * v2.z + v1.y * v2.y + v1.x * v2.x)
extern "C" void FireSniper_Patch(CVector& m_vecFront)
{
    float dotprod = DotProduct(m_vecFront, MoonVector);
    if(dotprod > 0.985f) *MoonSize = (*MoonSize + 1) % 8; // 0.997 -> 0.985
}
__attribute__((optnone)) __attribute__((naked)) void FireSniper_Inject(void)
{
  #ifdef AML32
    asm volatile(
        "MOV             R0, R9\n"
        "BL              FireSniper_Patch\n");
    asm volatile(
        "MOV             PC, %0\n"
    :: "r" (FireSniper_BackTo));
  #else // AML64
    asm volatile(
        "ADD             X0, SP, #0x10\n"
        "STR             S1, [SP, #-16]!\n" // PUSH {S1}
        "BL              FireSniper_Patch\n");
    asm volatile(
        "MOV             X0, %0\n"
    :: "r" (FireSniper_BackTo));
    asm volatile(
        "LDR             S1, [SP], #16\n" // POP {S1}
        "BR              X0\n");
  #endif
}
#endif

uintptr_t WeatherUpdate_BackTo;
extern "C" void WeatherUpdate_Patch()
{
  #ifndef GTA3_TARGET
    if(CloudCoverage && InterpolationValue && NewWeatherType && OldWeatherType &&
       *NewWeatherType != 0 && *NewWeatherType != 4 && *OldWeatherType != 0 && *OldWeatherType != 4)
    {
        *CloudCoverage += *InterpolationValue;
    }
  #endif

  #ifdef GTA3_TARGET
    if(GTA3DebugForceRainbow && Rainbow)
    {
        *Rainbow = GTA3DebugForceRainbowValue;
    }
    else if(GTA3FixRainbowUpdate && Rainbow && InterpolationValue && NewWeatherType && OldWeatherType && ms_nGameClockHours)
    {
        if(*OldWeatherType == 2 && *NewWeatherType == 0 && *InterpolationValue < 0.5f && *ms_nGameClockHours > 6 && *ms_nGameClockHours < 21)
            *Rainbow = 1.0f - fabsf(*InterpolationValue - 0.25f);
        else
            *Rainbow = 0.0f;
    }
  #endif
}

#ifdef GTA3_TARGET
DECL_HOOKv(GTA3WeatherUpdate)
{
    GTA3WeatherUpdate();
    WeatherUpdate_Patch();
}
#endif

__attribute__((optnone)) __attribute__((naked)) void WeatherUpdate_Inject(void)
{
  #ifdef AML32
    asm volatile(
        "PUSH            {R1-R4}\n"
        "BL              WeatherUpdate_Patch\n"
        "POP             {R1-R4}\n");
    asm volatile(
        "MOV             PC, %0\n"
    :: "r" (WeatherUpdate_BackTo));
  #else // AML64
    asm volatile(
        "STR             X8, [SP, #-16]!\n" // PUSH {X8}
        "STR             X9, [SP, #-16]!\n" // PUSH {X9}
        "BL              WeatherUpdate_Patch\n");
    asm volatile(
        "MOV             X0, %0\n"
    :: "r" (WeatherUpdate_BackTo));
    asm volatile(
        "LDR             X9, [SP], #16\n" // POP {X9}
        "LDR             X8, [SP], #16\n" // POP {X8}
        "BR              X0\n");
  #endif
}

extern "C" void OnModLoad()
{
  #ifdef GTA3_TARGET
    logger->SetTag("GTA3 Skies");
    pGame = aml->GetLib("libR1.so");
    hGame = aml->GetLibHandle("libR1.so");
  #else
    logger->SetTag("Vice Skies");
    pGame = aml->GetLib("libGTAVC.so");
    hGame = aml->GetLibHandle("libGTAVC.so");
  #endif
    hasJPatch15 = aml->HasModOfVersion("net.rusjj.jpatch", "1.5");
    
    if(!pGame || !hGame)
    {
      #ifdef GTA3_TARGET
        logger->Error("This mod only supports GTA3 v1.8 AML builds using libR1.so. Stopping.");
      #else
        logger->Error("This mod only for Vice City. Stopping.");
      #endif
        return;
    }

    logger->Info("Warming up the code...");
    
  #ifdef GTA3_TARGET
    CanSeeOutSideFromCurrArea = CanSeeOutsideFallback;
  #else
    SET_TO(CanSeeOutSideFromCurrArea,  aml->GetSym(hGame, "_ZN5CGame25CanSeeOutSideFromCurrAreaEv"));
  #endif
    SET_TO(RwRenderStateSet,           aml->GetSym(hGame, "_Z16RwRenderStateSet13RwRenderStatePv"));
    SET_TO(InitSpriteBuffer,           aml->GetSym(hGame, "_ZN7CSprite16InitSpriteBufferEv"));
    SET_TO(FlushSpriteBuffer,          aml->GetSym(hGame, "_ZN7CSprite17FlushSpriteBufferEv"));
    SET_TO(CalcScreenCoors,            aml->GetSym(hGame, "_ZN7CSprite15CalcScreenCoorsERK5RwV3dPS0_PfS4_b"));
    SET_TO(RenderBufferedOneXLUSprite, aml->GetSym(hGame, "_ZN7CSprite26RenderBufferedOneXLUSpriteEfffffhhhsfh"));
    SET_TO(RenderBufferedOneXLUSprite_Rotate_Dimension, aml->GetSym(hGame, "_ZN7CSprite43RenderBufferedOneXLUSprite_Rotate_DimensionEfffffhhhsffh"));
    SET_TO(RenderBufferedOneXLUSprite_Rotate_Aspect,    aml->GetSym(hGame, "_ZN7CSprite40RenderBufferedOneXLUSprite_Rotate_AspectEfffffhhhsffh"));
    SET_TO(RenderBufferedOneXLUSprite_Rotate_2Colours,  aml->GetSym(hGame, "_ZN7CSprite42RenderBufferedOneXLUSprite_Rotate_2ColoursEfffffhhhhhhffffh"));
    SET_TO(GetATanOfXY,                aml->GetSym(hGame, "_ZN8CGeneral11GetATanOfXYEff"));
    SET_TO(RwIm3DTransform,            aml->GetSym(hGame, "_Z15RwIm3DTransformP18RxObjSpace3DVertexjP11RwMatrixTagj"));
    SET_TO(RwIm3DRenderIndexedPrimitive,aml->GetSym(hGame, "_Z28RwIm3DRenderIndexedPrimitive15RwPrimitiveTypePti"));
    SET_TO(RwIm3DEnd,                  aml->GetSym(hGame, "_Z9RwIm3DEndv"));
    
    SET_TO(SunBlockedByClouds,         aml->GetSym(hGame, "_ZN8CCoronas18SunBlockedByCloudsE"));
    SET_TO(Foggyness,                  aml->GetSym(hGame, "_ZN8CWeather9FoggynessE"));
    SET_TO(CloudCoverage,              aml->GetSym(hGame, "_ZN8CWeather13CloudCoverageE"));
    SET_TO(Rain,                       aml->GetSym(hGame, "_ZN8CWeather4RainE"));
    SET_TO(WetRoads,                   aml->GetSym(hGame, "_ZN8CWeather8WetRoadsE"));
    SET_TO(LightningFlash,             aml->GetSym(hGame, "_ZN8CWeather14LightningFlashE"));
    SET_TO(LightningBurst,             aml->GetSym(hGame, "_ZN8CWeather14LightningBurstE"));
    SET_TO(ms_cameraRoll,              aml->GetSym(hGame, "_ZN7CClouds13ms_cameraRollE"));
    SET_TO(CloudRotation,              aml->GetSym(hGame, "_ZN7CClouds13CloudRotationE"));
    SET_TO(Rainbow,                    aml->GetSym(hGame, "_ZN8CWeather7RainbowE"));
    SET_TO(SunScreenX,                 aml->GetSym(hGame, "_ZN8CCoronas10SunScreenXE"));
    SET_TO(SunScreenY,                 aml->GetSym(hGame, "_ZN8CCoronas10SunScreenYE"));
    SET_TO(InterpolationValue,         aml->GetSym(hGame, "_ZN8CWeather18InterpolationValueE"));
    SET_TO(gpCoronaTexture,            aml->GetSym(hGame, "gpCoronaTexture"));
    SET_TO(gpCloudTex,                 aml->GetSym(hGame, "gpCloudTex"));
    SET_TO(ms_nGameClockHours,         aml->GetSym(hGame, "_ZN6CClock18ms_nGameClockHoursE"));
    SET_TO(ms_nGameClockMinutes,       aml->GetSym(hGame, "_ZN6CClock20ms_nGameClockMinutesE"));
    SET_TO(ms_nGameClockSeconds,       aml->GetSym(hGame, "_ZN6CClock20ms_nGameClockSecondsE"));
    SET_TO(m_nCurrentLowCloudsRed,     aml->GetSym(hGame, "_ZN10CTimeCycle22m_nCurrentLowCloudsRedE"));
    SET_TO(m_nCurrentLowCloudsGreen,   aml->GetSym(hGame, "_ZN10CTimeCycle24m_nCurrentLowCloudsGreenE"));
    SET_TO(m_nCurrentLowCloudsBlue,    aml->GetSym(hGame, "_ZN10CTimeCycle23m_nCurrentLowCloudsBlueE"));
    SET_TO(m_nCurrentFluffyCloudsTopRed,      aml->GetSym(hGame, "_ZN10CTimeCycle28m_nCurrentFluffyCloudsTopRedE"));
    SET_TO(m_nCurrentFluffyCloudsTopGreen,    aml->GetSym(hGame, "_ZN10CTimeCycle30m_nCurrentFluffyCloudsTopGreenE"));
    SET_TO(m_nCurrentFluffyCloudsTopBlue,     aml->GetSym(hGame, "_ZN10CTimeCycle29m_nCurrentFluffyCloudsTopBlueE"));
    SET_TO(m_nCurrentFluffyCloudsBottomRed,   aml->GetSym(hGame, "_ZN10CTimeCycle31m_nCurrentFluffyCloudsBottomRedE"));
    SET_TO(m_nCurrentFluffyCloudsBottomGreen, aml->GetSym(hGame, "_ZN10CTimeCycle33m_nCurrentFluffyCloudsBottomGreenE"));
    SET_TO(m_nCurrentFluffyCloudsBottomBlue,  aml->GetSym(hGame, "_ZN10CTimeCycle32m_nCurrentFluffyCloudsBottomBlueE"));
    SET_TO(m_CurrentStoredValue,       aml->GetSym(hGame, "_ZN10CTimeCycle20m_CurrentStoredValueE"));
    SET_TO(IndividualRotation,         aml->GetSym(hGame, "_ZN7CClouds18IndividualRotationE"));
    SET_TO(m_snTimeInMilliseconds,     aml->GetSym(hGame, "_ZN6CTimer22m_snTimeInMillisecondsE"));
    SET_TO(NewWeatherType,             aml->GetSym(hGame, "_ZN8CWeather14NewWeatherTypeE"));
    SET_TO(OldWeatherType,             aml->GetSym(hGame, "_ZN8CWeather14OldWeatherTypeE"));
    SET_TO(RsGlobal,                   aml->GetSym(hGame, "RsGlobal"));
    SET_TO(TheCamera,                  aml->GetSym(hGame, "TheCamera"));
    SET_TO(m_VectorToSun,              aml->GetSym(hGame, "_ZN10CTimeCycle13m_VectorToSunE"));
  #ifdef GTA3_TARGET
    ms_fAspectRatio = &AspectRatioFallback;
    ExtraSunnyness = &ExtraSunnynessFallback;
    MoonSize = &MoonSizeFallback;
    ForceVisibleClouds = cfg->GetBool("ForceVisibleClouds", false);
    LogRenderHook = cfg->GetBool("LogRenderHook", true);
    LogCameraCandidates = cfg->GetBool("LogCameraCandidates", false);
    RenderFluffyClouds = cfg->GetBool("RenderFluffyClouds", true);
    DebugSprite = cfg->GetBool("DebugSprite", false);
    DebugSpriteScreenSpace = cfg->GetBool("DebugSpriteScreenSpace", false);
    GTA3ScreenSpaceLowClouds = cfg->GetBool("GTA3ScreenSpaceLowClouds", false);
    GTA3LowCloudUseCoronaTexture = cfg->GetBool("GTA3LowCloudUseCoronaTexture", false);
    GTA3DrawLowCloudsInBackground = cfg->GetBool("GTA3DrawLowCloudsInBackground", false);
    GTA3DrawCloudsAfterScene = cfg->GetBool("GTA3DrawCloudsAfterScene", false);
    GTA3DrawCloudsInHorizon = cfg->GetBool("GTA3DrawCloudsInHorizon", false);
    GTA3DrawSkyBeforeWorld = cfg->GetBool("GTA3DrawSkyBeforeWorld", true);
    GTA3MoonMovesWithTime = cfg->GetBool("GTA3MoonMovesWithTime", false);
    GTA3FixRainbowUpdate = cfg->GetBool("GTA3FixRainbowUpdate", true);
    GTA3DebugForceRainbow = cfg->GetBool("GTA3DebugForceRainbow", false);
    CameraPosOffset = cfg->GetInt("CameraPosOffset", 0x34);
    if(CameraPosOffset == 0x30) CameraPosOffset = 0x34;
    DebugSpriteDistance = cfg->GetFloat("DebugSpriteDistance", 120.0f);
    DebugSpriteSize = cfg->GetFloat("DebugSpriteSize", 80.0f);
    GTA3DebugForceRainbowValue = cfg->GetFloat("GTA3DebugForceRainbowValue", 1.0f);
    GTA3RainbowWorldX = cfg->GetFloat("GTA3RainbowWorldX", 35.0f);
    GTA3RainbowWorldY = cfg->GetFloat("GTA3RainbowWorldY", -100.0f);
    GTA3RainbowWorldZ = cfg->GetFloat("GTA3RainbowWorldZ", 22.0f);
    GTA3RainbowWorldSpacing = cfg->GetFloat("GTA3RainbowWorldSpacing", 1.5f);
    GTA3RainbowWorldWidthScale = cfg->GetFloat("GTA3RainbowWorldWidthScale", 2.0f);
    GTA3RainbowWorldHeightScale = cfg->GetFloat("GTA3RainbowWorldHeightScale", 50.0f);
    GTA3LowCloudScreenY = cfg->GetFloat("GTA3LowCloudScreenY", 0.18f);
    GTA3LowCloudWidth = cfg->GetFloat("GTA3LowCloudWidth", 650.0f);
    GTA3LowCloudHeight = cfg->GetFloat("GTA3LowCloudHeight", 110.0f);
    GTA3LowCloudDriftSpeed = cfg->GetFloat("GTA3LowCloudDriftSpeed", 0.012f);
    GTA3CelestialCloudFadeScale = cfg->GetFloat("GTA3CelestialCloudFadeScale", 0.65f);
    GTA3MovingMoonDistance = cfg->GetFloat("GTA3MovingMoonDistance", 150.0f);
    GTA3MovingMoonHeightScale = cfg->GetFloat("GTA3MovingMoonHeightScale", 50.0f);
    GTA3UseRe3LowClouds = cfg->GetBool("GTA3UseRe3LowClouds", true);
    GTA3Re3LowCloudDistanceScale = cfg->GetFloat("GTA3Re3LowCloudDistanceScale", 800.0f);
    GTA3Re3LowCloudBaseZ = cfg->GetFloat("GTA3Re3LowCloudBaseZ", 40.0f);
    GTA3Re3LowCloudZScale = cfg->GetFloat("GTA3Re3LowCloudZScale", 60.0f);
    GTA3Re3LowCloudWidthScale = cfg->GetFloat("GTA3Re3LowCloudWidthScale", 320.0f);
    GTA3Re3LowCloudHeightScale = cfg->GetFloat("GTA3Re3LowCloudHeightScale", 40.0f);
    GTA3LowCloudAlpha = cfg->GetInt("GTA3LowCloudAlpha", 255);
    if(RsGlobal && RsGlobal->x > 1 && RsGlobal->y > 1) AspectRatioFallback = (float)RsGlobal->x / (float)RsGlobal->y;
  #else
    SET_TO(ms_fAspectRatio,            aml->GetSym(hGame, "_ZN5CDraw15ms_fAspectRatioE"));
    SET_TO(ExtraSunnyness,             aml->GetSym(hGame, "_ZN8CWeather14ExtraSunnynessE"));
    SET_TO(MoonSize,                   aml->GetSym(hGame, "_ZN8CCoronas8MoonSizeE"));
  #endif
  #ifdef GTA3_TARGET
    CamPos = (CVector*)(TheCamera + CameraPosOffset);
  #else
    CamPos = (CVector*)(TheCamera + 0x30); // both in 1.09 and 1.12
  #endif
    
  #ifdef GTA3_TARGET
    HOOK(RenderBackground, aml->GetSym(hGame, "_ZN7CClouds16RenderBackgroundEsssssss"));
    HOOK(RenderHorizon, aml->GetSym(hGame, "_ZN7CClouds13RenderHorizonEv"));
    HOOKBL(RenderEverythingBarRoads, pGame + 0x1C0374 + 0x1);
    HOOK(RenderClouds, aml->GetSym(hGame, "_Z11RenderScenev"));
    HOOK(GTA3WeatherUpdate, aml->GetSym(hGame, "_ZN8CWeather6UpdateEv"));
    GTA3SKIES_LOG("Loaded. pGame=%p hGame=%p RenderBackground=%p RenderHorizon=%p DoRWRenderHorizon=%p RenderEverythingBarRoads=%p RenderScene=%p WeatherUpdate=%p beforeWorldHook=%p aspect=%.3f cameraOffset=0x%X ForceVisibleClouds=%d RenderFluffyClouds=%d DebugSprite=%d screenSpace=%d LowCloudScreen=%d LowCloudBg=%d LowCloudHorizon=%d SkyBeforeWorld=%d LowCloudCorona=%d MoonMoves=%d FixRainbow=%d ForceRainbow=%d ForceRainbowValue=%.2f RainbowWorld=(%.1f,%.1f,%.1f) RainbowScale=(%.2f,%.1f) LogRenderHook=%d",
                  (void*)pGame,
                  hGame,
                  (void*)aml->GetSym(hGame, "_ZN7CClouds16RenderBackgroundEsssssss"),
                  (void*)aml->GetSym(hGame, "_ZN7CClouds13RenderHorizonEv"),
                  (void*)aml->GetSym(hGame, "_Z17DoRWRenderHorizonv"),
                  (void*)aml->GetSym(hGame, "_ZN9CRenderer24RenderEverythingBarRoadsEv"),
                  (void*)aml->GetSym(hGame, "_Z11RenderScenev"),
                  (void*)aml->GetSym(hGame, "_ZN8CWeather6UpdateEv"),
                  (void*)(pGame + 0x1C0374 + 0x1),
                  AspectRatioFallback,
                  CameraPosOffset,
                  ForceVisibleClouds,
                  RenderFluffyClouds,
                  DebugSprite,
                  DebugSpriteScreenSpace,
                  GTA3ScreenSpaceLowClouds,
                  GTA3DrawLowCloudsInBackground,
                  GTA3DrawCloudsInHorizon,
                  GTA3DrawSkyBeforeWorld,
                  GTA3LowCloudUseCoronaTexture,
                  GTA3MoonMovesWithTime,
                  GTA3FixRainbowUpdate,
                  GTA3DebugForceRainbow,
                  GTA3DebugForceRainbowValue,
                  GTA3RainbowWorldX,
                  GTA3RainbowWorldY,
                  GTA3RainbowWorldZ,
                  GTA3RainbowWorldWidthScale,
                  GTA3RainbowWorldHeightScale,
                  LogRenderHook);
  #else
    HOOKBL(RenderClouds, pGame + BYBIT(0x14EA6E + 0x1, 0x1FA750)); // RenderScene
    HOOKBL(RenderClouds, pGame + BYBIT(0x14D8DC + 0x1, 0x1F99DC)); // NewTileRendererCB
  #endif
    
  #if defined(IMPROVED_MOON) && !defined(GTA3_TARGET)
    FireSniper_BackTo = pGame + BYBIT(0x26B056 + 0x1, 0x363758);
    aml->Redirect(pGame + BYBIT(0x26B036 + 0x1, 0x36373C), (uintptr_t)FireSniper_Inject);

    // A fix for moon disappearing on Sunny->Extrasunny weather !!!
    WeatherUpdate_BackTo = pGame + BYBIT(0x211F88 + 0x1, 0x2F8708);
    aml->Redirect(pGame + BYBIT(0x211F7A + 0x1, 0x2F86F4), (uintptr_t)WeatherUpdate_Inject);
  #endif

  #ifdef STARRY_SKIES
    InitializeThoseStars();
  #endif
}

#ifdef GTA3_TARGET
Config cfgLocal("GTA3Skies"); Config* cfg = &cfgLocal;
#else
Config cfgLocal("ViceSkies"); Config* cfg = &cfgLocal;
#endif
