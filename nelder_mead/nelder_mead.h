// Copyright (c) 2023, Chuan Tian
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#ifndef TSOLVER_NELDER_MEAD_H_
#define TSOLVER_NELDER_MEAD_H_

#include <Eigen/Dense>
#include <algorithm>
#include <numeric>

namespace Tsolver
{
using Eigen::VectorXd;
using std::array;
using std::vector;

struct Simplex
{
    const int d;
    const double alpha, gamma, rho, sigma;
    vector<VectorXd> points;
    VectorXd x0;
    double (*fn)(const double*);

    Simplex(int d, double (*fn)(const double*), double alpha = 1, double gamma = 2, double rho = 0.5,
            double sigma = 0.5)
        : d(d), alpha(alpha), gamma(gamma), rho(rho), sigma(sigma), fn(fn)
    {
        points.resize(d + 1, VectorXd(d));
        for (auto& i : points)
        {
            i.setRandom();
            i = i * 0.1;
        }
    }

    void order()
    {
        std::ranges::sort(points, [&](const VectorXd& a, const VectorXd& b) { return fn(a.data()) < fn(b.data()); });
        x0 = VectorXd(d).setZero();
        x0 = (static_cast<double>(1) / d) * std::accumulate(points.begin(), points.begin() + d, x0);
    }

    [[nodiscard]] VectorXd reflection() const
    {
        return x0 + alpha * (x0 - points.back());
    }

    [[nodiscard]] VectorXd expansion(const VectorXd& xr) const
    {
        return x0 + gamma * (xr - x0);
    }

    [[nodiscard]] VectorXd contraction(const VectorXd& xr, const int id) const
    {
        if (id == 0)
            return x0 + rho * (xr - x0);
        return x0 + rho * (points.back() - x0);
    }

    void shrink()
    {
        const auto s = points.size();
        for (int i = 1; i < s; i++)
        {
            points[i] = points[1] + sigma * (points[i] - points[1]);
        }
    }
};

inline vector<double> cast_to_vector(const VectorXd& vxd)
{
    auto ret = vector<double>(vxd.data(), vxd.data() + vxd.rows());
    return ret;
}

inline vector<double> nelder_mead(double (*f)(const double*), const int d)
{
    Simplex simplex(d, f);
    bool converge = false;
    for (int i = 0; i < 1000000; i++)
    {
        simplex.order();
        const VectorXd xr = simplex.reflection();
        const VectorXd x1 = simplex.points[1];
        const VectorXd xn = simplex.points[d - 1];
        const VectorXd x_last = simplex.points.back();
        const double f_xr = f(xr.data());
        const double f_x1 = f(x1.data());
        if (f_x1 <= f_xr && f_xr < f(xn.data()))
            simplex.points.back() = xr;
        else if (f_xr < f_x1)
        {
            const VectorXd xe = simplex.expansion(xr);
            if (f(xe.data()) < f_xr)
                simplex.points.back() = xe;
            else
                simplex.points.back() = xr;
        }
        else if (f_xr < f(x_last.data()))
        {
            const VectorXd xc = simplex.contraction(xr, 0);
            if (f(xc.data()) < f_xr)
                simplex.points.back() = xc;
            else
                simplex.shrink();
        }
        else
        {
            const VectorXd xc = simplex.contraction(xr, 1);
            if (f(xc.data()) < f(x_last.data()))
                simplex.points.back() = xc;
            else
                simplex.shrink();
        }
        VectorXd ds = simplex.points.back() - simplex.points.front();
        if (ds.norm() < 1e-8)
        {
            printf("Terminal condition met at iteration %d : l2 norm < 1e-8\n", i);
            converge = true;
            break;
        }
    }
    if (!converge)
        printf("WARNING: algorithm failed to coverage!\n");
    return cast_to_vector(simplex.points[0]);
}
} // namespace Tsolver
#endif
