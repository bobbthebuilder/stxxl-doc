/***************************************************************************
 *  examples/containers/stack1.cpp
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
#include <stack>
#include <stxxl/stack>

int main() 
{
  // template parameter <data_type, externality, behaviour, blocks_per_page, block_size, internal_stack_type, 
  //                     migrating_critical_size, allocation_strategy, size_type>
  typedef stxxl::STACK_GENERATOR<int, stxxl::external, stxxl::grow_shrink2, 4, 1*1024*1024, stxxl::RC>::result external_stack;
  
  // prefetch/write pool with 10 blocks prefetching and 10 block write cache 
  stxxl::read_write_pool<external_stack::block_type> pool(10, 10);

  // create a stack with prefetch aggressiveness of 0
  external_stack my_stack(pool, 0);
   
  // grow-shrink routine: 1) push random values on stack and 2) pop all except the lowest value and start again
  stxxl::random_number32 random;
  stxxl::int64 number_of_elements = 128 * 1024 * 1024;

   for (int k = 0; k < 10; k++) {
    STXXL_MSG("push...");
    for (stxxl::int64 i = 0; i < number_of_elements; i++)
      {
	my_stack.push(random());
      }

    // gives the hint that shrinking is imminent and always prefetch 5 buffers from now on                              
    my_stack.set_prefetch_aggr(5);
    STXXL_MSG("pop...");
    for (stxxl::int64 j = 0; j < number_of_elements - 1; j++)
      {
	my_stack.pop();
      }
  }
  
  // migrating stack example with threshold 100000
  typedef stxxl::STACK_GENERATOR<double, stxxl::migrating, stxxl::normal, 4, 1*1024*1024, std::stack<double>, 100000>::result mig_stack;
  mig_stack m_stack(); 

  return 0;
}
//! [example]
