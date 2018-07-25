#ifndef SIMULATION_NORMAL_DEPTH_MAP_SRC_TOOLS_HPP_
#define SIMULATION_NORMAL_DEPTH_MAP_SRC_TOOLS_HPP_

// C++ includes
#include <vector>

// OSG includes
#include <osg/Node>
#include <osg/Geode>
#include <osg/ref_ptr>
#include <osg/TriangleFunctor>
#include <osg/Texture2D>
#include <osg/Image>


namespace normal_depth_map {

    /**
     * @brief compute Underwater Signal Attenuation coefficient
     *
     *  This method is based on paper "A simplified formula for viscous and
     *  chemical absorption in sea water". The method computes the attenuation
     *  coefficient that will be used on shader normal intensite return.
     *
     *  @param double frequency: sound frequency in kHz.
     *  @param double temperature: water temperature in Celsius degrees.
     *  @param double depth: distance from water surface in meters.
     *  @param double salinity: amount of salt dissolved in a body of water in ppt.
     *  @param double acidity: pH water value.
     *
     *  @return double coefficient attenuation value
     */

    double underwaterSignalAttenuation( const double frequency,
                                        const double temperature,
                                        const double depth,
                                        const double salinity,
                                        const double acidity);

    /**
     * @brief
     *
     */
    struct TriangleStruct {
        std::vector< osg::Vec3 > data;
        TriangleStruct *left, *right;

        TriangleStruct()
            : data(5, osg::Vec3(0,0,0))
            , left(NULL)
            , right(NULL){};

        TriangleStruct(osg::Vec3 v1, osg::Vec3 v2, osg::Vec3 v3)
            : data(5, osg::Vec3(0,0,0))
            , left(NULL)
            , right(NULL) {

            setTriangle(v1, v2, v3);
        };

        void setTriangle(osg::Vec3 v1, osg::Vec3 v2, osg::Vec3 v3){
            data[0] = v1;                                  // vertex 1
            data[1] = v2;                                  // vertex 2
            data[2] = v3;                                  // vertex 3
            data[3] = (v1 + v2 + v3) / 3;                  // centroid
            data[4] = (v2 - v1).operator ^(v3 - v1);       // normal of triangle's plane
            data[4].normalize();
        };

        std::vector<float> getAllDataAsVector(){
            std::vector<float> output( data.size() * 3, 0.0);
            for (unsigned int i = 0; i < output.size(); i++)
                output[i] = data[i/3][i%3];
            return output;
        }
    };

    /**
     * @brief
     *
     */
    struct TrianglesCollection{
        std::vector<TriangleStruct> triangles;
        osg::Matrix local_2_world;

        inline void operator ()(const osg::Vec3& v1,
                                const osg::Vec3& v2,
                                const osg::Vec3& v3,
                                bool treatVertexDataAsTemporary) {

            // transform vertice coordinates to world coordinates
            osg::Vec3 v1_w = v1 * local_2_world;
            osg::Vec3 v2_w = v2 * local_2_world;
            osg::Vec3 v3_w = v3 * local_2_world;
            triangles.push_back(TriangleStruct(v1_w, v2_w, v3_w));
        };
    };

    /**
     * @brief
     *
     */
    inline void swap(TriangleStruct *a, TriangleStruct *b)
    {
        std::vector<osg::Vec3> tmp = a->data;
        a->data = b->data;
        b->data = tmp;
    }

    /**
     * @brief
     *
     */
    TriangleStruct *findMedian(TriangleStruct *start, TriangleStruct *end, int idx);

    /**
     * @brief
     *
     */
    TriangleStruct *makeTree(TriangleStruct *t, int len, int idx, int dim);

    /**
     * @brief
     *
     */
    int height(TriangleStruct *node);

    /**
     * @brief
     *
     */
    void getGivenLevel(TriangleStruct *node, std::vector<TriangleStruct> &vec, int level);

    /**
     * @brief
     *
     */
    void levelOrder(TriangleStruct *node, std::vector<TriangleStruct>& vec);

    /**
     * @brief
     *
     */
    class TrianglesVisitor : public osg::NodeVisitor {
    public:

        osg::TriangleFunctor<TrianglesCollection> triangles_data;

        TrianglesVisitor();
        void apply( osg::Geode& geode );
    };

    /**
     * @brief
     *
     */
    void convertTrianglesToTextures(
                    std::vector<TriangleStruct> triangles,
                    osg::ref_ptr<osg::Texture2D>& texture);

    }

    /**
    * @brief
    *
    */
    template < typename T >
    void setOSGImagePixel(  osg::ref_ptr<osg::Image>& image,
                            unsigned int x,
                            unsigned int y,
                            unsigned int channel,
                            T value ) {

        bool valid = ( y < (unsigned int) image->s() )
                        && ( x < (unsigned int) image->t() )
                        && ( channel < (unsigned int) image->r() );

        if( !valid )
            return;

        uint step = (x*image->s() + y) * image->r() + channel;

        T* data = (T*) image->data();
        data = data + step;
        *data = value;
    }

    /**
     * @brief
     *
     */
    template<typename T>
    void vec2osgimg( std::vector<T> const& vec, osg::ref_ptr<osg::Image>& image) {
        image = new osg::Image;
        image->allocateImage(vec.size(), 1, 3, GL_RGB, GL_FLOAT);
        image->setInternalTextureFormat(GL_RGB32F_ARB);

        float* data = (float*)image->data(0);
        memcpy(data, &vec[0], vec.size() * sizeof(T));
    }

    /**
     * @brief
     *
     */
    template<typename T>
    void osgimg2vec( osg::ref_ptr<osg::Image> const& image, std::vector<T>& vec) {
        T* data = (T*) image->data();
        vec = std::vector<T>(data, data + image->s());
    }

#endif
