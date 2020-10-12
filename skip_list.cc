#include <optional>
#include <iostream>
#include <vector>
#include <array>
#include <random>
#include <chrono>
#include <string>
#include <map>

template<typename KEY, typename VALUE>
class skip_list {
public:
    static const int max_lvl = 24;
    using keyval_t = std::pair<KEY,VALUE>;
    struct node {
      node(): keyval(), size(max_lvl)
      {
	for(int i=0;i<size;++i) next[i] = nullptr;
      }
      node(KEY k, VALUE v, int lvl): keyval(std::make_pair(k,v)), size(lvl)
      {
	for(int i=0;i<size;++i) next[i] = nullptr;
      }
      std::optional<keyval_t> keyval;
      int size;
      //std::array<node*, max_lvl> next;
      node* next[1];
    };
    node* create_node(KEY k, VALUE v, int lvl) {
      int sz = sizeof(node) + sizeof(node*)*(lvl-1);
      void* mem = ::operator new(sz);
      return  new (mem) node(k, v, lvl);
    }
    node* create_node() {
      int sz = sizeof(node) + sizeof(node*)*(max_lvl-1);
      void* mem = ::operator  new(sz);
      return  new (mem) node;
    }

  skip_list(): mt(rnd()), dist(0, std::numeric_limits<int>::max())  {
    head = create_node();   
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
  node* search(node* ptr,const KEY& target, int lvl) const {
        if (ptr == nullptr) {
            return nullptr;
        }
        if (ptr->keyval.has_value() && ((*ptr->keyval).first == target))
            return ptr;
        node* nxt = ptr->next[lvl];
        if (nxt == nullptr || target<(*nxt->keyval).first)
            return lvl == 0? nullptr: search(ptr, target, lvl-1);
        else
            return search(nxt, target, lvl);
    }
    
    std::optional<std::reference_wrapper<VALUE>> search(const KEY& target) const {
        if (auto ptr = search(head, target, m_lvl)) {
            return {(*ptr->keyval).second};
        }
        return std::nullopt;
    }
    std::optional<keyval_t> search_kv(const KEY& target) const {
        if (auto ptr = search(head, target, m_lvl)) {
            return ptr->keyval;
        }
        return std::nullopt;
    }
    node* insert(node* ptr, node* cur, int lvl) {
        node* nxt = ptr->next[lvl];
        if (nxt == nullptr || (*cur->keyval).first < (*nxt->keyval).first) {
            if (lvl < cur->size) {
                ptr->next[lvl] = cur;
                cur->next[lvl] = nxt;
            }
            return lvl == 0? cur : insert(ptr, cur, lvl-1);
        } else
            return insert(nxt, cur, lvl);
    }
    void add(KEY k, VALUE v) {
        node* n = create_node(std::move(k),std::move(v), rand_lvl());
        insert(head, n, m_lvl);
    }
    
    bool erase(node* ptr,const KEY& k, int lvl) {
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
		  nxt->~node();
		  ::operator delete(reinterpret_cast<void*>(nxt));
		  return true;
                }
                return erase(ptr, k, lvl-1);
        } else 
                return erase(nxt, k, lvl);
    }
    
    bool erase(const KEY& num) {
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

    int test = 3;
    if (ac>2) {
      test = std::stoi(av[2]);
    }
    
    skip_list<int,int> sk;
    sk.add(1,1);
    sk.add(100,100);
    sk.add(3,3);
    auto r = sk.search(3);
    auto check = [](int k, int v, const auto& r) {
                     if (!r.has_value())
                         std::cout<<"no "<<k<<std::endl;
                     else {
                         if (*r!=v) {
                             std::cout<< "v!=rv " <<v<<"!="<<*r<<std::endl;
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
    if (test & 1) {
      for(int v: data)
        sk.add(v,v);
    }
    auto end = std::chrono::steady_clock::now();
    std::cout<<"Ending inserting into skip list.\n";
    auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    std::cout<<elapsed.count()* std::chrono::nanoseconds::period::num /
        std::chrono::nanoseconds::period::den <<" sec ";
    std::cout<<elapsed.count()* std::chrono::nanoseconds::period::num %
        std::chrono::nanoseconds::period::den <<" nSec "<<std::endl;



    std::cout<<"Max level: "<<sk.m_lvl<<"\n";
    /*
    auto p = sk.head;
    std::map<int,int> cnt;
    while(p) {
        cnt[p->size]++;
        p = p->next[0];
    }
    for(const auto& x: cnt) {
        std::cout<<x.first<<" sz : "<<x.second<<" cnt\n";
    }
    */
    std::cout<<"Starting searching skip list.\n";
    start = std::chrono::steady_clock::now();
    //std::map<int,int> op_count;
    int skcount = 0;
    if (test & 1) {
      for(int v: data) {
          auto r = sk.search(v);
          skcount += r.has_value()?1:0;
      }
    }
    end = std::chrono::steady_clock::now();
    std::cout<<"Ending searching skip list. "<<skcount<<" goods.\n";
    elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    std::cout<<elapsed.count()* std::chrono::nanoseconds::period::num /
        std::chrono::nanoseconds::period::den <<" sec ";
    std::cout<<elapsed.count()* std::chrono::nanoseconds::period::num %
        std::chrono::nanoseconds::period::den <<" nSec "<<std::endl;

    //for(const auto& x: op_count) {
    //    std::cout<<x.first<<" stp : "<<x.second<<" cnt\n";
    //}
    std::map<int,int> rb;
    std::cout<<"Starting inserting into RB map.\n";
    start = std::chrono::steady_clock::now();
    if (test & 2) {
      for(int v: data)
        rb[v]=v;
    }
    end = std::chrono::steady_clock::now();
    std::cout<<"Ending inserting into RB map.\n";
    elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    std::cout<<elapsed.count()* std::chrono::nanoseconds::period::num /
        std::chrono::nanoseconds::period::den <<" sec ";
    std::cout<<elapsed.count()* std::chrono::nanoseconds::period::num %
        std::chrono::nanoseconds::period::den <<" nSec "<<std::endl;

    std::cout<<"Starting searching RB map.\n";
    start = std::chrono::steady_clock::now();
    int rbcount = 0;
    if (test & 2) {
      for(int v: data)
          rbcount += rb.find(v) == rb.end()?0:1;
    }
    end = std::chrono::steady_clock::now();
    std::cout<<"Ending searching RB map. "<<rbcount<<" goods. \n";
    elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    std::cout<<elapsed.count()* std::chrono::nanoseconds::period::num /
        std::chrono::nanoseconds::period::den <<" sec ";
    std::cout<<elapsed.count()* std::chrono::nanoseconds::period::num %
        std::chrono::nanoseconds::period::den <<" nSec "<<std::endl;


}
