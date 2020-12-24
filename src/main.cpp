#include <iostream>
#include <fstream>
#include "../include/combination_generator.h"
#include "../include/placement_strategy.h"
#include "../include/common.h"
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
    /*
    //Test For Placement_Strategy
    int k, l, g, c;
    while (true)
    {
        std::cin >> k >> l >> g >> c;
        node_dispatcher node_dis(k, l, g, c);
        node_dis.ShowLayout();
        //
    }
*/
    int k, l, g, c;
    std::cout << "input your k-l-g-c:" << std::endl;
    std::cin >> k >> l >> g >> c;
    assert(k%l==0);
    assert(k/l==g);
    node_dispatcher node_dis(k, l, g, c);
    int required_local_cluster = node_dis.GetLocalClusterNum();
    int required_residue_cluster = node_dis.GetResidueClusterNum();
    int required_global_cluster = node_dis.GetGlobalClusterNum();
    node_dis.ShowLayout();

    auto localmap = node_dis.GetLocalClusterInfo();
    auto residuemap = node_dis.GetResidueClusterInfo();
    auto globalmap = node_dis.GetGlobalClusterInfo();

    //since considering c = 2*required_cluster_num is enough
    c = std::min(c, 2 * (required_local_cluster + required_residue_cluster + required_global_cluster));

    //suppose existed stripe (0,1,2,...,n) where global cluster is n
    std::vector<std::vector<std::pair<int, int>>> cost(2 /* 2 stripe */, std::vector<std::pair<int, int>>(c, {0, 0}));
    combination_generator comb(0, c - 1, required_local_cluster + required_residue_cluster + required_global_cluster);

    //fill in first stripe and calculate datanodenum and paritynodenum per cluster
    std::vector<int> datanode_distribution(required_local_cluster + required_residue_cluster, 0);
    int start = 0;
    for (int i = 0; i < required_local_cluster; ++i)
    {
        int datanode_num = 0;
        for (auto e : localmap[i])
        {
            if (e < k)
                datanode_num++;
        }
        datanode_distribution[start] = datanode_num;
        cost[0][start++] = {datanode_num, localmap[i].size() - datanode_num};
    }

    for (int i = 0; i < required_residue_cluster; ++i)
    {
        int datanode_num = 0;
        for (auto e : residuemap[i])
        {
            if (e < k)
                datanode_num++;
        }
        datanode_distribution[start] = datanode_num;
        cost[0][start++] = {datanode_num, residuemap[i].size() - datanode_num};
    }

    for (int i = 0; i < required_global_cluster; ++i)
    {
        cost[0][start++] = {0, globalmap[i].size()};
    }

    std::fstream fs("simulation_result",std::ios::out);
    fs<<"for fixed first stripe layout\n";
    fs<<"given second stripe layout and following its minimum typeI typeII costs:\n";
    fs.flush();
    //random distributed second stripe
    //generate random cluster index , last one is global cluster
    do
    {
        auto [clusterindex, flag] = comb.Generate();
        std::sort(clusterindex.begin(), clusterindex.end());
        do
        {
            //reset cost[1] which indicates second stripe
            cost[1].assign(c, {0, 0});

            int curmax = -1;
            int max_datanode_cluster_index = -1;
            int type_II_migration_num = 0;
            int index = 0;
            for (int i = 0; i < required_local_cluster; ++i)
            {
                if (clusterindex[index] < required_local_cluster + required_residue_cluster && (cost[0][clusterindex[index]].first + cost[0][clusterindex[index]].second) != 0)
                {
                    //this cluster second stripe located need a migration
                    type_II_migration_num += localmap[i].size();
                }
                cost[1][clusterindex[index]] = {datanode_distribution[index],
                                                localmap[i].size() - datanode_distribution[index]};
                if (cost[1][clusterindex[index]].first + cost[0][clusterindex[index]].first > curmax)
                {
                    max_datanode_cluster_index = clusterindex[index];
                    curmax = cost[1][clusterindex[index]].first + cost[0][clusterindex[index]].first;
                }
                index++;
            }

            for (int i = 0; i < required_residue_cluster; ++i)
            {
                if (clusterindex[index] < required_residue_cluster + required_local_cluster && (cost[0][clusterindex[index]].first + cost[0][clusterindex[index]].second) != 0)
                {
                    //this cluster second stripe located need a migration
                    type_II_migration_num += residuemap[i].size();
                }
                cost[1][clusterindex[index]] = {datanode_distribution[index],
                                                residuemap[i].size() - datanode_distribution[index]};
                if (cost[1][clusterindex[index]].first + cost[0][clusterindex[index]].first > curmax)
                {
                    max_datanode_cluster_index = clusterindex[index];
                    curmax = cost[1][clusterindex[index]].first + cost[0][clusterindex[index]].first;
                }
                index++;
            }

            for (int i = 0; i < required_global_cluster; ++i)
            {
                cost[1][clusterindex[index]] = {0, globalmap[i].size()};
                index++;
            }
            int type_I_cost = 2 * k - curmax;
            int type_II_cost = type_II_migration_num;

            //debug
            std::cout << "for fixed first stripe layout : "<<std::endl;
            node_dis.ShowLayout();
            std::cout << "choose following cluster to place second stripe in order : " << std::endl;
            std::for_each(clusterindex.cbegin(),clusterindex.cend(),
            [&fs](int i){ std::cout << i <<" "; fs << i <<" ";});
            std::cout << std::endl;
            fs<<"\n";

            std::cout <<"then the minimum type_I and type_II cost are : " <<std::endl;
            std::cout << type_I_cost <<" and " <<type_II_cost << std::endl;
            //
           
            // transition_cost_I_II.emplace_back(std::make_pair(type_I_cost, type_II_cost)); //this is the minimum for this randomly chosen clusters
            
            fs<<type_I_cost<<"-"<<type_II_cost<<"\n";
            fs.flush();
        } while (std::next_permutation(clusterindex.begin(), clusterindex.end()));
        if(flag) break;
    } while (true);
}
