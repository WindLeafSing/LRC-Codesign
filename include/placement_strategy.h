#pragma once
#include <iostream>
#include <cmath>
#include <cassert>
#include <map>
#include <set>
#include <vector>
#include <numeric>
#include <algorithm>
#include <tuple>
#include <utility>

struct placement_strategy
{
    int m_local_cluster;
    int m_global_cluster = 1; // to do
    int m_residue_cluster;
    int m_residue_overflow_cluster = 0;
    int m_local_cluster_overflow = 0;

private:
    std::tuple<int, int, int, int, int> cluster_info;

public:
    placement_strategy() = default;
    placement_strategy(int k, int l, int g)
    {
        if (k % l)
        {
            std::cerr << "l must divides k!" << std::endl;
            return;
        }
        int locality = k / l + 1;
        int node_per_local_cluster = g + 1;
        int residue_node_per_local_group = 0;
        int local_groups_per_local_cluster = 1;
        if (node_per_local_cluster >= locality)
        {
            while (local_groups_per_local_cluster * locality <= g + local_groups_per_local_cluster)
            {
                local_groups_per_local_cluster++;
            }
            local_groups_per_local_cluster--;
            m_local_cluster = l / local_groups_per_local_cluster;
            if (l / local_groups_per_local_cluster)
            {
                m_local_cluster_overflow = 1;
            }
        }
        else
        {
            //too large a local group is
            local_groups_per_local_cluster = 0;                               //even single local group is overflow
            residue_node_per_local_group = locality % node_per_local_cluster; //contain local parity
            m_local_cluster = ((locality - 1) * l / (node_per_local_cluster));
        }

        int local_groups_per_residue_cluster = 1;
        if (residue_node_per_local_group > 1)
        {
            while (local_groups_per_residue_cluster * residue_node_per_local_group <= g + local_groups_per_residue_cluster)
                local_groups_per_residue_cluster++;
            local_groups_per_residue_cluster--;
            m_residue_cluster = ceil((double)l / (double)local_groups_per_residue_cluster);
            //remain ...
            if (l % local_groups_per_residue_cluster)
            {
                m_residue_overflow_cluster = 1;
            }
        }
        else if (residue_node_per_local_group == 1)
        {
            local_groups_per_residue_cluster = l;
            m_residue_cluster = 1;
        }
        else
        {
            m_residue_cluster = 0;
        }

        cluster_info = {local_groups_per_local_cluster, std::min(k / l, node_per_local_cluster),
                        local_groups_per_residue_cluster, residue_node_per_local_group, g};
    }

    std::tuple<int, int, int, int, int> GetClusterConfig() const
    {
        return cluster_info;
    }
};

