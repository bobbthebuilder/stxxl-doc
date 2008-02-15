/***************************************************************************
 *            block_cache.h
 *
 *  Jul 2007, Markus Westphal
 ****************************************************************************/


#ifndef STXXL_CONTAINERS_HASHMAP__BLOCK_CACHE_H
#define STXXL_CONTAINERS_HASHMAP__BLOCK_CACHE_H

#include <stxxl>

#ifdef BOOST_MSVC
  #include <hash_map>
#else
  #include <ext/hash_map>
#endif


__STXXL_BEGIN_NAMESPACE


namespace hash_map
{
	template <class BlockType>
	class block_cache
	{		
		public:
			typedef BlockType_                      block_type;
			typedef typename block_type::bid_type   bid_type;
			typedef typename block_type::value_type subblock_type;
			typedef typename subblock_type::bid_type subblock_bid_type;
		
		private:
			/* Test BIDs for equality */
			struct bid_eq
			{
  				bool operator () (const bid_type & a, const bid_type & b) const
				{
      				return (a.storage == b.storage &&  a.offset == b.offset);
 				}
			};

			/* Hash-functor: Maps BIDs to integers */
			struct bid_hash
			{
					size_t operator()(const bid_type & bid) const
					{
				  		return longhash1(bid.offset + uint64(bid.storage)); 
					}
			};
			
			typedef stxxl::btree::lru_pager pager_type;
			typedef buffered_writer<block_type> writer_type;
  			typedef __gnu_cxx::hash_map < bid_type, int_type , bid_hash , bid_eq > bid_map_type;

			
			enum { valid_all = block_type::size };
			

			writer_type	  writer_;		/* writes back dirty blocks  */
			block_type *  wblock_;		/* used to pass dirty blocks to writer */
					
			std::vector<block_type *> blocks_;				/* cached blocks         */
			std::vector<bid_type>     bids_;				/* bids of cached blocks */
			std::vector<int_type>     retain_count_;		/* */
			std::vector<bool>         dirty_;				/* true iff block has been altered while in cache */
			std::vector<int_type>     valid_subblock_;		/* valid_all or the actually loaded subblock's index */
			std::vector<int_type>     free_blocks_;			/* free blocks as indices to blocks_-vector */
			std::vector<request_ptr>  reqs_;				
								
			bid_map_type  bid_map_;
			pager_type    pager_;

			/* statistics */
			int64 n_found;
			int64 n_not_found;
			int64 n_read;
			int64 n_written;
			int64 n_clean_forced;
			int64 n_wrong_subblock;
					
		public:
			//! \brief Construct a new block-cache.
			//! \param cache_size cache-size in number of blocks
			block_cache( unsigned_type cache_size ) : 
					writer_(config::get_instance()->disks_number()*2, 1),
					wblock_(writer_.get_free_block()),
					pager_(cache_size),
					n_found(0),
					n_not_found(0),
					n_read(0),
					n_written(0),
					n_clean_forced(0),
					n_wrong_subblock(0)
			{
				blocks_.reserve(cache_size);
				bids_.reserve(cache_size);
				reqs_.resize(cache_size);
				free_blocks_.reserve(cache_size);
				dirty_.resize(cache_size, false);
				valid_subblock_.resize(cache_size);
				retain_count_.resize(cache_size);
				
				for(unsigned_type i=0; i<cache_size; i++)
				{
					blocks_.push_back(new block_type());
					free_blocks_.push_back(i);
				}
				
//				pager_type tmp_pager(cache_size);
//				std::swap(pager_, tmp_pager);
			}

			//! \brief Cache-size
			unsigned_type size() const
			{
				return blocks_.size();
			}
			
			~block_cache()
			{
				STXXL_VERBOSE1("hash_map::block_cache destructor addr="<<this)
				
				typename bid_map_type::const_iterator i = bid_map_.begin();
				for(; i != bid_map_.end(); ++i)
				{
					const unsigned_type i_block = (*i).second;
					if (reqs_[i_block].valid())
						reqs_[i_block]->wait();
						
					if (dirty_[i_block])
					{
						memcpy(wblock_, blocks_[i_block], block_type::raw_size);
						wblock_ = writer_.write(wblock_, bids_[i_block]);
					}
				}
				
				for (unsigned_type i=0; i<size(); ++i)
					delete blocks_[i];
					
				writer_.flush();
			}
			

