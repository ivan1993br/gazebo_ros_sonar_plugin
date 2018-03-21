// C++ includes
#include "Tools.hpp"
#include <cmath>

namespace normal_depth_map {

double underwaterSignalAttenuation( const double frequency,
                                    const double temperature,
                                    const double depth,
                                    const double salinity,
                                    const double acidity) {

    double frequency2 = frequency * frequency;

    // borid acid and magnesium sulphate relaxation frequencies (in kHz)
    double f1 = 0.78 * pow(salinity / 35, 0.5) * exp(temperature / 26);
    double f2 = 42 * exp(temperature / 17);

    // borid acid contribution
    double borid = 0.106 * ((f1 * frequency2) / (frequency2 + f1 * f1)) * exp((acidity - 8) / 0.56);

    // magnesium sulphate contribuion
    double magnesium = 0.52 * (1 + temperature / 43) * (salinity / 35)
                        * ((f2 * frequency2) / (frequency2 + f2 * f2)) * exp(-depth / 6000);

    // freshwater contribution
    double freshwater = 0.00049 * frequency2 * exp(-(temperature / 27 + depth / 17000));

    // absorptium attenuation coefficient in dB/km
    double attenuation = borid + magnesium + freshwater;

    // convert dB/km to dB/m
    attenuation = attenuation / 1000.0;

    // convert dB/m to Pa/m
    attenuation = pow(10, -attenuation / 20);
    attenuation = -log(attenuation);

    return attenuation;
}

TrianglesVisitor::TrianglesVisitor() {
    setTraversalMode( osg::NodeVisitor::TRAVERSE_ALL_CHILDREN );
};

void TrianglesVisitor::apply( osg::Geode& geode ) {

    for ( unsigned int i = 0; i< geode.getNumDrawables(); ++i ) {
        triangles_data.local_2_world = osg::computeLocalToWorld(getNodePath());
        geode.getDrawable(i)->accept(triangles_data);
    }
}


void convertTrianglesToTextures(
                      std::vector<TriangleStruct>* triangles,
                      std::vector< osg::ref_ptr<osg::Texture2D> >& textures){

    std::vector< osg::ref_ptr<osg::Image> > images(
                                              (*triangles)[0]._data.size(),
                                              new osg::Image);

    for (unsigned int i = 0; i < images.size(); i++){
        images[i]->allocateImage(triangles->size(), 1, 3, GL_RGB, GL_FLOAT);

        for (unsigned int j = 0; j < triangles->size(); j++) {
            setOSGImagePixel(images[i], 0, i, 0, (*triangles)[j]._data[i].x());
            setOSGImagePixel(images[i], 0, i, 1, (*triangles)[j]._data[i].y());
            setOSGImagePixel(images[i], 0, i, 2, (*triangles)[j]._data[i].z());
        }

        osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
        texture->setTextureSize(images[i]->s(), images[i]->t());
        texture->setResizeNonPowerOfTwoHint(false);
        texture->setUnRefImageDataAfterApply(true);
        texture->setImage(images[i]);
        textures.push_back(texture);
    }

}


}
