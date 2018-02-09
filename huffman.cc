#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <cstdint>
#include <unordered_map>
#include <bitset>
#include <stdexcept>

namespace pack {

  using bits=std::vector<std::uint8_t>;
  using prefix_map=std::unordered_map<std::uint8_t,bits>;

  // There is a problem with word_size. The longest bit prefix 
  // length can be equal to unique charachters amount - 1 
  // Fortunately in order to have maximum trie height characters 
  // frequences should be in fibbonachi sequence. May be better 
  // to store frequences and recreate trie every time.
 
  constexpr int word_size=64;
  using word_t=std::uint64_t;

  struct node {
    node (int f,char c): freq(f),chr(c),zero_link(nullptr),
					one_link(nullptr)
    {}
    int freq;
    std::uint8_t chr;
    node* zero_link;
    node* one_link;
  };


  struct prefix {
    std::uint8_t chr;
    std::uint8_t plen;
    void set(int num, bool val) {
      pref &= ~(1<<num);
      if (val)
	pref |= 1<<num;
    }
    bool get(int num) const {
      return pref & (1<<num);
    }
    // Should be array of bytes in order not to deal with MSB & LSB.
    word_t pref; 
  };
  
  
class trie {
public:
  trie(): root(nullptr)
  {}
  void from_vector(const std::vector<prefix>& tbl);
  void from_frequences(const std::vector<int>& chars) {
    root = build_from_frequences(chars);
  }
  prefix_map to_map(void) {
    return to_map(root);
  }
  template<typename FUNC>
  void decode(const std::string& text,FUNC& f,int padding_bits);
  prefix find(std::bitset<word_size> bit_sequence,int blen) {
    return find_char(root,bit_sequence,0,blen);
  }
private:
  prefix_map  to_map(node* prefixes);
  prefix find_char(node* rt,std::bitset<word_size> bit_sequence,
		   int bit_pos,int blen);
  void walk(node* rt,const bits& pref,prefix_map& tbl);
  node* build_from_frequences(const std::vector<int>& chars); 
  void add_leaf(node* n,const prefix& ent, int bit_pos);
  node* root;
};
  
  
  void
  trie::add_leaf(node* nroot,const prefix& entry, int bit_pos)
  {
    if (bit_pos == entry.plen) { // it's a leaf
      nroot->chr=entry.chr;
    } else {
      if (entry.get(bit_pos)) {
	if (!nroot->one_link) 
	  nroot->one_link= new node(0,0);
	add_leaf(nroot->one_link,entry,bit_pos+1);
      } else {
	if (!nroot->zero_link)
	  nroot->zero_link = new node(0,0);
	add_leaf(nroot->zero_link,entry,bit_pos+1);
      }
    }
  }

  void
  trie::from_vector(const std::vector<prefix>& tbl)
  {
    root=new node(0,0);
    for(auto& entry: tbl) {
      add_leaf(root,entry,0);
  }
  }
  
  struct gt {
    bool operator()(const node* lhs,const  node* rhs) {
      return lhs->freq>rhs->freq;
    } 
  };

  node* 
  trie::build_from_frequences(const std::vector<int>& chars) 
  {
    std::priority_queue<node*,std::vector<node*>,gt> min_heap;
    
    for(int c=0;c<256;c++) {
      if (chars[c]) {
	min_heap.push(new node(chars[c],c));
      }
    }
    
    node* one=nullptr,*two=nullptr;
    while(!min_heap.empty()) {
      one=min_heap.top();
      min_heap.pop();
      if (!min_heap.empty()) {
	two=min_heap.top();
	min_heap.pop();
	
	node* new_node = new node(one->freq+two->freq,0);
	new_node->zero_link=one;
	new_node->one_link=two;
	
	min_heap.push(new_node);
      }
    }
    return one;
  }




  void
  trie::walk(node* root,const bits& pref, prefix_map& tbl)
  {
    if (!root) {
      throw std::logic_error("Fresh builded trie nil node walk");
    }
    if (!root->zero_link && !root->one_link) {
      // It's a leaf with char
      tbl[root->chr]=pref;
    } else {
      bits next(pref);
      
      next.push_back(0);
      walk(root->zero_link,next,tbl);
      
      next.back()=1;
      walk(root->one_link,next,tbl);
    }
  }

  prefix_map
  trie::to_map(node* prefixes)
  {
    prefix_map tbl;
    walk(prefixes,bits(),tbl);
    return tbl;
  }
  
  prefix 
  trie::find_char(node* r,std::bitset<word_size> bit_sequence,
		  int bit_pos, int blen)
  {
    
    if (!r ) { 
      throw std::logic_error("Invalid bits sequence in data: "
			     "down to nil leaf (trie corrupted)");
    }
    if (bit_pos>blen) {
      throw std::logic_error("Invalid bits sequence in data: too "
			     "much bits in a prefix");
    }
    if ((r->zero_link==nullptr) && (r->one_link==nullptr)) {
      prefix ret {r->chr,static_cast<std::uint8_t>(bit_pos),
	  static_cast<word_t>(bit_sequence.to_ullong())};
      
      while(bit_pos<word_size) {
	ret.set(bit_pos++,0);
      }
      return ret;
    } else {
      if (bit_sequence[bit_pos]) 
	return find_char(r->one_link,bit_sequence,bit_pos+1,blen);
      else
	return find_char(r->zero_link,bit_sequence,bit_pos+1,blen);
    }
  }

