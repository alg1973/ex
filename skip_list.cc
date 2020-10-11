#include <optional>
#include <iostream>
#include <vector>
#include <array>
#include <random>
#include <chrono>
#include <string>

class skip_list {
public:
    static const int max_lvl = 32;
    using keyval_t = std::pair<int,int>;
    struct node {
        node(): keyval(), size(max_lvl), next({})
            {}
        node(int k, int v, int lvl): keyval(std::make_pair(k,v)), size(lvl), next({})
            {}
        std::optional<keyval_t> keyval;
        int size;
        std::array<node*, max_lvl> next;
    };
    skip_list(): mt(rnd()), dist(0, std::numeric_limits<int>::max())  {
        head = new node;   
    }
    int rand_lvl() {
        int dice = dist(mt);
        for(int i = 1, t=2; i<max_lvl; ++i, t+=t) {
            if (dice > std::numeric_limits<int>::max()/t) {
                m_lvl = std::max(m_lvl, i);
                return i;
            }
        }
        return max_lvl;
    }
    node* search(node* ptr, int target, int lvl) {
        if (ptr == nullptr)
            return nullptr;
        if (ptr->keyval.has_value() && ((*ptr->keyval).first == target))
            return ptr;
        node* nxt = ptr->next[lvl];
        if (nxt == nullptr || target<(*nxt->keyval).first)
            return lvl == 0? nullptr: search(ptr, target, lvl-1);
        else
            return search(nxt, target, lvl);
    }
    std::optional<keyval_t> search(int target) {
        if (auto ptr = search(head, target, m_lvl)) {
            return ptr->keyval;
        }
        return std::nullopt;
    }
    node* insert(node* ptr, node* cur, int lvl) {
        node* nxt = ptr->next[lvl];
        if (nxt == nullptr || (*cur->keyval).first < (*nxt->keyval).first) {
            if (lvl < cur->next.size()) {
                ptr->next[lvl] = cur;
                cur->next[lvl] = nxt;
            }
            return lvl == 0? cur : insert(ptr, cur, lvl-1);
        } else
            return insert(nxt, cur, lvl);
    }
    void add(int k, int v) {
        node* n = new node (k,v, rand_lvl());
        insert(head, n, m_lvl);
    }
    
    bool erase(node* ptr, int k, int lvl) {
        if (ptr == nullptr)
            return false;
        node* nxt = ptr->next[lvl];
        if (nxt == nullptr || k < (*nxt->keyval).first) {
            if (lvl == 0)
                return false;
            return erase(ptr, k, lvl-1);
        }
    
        if ((*nxt->keyval).first == k) {
                ptr->next[lvl] = nxt->next[lvl];
                if (lvl == 0) {
                    delete nxt;
                    return true;
                }
                return erase(ptr, k, lvl-1);
        } else 
                return erase(nxt, k, lvl);
    }
    
    bool erase(int num) {
        return erase(head, num, m_lvl);   
    }
    int m_lvl = 0;
    node* head;
    std::random_device rnd;
    std::mt19937 mt;
    std::uniform_int_distribution<> dist;
};




int
main(int ac, char* av[])
{
    int iters = 1000;

    if(ac>1)
        iters = std::stoi(av[1]);
    
    skip_list sk;
    sk.add(1,1);
    sk.add(100,100);
    sk.add(3,3);
    auto r = sk.search(3);
    auto check = [](int k, int v, const auto& r) {
                     if (!r.has_value())
                         std::cout<<"no "<<k<<std::endl;
                     else {
                         auto [rk, rv] = *r;
                         if (rk!=k) {
                             std::cout<< "k!=rk " <<k<<"!="<<rk<<std::endl;
                         }
                         if (rv!=v) {
                             std::cout<< "v!=rv " <<v<<"!="<<rv<<std::endl;
                         }
                     }
                 };
    check(3,3,r);
    r = sk.search(0);
    check(0,0,r);
    sk.add(1000,1000);
    sk.add(3000,3000);
    r = sk.search(3000);
    check(3000,3000,r);
    sk.erase(3000);
    r = sk.search(3000);
    check(3000,3000,r);

    std::random_device rnd;
    std::mt19937 mt(rnd());
    std::uniform_int_distribution<> dist(0,std::numeric_limits<int>::max());

    //constexpr int iters = 5000000;
    std::vector<int> data;
    data.reserve(iters);
    std::cout<<"Starting gen data.\n";
    for(int i=0;i<iters;++i) {
        data.push_back(dist(mt));
    }
    std::cout<<"Ending gen data.\n";

    std::cout<<"Starting inserting into skip list.\n";
    
    auto start = std::chrono::steady_clock::now();
    for(int v: data)
        sk.add(v,v);
    auto end = std::chrono::steady_clock::now();
    std::cout<<"Ending inserting into skip list.\n";
    auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    std::cout<<elapsed.count()* std::chrono::nanoseconds::period::num /
        std::chrono::nanoseconds::period::den <<" sec ";
    std::cout<<elapsed.count()* std::chrono::nanoseconds::period::num %
        std::chrono::nanoseconds::period::den <<" nSec "<<std::endl;


    /*
    std::cout<<"Max level: "<<sk.m_lvl<<" Node sizes:\n";
    auto p = sk.head;
    while(p) {
        std::cout<<p->size<<"\n";
        p = p->next[0];
    }
    */
    
    std::cout<<"Starting searching skip list.\n";
    start = std::chrono::steady_clock::now();
    for(int v: data)
        if (sk.search(v) == std::nullopt)
            std::cout<<"missing "<<v<<"\n";
    end = std::chrono::steady_clock::now();
    std::cout<<"Ending inserting into skip list.\n";
    elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    std::cout<<elapsed.count()* std::chrono::nanoseconds::period::num /
        std::chrono::nanoseconds::period::den <<" sec ";
    std::cout<<elapsed.count()* std::chrono::nanoseconds::period::num %
        std::chrono::nanoseconds::period::den <<" nSec "<<std::endl;


    std::cout<<"Ending searching skip list.\n";
}
