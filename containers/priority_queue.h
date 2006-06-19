 #ifndef PRIORITY_QUEUE_HEADER
 #define PRIORITY_QUEUE_HEADER
 /***************************************************************************
 *            priority_queue.h
 *
 *  Thu Jul  3 15:22:50 2003
 *  Copyright  2003  Roman Dementiev
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/
#include "../common/utils.h"
#include "../mng/prefetch_pool.h"
#include "../mng/write_pool.h"
#include "../mng/mng.h"
#include "../common/tmeta.h"
#include <queue>
#include <list>
#include <iterator>

__STXXL_BEGIN_NAMESPACE

//! \addtogroup stlcontinternals
//!
//! \{

/*! \internal
*/
namespace priority_queue_local
{
  
  
/////////////////////////////////////////////////////////////////////
// auxiliary functions

// merge sz element from the two sentinel terminated input
// sequences *f0 and *f1 to "to"
// advance *fo and *f1 accordingly.
// require: at least sz nonsentinel elements available in f0, f1
// require: to may overwrite one of the sources as long as
//   *fx + sz is before the end of fx
template <class Value_,class Cmp_>
void merge(Value_ **f0,
           Value_ **f1,
           Value_  *to, int sz, Cmp_ cmp) 
{
  Value_ *from0   = *f0;
  Value_ *from1   = *f1;
  Value_ *done    = to + sz;

  while (to != done)
  {
    if(cmp(*from0,*from1))
    {
      *to = *from1;
      ++from1;
    }
    else
    {
      *to = *from0; 
      ++from0; 
    }
    ++to;
  }
  *f0   = from0;
  *f1   = from1;
}


// merge sz element from the three sentinel terminated input
// sequences *f0, *f1 and *f2 to "to"
// advance *f0, *f1 and *f2 accordingly.
// require: at least sz nonsentinel elements available in f0, f1 and f2
// require: to may overwrite one of the sources as long as
//   *fx + sz is before the end of fx
template <class Value_,class Cmp_>
void merge3(
           Value_ **f0,
           Value_ **f1,
           Value_ **f2,
           Value_  *to, int sz,Cmp_ cmp) 
{
  Value_ *from0   = *f0;
  Value_ *from1   = *f1;
  Value_ *from2   = *f2;
  Value_ *done    = to + sz;

  if (cmp(*from1,*from0)) {
    if (cmp(*from2,*from1))   { goto s012; }
    else { 
      if (cmp(*from0,*from2)) { goto s201; }
      else             { goto s021; }
    }
  } else {
    if (cmp(*from2,*from1)) {
      if (cmp(*from2,*from0)) { goto s102; }
      else             { goto s120; }
    } else             { goto s210; }
  }

#define Merge3Case(a,b,c)\
  s ## a ## b ## c :\
  if (to == done) goto finish;\
  *to = * from ## a ;\
  ++to;\
  ++from ## a ;\
  if (cmp(*from ## b , *from ## a )) goto s ## a ## b ## c;\
  if (cmp(*from ## c , *from ## a )) goto s ## b ## a ## c;\
  goto s ## b ## c ## a;

  // the order is choosen in such a way that 
  // four of the trailing gotos can be eliminated by the optimizer
  Merge3Case(0, 1, 2);
  Merge3Case(1, 2, 0);
  Merge3Case(2, 0, 1);
  Merge3Case(1, 0, 2);
  Merge3Case(0, 2, 1);
  Merge3Case(2, 1, 0);

 finish:
  *f0   = from0;
  *f1   = from1;
  *f2   = from2;
}


// merge sz element from the three sentinel terminated input
// sequences *f0, *f1, *f2 and *f3 to "to"
// advance *f0, *f1, *f2 and *f3 accordingly.
// require: at least sz nonsentinel elements available in f0, f1, f2 and f2
// require: to may overwrite one of the sources as long as
//   *fx + sz is before the end of fx
template <class Value_, class Cmp_>
void merge4(
           Value_ **f0,
           Value_ **f1,
           Value_ **f2,
           Value_ **f3,
           Value_  *to, int sz, Cmp_ cmp) 
{
  Value_ *from0   = *f0;
  Value_ *from1   = *f1;
  Value_ *from2   = *f2;
  Value_ *from3   = *f3;
  Value_ *done    = to + sz;

#define StartMerge4(a, b, c, d)\
  if ( (!cmp(*from##a ,*from##b )) && (!cmp(*from##b ,*from##c )) && (!cmp(*from##c ,*from##d )) )\
    goto s ## a ## b ## c ## d ;

  // b>a c>b d>c
  // a<b b<c c<d
  // a<=b b<=c c<=d
  // !(a>b) !(b>c) !(c>d)
  
  StartMerge4(0, 1, 2, 3);
  StartMerge4(1, 2, 3, 0);
  StartMerge4(2, 3, 0, 1);
  StartMerge4(3, 0, 1, 2);

  StartMerge4(0, 3, 1, 2);
  StartMerge4(3, 1, 2, 0);
  StartMerge4(1, 2, 0, 3);
  StartMerge4(2, 0, 3, 1);

  StartMerge4(0, 2, 3, 1);
  StartMerge4(2, 3, 1, 0);
  StartMerge4(3, 1, 0, 2);
  StartMerge4(1, 0, 2, 3);

  StartMerge4(2, 0, 1, 3);
  StartMerge4(0, 1, 3, 2);
  StartMerge4(1, 3, 2, 0);
  StartMerge4(3, 2, 0, 1);

  StartMerge4(3, 0, 2, 1);
  StartMerge4(0, 2, 1, 3);
  StartMerge4(2, 1, 3, 0);
  StartMerge4(1, 3, 0, 2);

  StartMerge4(1, 0, 3, 2);
  StartMerge4(0, 3, 2, 1);
  StartMerge4(3, 2, 1, 0);
  StartMerge4(2, 1, 0, 3);

#define Merge4Case(a, b, c, d)\
  s ## a ## b ## c ## d:\
  if (to == done) goto finish;\
  *to = *from ## a ;\
  ++to;\
  ++from ## a ;\
  if (cmp(*from ## c , *from ## a))\
  {\
    if (cmp(*from ## b, *from ## a )) \
      goto s ## a ## b ## c ## d; \
    else \
      goto s ## b ## a ## c ## d; \
  }\
  else \
  {\
    if (cmp(*from ## d, *from ## a))\
      goto s ## b ## c ## a ## d; \
    else \
      goto s ## b ## c ## d ## a; \
  }
  
  Merge4Case(0, 1, 2, 3);
  Merge4Case(1, 2, 3, 0);
  Merge4Case(2, 3, 0, 1);
  Merge4Case(3, 0, 1, 2);

  Merge4Case(0, 3, 1, 2);
  Merge4Case(3, 1, 2, 0);
  Merge4Case(1, 2, 0, 3);
  Merge4Case(2, 0, 3, 1);

  Merge4Case(0, 2, 3, 1);
  Merge4Case(2, 3, 1, 0);
  Merge4Case(3, 1, 0, 2);
  Merge4Case(1, 0, 2, 3);

  Merge4Case(2, 0, 1, 3);
  Merge4Case(0, 1, 3, 2);
  Merge4Case(1, 3, 2, 0);
  Merge4Case(3, 2, 0, 1);

  Merge4Case(3, 0, 2, 1);
  Merge4Case(0, 2, 1, 3);
  Merge4Case(2, 1, 3, 0);
  Merge4Case(1, 3, 0, 2);

  Merge4Case(1, 0, 3, 2);
  Merge4Case(0, 3, 2, 1);
  Merge4Case(3, 2, 1, 0);
  Merge4Case(2, 1, 0, 3);

 finish:
  *f0   = from0;
  *f1   = from1;
  *f2   = from2;
  *f3   = from3;
}

  
  
  
  
  template <  class BlockType_, 
              class Cmp_,
              unsigned Arity_,
              class AllocStr_ = STXXL_DEFAULT_ALLOC_STRATEGY>
  class ext_merger
  {
    public:
      typedef stxxl::int64 size_type;
      typedef BlockType_ block_type;
      typedef typename block_type::bid_type bid_type;
      typedef typename block_type::value_type value_type;
      typedef Cmp_ comparator_type;
      typedef AllocStr_ alloc_strategy;
    
      enum { arity = Arity_ };
    
    protected:
      struct sequence_type
      {
        unsigned current;
        block_type * block;
        std::list<bid_type> * bids;
        sequence_type() {}
        sequence_type(unsigned c,block_type * bl, std::list<bid_type> * bi):
          current(c),block(bl),bids(bi) {}
        value_type & operator *()
        {
          return (*block)[current];
        }
      };
    
      typedef typename std::list<sequence_type>::iterator sequences_iterator;
      
      struct sequence_element
      {
        value_type value;
        sequences_iterator sequence;
        sequence_element() {}
        sequence_element(const value_type & v, const sequences_iterator & s):
          value(v),sequence(s) {}
      };
      struct sequence_element_comparator
      {
        bool operator () (const sequence_element & a, const sequence_element & b) const
        {
          return comparator_type()(a.value,b.value);
        }
      };
      
      unsigned nsequences;
      size_type nelements;
      std::list<sequence_type> sequences;
      //typename std::list<sequence_type>::iterator last_sequence;
      std::priority_queue< 
                  sequence_element,
                  std::vector<sequence_element>,
                  sequence_element_comparator> min_elements;
      
      prefetch_pool<block_type> * p_pool;
      write_pool<block_type> * w_pool;
	private:
	  ext_merger(const ext_merger &); // forbiden
	  ext_merger & operator = (const ext_merger &);// forbiden
    public:
	  ext_merger(): nsequences(0),nelements(0) {}
      ext_merger( prefetch_pool<block_type> * p_pool_,
                  write_pool<block_type> * w_pool_):
                      nsequences(0),nelements(0),
                      p_pool(p_pool_),
                      w_pool(w_pool_)
      {
        STXXL_VERBOSE2("ext_merger::ext_merger(...)")
      }
	  void set_pools(prefetch_pool<block_type> * p_pool_,
                  			   write_pool<block_type> * w_pool_)
	 {
		 p_pool = p_pool_;
		 w_pool = w_pool_;
	 }
	  void swap(ext_merger & obj)
	  {
		  std::swap(nsequences,obj.nsequences);
          std::swap(nelements,obj.nelements);
          std::swap(sequences,obj.sequences);
      	  std::swap(min_elements,obj.min_elements);
      	  // std::swap(p_pool,obj.p_pool);
      	  // std::swap(w_pool,obj.w_pool);
	  }
      unsigned mem_cons() const // only rough estimation
      {
        return (nsequences * block_type::raw_size);
      }
      virtual ~ext_merger()
      {
        STXXL_VERBOSE2("ext_merger::~ext_merger()")
        block_manager * bm = block_manager::get_instance();
        sequences_iterator i = sequences.begin();
        for(;i!=sequences.end();++i)
        {
          bm->delete_blocks(i->bids->begin(),i->bids->end());
          delete i->block;
          delete i->bids;
        }
      }
      template <class OutputIterator>
      void multi_merge(OutputIterator begin, OutputIterator end)
      {
        STXXL_VERBOSE2("ext_merger::multi_merge(...)")
        while(begin != end)
        {
          assert(nelements);
          assert(nsequences);
          assert(!min_elements.empty());
          
          sequence_element m = min_elements.top();
          min_elements.pop();
          STXXL_VERBOSE2("ext_merger::multi_merge(...) extracting value: " << m.value)
          *begin = m.value;
          ++begin;
          --nelements;
          sequence_type & s = *(m.sequence);
          ++(s.current);
          if(s.current == s.block->size )
          {
            STXXL_VERBOSE2("ext_merger::multi_merge(...) crossing block border ")
            // go to the next block
            if(s.bids->empty()) // if there is no next block
            {
              STXXL_VERBOSE2("ext_merger::multi_merge(...) it was the last block in the sequence ")
              --nsequences;
              delete s.bids;
              delete s.block;
              sequences.erase(m.sequence);
              continue;
            }
            else
            {
              STXXL_VERBOSE2("ext_merger::multi_merge(...) there is another block ")
              bid_type bid = s.bids->front();
              s.bids->pop_front();
              if(!(s.bids->empty()))
              {
                STXXL_VERBOSE2("ext_merger::multi_merge(...) one more block exists in a sequence: "<<
                "flushing this block in write cache (if not written yet) and giving hint to prefetcher")
                bid_type next_bid = s.bids->front();/*
                request_ptr r = w_pool->get_request(next_bid);
                if(r.valid()) 
                {
                  STXXL_VERBOSE2("ext_merger::multi_merge(...) block was in write pool: "<<
                    ((r->poll())?"already written":"not yet written") )
                  r->wait();
                }
                else
                {
                  STXXL_VERBOSE2("ext_merger::multi_merge(...) block was not in write pool ")
                }
                p_pool->hint(next_bid);*/
                p_pool->hint(next_bid,*w_pool);
              }
              p_pool->read(s.block,bid)->wait();
              block_manager::get_instance()->delete_block(bid);
              s.current = 0;
            }
          }
          m.value = *s;
          min_elements.push(m);
        }
      }
      bool spaceIsAvailable() const { return nsequences < arity; }
      
      template <class Merger>
      void insert_segment(Merger & another_merger, size_type segment_size)
      {
        STXXL_VERBOSE2("ext_merger::insert_segment(merger,...)")
        assert(segment_size);
        unsigned nblocks = segment_size / block_type::size; 
        STXXL_VERBOSE2("ext_merger::insert_segment(merger,...) inserting segment with "<<nblocks<<" blocks")
        //assert(nblocks); // at least one block
		STXXL_VERBOSE1("ext_merger::insert_segment nblocks="<<nblocks)
        if(nblocks == 0)
        {
          STXXL_VERBOSE1("ext_merger::insert_segment(merger,...) WARNING: inserting a segment with "<<
            nblocks<<" blocks")
          STXXL_VERBOSE1("THIS IS INEFFICIENT: TRY TO CHANGE PRIORITY QUEUE PARAMETERS")
        }
        unsigned first_size = segment_size % block_type::size;
        if(first_size == 0)
        {
          first_size = block_type::size;
          --nblocks;
        }
        block_manager * bm = block_manager::get_instance();
        std::list<bid_type> * bids = new std::list<bid_type>(nblocks);
        bm->new_blocks(alloc_strategy(),bids->begin(),bids->end());
        block_type * first_block = new block_type;
        another_merger.multi_merge(
            first_block->begin() + (block_type::size - first_size), 
            first_block->end());
        /*
        if(w_pool->size() < nblocks)
        {
          STXXL_MSG("ext_merger::insert_segment write pool is too small: "<<
            w_pool->size()<<" blocks, resizing to "<<nblocks)
          w_pool->resize(nblocks);
        }*/
        assert(w_pool->size() > 0);
        
        typename std::list<bid_type>::iterator curbid = bids->begin();
        for(unsigned i=0;i<nblocks;++i,++curbid)
        {
          block_type *b = w_pool->steal();
          another_merger.multi_merge(b->begin(),b->end());
          w_pool->write(b,*curbid);
        }
        
        insert_segment(bids,first_block,first_size);
      }
      size_type  size() const { return nelements; }
      
 protected:
      /*! \param first_size number of elements in the first block
 		*/
      void insert_segment(std::list<bid_type> * segment, block_type * first_block, unsigned first_size)
      {
        STXXL_VERBOSE1("ext_merger::insert_segment(segment_bids,...) "<<this)
        assert(first_size);
        ++nsequences;
        nelements += size_type(segment->size())*block_type::size + first_size;
        // assert(nsequences <= arity);
        if(nsequences>arity)
        {
          STXXL_VERBOSE1("ext_merger::insert_segment(..) "
                  "INSERTING SEGMENT OVER CAPACITY, CAPACITY:"<<arity<<" SEQUENCES: "<<nsequences)
        }
        sequence_type new_sequence;
        new_sequence.current = block_type::size - first_size;
        new_sequence.block = first_block;
        new_sequence.bids = segment;
        sequences.push_front(new_sequence);
        min_elements.push(sequence_element(*new_sequence,sequences.begin()));
        //last_sequence = sequences.begin();
      }
  };
  
  
  //////////////////////////////////////////////////////////////////////
// The data structure from Knuth, "Sorting and Searching", Section 5.4.1
template <class ValTp_,class Cmp_,unsigned KNKMAX>
class looser_tree
{
public:
  typedef ValTp_ value_type;
  typedef Cmp_ comparator_type;
  typedef value_type Element;
  
private:
  struct Entry 
  {
    value_type key;   // Key of Looser element (winner for 0)
    int index;        // number of loosing segment
  };

  comparator_type cmp;
  // stack of empty segments
  int empty[KNKMAX]; // indices of empty segments
  int lastFree;  // where in "empty" is the last valid entry?

  unsigned size_; // total number of elements stored
  unsigned logK; // log of current tree size
  unsigned k; // invariant k = 1 << logK

  Element dummy; // target of empty segment pointers

  // upper levels of looser trees
  // entry[0] contains the winner info
  Entry entry[KNKMAX]; 

  // leaf information
  // note that Knuth uses indices k..k-1
  // while we use 0..k-1
  Element * current[KNKMAX]; // pointer to actual element
  Element * segment[KNKMAX]; // start of Segments
  unsigned segment_size[KNKMAX];

  unsigned mem_cons_;
  
  // private member functions
  int initWinner(int root);
  void updateOnInsert(int node, const Element & newKey, int newIndex, 
                      Element * winnerKey, int * winnerIndex, int * mask);
  void deallocateSegment(int index);
  void doubleK();
  void compactTree();
  void rebuildLooserTree();
  bool segmentIsEmpty(int i);
  void multi_merge_k(Element * to, int l);
  template <unsigned LogK>
  void multi_merge_f(Element *to, int l)
  {
    //Entry *currentPos;
    //Element currentKey;
    //int currentIndex; // leaf pointed to by current entry
    Element *done = to + l;
    Entry    *regEntry   = entry;
    Element **regCurrent = current;
    int      winnerIndex = regEntry[0].index;
    Element  winnerKey   = regEntry[0].key;
    Element *winnerPos;
    //Element sup = dummy; // supremum
    
    assert(logK >= LogK);
    while (to != done)
    {
      winnerPos = regCurrent[winnerIndex];
      
      // write result
      *to   = winnerKey;
      
      // advance winner segment
      ++winnerPos;
      regCurrent[winnerIndex] = winnerPos;
      winnerKey = *winnerPos;
      
      // remove winner segment if empty now
      if (is_sentinel(winnerKey))
      { 
        deallocateSegment(winnerIndex); 
      } 
      ++to;
      
      // update looser tree
#define TreeStep(L)\
      if (1 << LogK >= 1 << L) {\
        Entry  *pos##L = regEntry+((winnerIndex+(1<<LogK)) >> (((int(LogK-L)+1)>=0)?((LogK-L)+1):0));\
        Element key##L = pos##L->key;\
        if (cmp(winnerKey,key##L)) {\
          int index##L  = pos##L->index;\
          pos##L->key   = winnerKey;\
          pos##L->index = winnerIndex;\
          winnerKey     = key##L;\
          winnerIndex   = index##L;\
        }\
      }
      TreeStep(10);
      TreeStep(9);
      TreeStep(8);
      TreeStep(7);
      TreeStep(6);
      TreeStep(5);
      TreeStep(4);
      TreeStep(3);
      TreeStep(2);
      TreeStep(1);
#undef TreeStep      
    }
    regEntry[0].index = winnerIndex;
    regEntry[0].key   = winnerKey;  
  }
 
public:
  bool is_sentinel(const Element & a)
  {
    return !(cmp(cmp.min_value(),a));
  }
  bool not_sentinel(const Element & a)
  {
    return cmp(cmp.min_value(),a);
  }
private:
	looser_tree & operator = (const looser_tree &); // forbidden
	looser_tree(const looser_tree &); // forbidden
public:
  looser_tree();
  ~looser_tree();
  void init(); 

  void swap(looser_tree & obj)
  {
	    std::swap(cmp,obj.cmp);
  		swap_1D_arrays(empty,obj.empty,KNKMAX);
  		std::swap(lastFree,obj.lastFree);
		std::swap(size_,obj.size_);
  		std::swap(logK,obj.logK);
  		std::swap(k,obj.k);
 		std::swap(dummy,obj.dummy);
		swap_1D_arrays(entry,obj.entry,KNKMAX);
		swap_1D_arrays(current,obj.current,KNKMAX);
		swap_1D_arrays(segment,obj.segment,KNKMAX);
  		swap_1D_arrays(segment_size,obj.segment_size,KNKMAX);
  		std::swap(mem_cons_,obj.mem_cons_);
  }
	
  void multi_merge(Element * begin, Element * end)
  {
    multi_merge(begin,end-begin);
  }
  void multi_merge(Element *, unsigned l);
  
  unsigned mem_cons() const { return mem_cons_; }
  bool  spaceIsAvailable() // for new segment
  { return k < KNKMAX || lastFree >= 0; } 
     
  void insert_segment(Element *to, unsigned sz); // insert segment beginning at to
  unsigned  size() { return size_; }
};  

///////////////////////// LooserTree ///////////////////////////////////
template <class ValTp_,class Cmp_,unsigned KNKMAX>
looser_tree<ValTp_,Cmp_,KNKMAX>::looser_tree() : lastFree(0), size_(0), logK(0), k(1),mem_cons_(0)
{
  empty  [0] = 0;
  segment[0] = 0;
  current[0] = &dummy;
  // entry and dummy are initialized by init
  // since they need the value of supremum
  init();
}

template <class ValTp_,class Cmp_,unsigned KNKMAX>
void looser_tree<ValTp_,Cmp_,KNKMAX>::init()
{
  dummy      = cmp.min_value();
  rebuildLooserTree();
  assert(current[entry[0].index] == &dummy);
}


// rebuild looser tree information from the values in current
template <class ValTp_,class Cmp_,unsigned KNKMAX>
void looser_tree<ValTp_,Cmp_,KNKMAX>::rebuildLooserTree()
{  
  int winner = initWinner(1);
  entry[0].index = winner;
  entry[0].key   = *(current[winner]);
}


// given any values in the leaves this
// routing recomputes upper levels of the tree
// from scratch in linear time
// initialize entry[root].index and the subtree rooted there
// return winner index
template <class ValTp_,class Cmp_,unsigned KNKMAX>
int looser_tree<ValTp_,Cmp_,KNKMAX>::initWinner(int root)
{
  if (root >= int(k)) { // leaf reached
    return root - k;
  } else {
    int left  = initWinner(2*root    );
    int right = initWinner(2*root + 1);
    Element lk    = *(current[left ]);
    Element rk    = *(current[right]);
    if (!(cmp(lk,rk))) { // right subtree looses
      entry[root].index = right;
      entry[root].key   = rk;
      return left;
    } else {
      entry[root].index = left;
      entry[root].key   = lk;
      return right;
    }
  }
}


// first go up the tree all the way to the root
// hand down old winner for the respective subtree
// based on new value, and old winner and looser 
// update each node on the path to the root top down.
// This is implemented recursively
template <class ValTp_,class Cmp_,unsigned KNKMAX>
void looser_tree<ValTp_,Cmp_,KNKMAX>::updateOnInsert(
               int node, 
               const Element  & newKey, 
               int      newIndex, 
               Element *winnerKey, 
               int *winnerIndex, // old winner
               int *mask) // 1 << (ceil(log KNK) - dist-from-root)
{
  if (node == 0) { // winner part of root
    *mask = 1 << (logK - 1);    
    *winnerKey   = entry[0].key;
    *winnerIndex = entry[0].index;
    if (cmp(entry[node].key,newKey)) 
    {
      entry[node].key   = newKey;
      entry[node].index = newIndex;
    }
  } else {
    updateOnInsert(node >> 1, newKey, newIndex, winnerKey, winnerIndex, mask);
    Element looserKey   = entry[node].key;
    int looserIndex = entry[node].index;
    if ((*winnerIndex & *mask) != (newIndex & *mask)) { // different subtrees
      if (cmp(looserKey,newKey)) { // newKey will have influence here
        if (cmp(*winnerKey,newKey) ) { // old winner loses here
          entry[node].key   = *winnerKey;
          entry[node].index = *winnerIndex;
        } else { // new entry looses here
          entry[node].key   = newKey;
          entry[node].index = newIndex;
        }
      } 
      *winnerKey   = looserKey;
      *winnerIndex = looserIndex;
    }
    // note that nothing needs to be done if
    // the winner came from the same subtree
    // a) newKey <= winnerKey => even more reason for the other tree to loose
    // b) newKey >  winnerKey => the old winner will beat the new
    //                           entry further down the tree
    // also the same old winner is handed down the tree

    *mask >>= 1; // next level
  }
}


// make the tree two times as wide
// may only be called if no free slots are left ?? necessary ??
template <class ValTp_,class Cmp_,unsigned KNKMAX>
void looser_tree<ValTp_,Cmp_,KNKMAX>::doubleK()
{
  // make all new entries empty
  // and push them on the free stack
  assert(lastFree == -1); // stack was empty (probably not needed)
  assert(k < KNKMAX);
  for (int i = 2*k - 1;  i >= int(k);  i--)
  {
    current[i] = &dummy;
    segment[i] = NULL;
    lastFree++;
    empty[lastFree] = i;
  }

  // double the size
  k *= 2;  logK++;

  // recompute looser tree information
  rebuildLooserTree();
}


// compact nonempty segments in the left half of the tree
template <class ValTp_,class Cmp_,unsigned KNKMAX>
void looser_tree<ValTp_,Cmp_,KNKMAX>::compactTree()
{
  assert(logK > 0);

  // compact all nonempty segments to the left
  int from = 0;
  int to   = 0;
  for(;  from < int(k);  from++)
  {
    if (not_sentinel(*(current[from])))
    {
      segment_size[to] = segment_size[from];
      current[to] = current[from];
      segment[to] = segment[from];
      to++;
    }/*
    else
    {
      if(segment[from])
      {
        STXXL_VERBOSE2("looser_tree::compactTree() deleting segment "<<from<<
					" address: "<<segment[from]<<" size: "<<segment_size[from])
        delete [] segment[from];
        segment[from] = 0;
        mem_cons_ -= segment_size[from];
      }
    }*/
  }

  // half degree as often as possible
  while (to < int(k/2)) {
    k /= 2;  logK--;
  }

  // overwrite garbage and compact the stack of empty segments
  lastFree = -1; // none free
  for (;  to < int(k);  to++) {
    // push 
    lastFree++;
    empty[lastFree] = to;

    current[to] = &dummy;
  }

  // recompute looser tree information
  rebuildLooserTree();
}


// insert segment beginning at to
// require: spaceIsAvailable() == 1 
template <class ValTp_,class Cmp_,unsigned KNKMAX>
void looser_tree<ValTp_,Cmp_,KNKMAX>::insert_segment(Element *to, unsigned sz)
{
  STXXL_VERBOSE2("looser_tree::insert_segment("<< to <<","<< sz<<")")
  //std::copy(to,to + sz,std::ostream_iterator<ValTp_>(std::cout, "\n"));
  
  if (sz > 0)
  {
    assert( not_sentinel(to[0])   );
    assert( not_sentinel(to[sz-1]));
    // get a free slot
    if (lastFree < 0) { // tree is too small
      doubleK();
    }
    int index = empty[lastFree];
    lastFree--; // pop


    // link new segment
    current[index] = segment[index] = to;
    segment_size[index] = (sz + 1)*sizeof(value_type);
    mem_cons_ += (sz + 1)*sizeof(value_type);
    size_ += sz;
     
    // propagate new information up the tree
    Element dummyKey;
    int dummyIndex;
    int dummyMask;
    updateOnInsert((index + k) >> 1, *to, index, 
                   &dummyKey, &dummyIndex, &dummyMask);
  } else {
    // immediately deallocate
    // this is not only an optimization 
    // but also needed to keep empty segments from
    // clogging up the tree
    delete [] to; 
  }
}


template <class ValTp_,class Cmp_,unsigned KNKMAX>
looser_tree<ValTp_,Cmp_,KNKMAX>::~looser_tree()
{
  STXXL_VERBOSE2("looser_tree::~looser_tree()")
  for(unsigned i=0;i<k;++i)
  {
    if(segment[i])
    {
      STXXL_VERBOSE2("looser_tree::~looser_tree() deleting segment "<<i)
      delete [] segment[i];
      mem_cons_ -= segment_size[i];
    }
  }
  // check whether we did not loose memory
  assert(mem_cons_ == 0);
}

// free an empty segment .
template <class ValTp_,class Cmp_,unsigned KNKMAX>
void looser_tree<ValTp_,Cmp_,KNKMAX>::deallocateSegment(int index)
{
  // reroute current pointer to some empty dummy segment
  // with a sentinel key
	STXXL_VERBOSE2("looser_tree::deallocateSegment() deleting segment "<<
		index<<" address: "<<segment[index]<<" size: "<<segment_size[index])
  current[index] = &dummy;

  // free memory
  delete [] segment[index];
  segment[index] = 0;
  mem_cons_ -= segment_size[index];
  
  // push on the stack of free segment indices
  lastFree++;
  empty[lastFree] = index;
}


// delete the l smallest elements and write them to "to"
// empty segments are deallocated
// require:
// - there are at least l elements
// - segments are ended by sentinels
template <class ValTp_,class Cmp_,unsigned KNKMAX>
void looser_tree<ValTp_,Cmp_,KNKMAX>::multi_merge(Element *to, unsigned l)
{
  STXXL_VERBOSE3("looser_tree::multi_merge("<< to <<","<< l<<")")
  
  /*
  multi_merge_k(to,l);
  */
  switch(logK) {
  case 0: 
    assert(k == 1);
    assert(entry[0].index == 0);
    assert(lastFree == -1 || l == 0);
    //memcpy(to, current[0], l * sizeof(Element));
    std::copy(current[0],current[0]+l,to);
    current[0] += l;
    entry[0].key = **current;
    if (segmentIsEmpty(0)) deallocateSegment(0); 
    break;
  case 1:
    assert(k == 2);
    merge(current + 0, current + 1, to, l,cmp);
    rebuildLooserTree();
    if (segmentIsEmpty(0)) deallocateSegment(0); 
    if (segmentIsEmpty(1)) deallocateSegment(1); 
    break;
  case 2:
    assert(k == 4);
    merge4(current + 0, current + 1, current + 2, current + 3, to, l,cmp);
    rebuildLooserTree();
    if (segmentIsEmpty(0)) deallocateSegment(0); 
    if (segmentIsEmpty(1)) deallocateSegment(1); 
    if (segmentIsEmpty(2)) deallocateSegment(2); 
    if (segmentIsEmpty(3)) deallocateSegment(3);
    break;
  case  3: multi_merge_f<3>(to, l); break;
  case  4: multi_merge_f<4>(to, l); break;
  case  5: multi_merge_f<5>(to, l); break;
  case  6: multi_merge_f<6>(to, l); break;
  case  7: multi_merge_f<7>(to, l); break;
  case  8: multi_merge_f<8>(to, l); break;
  case  9: multi_merge_f<9>(to, l); break;
  case 10: multi_merge_f<10>(to, l); break; 
  default: multi_merge_k(to, l); break;
  }
  
  
  
  size_ -= l;

  // compact tree if it got considerably smaller
  if (k > 1 && int(lastFree) >= int(3*k/5 - 1) ) { 
    // using k/2 would be worst case inefficient
    compactTree(); 
  }
  //std::copy(to,to + l,std::ostream_iterator<ValTp_>(std::cout, "\n"));
}


// is this segment empty and does not point to dummy yet?
template <class ValTp_,class Cmp_,unsigned KNKMAX>
inline bool looser_tree<ValTp_,Cmp_,KNKMAX>::segmentIsEmpty(int i)
{
  return (is_sentinel(*(current[i])) &&  (current[i] != &dummy));
}

// multi-merge for fixed K
/*
template <class ValTp_,class Cmp_,unsigned KNKMAX> template <unsigned LogK>
void looser_tree<ValTp_,Cmp_,KNKMAX>::multi_merge_f<LogK>(Element *to, int l)
{
}
*/

// multi-merge for arbitrary K
template <class ValTp_,class Cmp_,unsigned KNKMAX>
void looser_tree<ValTp_,Cmp_,KNKMAX>::
multi_merge_k(Element *to, int l)
{
  Entry *currentPos;
  Element currentKey;
  int currentIndex; // leaf pointed to by current entry
  int kReg = k;
  Element *done = to + l;
  int      winnerIndex = entry[0].index;
  Element  winnerKey   = entry[0].key;
  Element *winnerPos;
  
  while (to != done)
  {
    winnerPos = current[winnerIndex];
    
    // write result
    *to   = winnerKey;

    // advance winner segment
    ++winnerPos;
    current[winnerIndex] = winnerPos;
    winnerKey = *winnerPos;

    // remove winner segment if empty now
    if (is_sentinel(winnerKey)) // 
      deallocateSegment(winnerIndex); 
    
    // go up the entry-tree
    for (int i = (winnerIndex + kReg) >> 1;  i > 0;  i >>= 1) {
      currentPos = entry + i;
      currentKey = currentPos->key;
      if (cmp(winnerKey,currentKey)) {
        currentIndex      = currentPos->index;
        currentPos->key   = winnerKey;
        currentPos->index = winnerIndex;
        winnerKey         = currentKey;
        winnerIndex       = currentIndex;
      }
    }

    ++to;
  }
  entry[0].index = winnerIndex;
  entry[0].key   = winnerKey;  
}

};

/*

KNBufferSize1 = 32; 
KNN = 512; // bandwidth
KNKMAX = 64;  // maximal arity
KNLevels = 4; // overall capacity >= KNN*KNKMAX^KNLevels
LogKNKMAX = 6;  // ceil(log KNK)
*/

// internal memory consumption >= N_*(KMAX_^Levels_) + ext

template <
      class Tp_,
      class Cmp_,
      unsigned BufferSize1_ = 32, // equalize procedure call overheads etc. 
      unsigned N_ = 512, // bandwidth
      unsigned IntKMAX_ = 64, // maximal arity for internal mergers
      unsigned IntLevels_ = 4, 
      unsigned BlockSize_ = (2*1024*1024),
      unsigned ExtKMAX_ = 64, // maximal arity for external mergers
      unsigned ExtLevels_ = 2,
      class AllocStr_ = STXXL_DEFAULT_ALLOC_STRATEGY
      >
struct priority_queue_config
{
  typedef Tp_ value_type;
  typedef Cmp_ comparator_type;
  typedef AllocStr_ alloc_strategy_type;
  enum
  {
    BufferSize1 = BufferSize1_,
    N = N_,
    IntKMAX = IntKMAX_,
    IntLevels = IntLevels_,
    ExtLevels = ExtLevels_,
    BlockSize = BlockSize_,
    ExtKMAX = ExtKMAX_,
  };
};

__STXXL_END_NAMESPACE

namespace std
{
	template <  class BlockType_, 
          		class Cmp_,
              	unsigned Arity_,
              	class AllocStr_ >
	void swap(stxxl::priority_queue_local::ext_merger<BlockType_,Cmp_,Arity_,AllocStr_> & a,
			  stxxl::priority_queue_local::ext_merger<BlockType_,Cmp_,Arity_,AllocStr_> & b )
	{
		a.swap(b);
	}
	template <class ValTp_,class Cmp_,unsigned KNKMAX>
	void swap(	stxxl::priority_queue_local::looser_tree<ValTp_,Cmp_,KNKMAX> & a,
				stxxl::priority_queue_local::looser_tree<ValTp_,Cmp_,KNKMAX> & b)
	{
		a.swap(b);
	}		
}

__STXXL_BEGIN_NAMESPACE

//! \brief External priority queue data structure
template <class Config_>
class priority_queue
{
public:
  typedef Config_ Config;
  enum
  {
    BufferSize1 = Config::BufferSize1,
    N = Config::N,
    IntKMAX = Config::IntKMAX,
    IntLevels = Config::IntLevels,
    ExtLevels = Config::ExtLevels,
    Levels = Config::IntLevels + Config::ExtLevels,
    BlockSize = Config::BlockSize,
    ExtKMAX = Config::ExtKMAX
  };
  
  //! \brief The type of object stored in the \b priority_queue
  typedef typename Config::value_type value_type;
  //! \brief Comparison object
  typedef typename Config::comparator_type comparator_type;
  typedef typename Config::alloc_strategy_type alloc_strategy_type; 
  //! \brief An unsigned integral type (64 bit)
  typedef stxxl::int64 size_type;
  typedef typed_block<BlockSize,value_type> block_type;
  
  
protected:
  
  typedef std::priority_queue<value_type,std::vector<value_type>,comparator_type> 
                      insert_heap_type;
  
  typedef priority_queue_local::looser_tree<
            value_type,
            comparator_type,
            IntKMAX>  int_merger_type;
  
  typedef priority_queue_local::ext_merger<
            block_type,
            comparator_type,
            ExtKMAX,
            alloc_strategy_type>   ext_merger_type;

  
  int_merger_type itree[IntLevels];
  prefetch_pool<block_type> & p_pool;
  write_pool<block_type>    & w_pool;
  ext_merger_type * etree;

  // one delete buffer for each tree (extra space for sentinel)
  value_type   buffer2[Levels][N + 1]; // tree->buffer2->buffer1
  value_type * minBuffer2[Levels];

  // overall delete buffer
  value_type   buffer1[BufferSize1 + 1];
  value_type * minBuffer1;

  comparator_type cmp;
  
  // insert buffer
  insert_heap_type insertHeap;

  // how many levels are active
  int activeLevels;
  
  // total size not counting insertBuffer and buffer1
  size_type size_;
  bool  deallocate_pools;

  // private member functions
  void refillBuffer1();
  int refillBuffer2(int k);
  
  int makeSpaceAvailable(int level);
  void emptyInsertHeap();
  
  value_type getSupremum() const { return cmp.min_value(); } //{ return buffer2[0][KNN].key; }
  int getSize1( ) const { return ( buffer1 + BufferSize1) - minBuffer1; }
  int getSize2(int i) const { return &(buffer2[i][N])     - minBuffer2[i]; }
  
    
  // forbidden cals
  priority_queue();
  priority_queue & operator = (const priority_queue &);  
  priority_queue(const priority_queue & );
public:
    
  //! \brief Constructs external priority queue object
  //! \param p_pool_ pool of blocks that will be used 
  //! for data prefetching for the disk<->memory transfers 
  //! happenning in the priority queue. Larger pool size 
  //! helps to speed up operations.
  //! \param w_pool_ pool of blocks that will be used 
  //! for writing data for the memory<->disk transfers 
  //! happenning in the priority queue. Larger pool size 
  //! helps to speed up operations.
  priority_queue(prefetch_pool<block_type> & p_pool_, write_pool<block_type> & w_pool_);
    
  //! \brief Constructs external priority queue object
  //! \param p_pool_mem memory (in bytes) for prefetch pool that will be used 
  //! for data prefetching for the disk<->memory transfers 
  //! happenning in the priority queue. Larger pool size 
  //! helps to speed up operations.
  //! \param w_pool_mem memory (in bytes) for buffered write pool that will be used 
  //! for writing data for the memory<->disk transfers 
  //! happenning in the priority queue. Larger pool size 
  //! helps to speed up operations.
  priority_queue(unsigned p_pool_mem, unsigned w_pool_mem);

  void swap(priority_queue & obj)
  {
	  //swap_1D_arrays(itree,obj.itree,IntLevels); // does not work in g++ 3.4.3 :( bug?
	  for(unsigned i=0;i<IntLevels;++i) std::swap(itree[i],obj.itree[i]);
      // std::swap(p_pool,obj.p_pool);
      // std::swap(w_pool,obj.w_pool);
      std::swap(etree,obj.etree);
	  for(unsigned i1=0;i1<Levels;++i1)
		for(unsigned i2=0;i2< (N + 1);++i2)
			std::swap(buffer2[i1][i2],obj.buffer2[i1][i2]);
	  swap_1D_arrays(minBuffer2,obj.minBuffer2,Levels);
      swap_1D_arrays(buffer1,obj.buffer1,BufferSize1 + 1); 
      std::swap(minBuffer1,obj.minBuffer1);
	  std::swap(cmp,obj.cmp);
  	  std::swap(insertHeap,obj.insertHeap);
	  std::swap(activeLevels,obj.activeLevels);
	  std::swap(size_,obj.size_);
	  //std::swap(deallocate_pools,obj.deallocate_pools);
  }
    
  virtual ~priority_queue();
  
  //! \brief Returns number of elements contained
  //! \return number of elements contained
  size_type size() const;
  
  //! \brief Returns true if queue has no elements
  //! \return \b true if queue has no elements, \b false otherwise
  bool empty() const { return (size()==0); }
  
  //! \brief Returns "largest" element
  //!
  //! Returns a const reference to the element at the 
  //! top of the priority_queue. The element at the top is 
  //! guaranteed to be the largest element in the \b priority queue, 
  //! as determined by the comparison function \b Config_::comparator_type 
  //! (the same as the second parameter of PRIORITY_QUEUE_GENERATOR utility 
  //! class). That is, 
  //! for every other element \b x in the priority_queue, 
  //! \b Config_::comparator_type(Q.top(), x) is false. 
  //! Precondition: \c empty() is false.
  const value_type & top() const;
  
  //! \brief Removes the element at the top
  //!
  //! Removes the element at the top of the priority_queue, that 
  //! is, the largest element in the \b priority_queue. 
  //! Precondition: \c empty() is \b false. 
  //! Postcondition: \c size() will be decremented by 1.
  void  pop();
  
  //! \brief Inserts x into the priority_queue.
  //!
  //! Inserts x into the priority_queue. Postcondition: 
  //! \c size() will be incremented by 1.
  void  push(const value_type & obj);
  
  //! \brief Returns number of bytes consumed by 
  //! the \b priority_queue
  //! \brief number of bytes consumed by the \b priority_queue from 
  //! the internal memory not including pools (see the constructor)
  unsigned mem_cons() const 
  {
    unsigned dynam_alloc_mem(0),i(0);
    //dynam_alloc_mem += w_pool.mem_cons();
    //dynam_alloc_mem += p_pool.mem_cons();
    for(;i<IntLevels;++i)
      dynam_alloc_mem += itree[i].mem_cons();
    for(i=0;i<ExtLevels;++i)
      dynam_alloc_mem += etree[i].mem_cons();
    
    return (  sizeof(*this) + 
              sizeof(ext_merger_type)*ExtLevels + 
              dynam_alloc_mem );
  }
};


template <class Config_>  
inline typename priority_queue<Config_>::size_type priority_queue<Config_>::size() const 
{ 
  return 
    size_ + 
    insertHeap.size() - 1 + 
    ((buffer1 + BufferSize1) - minBuffer1); 
}


template <class Config_>
inline const typename priority_queue<Config_>::value_type & priority_queue<Config_>::top() const
{
  assert(!insertHeap.empty());
  
  if( /*(!insertHeap.empty()) && */ cmp(*minBuffer1,insertHeap.top()))
    return insertHeap.top();
  
  return *minBuffer1;
}

template <class Config_>
inline void priority_queue<Config_>::pop()
{
  //STXXL_VERBOSE1("priority_queue::pop()")
  assert(!insertHeap.empty());
  
  if(/*(!insertHeap.empty()) && */ cmp(*minBuffer1,insertHeap.top()))
  {
    insertHeap.pop();
  }
  else
  {
    assert(minBuffer1 < buffer1 + BufferSize1);
    ++minBuffer1;
    if (minBuffer1 == buffer1 + BufferSize1)
      refillBuffer1();
  }
}

template <class Config_>
inline void priority_queue<Config_>::push(const value_type & obj)
{
  //STXXL_VERBOSE3("priority_queue::push("<< obj <<")")
  assert(itree->not_sentinel(obj));
  if(insertHeap.size() == N + 1) 
     emptyInsertHeap();
  
  assert(!insertHeap.empty());
  
  insertHeap.push(obj);
}



////////////////////////////////////////////////////////////////

template <class Config_>
priority_queue<Config_>::priority_queue(prefetch_pool<block_type> & p_pool_, write_pool<block_type> & w_pool_) : 
  p_pool(p_pool_),w_pool(w_pool_),
  activeLevels(0), size_(0),
  deallocate_pools(false)
{
  STXXL_VERBOSE2("priority_queue::priority_queue()")
  //etree = new ext_merger_type[ExtLevels](p_pool,w_pool);
  etree = new ext_merger_type[ExtLevels];
  for(int j=0;j<ExtLevels;++j)
	  etree[j].set_pools(&p_pool,&w_pool);
  value_type sentinel = cmp.min_value();
  buffer1[BufferSize1] = sentinel; // sentinel
  insertHeap.push(sentinel); // always keep the sentinel
  minBuffer1 = buffer1 + BufferSize1; // empty
  for (int i = 0;  i < Levels;  i++)
  { 
    buffer2[i][N] = sentinel; // sentinel
    minBuffer2[i] = &(buffer2[i][N]); // empty
  }
  
}

template <class Config_>
priority_queue<Config_>::priority_queue(unsigned p_pool_mem, unsigned w_pool_mem) : 
  p_pool(*(new prefetch_pool<block_type>(p_pool_mem/BlockSize))),
  w_pool(*(new write_pool<block_type>(w_pool_mem/BlockSize))),
  activeLevels(0), size_(0),
  deallocate_pools(true)
{
  STXXL_VERBOSE2("priority_queue::priority_queue()")
  etree = new ext_merger_type[ExtLevels];
	for(int j=0;j<ExtLevels;++j)
	  etree[j].set_pools(&p_pool,&w_pool);
  value_type sentinel = cmp.min_value();
  buffer1[BufferSize1] = sentinel; // sentinel
  insertHeap.push(sentinel); // always keep the sentinel
  minBuffer1 = buffer1 + BufferSize1; // empty
  for (int i = 0;  i < Levels;  i++)
  { 
    buffer2[i][N] = sentinel; // sentinel
    minBuffer2[i] = &(buffer2[i][N]); // empty
  }
  
}

template <class Config_>
priority_queue<Config_>::~priority_queue()
{
  STXXL_VERBOSE2("priority_queue::~priority_queue()")
  if(deallocate_pools)
  {
	  delete &p_pool;
	  delete &w_pool;
  }
  delete [] etree;
}

//--------------------- Buffer refilling -------------------------------

// refill buffer2[j] and return number of elements found
template <class Config_>
int priority_queue<Config_>::refillBuffer2(int j)
{
  STXXL_VERBOSE2("priority_queue::refillBuffer2("<<j<<")")
  
  value_type * oldTarget;
  int deleteSize;
  size_type treeSize = (j<IntLevels) ? itree[j].size() : etree[ j - IntLevels].size();
  int bufferSize = (&(buffer2[j][0]) + N) - minBuffer2[j];
  if (treeSize + bufferSize >= size_type(N) ) 
  { // buffer will be filled
    oldTarget = &(buffer2[j][0]);
    deleteSize = N - bufferSize;
  }
  else
  {
    oldTarget = &(buffer2[j][0]) + N - int(treeSize) - bufferSize;
    deleteSize = treeSize;
  }

  // shift  rest to beginning
  // possible hack:
  // - use memcpy if no overlap
  memmove(oldTarget, minBuffer2[j], bufferSize * sizeof(value_type));
  minBuffer2[j] = oldTarget;

  // fill remaining space from tree
  if(j<IntLevels)
    itree[j].multi_merge(oldTarget + bufferSize, deleteSize);
  else
    etree[j-IntLevels].multi_merge(oldTarget + bufferSize, 
            oldTarget + bufferSize + deleteSize);
  
  //STXXL_MSG(deleteSize + bufferSize)
  //std::copy(oldTarget,oldTarget + deleteSize + bufferSize,std::ostream_iterator<value_type>(std::cout, "\n"));
  
  return deleteSize + bufferSize;
}
 
 
// move elements from the 2nd level buffers 
// to the buffer
template <class Config_>
void priority_queue<Config_>::refillBuffer1() 
{
  STXXL_VERBOSE2("priority_queue::refillBuffer1()")
  
  size_type totalSize = 0;
  int sz;
  for (int i = activeLevels - 1;  i >= 0;  i--)
  {
    if((&(buffer2[i][0]) + N) - minBuffer2[i] < BufferSize1)
    {
      sz = refillBuffer2(i);
      // max active level dry now?
      if (sz == 0 && i == activeLevels - 1)
        --activeLevels;
      else 
        totalSize += sz;
      
    }
    else
    {
      totalSize += BufferSize1; // actually only a sufficient lower bound
    }
  }
  
  if(totalSize >= BufferSize1) // buffer can be filled
  { 
    minBuffer1 = buffer1;
    sz = BufferSize1; // amount to be copied
    size_ -= size_type(BufferSize1); // amount left in buffer2
  }
  else
  {
    minBuffer1 = buffer1 + BufferSize1 - totalSize;
    sz = totalSize;
    assert(size_ == sz); // trees and buffer2 get empty
    size_ = 0;
  }

  // now call simplified refill routines
  // which can make the assumption that
  // they find all they are asked to find in the buffers
  minBuffer1 = buffer1 + BufferSize1 - sz;
  STXXL_VERBOSE2("Active levels = "<<activeLevels)
  switch(activeLevels)
  {
  case 0: break;
  case 1: 
          std::copy(minBuffer2[0],minBuffer2[0] + sz,minBuffer1);
          minBuffer2[0] += sz;
          break;
  case 2: priority_queue_local::merge(
                &(minBuffer2[0]), 
                &(minBuffer2[1]), minBuffer1, sz,cmp);
          break;
  case 3: priority_queue_local::merge3(
                 &(minBuffer2[0]), 
                 &(minBuffer2[1]),
                 &(minBuffer2[2]), minBuffer1, sz,cmp);
          break;
  case 4: 
    STXXL_VERBOSE2("=1="<<minBuffer2[0][0]) //std::copy(minBuffer2[0],(&(buffer2[0][0])) + N,std::ostream_iterator<value_type>(std::cout, ","));
    STXXL_VERBOSE2("=2="<<minBuffer2[1][0]) //std::copy(minBuffer2[1],(&(buffer2[1][0])) + N,std::ostream_iterator<value_type>(std::cout, ","));
    STXXL_VERBOSE2("=3="<<minBuffer2[2][0]) //std::copy(minBuffer2[2],(&(buffer2[2][0])) + N,std::ostream_iterator<value_type>(std::cout, ","));
    STXXL_VERBOSE2("=4="<<minBuffer2[3][0]) //std::copy(minBuffer2[3],(&(buffer2[3][0])) + N,std::ostream_iterator<value_type>(std::cout, ","));
          priority_queue_local::merge4(
                 &(minBuffer2[0]), 
                 &(minBuffer2[1]),
                 &(minBuffer2[2]),
                 &(minBuffer2[3]), minBuffer1, sz,cmp);
          break;
  default:
        STXXL_ERRMSG("Number of buffers on 2nd level in stxxl::priority_queue is currently limited to 4")
        abort();
  }
  
  //std::copy(minBuffer1,minBuffer1 + sz,std::ostream_iterator<value_type>(std::cout, "\n"));
}

//--------------------------------------------------------------------

// check if space is available on level k and
// empty this level if necessary leading to a recursive call.
// return the level where space was finally available
template <class Config_>
int priority_queue<Config_>::makeSpaceAvailable(int level)
{
  STXXL_VERBOSE2("priority_queue::makeSpaceAvailable("<<level<<")")
  int finalLevel;
  assert(level < Levels);
  assert(level <= activeLevels);
  
  if (level == activeLevels) 
    activeLevels++; 
  
  const bool spaceIsAvailable_ = 
    (level < IntLevels) ? itree[level].spaceIsAvailable() 
                        : ((level == Levels - 1)?true:(etree[level - IntLevels].spaceIsAvailable())) ;
  
  if(spaceIsAvailable_)
  { 
    finalLevel = level;
  }
  else
  {
    finalLevel = makeSpaceAvailable(level + 1);
    
    if(level < IntLevels - 1) // from internal to internal tree
    {
      int segmentSize = itree[level].size();
      value_type * newSegment = new value_type[segmentSize + 1];
      itree[level].multi_merge(newSegment, segmentSize); // empty this level
      
      newSegment[segmentSize] = buffer1[BufferSize1]; // sentinel
      // for queues where size << #inserts
      // it might make sense to stay in this level if
      // segmentSize < alpha * KNN * k^level for some alpha < 1
      itree[level + 1].insert_segment(newSegment, segmentSize);
    }
    else
    { 
      if(level == IntLevels - 1) // from internal to external tree
      {
        const int segmentSize = itree[IntLevels - 1].size();
        etree[0].insert_segment(itree[IntLevels - 1],segmentSize);
      }
      else // from external to external tree
      {
        const size_type segmentSize = etree[level - IntLevels].size();
        etree[level - IntLevels + 1].insert_segment(etree[level - IntLevels],segmentSize);
      }
    }
  }
  return finalLevel;
}


// empty the insert heap into the main data structure
template <class Config_>
void priority_queue<Config_>::emptyInsertHeap()
{
  STXXL_VERBOSE2("priority_queue::emptyInsertHeap()")
  const value_type sup = getSupremum();

  // build new segment
  value_type *newSegment = new value_type[N + 1];
  value_type *newPos = newSegment;

  // put the new data there for now
  //insertHeap.sortTo(newSegment);
  value_type * SortTo = newSegment;
  const value_type * SortEnd = newSegment + N;
  while(SortTo != SortEnd)
  {
    assert(!insertHeap.empty());
    *SortTo = insertHeap.top();
    insertHeap.pop();
    ++SortTo;
  }
  
  assert(insertHeap.size() == 1);
  
  newSegment[N] = sup; // sentinel

  // copy the buffer1 and buffer2[0] to temporary storage
  // (the temporary can be eliminated using some dirty tricks)
  const int tempSize = N + BufferSize1;
  value_type temp[tempSize + 1]; 
  int sz1 = getSize1();
  int sz2 = getSize2(0);
  value_type * pos = temp + tempSize - sz1 - sz2;
  std::copy(minBuffer1,minBuffer1 + sz1 ,pos);
  std::copy(minBuffer2[0],minBuffer2[0] + sz2, pos + sz1);
  temp[tempSize] = sup; // sentinel

  // refill buffer1
  // (using more complicated code it could be made somewhat fuller
  // in certein circumstances)
  priority_queue_local::merge(&pos, &newPos, minBuffer1, sz1,cmp);

  // refill buffer2[0]
  // (as above we might want to take the opportunity
  // to make buffer2[0] fuller)
  priority_queue_local::merge(&pos, &newPos, minBuffer2[0], sz2,cmp);

  // merge the rest to the new segment
  // note that merge exactly trips into the footsteps
  // of itself
  priority_queue_local::merge(&pos, &newPos, newSegment, N,cmp);
  
  // and insert it
  int freeLevel = makeSpaceAvailable(0);
  assert(freeLevel == 0 || itree[0].size() == 0);
  itree[0].insert_segment(newSegment, N);

  // get rid of invalid level 2 buffers
  // by inserting them into tree 0 (which is almost empty in this case)
  if (freeLevel > 0)
  {
    for (int i = freeLevel;  i >= 0;  i--)
    { // reverse order not needed 
      // but would allow immediate refill
      
      newSegment = new value_type[getSize2(i) + 1]; // with sentinel
      std::copy(minBuffer2[i],minBuffer2[i] + getSize2(i) + 1,newSegment);
      itree[0].insert_segment(newSegment, getSize2(i));
      minBuffer2[i] = buffer2[i] + N; // empty
    }
  }

  // update size
  size_ += size_type(N); 

  // special case if the tree was empty before
  if (minBuffer1 == buffer1 + BufferSize1) 
    refillBuffer1();
}

namespace priority_queue_local
{
  struct Parameters_for_priority_queue_not_found_Increase_IntM
  {
    enum { fits = false };
    typedef Parameters_for_priority_queue_not_found_Increase_IntM result;
  };
  
  struct dummy
  {
    enum { fits = false };
    typedef dummy result;
  };
  
  template <int E_,int IntM_,unsigned MaxS_,int B_,int m_,bool stop = false>
  struct find_B_m
  {
    typedef find_B_m<E_,IntM_,MaxS_,B_,m_,stop> Self;
    enum { 
      k = IntM_/B_ ,
      E = E_,
      IntM = IntM_,
      B = B_,
      m = m_,
      c = k - m_,
      // memory occ. by block must be at least 10 times larger than size of ext sequence
      // && satisfy memory req && if we have two ext mergers their degree must be at least 64=m/2
      fits = c>10 && ((k-m)*(m)*(m*B/(E*4*1024))) >= int(MaxS_) && (MaxS_<((k-m)*m/(2*E))*1024 || m >= 128),
      step = 1
    };
    
    typedef typename find_B_m<E,IntM,MaxS_,B,m+step,fits || (m >= k- step)>::result candidate1;
    typedef typename find_B_m<E,IntM,MaxS_,B/2,1,fits || candidate1::fits >::result candidate2;
    typedef typename IF<fits,Self, typename IF<candidate1::fits,candidate1,candidate2>::result >::result result;
    
  };
  
  // specialization for the case when no valid parameters are found
  template <int E_,int IntM_,unsigned MaxS_,bool stop>
  struct find_B_m<E_,IntM_,MaxS_,2048,1,stop>
  {
    enum { fits = false };
    typedef Parameters_for_priority_queue_not_found_Increase_IntM result;
  };
  
  // to speedup search
  template <int E_,int IntM_,unsigned MaxS_,int B_,int m_>
  struct find_B_m<E_,IntM_,MaxS_,B_,m_,true>
  {
    enum { fits = false };
    typedef dummy result;
  };
  
  template <int E_,int IntM_,unsigned MaxS_>
  struct find_settings
  {
    typedef typename find_B_m<E_,IntM_,MaxS_,(8*1024*1024),1>::result result;
  };

  struct Parameters_not_found_Try_to_change_Tune_parameter
  {
    typedef Parameters_not_found_Try_to_change_Tune_parameter result;
  };
  
  
  template <int AI_,int X_,int CriticalSize_>
  struct compute_N
  {
    typedef compute_N<AI_,X_,CriticalSize_> Self;
    enum
    {
      X = X_,
      AI = AI_,
      N = X/(AI*AI)
    };
    typedef typename IF<(N>=CriticalSize_),Self, typename compute_N<AI/2,X,CriticalSize_>::result >::result result;
  };
  
  template <int X_,int CriticalSize_>
  struct compute_N<1,X_,CriticalSize_>
  {
    typedef Parameters_not_found_Try_to_change_Tune_parameter result;
  };

};

//! \}

//! \addtogroup stlcont
//! \{

//! \brief Priority queue type generator

//! Implements a data structure from "Peter Sanders. Fast Priority Queues 
//! for Cached Memory. ALENEX'99" for external memory.
//! <BR>
//! Template parameters:
//! - Tp_ type of the contained objects
//! - Cmp_ the comparison type used to determine 
//! whether one element is smaller than another element. 
//! If Cmp_(x,y) is true, then x is smaller than y. The element 
//! returned by Q.top() is the largest element in the priority 
//! queue. That is, it has the property that, for every other 
//! element \b x in the priority queue, Cmp_(Q.top(), x) is false.
//! Cmp_ must also provide min_value method, that returns value of type Tp_ that is 
//! smaller than any element of the queue \b x , i.e. Cmp_(Cmp_.min_value(),x) is
//! always \b true . <BR> 
//! <BR>
//! Example: comparison object for priority queue 
//! where \b top() returns the \b smallest contained integer: 
//! \verbatim
//! struct CmpIntGreater
//! {
//!   bool operator () (const int & a, const int & b) const { return a>b; }
//!   int min_value() const  { return std::numeric_limits<int>::max(); }
//! };
//! \endverbatim
//! Example: comparison object for priority queue 
//! where \b top() returns the \b largest contained integer: 
//! \verbatim
//! struct CmpIntLess
//! {
//!   bool operator () (const int & a, const int & b) const { return a<b; }
//!   int min_value() const  { return std::numeric_limits<int>::min(); }
//! };
//! \endverbatim
//! Note that Cmp_ must define strict weak ordering.
//! (<A HREF="http://www.sgi.com/tech/stl/StrictWeakOrdering.html">see what it is</A>)
//! - \c IntM_ upper limit for internal memory consumption in bytes. 
//! - \c MaxS_ upper limit for number of elements contained in the priority queue (in 1024 units).
//! Example: if you are sure that priority queue contains no more than 
//! one million elements in a time, then the right parameter is (1000000/1024)= 976 .
//! - \c Tune_ tuning parameter. Try to play with it if the code does not compile 
//! (larger than default values might help). Code does not compile
//! if no suitable internal parameters were found for given IntM_ and MaxS_. 
//! It might also happen that given IntM_ is too small for given MaxS_, try larger values.
//! <BR>
//! \c PRIORITY_QUEUE_GENERATOR is template meta program that searches 
//! for \b 7 configuration parameters of \b stxxl::priority_queue that both 
//! minimize internal memory consumption of the priority queue to 
//! match IntM_ and maximize performance of priority queue operations.
//! Actual memory consumption might be larger (use 
//! \c stxxl::priority_queue::mem_cons() method to track it), since the search 
//! assumes rather optimistic schedule of push'es and pop'es for the
//! estimation of the maximum memory consumption. To keep actual memory 
//! requirements low increase the value of MaxS_ parameter.
//! <BR>
//! For functioning a priority queue object requires two pools of blocks 
//! (See constructor of \c priority_queue ). To construct \c \<stxxl\> block 
//! pools you might need \b block \b type that will be used by priority queue.
//! Note that block's size and hence it's type is generated by 
//! the \c PRIORITY_QUEUE_GENERATOR in compile type from IntM_, MaxS_ and sizeof(Tp_) and
//! not given directly by user as a template parameter. Block type can be extracted as
//! \c PRIORITY_QUEUE_GENERATOR<some_parameters>::result::block_type .
//! For an example see p_queue.cpp .
//! Configured priority queue type is available as \c PRIORITY_QUEUE_GENERATOR<>::result. <BR> <BR>
template <class Tp_,class Cmp_,unsigned IntM_,unsigned MaxS_,unsigned Tune_=6>
class PRIORITY_QUEUE_GENERATOR
{
  public:
  typedef typename priority_queue_local::find_settings<sizeof(Tp_),IntM_,MaxS_>::result settings;
  enum{
     B = settings::B,
     m = settings::m,
     X = B*(settings::k - m)/settings::E,
     Buffer1Size = 32 
  };
  typedef typename priority_queue_local::compute_N<(1<<Tune_),X,4*Buffer1Size>::result ComputeN; 
  enum
  {
     N = ComputeN::N,
     AI = ComputeN::AI,
     AE = (m/2 < 2)?2:(m/2)
  };
public:
  enum {
    // Estimation of maximum internal memory consumption (in bytes)
    EConsumption = X*settings::E + settings::B*AE + ((MaxS_/X)/AE)*settings::B*1024
  };
  /*
      unsigned BufferSize1_ = 32, // equalize procedure call overheads etc. 
      unsigned N_ = 512, // bandwidth
      unsigned IntKMAX_ = 64, // maximal arity for internal mergers
      unsigned IntLevels_ = 4, 
      unsigned BlockSize_ = (2*1024*1024),
      unsigned ExtKMAX_ = 64, // maximal arity for external mergers
      unsigned ExtLevels_ = 2,
  */
  typedef priority_queue<priority_queue_config<Tp_,Cmp_,Buffer1Size,N,AI,2,B,AE,2> > result;
};

//! \}

__STXXL_END_NAMESPACE


namespace std
{
	template <class Config_>
	void swap(stxxl::priority_queue<Config_> & a,
	          stxxl::priority_queue<Config_> & b)
	{
		a.swap(b);
	}
}

#endif