  class bin_stream {
  public:
    bin_stream(const std::string& st): stream(st),position(0),
				       bits_count(0),so_far(0)
    {}
    int get_bits(std::uint8_t sz,word_t& buf);
    bool is_eof(void) {
      return position>=stream.size();
    }
  private:
    const std::string& stream;
    std::string::size_type position;
    int bits_count;
    word_t so_far;
  };
  
  // Should be changed to array of bytes. 
  int
  bin_stream::get_bits(std::uint8_t sz,word_t& buf)
  {
    if (sz>word_size) sz=word_size;
    
    int readed=0;
    buf=0;
    
    while (sz>0 && (position<stream.size() || bits_count)) {
      if (bits_count>=sz) {
	if (sz==word_size) {
	  buf=so_far;
	  so_far=0;
	} else {
	  buf |= (so_far & (~(-1LL<<sz)))<<readed;
	  so_far >>=sz;
	}
	bits_count -=sz;
	readed +=sz;
	return readed;
      } else if (bits_count>0) { // sz > bits_count
	buf |=so_far<<readed;
	readed +=bits_count;
	sz -= bits_count;
	so_far=0;
	bits_count=0;
      }
      if (position<stream.size()) {
	int bytes_read=0;
	while((bytes_read<8) && (position<stream.size())) {
	  so_far |=(0xff & static_cast<word_t>(stream[position++]))
		    <<(bytes_read*(word_size/8));
	  bytes_read++;
	}
	bits_count=bytes_read*(word_size/8);
      }
    }
    return readed;
  }

  template<typename FUNC>
  void 
  trie::decode(const std::string& text,FUNC& f,int padding_bits) 
  {
    
    std::bitset<word_size> current_word(0);
    std::uint8_t bits_so_far=0;
    word_t next_bits=0;
    
    bin_stream bits_stream(text);
    for(;;) {
      int read_len;
      if (bits_so_far<word_size) {
	int read_len=bits_stream.get_bits(word_size-bits_so_far,
					  next_bits);
	if (read_len>0) {
	  next_bits <<=bits_so_far;
	  current_word |=next_bits;
	  bits_so_far+=read_len;
	} 
      }
      if (bits_so_far==0 || (bits_stream.is_eof() && 
          bits_so_far==padding_bits))
	break;
      
      prefix chr=find(current_word,bits_so_far);
      f(chr.chr);
      
      current_word >>=chr.plen;
      bits_so_far -=chr.plen;
    }
  }


  class bin_writer {
  public:
    bin_writer(std::ofstream& fl): file(fl),current_byte(8,0),
			           bits_collected(0)
    {}
    void write(const std::vector<std::uint8_t>& bits);
    void write_bit(std::uint8_t bit) {
      current_byte[bits_collected++]=bit;
      if (is_full()) {
	flash();
      }
    }
    bool is_full(void) {
      return bits_collected>=8;
    }
    int write_tail(void) {
      int have_bits=bits_collected;
      if (have_bits) {
	flash(); 
      }
      return 8-have_bits;
    }
    ~bin_writer() {
      write_tail();
    }
  private:
    void clear(void) {
      for(auto& p: current_byte)
      p=0;
      bits_collected=0;
    }
    void flash(void);
    std::uint8_t get_byte(void) {
      std::bitset<8> byte;
      for(int i=0;i<current_byte.size();++i)
	byte[i]=current_byte[i];
      return byte.to_ulong() & 0xff;
    }
    std::ofstream& file;
    bits current_byte;
    int bits_collected;
  };


  void
  bin_writer::write(const std::vector<uint8_t>& bits) 
  {
    for(int i=0;i<bits.size();++i) {
      write_bit(bits[i]);
    }
  }
  
  void
  bin_writer::flash(void)
  {
    std::uint8_t chr=get_byte();
    file.write((char*)&chr,1);
    clear();
  }
  
  void
  vector2bits(std::uint8_t chr,const bits& code, prefix& ent)
  {
    ent.chr=chr;
    ent.plen=code.size();
    ent.pref=0;
    for(int i=0;i<code.size();++i)
      ent.set(i,code[i]);
  }
  
  std::vector<prefix>
  save_table(const prefix_map& tbl) 
  {
    std::vector<prefix> result;
    for(auto& p: tbl) {
      result.emplace_back(prefix());
      vector2bits(p.first,p.second,result.back());
    }
    return result;
  }
  
