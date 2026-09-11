//
// Curves — ports of THREE.CatmullRomCurve3, THREE.LineCurve3 and the
// arc-length reparameterisation / Frenet frame machinery on THREE.Curve.
//
// Needed by tubeGeometry.h, which TDS-01's oscillator, filter, envelope and
// LFO shapes are all built from.
//
#pragma once
#include <vector>
#include <cmath>
#include <cstddef>
#include <limits>
#include <algorithm>
#include "geometryMath.h"

//=================================================================================
//CubicPoly — THREE.CatmullRomCurve3's internal interpolant
//=================================================================================
struct CubicPoly
{
    float c0 = 0.0f, c1 = 0.0f, c2 = 0.0f, c3 = 0.0f;

    void init(const float x0, const float x1, const float t0, const float t1)
    {
        c0 = x0;
        c1 = t0;
        c2 = -3.0f * x0 + 3.0f * x1 - 2.0f * t0 - t1;
        c3 =  2.0f * x0 - 2.0f * x1 +        t0 + t1;
    }

    void initCatmullRom(const float x0, const float x1, const float x2,
                        const float x3, const float tension)
    {
        init(x1, x2, tension * (x2 - x0), tension * (x3 - x1));
    }

    //THREE CubicPoly.initNonuniformCatmullRom — centripetal / chordal
    void initNonuniformCatmullRom(const float x0, const float x1, const float x2, const float x3,
                                  const float dt0, const float dt1, const float dt2)
    {
        float t1 = (x1 - x0) / dt0 - (x2 - x0) / (dt0 + dt1) + (x2 - x1) / dt1;
        float t2 = (x2 - x1) / dt1 - (x3 - x1) / (dt1 + dt2) + (x3 - x2) / dt2;

        t1 *= dt1;
        t2 *= dt1;

        init(x1, x2, t1, t2);
    }

    [[nodiscard]] float calc(const float t) const
    {
        const float t2 = t * t;
        const float t3 = t2 * t;
        return c0 + c1 * t + c2 * t2 + c3 * t3;
    }
};

//=================================================================================
//Curve base — arc-length reparameterisation and Frenet frames
//=================================================================================
class Curve
{
public:
    virtual ~Curve() = default;

    // Uniform-in-parameter sample.
    [[nodiscard]] virtual vec3 getPoint(float t) const = 0;

    // Uniform-in-arc-length sample (THREE.Curve.getPointAt).
    [[nodiscard]] vec3 getPointAt(const float u) const
    {
        return getPoint(getUtoTmapping(u));
    }

    //=========================================================
    //THREE.Curve.getLengths — cumulative chord lengths
    //=========================================================
    [[nodiscard]] const std::vector<float>& getLengths(const int divisions = 200) const
    {
        if (! mCacheArcLengths.empty() && mCachedDivisions == divisions)
            return mCacheArcLengths;

        mCachedDivisions = divisions;
        mCacheArcLengths.clear();
        mCacheArcLengths.reserve(static_cast<size_t>(divisions) + 1);

        mCacheArcLengths.push_back(0.0f);

        vec3  last = getPoint(0.0f);
        float sum  = 0.0f;

        for (int i = 1; i <= divisions; ++i)
        {
            const vec3 current = getPoint(static_cast<float>(i) / static_cast<float>(divisions));
            const vec3 d       = current - last;
            sum  += std::sqrt(d.dot(d));
            last  = current;
            mCacheArcLengths.push_back(sum);
        }
        return mCacheArcLengths;
    }

    //=========================================================
    //THREE.Curve.getUtoTmapping — binary search + linear interp
    //=========================================================
    [[nodiscard]] float getUtoTmapping(const float u) const
    {
        const std::vector<float>& arcLengths = getLengths();
        const auto il = static_cast<int>(arcLengths.size());

        const float targetArcLength = u * arcLengths[static_cast<size_t>(il - 1)];

        int low = 0, high = il - 1;
        int i   = 0;

        while (low <= high)
        {
            i = low + (high - low) / 2;
            const float comparison = arcLengths[static_cast<size_t>(i)] - targetArcLength;

            if (comparison < 0.0f)      { low  = i + 1; }
            else if (comparison > 0.0f) { high = i - 1; }
            else                        { high = i; break; }
        }

        i = high;

        if (i < 0) i = 0;

        if (std::abs(arcLengths[static_cast<size_t>(i)] - targetArcLength) < 1e-6f)
            return static_cast<float>(i) / static_cast<float>(il - 1);

        const float lengthBefore = arcLengths[static_cast<size_t>(i)];
        const float lengthAfter  = arcLengths[static_cast<size_t>(i + 1)];
        const float segmentLength = lengthAfter - lengthBefore;
        const float segmentFraction = (targetArcLength - lengthBefore) / segmentLength;

        return (static_cast<float>(i) + segmentFraction) / static_cast<float>(il - 1);
    }

