/***************************************************************************
 *  examples/containers/queue1.cpp
 *
 *  Part of the STXXL. See http://stxxl.sourceforge.net
 *
 *  Copyright (C) 2013 Daniel Feist <daniel.feist@student.kit.edu>
 *
 *  Distributed under the Boost Software License, Version 1.0.
 *  (See accompanying file LICENSE_1_0.txt or copy at
 *  http://www.boost.org/LICENSE_1_0.txt)
 **************************************************************************/

//! [example]
#include <stdint.h>
#include <iostream>


int main() 
{
  // template parameter <value_type, block_size, allocation_strategy, size_type> 
  typedef stxxl::queue<unsigned int> a_queue;
  
  // write_pool size = ?, prefetch_pool size = 1, blocks2prefetch = number of blocks in the prefetch pool (i.e. 1)
  a_queue my_queue;

  unsigned int random;
  stxxl::random_number32 rand32;
  stxxl::uint64 number_of_elements = (long long int)(1*1024) * (long long int)(1024 * 1024);
  
  // push random values in the queue
  for (stxxl::uint64 i = 0; i < number_of_elements; i++) 
    {
      random = rand32();    
      my_queue.push(random);      
    }

  unsigned int last_inserted = my_queue.back();
  STXXL_MSG("last element inserted: " << last_inserted);
      
  // identify smallest element than first_inserted, search in growth-direction (front->back)
  while(!my_queue.empty()) 
    {
      if (last_inserted > my_queue.front()) 
	{
	  STXXL_MSG("found smaller element: " << my_queue.front() << " than last inserted element");
	  break;
	}
      std::cout << my_queue.front() << " " << std::endl;
      my_queue.pop();    
    }
  

  return 0;
}
//! [example]