  void
  show_table(const std::vector<prefix>& tbl)
  {
    for(auto& e: tbl) {
      int val=e.chr;
      int len=e.plen;
      std::cout<<std::hex<<val<<" "<<len<<" "
	       <<std::bitset<word_size>(e.pref)<<std::endl;
    }
    
  }
  
  std::vector<int>
  count_frequence(const std::string& text) 
  {
    std::vector<int> frequence_table(256,0);
    for(char c: text) {
      frequence_table[static_cast<unsigned char>(c)]++;
    }
    return frequence_table;
  }
  

  
};  
  
using namespace pack;

std::string
read_file(const std::string& file_in)
{
  std::ifstream in;
  in.exceptions ( std::ifstream::failbit | std::ifstream::badbit );
  in.open(file_in, std::ios::ate);

  auto len = in.tellg();
  in.seekg (0, in.beg);
  
  std::string txt(len,0);
  in.read(&txt[0],len);
  return txt;
}


class char_writer {
public:
  char_writer(std::ofstream& o, std::streamoff original_len): 
					out(o),olen(original_len)
  {}
  void operator()(std::uint8_t chr) {
    out.write(reinterpret_cast<char*>(&chr),1);
  }
private:
  std::ofstream& out;
  std::streamoff olen;
};


void
decompress(const std::string& file_in, const std::string& file_out, 
	   bool verbose)
{
  
  std::ifstream in;
  in.exceptions ( std::ifstream::failbit | std::ifstream::badbit );
  in.open(file_in);
  
  
  
  if (in.get()!='H' || in.get()!='M')
    throw std::logic_error("No magic number in the source file");
  
  std::streamoff original_len=0;
  in.read(reinterpret_cast<char*>(&original_len),sizeof(original_len));
  
  

  if (original_len==0)
    return;
  
  int table_sz=0;
  in.read(reinterpret_cast<char*>(&table_sz),sizeof(table_sz));
  
  

  std::uint16_t padding=0;
  in.read(reinterpret_cast<char*>(&padding),sizeof(padding));
  
  std::vector<prefix> table(table_sz/sizeof(prefix),prefix());
  in.read(reinterpret_cast<char*>(&table[0]),table_sz);

  if (verbose)
    show_table(table);
	  
  trie symbols;
  symbols.from_vector(table);
  
  auto data_pos = in.tellg();
  in.seekg (0, in.end);
  auto end_pos = in.tellg();
  in.seekg (data_pos,in.beg);


  std::string txt(end_pos-data_pos,0);
  in.read(&txt[0],end_pos-data_pos);

  
  std::ofstream of;
  of.exceptions ( std::ifstream::failbit | std::ifstream::badbit );
  of.open(file_out);
  
  char_writer f(of,original_len);

  symbols.decode(txt,f,padding);

}



void
compress(const std::string& file_in, const std::string& file_out,
	 bool verbose)
{
   
  std::string txt=read_file(file_in);
  auto freq = count_frequence(txt);
  
  trie symbols;
  
  symbols.from_frequences(freq);
  
  auto tbl = symbols.to_map();
  
  auto saved_tbl = save_table(tbl);
  int sz=saved_tbl.size()*sizeof(prefix);

  if (verbose)
    show_table(saved_tbl);
  
  std::ofstream of;
  of.exceptions ( std::ifstream::failbit | std::ifstream::badbit );
  of.open(file_out);
  
  const char magic[]={'H','M'};
  of.write(magic,sizeof(magic));

  std::streamoff len=txt.size();
  of.write(reinterpret_cast<char*>(&len),sizeof(len));

  of.write(reinterpret_cast<char*>(&sz),sizeof(sz));
  auto padding_pos = of.tellp();
  std::uint16_t padding=0;
  of.write(reinterpret_cast<char*>(&padding),sizeof(padding));
  if (sz)
    of.write(reinterpret_cast<char*>(&saved_tbl[0]),sz);
  
  bin_writer bin(of);
  for(auto c: txt) {
    auto it=tbl.find(c);
    if (it!=tbl.end()) {
      bin.write(it->second);
    }
  }
  padding=bin.write_tail();
  if (padding!=0) {
    of.seekp(padding_pos);
    of.write(reinterpret_cast<char*>(&padding),sizeof(padding));
  }
}

void
usage(const char* prog)
{
  std::cerr<<"Usage: "<<prog<<" [-d|-c] infile outfile"<<std::endl; 
}

int
main(int ac, char* av[])
{
  try {
    if (ac==4) {
      if (std::string(av[1])=="-c")
	compress(av[2],av[3],false);
      else if (std::string(av[1])=="-d")
	decompress(av[2],av[3],false);
      else {
	usage(av[0]);
	exit(1);
      }
    } else {
      usage(av[0]);
      exit(1);
    }
  } catch (std::exception& e) {
    std::cerr<<"Error: "<<e.what()<<std::endl;
    exit(1);
  } catch (...) {
    std::cerr<<"Error: Unknown exception."<<std::endl;
    exit(1);
  }
  exit(0);
} 
