/***********************************************************
   File: thinfilm.hpp

   Status:   Version 1.0 Release 3
   Language: C++

   License: GNU Public License

   (c) Copyright LESO-PB 2011

   Address:
    EPFL ENAC IIC LESO-PB
    CH-1015 Lausanne

   Author: Mario Geiger
   E-Mail: geiger.mario@gmail.com

   Description: Header file to calculate the
                reflectance, transmittance,
                psi and delta of a
                multilayer coating.

   Limitations: Tested with Film Wizard and
                Tfcalc. absorbing incident
                medium, psi and delta are
                not verified.

   Function: 1) asin and acos for complex numbers (source Wikipedia)
             2) simulate a multlayer coating (source Iris Mack)

   Thread Safe: Yes


   Change History:
   Date         Author        Description
   27.06.2011   Mario Geiger  Initial release
   31.03.2012   Mario Geiger  Release 3, performance Matrix22
***********************************************************/

#ifndef THINFILM_H
#define THINFILM_H

#include <complex>          // for complex numbers
#include <cstdio>           // for printf
#include <vector>           // for a vector of layers


// ----------------------------------------------------------------------------
namespace thinfilm {

// complex typedef
typedef std::complex<double> complex;

// complex i value
const complex onei(0, 1);


// asin and acos for complex numbers
// source: http://fr.wikipedia.org/wiki/Trigonom%C3%A9trie_complexe
inline complex asin(const complex &z)
{
  return -onei * log(onei * z + sqrt(1.0 - z * z));
}


inline complex acos(const complex &z)
{
  return -onei * log(       z + sqrt(z * z - 1.0));
}


// ----------------------------------------------------------------------------
/**
  \
   \             /  Reflectance
    \           /
     \         /
      \       /
       \     /
        v   /

        Incident medium
            n - i*k
 -------------------------------
        Layer 0 medium
         d0 (n0 - i*k0)
 -------------------------------
             ...

 -------------------------------
        Layer i medium
         di (ni - i*ki)
 -------------------------------
             ...

 -------------------------------
        Layer n medium
         dn (nn - i*kn)
 -------------------------------
         Exit medium
            n - i*k
                    \
                     \
                      \
                       v  Transmittance

**/
// ----------------------------------------------------------------------------


struct Layer {
  // use the same unit of wavelength
  double thickness;

  // with k negative
  // example (1.5, -0.001)
  complex refractiveIndex;
};

struct Matrix22 {
  Matrix22() {}
  Matrix22(complex m11, complex m12, complex m21, complex m22) :
    m11(m11), m12(m12), m21(m21), m22(m22)
  {
  }

  Matrix22& operator*=(const Matrix22& b) {
    complex x = m11 * b.m11 + m12 * b.m21;
    m12 = m11 * b.m12 + m12 * b.m22;
    m11 = x;
    x = m21 * b.m11 + m22 * b.m21;
    m22 = m21 * b.m12 + m22 * b.m22;
    m21 = x;
    return *this;
  }