    //=========================================================
    //THREE.Curve.computeFrenetFrames — parallel transport
    //=========================================================
    struct FrenetFrames
    {
        std::vector<vec3> tangents;
        std::vector<vec3> normals;
        std::vector<vec3> binormals;
    };

    [[nodiscard]] FrenetFrames computeFrenetFrames(const int segments, const bool closed) const
    {
        FrenetFrames f;
        const size_t count = static_cast<size_t>(segments) + 1;
        f.tangents.resize(count);
        f.normals.resize(count);
        f.binormals.resize(count);

        for (size_t i = 0; i < count; ++i)
        {
            const float u = static_cast<float>(i) / static_cast<float>(segments);
            f.tangents[i] = getTangentAt(u).normalized();
        }

        // Initial normal: perpendicular to the smallest tangent component.
        vec3 normal{ 0.0f, 0.0f, 0.0f };

        const float tx = std::abs(f.tangents[0].x);
        const float ty = std::abs(f.tangents[0].y);
        const float tz = std::abs(f.tangents[0].z);

        float min = std::numeric_limits<float>::max();
        vec3  normalVec{ 0.0f, 0.0f, 0.0f };

        if (tx <= min) { min = tx; normalVec = { 1.0f, 0.0f, 0.0f }; }
        if (ty <= min) { min = ty; normalVec = { 0.0f, 1.0f, 0.0f }; }
        if (tz <= min) {           normalVec = { 0.0f, 0.0f, 1.0f }; }

        const vec3 vec = f.tangents[0].cross(normalVec).normalized();

        f.normals[0]   = f.tangents[0].cross(vec);
        f.binormals[0] = f.tangents[0].cross(f.normals[0]);

        for (size_t i = 1; i < count; ++i)
        {
            f.normals[i]   = f.normals[i - 1];
            f.binormals[i] = f.binormals[i - 1];

            vec3 v = f.tangents[i - 1].cross(f.tangents[i]);

            if (std::sqrt(v.dot(v)) > 1e-6f)
            {
                v = v.normalized();
                const float dotVal = std::clamp(f.tangents[i - 1].dot(f.tangents[i]), -1.0f, 1.0f);
                const float theta  = std::acos(dotVal);
                f.normals[i] = rotateAboutAxis(f.normals[i], v, theta);
            }

            f.binormals[i] = f.tangents[i].cross(f.normals[i]);
        }

        if (closed)
        {
            float theta = std::acos(std::clamp(f.normals[0].dot(f.normals[count - 1]), -1.0f, 1.0f));
            theta /= static_cast<float>(segments);

            if (f.tangents[0].dot(f.normals[0].cross(f.normals[count - 1])) > 0.0f)
                theta = -theta;

            for (size_t i = 1; i < count; ++i)
            {
                f.normals[i]   = rotateAboutAxis(f.normals[i], f.tangents[i],
                                                 theta * static_cast<float>(i));
                f.binormals[i] = f.tangents[i].cross(f.normals[i]);
            }
        }

        return f;
    }

protected:
    //THREE.Curve.getTangentAt — numerical derivative, matching Three's delta
    [[nodiscard]] vec3 getTangentAt(const float u) const
    {
        const float t = getUtoTmapping(u);
        return getTangent(t);
    }

    [[nodiscard]] vec3 getTangent(const float t) const
    {
        constexpr float delta = 0.0001f;
        float t1 = t - delta;
        float t2 = t + delta;

        if (t1 < 0.0f) t1 = 0.0f;
        if (t2 > 1.0f) t2 = 1.0f;

        const vec3 pt1 = getPoint(t1);
        const vec3 pt2 = getPoint(t2);

        return (pt2 - pt1).normalized();
    }

    static vec3 rotateAboutAxis(const vec3 v, const vec3 axis, const float angle)
    {
        // Rodrigues' rotation
        const float c = std::cos(angle);
        const float s = std::sin(angle);
        return v * c + axis.cross(v) * s + axis * (axis.dot(v) * (1.0f - c));
    }

private:
    mutable std::vector<float> mCacheArcLengths;
    mutable int                mCachedDivisions = -1;
};

//=================================================================================
//LineCurve3
//=================================================================================
class LineCurve3 final : public Curve
{
public:
    LineCurve3(const vec3 start, const vec3 end) : mStart(start), mEnd(end) {}

    [[nodiscard]] vec3 getPoint(const float t) const override
    {
        return mStart + (mEnd - mStart) * t;
    }

private:
    vec3 mStart{};
    vec3 mEnd{};
};