		private:
			/* Force a block from the cache; write back to disk if dirty */
			void __kick_block()
			{
				int_type i_block2kick;
				
				unsigned_type max_tries = size()+1;
				unsigned_type i = 0;
				do
				{
					++i;
					i_block2kick = pager_.kick();
					if (i == max_tries )
					{
						STXXL_FORMAT_ERROR_MSG(msg,"The block cache is too small, no block can be kicked out (all blocks are retained)!");
						throw std::runtime_error(msg.str());
					}
					pager_.hit(i_block2kick);
						
				} while (retain_count_[i_block2kick] > 0);

				if (valid_subblock_[i_block2kick] == valid_all && reqs_[i_block2kick].valid())
					reqs_[i_block2kick]->wait();
					
				if (dirty_[i_block2kick])
				{
					memcpy(wblock_, blocks_[i_block2kick], block_type::raw_size);
					wblock_ = writer_.write(wblock_, bids_[i_block2kick]);
					++n_written;
				}
				else
					++n_clean_forced;
					
				bid_map_.erase(bids_[i_block2kick]);
				free_blocks_.push_back(i_block2kick);
			}
			
			
		public:
			//! \brief Retain a block in cache. Blocks, that are retained by at least one client, won't get kicked. Make sure to release all retained blocks again.
			//! \param bid block, whose retain-count is to be increased
			//! \return true if block was cached, false otherwise
			bool retain_block(const bid_type & bid)
			{
				typename bid_map_type::const_iterator it = bid_map_.find(bid);
				if (it == bid_map_.end())
					return false;
					
				unsigned_type i_block = (*it).second;
				retain_count_[i_block]++;
				return true;
			}
			
			//! \brief Release a block (decrement retain-count). If the retain-count reaches 0, a block may be kicked again.
			//! \param bid block, whose retain-count is to be decremented
			//! \return true if operation was successfull (block cached and retain-count > 0), false otherwise
			bool release_block(const bid_type & bid)
			{
				typename bid_map_type::const_iterator it = bid_map_.find(bid);
				if (it == bid_map_.end())
					return false;
				
				unsigned_type i_block = (*it).second;
				if (retain_count_[i_block] == 0)
					return false;
				
				retain_count_[i_block]--;
				return true;
			}
			
			//! \brief Set given block's dirty-flag. Note: If the given block was only partially loaded, it will be completely reloaded.
			//! \return true if block cached, false otherwise
			bool make_dirty(const bid_type & bid)
			{
				typename bid_map_type::const_iterator it = bid_map_.find(bid);
				if (it == bid_map_.end())
					return false;
				
				unsigned_type i_block = (*it).second;
				
				// only complete blocks can be marked as dirty
				if (valid_subblock_[i_block] != valid_all)
				{
					reqs_[i_block] = blocks_[i_block]->read(bid);
					reqs_[i_block]->wait();
					valid_subblock_[i_block] = valid_all;
				}
				
				dirty_[i_block] = true;
				return true;
			}
			
		
			//! \brief Retrieve a subblock from the cache. If not yet cached, only the subblock will be loaded.
			//! \param bid block, to which the requested subblock belongs
			//! \param i_subblock index of requested subblock
			//! \return pointer to subblock
			subblock_type * get_subblock(const bid_type & bid, unsigned_type i_subblock)
			{
				block_type * block;
				unsigned_type i_block;
				n_read++;
				
				// block (partly) cached?
				typename bid_map_type::const_iterator it = _bid_map.find(bid);
				if (it != bid_map_.end())
				{
					i_block = (*it).second;
					block = blocks_[i_block];
					
					// complete block or wanted subblock is in the cache
					if (valid_subblock_[i_block] == valid_all || valid_subblock_[i_block] == i_subblock)
					{
						++n_found;
						if (valid_subblock_[i_block] == valid_all && reqs_[i_block].valid()) 
							reqs_[i_block]->poll() || reqs_[i_block]->wait();
						
						return &((*block)[i_subblock]);
					}
					
					// wrong subblock in cache
					else
					{
						++n_not_found;
						++n_wrong_subblock;
						// actually loading the subblock will be done below
						// note: if a client still holds a reference to the "old" subblock, it will find its data to be still valid.
					}
				}
				// block not cached
				else
				{
					n_not_found++;
					
					if (free_blocks_.empty())
						__kick_block();
					
					i_block = free_blocks_.back(), free_blocks_.pop_back();
					block = blocks_[i_block];

					bid_map_[bid] = i_block;
					bids_[i_block] = bid;
					dirty_[i_block] = false;
					retain_count_[i_block] = 0;
				}
				
				// now actually load the wanted subblock and store it within *block
				subblock_bid_type subblock_bid(bid.storage, bid.offset + i_subblock*subblock_type::raw_size);
				request_ptr req = ((*block)[i_subblock]).read(subblock_bid);
				req->wait();
				
				valid_subblock_[i_block] = i_subblock;
				pager_.hit(i_block);

				return &((*block)[i_subblock]);
			}
			
			
			//! \brief Load a block in advance.
			//! \param bid Identifier of the block to load
			void prefetch_block(const bid_type & bid)
			{
				unsigned_type i_block;
			
				// cached
				typename bid_map_type::const_iterator it = bid_map_.find(bid);
				if (it != bid_map_.end())
				{
					i_block = (*it).second;
					
					// complete block cached; we can finish here
					if (valid_subblock_[i_block] == valid_all)
						pager_.hit(i_block);
						return;
					}
					
					// only a single subblock is cached; we have to load the
					// complete block (see below)
				}
				// not even a subblock cached
				else {
					if (free_blocks_.empty())
						__kick_block();
				
					i_block = free_blocks_.back(), free_blocks_.pop_back();
				
					bid_map_[bid]  = i_block;
					bids_[i_block] = bid;
					retain_count_[i_block] = 0;
					dirty_[i_block] = false;
				}
				