class node_dispatcher
{

public:
    node_dispatcher(int p_k, int p_l, int p_g, int p_c) : m_k(p_k), m_l(p_l), m_g(p_g), m_c(p_c)
    {
        // assert(m_g == m_k / m_l);
        m_placement = placement_strategy(m_k, m_l, m_g);
        int m_local_cluster = m_placement.m_local_cluster;
        int m_local_cluster_overflow = m_placement.m_local_cluster_overflow;
        int m_residue_cluster = m_placement.m_residue_cluster;
        int m_global_cluster = m_placement.m_global_cluster;
        int m_residue_overflow_cluster = m_placement.m_residue_overflow_cluster;
        std::vector<int> datanode(m_k, 0);
        std::vector<int> localPnode(m_l, 0);
        std::vector<int> globalPnode(m_g, 0);

        std::iota(datanode.begin(), datanode.end(), 0);
        int pstart = datanode.back();
        std::iota(localPnode.begin(), localPnode.end(), pstart + 1);
        std::iota(globalPnode.begin(), globalPnode.end(), 0);

        auto [s1, r1, s2, r2, g] = m_placement.GetClusterConfig(); //r1 = data + p
        std::cout << "s1-r1-s2-r2-g:" << s1 << " " << r1 << " "
                  << s2 << " " << r2 << " " << g << std::endl;
        int locality = m_k / m_l + 1;
        int local_groups_per_residue_cluster = s2;
        if (s1 == 0)
        {
            int ratio_cluster_local_group = m_local_cluster / m_l;
            for (int i = 0; i < m_local_cluster; ++i)
            {
                int start = i / ratio_cluster_local_group;
                int inner = i % ratio_cluster_local_group;
                LocalClusterMap.insert(std::make_pair(i, std::set<int>(datanode.begin() + start * (locality - 1) + inner * r1,
                                                                       datanode.begin() + start * (locality - 1) + (inner + 1) * r1)));
            }
        }
        else
        {
            int start = 0;
            int stride = r1 * s1;
            for (int i = 0; i < m_local_cluster + m_local_cluster_overflow; ++i)
            {
                if ((i + 1) * stride <= m_k)
                {
                    std::set<int> s(datanode.begin() + i * stride, datanode.begin() + (i + 1) * stride);
                    if (!s.empty())
                        LocalClusterMap.insert(std::make_pair(i, s));
                }
                else
                {
                    std::set<int> s(datanode.begin() + i * stride, datanode.end());
                    if (!s.empty())
                        LocalClusterMap.insert(std::make_pair(i, s));
                }
            }
        }
        int localPindex = 0;
        int localdcounter = (r1)*m_local_cluster;
        int startdindex = 1 * (locality - 1) - r2 + 1;
        if (0 != m_residue_cluster)
        {
            for (int i = 0; i < m_residue_cluster + m_residue_overflow_cluster /* 0 or 1*/; ++i)
            {

                if (((localdcounter + (r2 - 1) * local_groups_per_residue_cluster)) <= m_k)
                {
                    //pick out residue into a set
                    std::set<int> s;
                    for (int j = 0; j < local_groups_per_residue_cluster; ++j)
                    {
                        for (int k = 0; k < (r2 - 1); ++k)
                            s.insert(datanode[startdindex + k]);
                        startdindex += locality - 1;
                    }
                    if (!s.empty())
                        ResidueClusterMap.insert(std::make_pair(i, s));
                    localdcounter += (r2 - 1) * local_groups_per_residue_cluster;
                }
                else
                {
                    if (localdcounter < m_k)
                        ResidueClusterMap.insert(std::make_pair(i, std::set<int>(datanode.begin() + startdindex,
                                                                                 datanode.end())));
                }
                for (int j = 0; localPindex < localPnode.size() && j < local_groups_per_residue_cluster; j++)
                {
                    ResidueClusterMap[i].insert(localPnode[localPindex++]);
                }
            }
        }
        else
        {
            for (int i = 0; i < m_local_cluster + m_local_cluster_overflow; ++i)
            {
                for (int j = 0; localPindex < m_l && j < s1; ++j)
                    LocalClusterMap[i].insert(localPnode[localPindex++]);
            }
        }

        GlobalClusterMap.insert(std::make_pair(0, std::set<int>(globalPnode.begin(),
                                                                globalPnode.end())));
    }

    void ShowLayout(std::ostream & os = std::cout) const
    {

        os << "Local Cluster Mapping : \n" ;
        for (const auto &p : LocalClusterMap)
        {
            os << "Type-I Cluster " << p.first << " : \n";
            std::for_each(p.second.cbegin(), p.second.cend(), [&os](int e) { os << e << " "; });
            os << "\n";
        }

        os << "Residue Cluster Mapping : \n";
        for (const auto &p : ResidueClusterMap)
        {
            os << "Type-II Cluster " << p.first << " : \n";
            std::for_each(p.second.cbegin(), p.second.cend(), [&os](int e) { os << e << " "; });
           os << "\n";
        }

        os << "Global Cluster Mapping : \n " ;
        for (const auto &p : GlobalClusterMap)
        {
            os << "Type-III Cluster " << p.first << " : \n";
            std::for_each(p.second.cbegin(), p.second.cend(), [&os](int e) { os << e << " "; });
            os << "\n";
        }
    }

    int GetLocalClusterNum() const
    {
        return LocalClusterMap.size();
    }

    int GetResidueClusterNum() const
    {
        return ResidueClusterMap.size();
    }

    int GetGlobalClusterNum() const
    {
        return GlobalClusterMap.size();
    }

    std::map<int, std::set<int>> GetLocalClusterInfo() const
    {
        return LocalClusterMap;
    }

    std::map<int, std::set<int>> GetResidueClusterInfo() const
    {
        return ResidueClusterMap;
    }

    std::map<int, std::set<int>> GetGlobalClusterInfo() const
    {
        return GlobalClusterMap;
    }

private:
    int m_k;
    int m_l;
    int m_g;
    int m_c;
    std::map<int, std::set<int>> LocalClusterMap;
    std::map<int, std::set<int>> ResidueClusterMap;
    std::map<int, std::set<int>> GlobalClusterMap;
    placement_strategy m_placement;
};