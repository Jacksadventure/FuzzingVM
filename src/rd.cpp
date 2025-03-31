#pragma once

#include<random>

int getRandomNumber(int max) {
    std::random_device rd;  // Obtain a random number from hardware
    std::mt19937 eng(rd()); // Seed the generator
    std::uniform_int_distribution<> distr(0,max); // Define the range
    return distr(eng);
}