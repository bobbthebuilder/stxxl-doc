#ifndef LOOSERTREE_HEADER
#define LOOSERTREE_HEADER

/***************************************************************************
 *            loosertree.h
 *
 *  Sat Aug 24 23:52:58 2002
 *  Copyright  2002  Roman Dementiev, Peter Sanders
 *  dementiev@mpi-sb.mpg.de
 ****************************************************************************/
#include "../common/utils.h"

__STXXL_BEGIN_NAMESPACE

template <typename run_cursor_type, 
					typename run_cursor_cmp_type,
					unsigned buffer_size>
class looser_tree
{
	looser_tree () {} // forbidden
	looser_tree (const looser_tree &);// forbidden
	looser_tree & operator = (const looser_tree &);// forbidden
	
	int logK;
	int k;
	int *entry;
	run_cursor_type *current;
	run_cursor_cmp_type cmp;
  
	int init_winner (int root)
	{
		if (root >= k)
		{
			return root - k;
		}
		else
		{
			int left = init_winner (2 * root);
			int right = init_winner (2 * root + 1);
			if (cmp (current[left], current[right]))
			{
				entry[root] = right;
				return left;
			}
			else
			{
				entry[root] = left;
				return right;
			}
		}
	}
public:
	typedef typename run_cursor_type::prefetcher_type prefetcher_type;
	typedef typename run_cursor_type::value_type value_type;
	
	looser_tree (
		prefetcher_type * p, 
		int nruns, 
		run_cursor_cmp_type c): cmp(c)
	{
		int i;
		logK = static_cast < int >(ceil (log (double(nruns)) / log (2.)));	// replace with something smart
		int kReg = k = (1 << logK);
		
		STXXL_VERBOSE2("looser_tree: logK="<<logK<<" nruns="<<nruns<<" K="<<kReg)
    
    #ifdef STXXL_SORT_SINGLE_PREFETCHER
		current = new run_cursor_type[kReg];
    	run_cursor_type::prefetcher() = p;
    #else
    	current = new run_cursor_type[kReg];
        for (i = 0; i < kReg; ++i)
            current[i].prefetcher() = p;
    #endif
		entry = new int[(kReg << 1)];
		// init cursors
		for (i = 0; i < nruns; ++i)
		{
			current[i].buffer = p->pull_block();
			//current[i].pos = 0; // done in constructor
			entry[kReg + i] = i;
		}

		for (i = nruns; i < kReg; ++i)
		{
			current[i].make_inf ();
			entry[kReg + i] = i;
		}

		entry[0] = init_winner (1);

	}
	~looser_tree()
	{
		delete [] current;
		delete [] entry;
	}
	
	void swap(looser_tree & obj)
	{
		std::swap(logK,obj.logK);
		std::swap(k,obj.k);
		std::swap(entry,obj.entry);
		std::swap(current,obj.current);
		std::swap(cmp,obj.cmp);
	}

private:
	template < unsigned LogK > void multi_merge_unrolled (value_type * to)
	{
		run_cursor_type *currentE, *winnerE;
		int *regEntry = entry;
		value_type *done = to + buffer_size;
		int winnerIndex = regEntry[0];

		while (LIKELY (to < done))
		{
			winnerE = current + winnerIndex;
			*(to++) = winnerE->current ();

			(*winnerE)++;


#define TreeStep(L)																																	    \
      if (LogK >= L )																																    \
      {																																							    \
				currentE = current +                                                            \
          regEntry[ (winnerIndex+(1<<LogK)) >> (((int(LogK-L)+1)>=0)?((LogK-L)+1):0) ];\
				if( cmp(*currentE,*winnerE) )																								    \
				{																																						    \
					std::swap(regEntry[(winnerIndex+(1<<LogK))                                         \
            >> (((int(LogK-L)+1)>=0)?((LogK-L)+1):0) ],winnerIndex);	                  \
					winnerE = currentE;																												    \
				}																																						    \
      }

			TreeStep (10);
			TreeStep (9);
			TreeStep (8);
			TreeStep (7);
			TreeStep (6);
			TreeStep (5);
			TreeStep (4);
			TreeStep (3);
			TreeStep (2);
			TreeStep (1);

#undef TreeStep

		}

		regEntry[0] = winnerIndex;
		
	};
	
	void multi_merge_unrolled_0 (value_type * to)
	{
		const value_type *done = to + buffer_size;
		while (to < done)
		{
			*to = current->current ();
			++to;
			(*current)++;
		}
	};

	void multi_merge_k (value_type * to)
	{
		run_cursor_type *currentE, *winnerE;
		int kReg = k;
		value_type *done = to + buffer_size;
		int winnerIndex = entry[0];

		while (LIKELY (to < done))
		{
			winnerE = current + winnerIndex;
			*(to++) = winnerE->current ();

			(*winnerE)++;

			for (int i = (winnerIndex + kReg) >> 1; i > 0;i >>= 1)
			{
				currentE = current + entry[i];

				if (cmp (*currentE, *winnerE))
				{
					std::swap (entry[i], winnerIndex);
					winnerE = currentE;
				}
			}
		}

		entry[0] = winnerIndex;
	};

public:
	void multi_merge (value_type * to)
	{
		switch (logK)
		{
		case 0:
			multi_merge_unrolled_0 (to);
			break;
		case 1:
			multi_merge_unrolled < 1 > (to);
			break;
		case 2:
			multi_merge_unrolled < 2 > (to);
			break;
		case 3:
			multi_merge_unrolled < 3 > (to);
			break;
		case 4:
			multi_merge_unrolled < 4 > (to);
			break;
		case 5:
			multi_merge_unrolled < 5 > (to);
			break;
		case 6:
			multi_merge_unrolled < 6 > (to);
			break;
		case 7:
			multi_merge_unrolled < 7 > (to);
			break;
		case 8:
			multi_merge_unrolled < 8 > (to);
			break;
		case 9:
			multi_merge_unrolled < 9 > (to);
			break;
		case 10:
			multi_merge_unrolled < 10 > (to);
			break;
		default:
			multi_merge_k (to);
			break;
		}
		
	}

};

__STXXL_END_NAMESPACE

namespace std
{
	template <	typename run_cursor_type, 
				typename run_cursor_cmp_type,
				unsigned buffer_size>
	void swap(stxxl::looser_tree<run_cursor_type,run_cursor_cmp_type,buffer_size> & a,
	          stxxl::looser_tree<run_cursor_type,run_cursor_cmp_type,buffer_size> & b)
	{
		a.swap(b);
	}
}

#endif
