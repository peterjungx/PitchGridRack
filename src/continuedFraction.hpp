#include <iostream>
#include <vector>
#include <cmath>
#include <limits>

struct Fraction {
    int numerator;
    int denominator;
    Fraction(int n = 0, int d = 1) : numerator(n), denominator(d) {}
    
    float toFloat() const {
        return static_cast<float>(numerator) / denominator;
    }
};

std::vector<int> continuedFraction(double x, int max_terms = 20) {
    std::vector<int> cf;
    while (max_terms) {
        int a = static_cast<int>(x);
        cf.push_back(a);
        x = 1 / (x - a);
        if (x == std::floor(x)) {
            cf.push_back(static_cast<int>(x));
            break;
        }
        max_terms--;
    }
    return cf;
}

std::vector<Fraction> convergents(const std::vector<int>& cf) {
    std::vector<Fraction> convs;
    int p_prev = 0, q_prev = 1;
    int p = 1, q = 0;
    int p_next, q_next;
    for (int a : cf) {
        p_next = a * p + p_prev;
        q_next = a * q + q_prev;
        convs.push_back(Fraction(p_next, q_next));
        p_prev = p;
        q_prev = q;
        p = p_next;
        q = q_next;
    }
    return convs;
}

Fraction closestRational(double x, double n) {
    auto cf = continuedFraction(x);
    auto convs = convergents(cf);
    
    for (const auto& frac : convs) {
        if (std::abs(x - frac.toFloat()) <= 1.0 / n) {
            return frac;
        }
    }
    // If no fraction is found within the tolerance, return the last fraction
    return convs.empty() ? Fraction(0, 1) : convs.back();
}