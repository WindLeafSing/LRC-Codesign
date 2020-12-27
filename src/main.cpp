#include <iostream>
#include <fstream>
#include <string>
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
    assert(k % l == 0);
    //assert(k / l == g);

    node_dispatcher node_dis(k, l, g, c);
    int required_local_cluster = node_dis.GetLocalClusterNum();
    int required_residue_cluster = node_dis.GetResidueClusterNum();
    int required_global_cluster = node_dis.GetGlobalClusterNum();
    node_dis.ShowLayout();
    assert(c >= 2 * (required_local_cluster + required_residue_cluster + required_global_cluster) - 1);
    auto localmap = node_dis.GetLocalClusterInfo();
    auto residuemap = node_dis.GetResidueClusterInfo();
    auto globalmap = node_dis.GetGlobalClusterInfo();

    //
    // 10 stripes
    int stripe_num = 10;

    std::vector<std::vector<int>> cluster_layout(stripe_num, std::vector<int>(c, -1));
    /*
    
     0   1   2   3   4   5   6   ...   c-1
     0  c1  c2  c3   0   0   c4  ...    0
     c2  0  c1  c4   0   0   c3  ...    0
     ...

    */

    //suppose existed stripe (0,1,2,...,n) where global cluster is n
    combination_generator comb(0, c - 1, required_local_cluster + required_residue_cluster + required_global_cluster);

    //fill in first stripe and calculate datanodenum and paritynodenum per cluster
    std::vector<std::pair<int, int>> datanode_distribution(required_local_cluster + required_residue_cluster, {0, 0});

    //randomly distributed into c clusters
    std::default_random_engine dre(42);
    std::vector<int> index(required_local_cluster + required_residue_cluster + required_global_cluster, -1);
    bool rollingflag = false;
    for (int i = 0; i < stripe_num; ++i)
    {
        std::tie(index, rollingflag) = comb.Generate();
        shuffle_helper(index);
        // std::cout << "after  shuffle  :";
        // std::for_each(index.cbegin(),index.cend(),[](int i ){std::cout<< i <<" ";});
        // std::cout <<  std::endl;
        //index[0] = i -> put c0 into cluser-i

        for (int j = 0; j < index.size(); ++j)
        {
            cluster_layout[i][index[j]] = j;
        }
    }

    //  debug display cluster layout
    for (const auto &v : cluster_layout)
    {
        for (auto i : v)
        {
            std::cout << i << " ";
        }
        std::cout << std::endl;
    }

    int start = 0;
    for (int i = 0; i < required_local_cluster; ++i)
    {
        int datanode_num = 0;
        for (auto e : localmap[i])
        {
            if (e < k)
                datanode_num++;
        }
        datanode_distribution[start++] = {datanode_num, localmap[i].size() - datanode_num};
    }

    for (int i = 0; i < required_residue_cluster; ++i)
    {
        int datanode_num = 0;
        for (auto e : residuemap[i])
        {
            if (e < k)
                datanode_num++;
        }
        datanode_distribution[start++] = {datanode_num, residuemap[i].size() - datanode_num};
    }

    for (int i = 0; i < required_global_cluster; ++i)
    {
        datanode_distribution[start++] = {0, globalmap[i].size()};
    }

    //random shuffle the "template" vector

    int max_datanode_per_cluster = (*std::max_element(datanode_distribution.cbegin(), datanode_distribution.cend(),
                                                      [](std::pair<int, int> p1, std::pair<int, int> p2) { return p1.first < p2.first; }))
                                       .first;
    std::fstream fs(std::string(std::to_string(k)).append("-").append(std::to_string(l)).append("-").append(std::to_string(g)).append("-").append(std::to_string(c)), std::ios::out|std::ios::in|std::ios::trunc);
    node_dis.ShowLayout(fs);
    fs.flush();
    //random distributed second stripe
    //generate random cluster index , last one is global cluster
    int type_I_cost = 2 * k;
    int type_II_cost_total = 0;
    std::vector<int> type_II_cost(stripe_num / 2, 0);
    for (int i = 0; i < stripe_num - 1; i += 2)
    {
        for (int j = 0; j < c; ++j)
        {
            if (cluster_layout[i][j] > 0 && cluster_layout[i + 1][j] > 0)
            {
                if (cluster_layout[i][j] < required_local_cluster + required_residue_cluster &&
                    cluster_layout[i + 1][j] << required_local_cluster + required_residue_cluster)
                {
                    type_II_cost[i] += std::min(datanode_distribution[cluster_layout[i][j]].first + datanode_distribution[cluster_layout[i][j]].second,
                                                datanode_distribution[cluster_layout[i + 1][j]].first + datanode_distribution[cluster_layout[i + 1][j]].second);
                    type_II_cost_total += type_II_cost[i];
                }
            }
        }
    }

    //display cluster layout
    int cluster_for_local = required_residue_cluster + required_local_cluster;
    fs << "I will show cluster layout for " << stripe_num << " stripes "
       << "over " << c << " clusters \n";
    fs << "An example for stipe 0 and stripe 1 : \n";
    for (int s = 0; s < 2; ++s)
    {
        fs << "Stripe " << s << " :\n";
        for (int i = 0; i < cluster_layout[s].size(); ++i)
        {
            fs << "Cluster " << i << " : ";
            if (cluster_layout[s][i] >= 0)
            {
                if (cluster_layout[s][i] < cluster_for_local)
                {
                    if (cluster_layout[s][i] < required_local_cluster)
                    {
                        auto &local_c = localmap[cluster_layout[s][i]];
                        for (auto node : local_c)
                        {
                            fs << (node < k ? std::string{"D"} + std::to_string(node) : std::string{"L"} + std::to_string(node-k)) << " ";
                        }
                        fs << "\n";
                    }
                    else
                    {
                        //residue cluster
                        auto &local_c = residuemap[cluster_layout[s][i] - required_local_cluster];
                        for (auto node : local_c)
                        {
                            fs << (node < k ? std::string{"D"} + std::to_string(node) : std::string{"L"} + std::to_string(node-k)) << " ";
                        }
                        fs << "\n";
                    }
                }
                else
                {
                    //global cluster
                    auto &local_c = globalmap[cluster_layout[s][i] - cluster_for_local];
                    for (auto node : local_c)
                    {
                        fs << std::string{"G"} + std::to_string(node) << " ";
                    }
                    fs << "\n";
                }
            }
            else
            {
                //empty cluster
                fs << " Empty Cluster \n";
            }
        }
    }

    
    //display cost
    fs << "To reconstruct global parity in a naive way, " << type_I_cost << " blocks "
        <<"will be retrived\n";
    fs << "To maintain single cluster failure tolerance, at lease "<<type_II_cost[0] << " blocks "
        <<"will be migrated\n";
    
    
    /*
    do
    {
        auto [clusterindex, flag] = comb.Generate();
        std::sort(clusterindex.begin(), clusterindex.end());
        fs << "choose these cluster : \n";
        std::for_each(clusterindex.cbegin(),clusterindex.cend(),
        [&fs](int e){ fs << e << " ";});
        fs<<"\n";fs.flush();
        do
        {
            //reset cost[1] which indicates second stripe
            cost[1].assign(c, {0, 0});

            int type_II_migration_num = 0;
            int index = 0;
            for (int i = 0; i < required_local_cluster; ++i)
            {
                if ((clusterindex[index] < required_local_cluster + required_residue_cluster) && (cost[0][clusterindex[index]].first + cost[0][clusterindex[index]].second) != 0)
                {
                    //this cluster second stripe located need a migration
                    type_II_migration_num += localmap[i].size();
                }
                cost[1][clusterindex[index]] = {datanode_distribution[index],
                                                localmap[i].size() - datanode_distribution[index]};
                
                index++;
            }

            for (int i = 0; i < required_residue_cluster; ++i)
            {
                if ((clusterindex[index] < required_residue_cluster + required_local_cluster) && (cost[0][clusterindex[index]].first + cost[0][clusterindex[index]].second) != 0)
                {
                    //this cluster second stripe located need a migration
                    type_II_migration_num += residuemap[i].size();
                }
                cost[1][clusterindex[index]] = {datanode_distribution[index],
                                                residuemap[i].size() - datanode_distribution[index]};
                
                index++;
            }

            for (int i = 0; i < required_global_cluster; ++i)
            {
                cost[1][clusterindex[index]] = {0, globalmap[i].size()};
                index++;
            }
            //always reencoding merged stripe which is doubled
            int type_I_cost = 2 * k ;
            
            int type_II_cost = type_II_migration_num;

            //debug
            std::cout << "for fixed first stripe layout : "<<std::endl;
            node_dis.ShowLayout();
            std::cout << "choose following cluster to place second stripe in order : " << std::endl;
            std::for_each(clusterindex.cbegin(),clusterindex.cend(),
            [&fs](int i){ std::cout << i <<" "; fs << i <<" ";});
            std::cout << std::endl;
            fs<<"\n";

            std::cout <<"then the type_II cost is : " <<std::endl;
            std::cout << type_II_cost << std::endl;
            //
           
            // transition_cost_I_II.emplace_back(std::make_pair(type_I_cost, type_II_cost)); //this is the minimum for this randomly chosen clusters
            
            fs<<type_II_cost<<"\n";
            fs.flush();
        } while (std::next_permutation(clusterindex.begin(), clusterindex.end()));
        if(flag) break;
    } while (true);
*/
    return 0;
}
