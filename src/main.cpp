#include <iostream>
#include "../include/combination_generator.h"
#include "../include/placement_strategy.h"

int main(int, char **)
{

    /* 
    //Test For Combination_generator
    combination_generator c(0, 10, 3);
    while (true)
    {
        auto [comb, flag] = c.Generate();
        std::for_each(comb.cbegin(), comb.cend(),
                      [](int e) { std::cout << e << " "; });
        std::cout << std::endl;
        if (flag)
            break;
    }
*/

    //Test For Placement_Strategy
    int k, l, g, c;
    while (true)
    {
        std::cin >> k >> l >> g >> c;
        node_dispatcher node_dis(k, l, g, c);
        node_dis.ShowLayout();
    }


    
}