				// now actually load the block
				reqs_[i_block]  = blocks_[i_block]->read(bid);
				valid_subblock_[i_block] = valid_all;
				pager_.hit(i_block);
			}
			
			//! \brief Write all dirty blocks back to disk
			void flush()
			{
				typename bid_map_type::const_iterator i = bid_map_.begin();
				for(; i != bid_map_.end(); ++i)
				{
					const unsigned_type i_block = (*i).second;
					if (dirty_[i_block])
					{
						memcpy(wblock_, blocks_[i_block], block_type::raw_size);
						wblock_ = writer_.write(wblock_, bids_[i_block]);
						dirty_[i_block] = false;
					}
				}
				writer_.flush();
			}
			
			//! \brief Empty cache; don't write back dirty blocks
			void clear()
			{
				free_blocks_.clear();
				for(unsigned_type i=0; i<size(); i++)
				{
					free_blocks_.push_back(i);
				}
				bid_map_.clear();
			}
			
			
			//! \brief Print statistics: Number of hits/misses, blocks forced from cache or written back.
			void print_statistics(std::ostream & o = std::cout) const
			{
				o << "Found blocks                      : " << n_found<<" ("<< 100.*double(n_found)/double(n_read)<<"%)"<<std::endl;
				o << "Not found blocks                  : " << n_not_found<<std::endl;
				o << "Read blocks                       : " << n_read<<std::endl;
				o << "Written blocks                    : " << n_written<<std::endl;
				o << "Clean blocks forced from the cache: " << n_clean_forced<<std::endl;
				o << "Wrong subblock cached             : " << n_wrong_subblock <<std::endl;
			}
			
			
			//! \brief Reset all counters to zero
			void reset_statistics() 
			{
				n_found=0;
				n_not_found=0;
				n_read =0;
				n_written =0;
				n_clean_forced =0;
				n_wrong_subblock = 0;
			}
		
		
			//! \brief Exchange contents of two caches
			//! \param obj cache to swap contents with
			void swap(block_cache & obj)
			{
				std::swap(blocks_, obj.blocks_);
				std::swap(bids_, obj.bids_);
				std::swap(reqs_, obj.reqs_);
				std::swap(free_blocks_, obj.free_blocks_);
				std::swap(valid_subblock_, obj.valid_subblock_);
				std::swap(bid_map_, obj.bid_map_);
				std::swap(pager_, obj.pager_);
				
				std::swap(n_found,obj.n_found);
				std::swap(n_not_found,obj.n_found);
				std::swap(n_read,obj.n_read);
				std::swap(n_written,obj.n_written);
				std::swap(n_clean_forced,obj.n_clean_forced);
			}
			
		
//		private:
			/* show currently cached blocks */
			void __dump_cache() const {
				for (unsigned i = 0; i < _blocks.size(); i++) {
					bid_type bid = _bids[i];
					if (_bid_map.count(bid) == 0) {
						std::cout << "Block " << i << ": empty\n";
						continue;
					}
					
					std::cout << "Block " << i << ": bid=" << _bids[i] << " dirty=" << _dirty[i] << " retain_count=" << _retain_count[i] << " valid_subblock=" << _valid_subblock[i] << "\n";
					for (unsigned k = 0; k < block_type::size; k++) {
						std::cout << "  Subbblock " << k << ": ";
						if (_valid_subblock[i] != valid_all && _valid_subblock[i] != k) {
							std::cout << "not valid\n";
							continue;
						}
						for (unsigned l = 0; l < block_type::value_type::size; l++) {
							std::cout << "(" << (*_blocks[i])[k][l].first << ", " << (*_blocks[i])[k][l].second << ") ";
						}
						std::cout << std::endl;
					}
				}
			}
	};
	
} /* namespace hash_map */

__STXXL_END_NAMESPACE

namespace std
{
	template <class HashMap>
	void swap(stxxl::hash_map::block_cache<HashMap> & a,
					stxxl::hash_map::block_cache<HashMap> & b )
	{
		a.swap(b);
	}
}

#endif /* STXXL_CONTAINERS_HASHMAP__BLOCK_CACHE_H */
