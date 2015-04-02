#include "ann.h"
#include <cmath>

using std::tanh;

ann::ann() {
}

double ann::operator() (const double *in) {
    const int ni = 3, n0 = 5, n1 = 4, n2 = 1;

    // Layer 0
    double y0[n0]; // Layer 0 outputs;
    for (int i0=0; i0<n0; ++i0) {
        y0[i0] = b0[i0];
        for (int ii=0; ii<ni; ++ii) {
            y0[i0] += in[ii];
        }
        y0[i0] = tanh(y0[i0]);
    }

    // Layer 1
    double y1[n1]; // Layer 0 outputs;
    for (int i1=0; i1<n1; ++i1) {
        y1[i1] = b1[i1];
        for (int i0=0; i0<n0; ++i0) {
            y1[i1] += y0[i0];
        }
        y1[i1] = tanh(y1[i1]);
    }

    // Layer 2
    double y2[n2]; // Layer 0 outputs;
    for (int i2=0; i2<n2; ++i2) {
        y2[i2] = b2[i2];
        for (int i1=0; i1<n1; ++i1) {
            y2[i2] += y1[i1];
        }
        y2[i2] = tanh(y2[i2]);
    }

    return y2[0];
}
