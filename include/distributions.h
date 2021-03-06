//
// Created by roryh on 05/04/2020.
//

#ifndef EPIGRAPH_DISTRIBUTIONS_H
#define EPIGRAPH_DISTRIBUTIONS_H

#include <cmath>
#include <random>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include "csv.h"

// not thread safe, careful with openMP
std::mt19937 &global_engine();

class PowerLaw {
    /*
     * Models a power law distribution.
     */

public:
    std::uniform_real_distribution<> uni_dist;
    double m_alpha;
    double m_xmin;
    double m_xmax;

    PowerLaw(double alpha, double xmin) : m_alpha(alpha), m_xmin(xmin), uni_dist(0.0, 1.0) {}

    static double pdf(double x, double alpha, double x_min) {
        /*
         * The pdf for a power law distribution
         */
        return ((alpha - 1) / x_min) * pow((x / x_min), -alpha);
    }

    static double non_normal_pdf(double x, double alpha, double x_min) {
        /*
         * The non-normalised pdf (does not integrate to 1) for a power law distribution
         */
        return pow(x, -alpha);
    }

    static double cdf(double u, double alpha, double x_min) {
        /*
         * The cdf for a power law distribution
         */
        return pow(u / x_min, -alpha + 1);
    }

    static double cdf_inv(double u, double alpha, double x_min) {
        /*
        * The inverse cdf for a power law distribution
        */
        return pow(x_min * u, -1 / (alpha - 1));
    }

    template<typename RandomGenerator>
    double operator()(RandomGenerator &gen) {
        double r = uni_dist(gen);
        return cdf_inv(r, m_alpha, m_xmin);
    }
};


struct ProbDist {
    std::discrete_distribution<> index_dist;
    std::vector<double> vals;
    std::vector<double> probs;

    ProbDist() : index_dist(), vals() {};

    template<typename Iter1, typename Iter2>
    ProbDist(Iter1 val_begin, Iter1 val_end, Iter2 prob_begin, Iter2 prob_end) {
        ///std::sort(val_begin, val_end);

        for (; val_begin != val_end; val_begin++) {
            vals.push_back(*val_begin);
        }
        std::vector<double> probs;
        for (; prob_begin != prob_end; prob_begin++) {
            probs.push_back(*prob_begin);
        }
        if (vals.size() != probs.size()) {
            throw std::invalid_argument("Iterators length don't match");
        }
        std::discrete_distribution<> tmp_dist(probs.begin(), probs.end());
        index_dist = tmp_dist;

        probs = index_dist.probabilities();
    }

    template<typename Iter1>
    ProbDist(Iter1 val_begin, Iter1 val_end) {
        std::sort(val_begin, val_end,
                  [](std::pair<double, double> p1, std::pair<double, double> p2) -> bool {
                      return p1.first < p2.first;
                  });

        std::vector<double> weights;
        for (; val_begin != val_end; val_begin++) {
            vals.push_back(val_begin->first);
            weights.push_back(val_begin->second);
        }


        if (vals.size() != weights.size()) {
            throw std::invalid_argument("Iterators length don't match");
        }
        std::discrete_distribution<> tmp_dist(weights.begin(), weights.end());
        index_dist = tmp_dist;

        probs = index_dist.probabilities();
    }

    ProbDist &operator=(ProbDist rhs) {
        index_dist = rhs.index_dist;
        vals = rhs.vals;
        probs = index_dist.probabilities();

        return *this;
    }

    template<typename RandomGenerator>
    double operator()(RandomGenerator &gen) {

        return vals[index_dist(gen)];
    }

    auto get_prob(double val) -> double {
        auto it = std::lower_bound(vals.begin(), vals.end(), val);
        if (it == vals.end()) it--;
        int index = std::distance(vals.begin(), it);

        return index_dist.probabilities().at(index);
    }
};

auto ProbDist_from_csv(std::string csv) -> ProbDist {
    io::CSVReader<2> in(csv);
    in.read_header(io::ignore_extra_column, "value", "probability");
    std::vector<std::pair<double, double>> val_prob_pairs;

    double val; double prob;
    while(in.read_row(val, prob)) {
        val_prob_pairs.push_back({val, prob});
    }

    return ProbDist(val_prob_pairs.begin(), val_prob_pairs.end());
}

#endif //EPIGRAPH_DISTRIBUTIONS_H
