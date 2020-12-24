#pragma once
#include <algorithm>
#include <random>
#include <vector>
#include <utility>


void shuffle_helper(std::vector<int> & comb)
{
    std::default_random_engine dre(42);
    std::uniform_int_distribution<int> uid(0,comb.size()-1);
    int shuffle_times = uid(dre);
    while(shuffle_times--)
    {
        int from = uid(dre);
        int to = from ;
        while(from==to) to=uid(dre);
        std::swap(comb[from],comb[to]);
    }
}

