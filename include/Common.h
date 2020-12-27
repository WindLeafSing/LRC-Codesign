#pragma once
#include <algorithm>
#include <random>
#include <vector>
#include <utility>


void shuffle_helper(std::vector<int> & comb)
{
    std::default_random_engine dre(rand());
    std::uniform_int_distribution<int> uid(0,comb.size()-1);
        int from = uid(dre);
        int to = uid(dre) ;
        while(from==to) to=uid(dre);
        std::swap(comb[from],comb[to]);

    
}