  /*
      ( m11   m12 )
      (           )
      ( m21   m22 )
     */
  complex m11;
  complex m12;
  complex m21;
  complex m22;
};

const Matrix22 operator*(Matrix22 a, const Matrix22& b)
{
  return a *= b;
}

// ----------------------------------------------------------------------------
inline void simulate(
    // cosine of insident angle
    complex incidentCosTheta,
    // wavelength of light (same unit as layers thickness)
    double lambda,
    // angle of polarization 0 mean P and pi/2 mean S polarization
    double polarization,
    // complex index of refraxion of insident medium,
    // k value must be nagative or null
    complex nIncident,
    // complex index of refraxion of exit medium,
    // k value must be nagative or null
    complex nExit,

    // array of layers
    // in the array the layers are presented from
    //           the insident one to the exit one
    std::vector<Layer> layers,

    // pointer for reflectance
    double *reflectance = 0,
    // pointer for transmittance need ptr of reflectance != 0
    double *transmittance = 0,
    // pointer for absorptance need ptr of transmittance != 0
    double *absorptance = 0,
    // pointers of psi and delta, they are optional
    double *psi = 0, double *delta = 0
                                     )
{
  // variables for optimization
  int layersAmount = layers.size();

  /**
      always for each P and S polarization

      1.1 : we calculate the admittance of the incident and exit medium

      1.2 : the big unique loop
            calculate :
            admittance of the layer
            shift phase of the layer
            characteristic matrix
            product of the layer matrix and the main matrix

      2   : we do strange equations with the main matrix to extract
            reflectance transmittance and absorptance
      **/

  // ------------------------------------------------------------------------
  // ---------- 1.1 ----------


  // calculate admittance of the incident medium for P and S polarization
  const complex admittanceIncidentP = nIncident / incidentCosTheta;
  const complex admittanceIncidentS = nIncident * incidentCosTheta;


  // calculate cosine of exit medium with snell law optimized
  //  sin(theta0)    n0  =   sin(theta1)    n1
  //       s0        n0  =        s1        n1
  // sqrt( 1 - c0² ) n0  =  sqrt( 1 - c1² ) n1
  // and solve for c1 :
  // c1 = sqrt( c0² n0² - n0² + n1² ) / n1
  const complex squareIncidentCosTheta =
      incidentCosTheta * incidentCosTheta;
  const complex ratioIncidentExit = nIncident / nExit;
  const complex exitCosTheta =
      std::sqrt(1. - (1. - squareIncidentCosTheta) * ratioIncidentExit*ratioIncidentExit);

  // calculate the exit medium admitance P and S
  const complex admittanceExitP = nExit / exitCosTheta;
  const complex admittanceExitS = nExit * exitCosTheta;








  // ------------------------------------------------------------------------
  // ---------- 1.2 ----------

  // mains matrix, the product of each layer matrix
  Matrix22 productMatrixP(1.0, 0.0, 0.0, 1.0);
  Matrix22 productMatrixS(1.0, 0.0 ,0.0, 1.0);
  // initialized to Identity Matrix
  /*
      ( 1    0 )
      (        )
      ( 0    1 )
     */



  // the matrix of the layer
  Matrix22 layerMatrixP;
  Matrix22 layerMatrixS;



  // i : for each layer
  for (int i = 0; i < layersAmount; ++i) {

    // layerRefractiveIndex complex number : n - ik

    // snell law
    const complex ratio = nIncident / layers[i].refractiveIndex;
    const complex layerCosTheta =
        std::sqrt(1. - (1. - squareIncidentCosTheta) * ratio*ratio);



    // then calculate the admittance of the layer P and S
    const complex admittanceLayerP =
        layers[i].refractiveIndex / layerCosTheta;
    const complex admittanceLayerS =
        layers[i].refractiveIndex * layerCosTheta;

    // now the delta dephasing of the layer
    const complex deltaLayer =
        -2. * M_PI * layers[i].refractiveIndex * layers[i].thickness * layerCosTheta / lambda;
    // juillet 2015 : il faut bien multiplier par le cos => il faut regarder les front d'ondes !
    // onde incidante ~ exp(i(kx-wt))
    // the thickness layer, is need to be the same unit of lambda


    // create the matrix layer
    const complex c = cos(deltaLayer);
    const complex s = sin(deltaLayer) * onei;

    layerMatrixP.m11 = c;
    layerMatrixP.m12 = s / admittanceLayerP;
    layerMatrixP.m21 = s * admittanceLayerP;
    layerMatrixP.m22 = c;


    layerMatrixS.m11 = c;
    layerMatrixS.m12 = s / admittanceLayerS;
    layerMatrixS.m21 = s * admittanceLayerS;
    layerMatrixS.m22 = c;


    // now we need to make the product of the main matrix with the layer matrix

    // do the matrix product
    productMatrixP *= layerMatrixP;
    productMatrixS *= layerMatrixS;
  }







  // ------------------------------------------------------------------------
  // ----------  2  ----------

  const complex bP = productMatrixP.m11 + productMatrixP.m12 * admittanceExitP;
  const complex cP = productMatrixP.m21 + productMatrixP.m22 * admittanceExitP;



  const complex bS = productMatrixS.m11 + productMatrixS.m12 * admittanceExitS;
  const complex cS = productMatrixS.m21 + productMatrixS.m22 * admittanceExitS;



  // calculate the reflectance
  const complex reflectionCoefficientP =
      (bP - cP / admittanceIncidentP) / (bP + cP / admittanceIncidentP);

  const complex reflectionCoefficientS =
      (bS - cS / admittanceIncidentS) / (bS + cS / admittanceIncidentS);



  if (reflectance != 0) {

    // norm returns the norm value of the complex number : norm(3+4i) = 25
    const double reflectanceP = std::norm(reflectionCoefficientP);
    const double reflectanceS = std::norm(reflectionCoefficientS);

    // sin^2 + cos^2 = 1
    double polP = cos(polarization);
    double polS = sin(polarization);
    polP *= polP;
    polS *= polS;

    // calculation of the reflectance weighted on the polarization
    *reflectance = polP * reflectanceP + polS * reflectanceS;

    if (transmittance != 0) {

      // warning ! : the transmittance is correct only if kIncident == 0
      if (imag(nIncident) != 0.0) {
        fprintf(stderr, "%s:%d : warning ! the transmittance"
                        " is maybe false (incident k != 0)", __FILE__, __LINE__);
      }

      //! il semble que ces résultats soit faux!
      const complex transmitionCoefficientP =
          2. / (bP + cP / admittanceIncidentP) * incidentCosTheta / exitCosTheta;
      const complex transmitionCoefficientS =
          2. / (bS + cS / admittanceIncidentS);

      // and the transmittance
      const double transmittanceP = std::norm(transmitionCoefficientP);
      const double transmittanceS = std::norm(transmitionCoefficientS);

      *transmittance = polP * transmittanceP + polS * transmittanceS;

      if (absorptance != 0)
        *absorptance = 1.0 - *reflectance - *transmittance;
    }
  }

  if (psi != 0 && delta != 0) {
    // abs returns the length of the complex number
    *psi = std::atan2(std::abs(reflectionCoefficientP), std::abs(reflectionCoefficientS));

    // arg return phase angle of the complex number
    *delta = std::arg(reflectionCoefficientP) - std::arg(reflectionCoefficientS);
  }
}


} // end namespace

#endif // THINFILM_H

