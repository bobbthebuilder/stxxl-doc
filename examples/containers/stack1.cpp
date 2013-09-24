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

  // template parameter <data_type, externality, behaviour, blocks_per_page, block_size, 
  //                     internal_stack_type, migrating_critical_size, allocation_strategy, size_type>  

  // define stxxl::normal stack with default parameters 
  typedef stxxl::STACK_GENERATOR<int>::result simple_stack;
  
  // create stack instance 
  simple_stack a_stack;

  stxxl::random_number32 random;
  stxxl::int64 number_of_elements = 128 * 1024 * 1024;

  // routine: 1) push random values on stack and 2) pop all except the lowest value and start again
  for (int k = 0; k < 5; k++) {
    STXXL_MSG("push...");
    for (stxxl::int64 i = 0; i < number_of_elements; i++)
      {
        a_stack.push(random());
      }

    STXXL_MSG("pop...");
    for (stxxl::int64 j = 0; j < number_of_elements - 1; j++)
      {
        a_stack.pop();
      }
  }

  return 0;
}
//! [example]
