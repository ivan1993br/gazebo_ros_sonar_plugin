#ifndef SIMULATION_NORMAL_DEPTH_MAP_SRC_TOOLS_HPP_
#define SIMULATION_NORMAL_DEPTH_MAP_SRC_TOOLS_HPP_


#include <vector>
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
template < typename T >
void setOSGImagePixel(osg::ref_ptr<osg::Image>& image,
  unsigned int x,
  unsigned int y,
  unsigned int channel,
  T value ){

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
struct TriangleStruct {
    std::vector< osg::Vec3 > _data;

    TriangleStruct()
      : _data(5, osg::Vec3(0,0,0)) {};

    TriangleStruct(osg::Vec3 v_1, osg::Vec3 v_2, osg::Vec3 v_3)
      : _data(5, osg::Vec3(0,0,0)) {

        setTriangle(v_1, v_2, v_3);
    };

    void setTriangle(osg::Vec3 v_1, osg::Vec3 v_2, osg::Vec3 v_3){

        _data[0] = v_1;
        _data[1] = v_2;
        _data[2] = v_3;
        _data[3] = (v_1 + v_2 + v_3) / 3; // centroid

        osg::Vec3 v1_v2 = v_2 - v_1;
        osg::Vec3 v1_v3 = v_3 - v_1;
        _data[4] = v1_v2.operator ^(v1_v3);
        _data[4].normalize(); // normal
    };

    std::vector<float> getAllDataAsVector(){
        std::vector<float> output( _data.size()*3, 0.0);
        for (unsigned int i = 0; i < output.size(); i++)
            output[i] = _data[i/3][i%3];
        return output;
    }

    bool operator < (const TriangleStruct& obj_1){
        return _data[3] < obj_1._data[3];
    }
};

/**
 * @brief
 *
 */
struct TrianglesCollection{

    std::vector<TriangleStruct>* triangles;
    osg::Matrix local_2_world;

    TrianglesCollection() {
        triangles = new std::vector<TriangleStruct>();
    };

    ~TrianglesCollection() {
        delete triangles;
    };

    inline void operator () (const osg::Vec3& v1,
                             const osg::Vec3& v2,
                             const osg::Vec3& v3,
                             bool treatVertexDataAsTemporary) {

        // transform vertice coordinates to world coordinates
        osg::Vec3 v1_w = v1 * local_2_world;
        osg::Vec3 v2_w = v2 * local_2_world;
        osg::Vec3 v3_w = v3 * local_2_world;
        triangles->push_back( TriangleStruct(v1_w, v2_w, v3_w) );
    };
};

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
                  std::vector<TriangleStruct>* triangles,
                  std::vector< osg::ref_ptr<osg::Texture2D> >& textures);

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
