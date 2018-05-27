#ifndef OPTICS_H
#define OPTICS_H

#include "geometry.h"

class SimulationContext;


class RaySegment
{
public:
    RaySegment();
    RaySegment(float *pt, float *dir, float w, int faceId = -1);
    ~RaySegment() = default;

    bool isValidEnd();
    void reset();

    RaySegment * nextReflect;
    RaySegment * nextRefract;
    RaySegment * prev;

    Vec3f pt;
    Vec3f dir;
    float w;
    int faceId;

    bool isFinished;
};


class Ray
{
public:
    Ray(float *pt, float *dir, float w, int faceId = -1);
    ~Ray();

    size_t totalNum();

    RaySegment *firstRaySeg;
    
};


class Optics
{
public:
    static void traceRays(SimulationContext &context);

private:
    static void initRays(int num, const float *dir, int face_num, const float *faces,
                         float *ray_pt, int *face_id);
    static void hitSurface(float n, int num, const float *dir, const float *norm,
                           float *reflect_dir, float *refract_dir, float *reflect_w);
    static void hitSurfaceHalide(float n, int num, const float *dir, const float *norm,
                                 float *reflect_dir, float *refract_dir, float *reflect_w);
    static void propagate(int num, const float *pt, const float *dir, int face_num, const float *faces,
                          float *new_pt, int *new_face_id);
    static void propagateHalide(int num, const float *pt, const float *dir,
                                int face_num, const float *faces, const int *face_id,
                                float *new_pt, int *new_face_id);

    static float getReflectRatio(float inc_angle, float n1, float n2);
    static void intersectLineFace(const float *pt, const float *dir, const float *face,
                                  float *p, float *t, float *alpha, float *beta);

    static std::default_random_engine& getGenerator();
    static std::uniform_real_distribution<float>& getDistribution();

    static std::default_random_engine* gen;
    static std::uniform_real_distribution<float>* dist;
};


class IceRefractiveIndex
{
public:
    static float n(float waveLength);

private:
    /* 
     * data from https://refractiveindex.info/?shelf=3d&book=crystals&page=ice
     */
    static constexpr float _wl[] = {350.0f, 400.0f, 410.0f, 420.0f, 430.0f, 440.0f, 450.0f, 460.0f, 470.0f,
        480.0f, 490.0f, 500.0f, 510.0f, 520.0f, 530.0f, 540.0f, 550.0f, 560.0f, 570.0f, 580.0f, 590.0f, 
        600.0f, 610.0f, 620.0f, 630.0f, 640.0f, 650.0f, 660.0f, 670.0f, 680.0f, 690.0f, 700.0f, 710.0f, 
        720.0f, 730.0f, 740.0f, 750.0f, 760.0f, 770.0f, 780.0f, 790.0f, 800.0f, 810.0f, 820.0f, 830.0f, 
        840.0f, 850.0f, 860.0f, 870.0f, 880.0f, 890.0f, 900.0f}; 
    static constexpr float _n[] = {1.3249f, 1.3194f, 1.3185f, 1.3177f, 1.3170f, 1.3163f, 1.3157f, 1.3151f,
        1.3145f, 1.3140f, 1.3135f, 1.3130f, 1.3126f, 1.3122f, 1.3118f, 1.3114f, 1.3110f, 1.3106f, 1.3103f, 
        1.3100f, 1.3097f, 1.3094f, 1.3091f, 1.3088f, 1.3085f, 1.3083f, 1.3080f, 1.3078f, 1.3076f, 1.3073f, 
        1.3071f, 1.3069f, 1.3067f, 1.3065f, 1.3062f, 1.3060f, 1.3058f, 1.3057f, 1.3055f, 1.3053f, 1.3051f, 
        1.3049f, 1.3047f, 1.3045f, 1.3044f, 1.3042f, 1.3040f, 1.3038f, 1.3037f, 1.3035f, 1.3033f, 1.3032f}; 
};


class RaySegmentFactory
{
public:
    ~RaySegmentFactory();

    static RaySegmentFactory * getInstance();

    RaySegment * getRaySegment(float *pt, float *dir, float w, int faceId);
    void clear();

private:
    RaySegmentFactory();

    static RaySegmentFactory *instance;
    static const uint32_t chunkSize = 1024 * 64;

    std::vector<RaySegment*> segments;
    uint32_t nextUnusedId;
    uint32_t currentChunkId;
};

#endif // OPTICS_H