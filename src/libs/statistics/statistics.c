#include "statistics.h"
#include "../../common.h"
#include <math.h>
#include <stdlib.h>

/// Regularized lower incomplete gamma function
double rlgamma(double a, double x) {
    const double epsilon = 0.000000000000001;
    const double big = 4503599627370496.0;
    const double bigInv = 2.22044604925031308085e-16;
    ASSERT(a > 0 && x > 0);

    if (x == 0)
        return 0.0f;

    double ax = (a * log(x)) - x - lgamma(a);
    if (ax < -709.78271289338399)
        return a < x ? 1.0 : 0.0;

    if (x <= 1 || x <= a) {
            double r2 = a;
            double c2 = 1;
            double ans2 = 1;

        do {
            r2 = r2 + 1;
            c2 = c2 * x / r2;
            ans2 += c2;
        } while ((c2 / ans2) > epsilon);

        return exp(ax) * ans2 / a;
    }

    int c = 0;
    double y = 1 - a;
    double z = x + y + 1;
    double p3 = 1;
    double q3 = x;
    double p2 = x + 1;
    double q2 = z * x;
    double ans = p2 / q2;
    double error;

    do {
        c++;
        y += 1;
        z += 2;
        double yc = y * c;
        double p = (p2 * z) - (p3 * yc);
        double q = (q2 * z) - (q3 * yc);

        if (q != 0) {
            double nextans = p / q;
            error = abs((ans - nextans) / nextans);
            ans = nextans;
        } else {
            // zero div, skip
            error = 1;
        }

        // shift
        p3 = p2;
        p2 = p;
        q3 = q2;
        q2 = q;

        // normalize fraction when the numerator becomes large
        if (abs(p) > big) {
            p3 *= bigInv;
            p2 *= bigInv;
            q3 *= bigInv;
            q2 *= bigInv;
        }
    } while (error > epsilon);

    return 1.0 - (exp(ax) * ans);
}

inline double chi2_cdf(double x, int dof) {
    if (dof < 1 || x < 0) {
        return 0.0;
    } else if (dof == 2) {
        return 1.0 - exp(-0.5*x);
    } else {
        return rlgamma(0.5 * dof, 0.5 * x);
    }
}
