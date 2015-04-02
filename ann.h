#ifndef ANN_H
#define ANN_H

class ann {
public:
    double w0[3][5], b0[5]; // Layer 0
    double w1[5][4], b1[4]; // Layer 1
    double w2[4][1], b2[1]; // Layer 1
    ann();

    bool load(const char *file);

    double operator() (const double *in);
};

#endif // ANN_H