//=================================================================================
//CatmullRomCurve3 — 'catmullrom' variant with tension (the one TDS-01 uses)
//=================================================================================
enum class CatmullRomType { Centripetal, Chordal, CatmullRom };

class CatmullRomCurve3 final : public Curve
{
public:
    // THREE's default is 'centripetal'; pass CatmullRom explicitly to match
    // `new CatmullRomCurve3(points, false, 'catmullrom', tension)`.
    explicit CatmullRomCurve3(std::vector<vec3> points,
                              const bool  closed  = false,
                              const float tension = 0.5f,
                              const CatmullRomType type = CatmullRomType::Centripetal)
        : mPoints(std::move(points)), mClosed(closed), mTension(tension), mType(type) {}

    [[nodiscard]] vec3 getPoint(const float t) const override
    {
        const auto l = static_cast<int>(mPoints.size());

        const float p = static_cast<float>(l - (mClosed ? 0 : 1)) * t;
        auto  intPoint = static_cast<int>(std::floor(p));
        float weight   = p - static_cast<float>(intPoint);

        if (mClosed)
        {
            intPoint += intPoint > 0 ? 0
                      : (static_cast<int>(std::floor(static_cast<float>(std::abs(intPoint))
                                                     / static_cast<float>(l))) + 1) * l;
        }
        else if (weight == 0.0f && intPoint == l - 1)
        {
            intPoint = l - 2;
            weight   = 1.0f;
        }

        vec3 p0{}, p3{};

        if (mClosed || intPoint > 0) p0 = mPoints[static_cast<size_t>((intPoint - 1) % l)];
        else                         p0 = mPoints[0] - (mPoints[1] - mPoints[0]);

        const vec3 p1 = mPoints[static_cast<size_t>(intPoint % l)];
        const vec3 p2 = mPoints[static_cast<size_t>((intPoint + 1) % l)];

        if (mClosed || intPoint + 2 < l) p3 = mPoints[static_cast<size_t>((intPoint + 2) % l)];
        else                             p3 = mPoints[static_cast<size_t>(l - 1)]
                                            + (mPoints[static_cast<size_t>(l - 1)]
                                             - mPoints[static_cast<size_t>(l - 2)]);

        CubicPoly px, py, pz;

        if (mType == CatmullRomType::Centripetal || mType == CatmullRomType::Chordal)
        {
            const float pw = (mType == CatmullRomType::Chordal) ? 0.5f : 0.25f;

            const auto distSq = [](const vec3 a, const vec3 b) {
                const vec3 d = a - b;
                return d.dot(d);
            };

            float dt0 = std::pow(distSq(p0, p1), pw);
            float dt1 = std::pow(distSq(p1, p2), pw);
            float dt2 = std::pow(distSq(p2, p3), pw);

            // safety check for repeated points
            if (dt1 < 1e-4f) dt1 = 1.0f;
            if (dt0 < 1e-4f) dt0 = dt1;
            if (dt2 < 1e-4f) dt2 = dt1;

            px.initNonuniformCatmullRom(p0.x, p1.x, p2.x, p3.x, dt0, dt1, dt2);
            py.initNonuniformCatmullRom(p0.y, p1.y, p2.y, p3.y, dt0, dt1, dt2);
            pz.initNonuniformCatmullRom(p0.z, p1.z, p2.z, p3.z, dt0, dt1, dt2);
        }
        else
        {
            px.initCatmullRom(p0.x, p1.x, p2.x, p3.x, mTension);
            py.initCatmullRom(p0.y, p1.y, p2.y, p3.y, mTension);
            pz.initCatmullRom(p0.z, p1.z, p2.z, p3.z, mTension);
        }

        return { px.calc(weight), py.calc(weight), pz.calc(weight) };
    }

private:
    std::vector<vec3> mPoints;
    bool              mClosed  = false;
    float             mTension = 0.5f;
    CatmullRomType    mType    = CatmullRomType::Centripetal;
};

//=================================================================================
//SubRangeCurve — exposes a slice of another curve as t in [0, 1].
//
//Needed wherever a catmull-rom carries ghost control points that shape the end
//tangents but must not themselves be drawn (the ADSR and AR ramps).
//=================================================================================
class SubRangeCurve final : public Curve
{
public:
    SubRangeCurve(const Curve& inner, const float tMin, const float tMax)
        : mInner(inner), mMin(tMin), mMax(tMax) {}

    [[nodiscard]] vec3 getPoint(const float t) const override
    {
        return mInner.getPoint(mMin + (mMax - mMin) * t);
    }

private:
    const Curve& mInner;
    float        mMin = 0.0f;
    float        mMax = 1.0f;
};
