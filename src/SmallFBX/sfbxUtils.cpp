#include "pch.h"
#include "sfbxInternal.h"
#include "sfbxObject.h"
#include "sfbxGeometry.h"
#include "sfbxDeformer.h"
#include "sfbxUtil.h"

namespace sfbx {

bool Escape(std::string& v)
{
    bool ret = false;
    for (char c : v) {
        if (c == '"' || c == '\n' || c == '\r') {
            ret = true;
            break;
        }
    }
    if (!ret)
        return ret;

    std::string tmp;
    for (char c : v) {
        if (c == '"')
            tmp += "&quot;";
        else if (c == '\n')
            tmp += "&lf;";
        else if (c == '\r')
            tmp += "&cr;";
        else
            tmp += c;
    }
    v.swap(tmp);
    return ret;
}

std::string Base64Encode(span<char> src)
{
    static const char* s_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::string dst;
    // result size will be about 133% of src. so this is a bit overkill.
    dst.reserve(src.size() * 2);

    size_t n = src.size();
    for (size_t i = 0; i < n; ++i) {
        switch (i % 3) {
        case 0:
            dst.push_back(s_table[(src[i] & 0xFC) >> 2]);
            if (i + 1 == n) {
                dst.push_back(s_table[(src[i] & 0x03) << 4]);
                dst.push_back('=');
                dst.push_back('=');
            }
            break;

        case 1:
            dst.push_back(s_table[((src[i - 1] & 0x03) << 4) | ((src[i + 0] & 0xF0) >> 4)]);
            if (i + 1 == n) {
                dst.push_back(s_table[(src[i] & 0x0F) << 2]);
                dst.push_back('=');
            }
            break;

        case 2:
            dst.push_back(s_table[((src[i - 1] & 0x0F) << 2) | ((src[i + 0] & 0xC0) >> 6)]);
            dst.push_back(s_table[src[i] & 0x3F]);
            break;
        }
    }
    return dst;
}

RawVector<int> Triangulate(span<int> counts, span<int> indices)
{
    size_t num_triangles = 0;
    for (int c : counts) {
        if (c >= 3)
            num_triangles += c - 2;
    }

    RawVector<int> ret(num_triangles * 3);
    int* dst = ret.data();
    for (int c : counts) {
        if (c >= 3) {
            for (int fi = 0; fi < c - 2; ++fi) {
                *dst++ = indices[0];
                *dst++ = indices[1 + fi];
                *dst++ = indices[2 + fi];
            }
        }
    }
    return ret;
}

// Mul: e.g. [](float4x4, float3) -> float3
template<class Vec, class Mul>
static bool DeformImpl(span<Vec> dst, const JointWeights& jw, const JointMatrices& jm, span<Vec> src, const Mul& mul)
{
    if (jw.counts.size() != src.size() || jw.counts.size() != dst.size()) {
        sfbxPrint("Skin::deformImpl(): vertex count mismatch\n");
        return false;
    }

    const JointWeight* weights = jw.weights.data();
    const float4x4* matrices = jm.joint_transform.data();
    size_t nvertices = src.size();
    for (size_t vi = 0; vi < nvertices; ++vi) {
        Vec p = src[vi];
        Vec r{};
        int cjoints = jw.counts[vi];
        for (int bi = 0; bi < cjoints; ++bi) {
            JointWeight w = weights[bi];
            r += mul(matrices[w.index], p) * w.weight;
        }
        dst[vi] = r;
        weights += cjoints;
    }
    return true;
}

bool DeformPoints(span<float3> dst, const JointWeights& jw, const JointMatrices& jm, span<float3> src)
{
    return DeformImpl(dst, jw, jm, src,
        [](float4x4 m, float3 p) { return mul_p(m, p); });
}

bool DeformVectors(span<float3> dst, const JointWeights& jw, const JointMatrices& jm, span<float3> src)
{
    return DeformImpl(dst, jw, jm, src,
        [](float4x4 m, float3 p) { return mul_v(m, p); });
}



char CounterStream::StreamBuf::s_dummy_buf[1024];

CounterStream::StreamBuf::StreamBuf()
{
    this->setp(s_dummy_buf, s_dummy_buf + std::size(s_dummy_buf));
}

int CounterStream::StreamBuf::overflow(int c)
{
    m_size += uint64_t(this->pptr() - this->pbase()) + 1;
    this->setp(s_dummy_buf, s_dummy_buf + std::size(s_dummy_buf));
    return c;
}

int CounterStream::StreamBuf::sync()
{
    m_size += uint64_t(this->pptr() - this->pbase());
    this->setp(s_dummy_buf, s_dummy_buf + std::size(s_dummy_buf));
    return 0;
}

void CounterStream::StreamBuf::reset()
{
    m_size = 0;
    this->setp(s_dummy_buf, s_dummy_buf + std::size(s_dummy_buf));
}

CounterStream::CounterStream() : std::ostream(&m_buf) {}
uint64_t CounterStream::size() { m_buf.sync(); return m_buf.m_size; }
void CounterStream::reset() { m_buf.reset(); }

} // namespace sfbx
