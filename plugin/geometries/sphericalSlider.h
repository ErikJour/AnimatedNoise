//
// Created by Erik Jourgensen on 7/13/26.
//

#ifndef ANIMATEDNOISE_SPHERICALSLIDER_H
#define ANIMATEDNOISE_SPHERICALSLIDER_H
#include <sphereGeometry.h>

class SphericalSlider
{
    public:
    SphericalSlider();
    ~SphericalSlider();

    static void buildSphere(std::vector<SphereVertex>& vertices,
                                 std::vector<SphereIndex>& indices,
                                    const float radius = 0.0666f,
                                    const float angle = 0.0f,
                                    int widthSegments = 64,
                                    int heightSegments = 32,
                                    const float phiStart = 0.0f,
                                    const float phiLength = PI * 2,
                                    const float thetaStart = 0.0f,
                                    const float thetaLength = PI)
        {
            juce::ignoreUnused(angle);
            widthSegments = std::max( 3, static_cast<int>(std::floor( widthSegments ) ) );
            heightSegments = std::max( 2, static_cast<int>(std::floor( heightSegments ) ) );
            const float thetaEnd = std::min( thetaStart + thetaLength, PI );

            std::vector<uint32_t> grid;
            grid.reserve((static_cast<size_t>(heightSegments) + 1) * (static_cast<size_t>(widthSegments) + 1));

            SphereVertex vertex = {};

            std::vector<float> normals = {};
            std::vector<float> uvs = {};

            for (int iy = 0; iy <= static_cast<int>(heightSegments); iy++)
            {
                std::vector<int> verticesRow = {};

                const float v = static_cast<float>(iy) / static_cast<float>(heightSegments);


                for (int ix = 0; ix <= widthSegments; ix++)
                {
                    const float u = static_cast<float>(ix) / static_cast<float>(widthSegments);

                    //vertex
                    vertex.x = -radius * std::cos( phiStart + u * phiLength) * std::sin (thetaStart + v * thetaLength);
                    vertex.y = radius * std::cos( thetaStart + v * thetaLength );
                    vertex.z = radius * std::sin( phiStart + u * phiLength ) * std::sin( thetaStart + v * thetaLength );

                    vertices.push_back({
                   vertex.x, vertex.y, vertex.z,
                   0.0f, 1.0f, 0.0f,
                   1.0f, 1.0f, 1.0f
                   });
                    grid.push_back(static_cast<uint32_t>(vertices.size() - 1));
                }
            }

            for (int iy = 0; iy < heightSegments; iy++)
            {
                for (int ix = 0; ix < widthSegments; ix++)
                {
                    const int rowWidth = widthSegments + 1;
                    const auto a = grid[ static_cast<uint32_t>(iy) * static_cast<uint32_t>(rowWidth) + static_cast<uint32_t>((ix + 1)) ];
                    const auto b = grid[ static_cast<uint32_t>(iy) * static_cast<uint32_t>(rowWidth) + static_cast<uint32_t>(ix) ];
                    const auto c = grid[ static_cast<uint32_t>((iy + 1) * rowWidth + ix) ];
                    const auto d = grid[ static_cast<uint32_t>((iy + 1) * rowWidth + (ix + 1)) ];

                    if (iy != 0 || thetaStart > 0) {
                        indices.push_back(static_cast<uint16_t>(a));
                        indices.push_back(static_cast<uint16_t>(b));
                        indices.push_back(static_cast<uint16_t>(d));
                    }
                    if (iy != heightSegments - 1 || thetaEnd < PI) {
                        indices.push_back(static_cast<uint16_t>(b));
                        indices.push_back(static_cast<uint16_t>(c));
                        indices.push_back(static_cast<uint16_t>(d));
                    }
                }
            }
        }

};

#endif //ANIMATEDNOISE_SPHERICALSLIDER_H