/***************************************************************************
 *  examples/containers/vector1.cpp
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
#include <iostream>
#include <stxxl/vector>
#include <stdlib.h>
#include <time.h>

int main()
{ 
  // template parameter <value_type, page_size, number_of_pages, block_size, alloc_strategy, paging_strategy> 
  typedef stxxl::VECTOR_GENERATOR<int, 4, 8, 1*1024*1024, stxxl::RC, stxxl::lru>::result vector_type;  

  vector_type V;
  int counter = 0;

  srand (time(NULL));
  
  // fill vector with random integers
  for (size_t i = 0; i < 10000000; ++i) {
    V.push_back(rand() % 100000);  
  }

  // return size    
  std::cout << "size of V? " << V.size() << std::endl;

  // random access operator[] to receive a const reference to the element at position n                
  const vector_type &cv = V;
  std::cout << "V[0]=" << cv.front() << " " << ", V[n]=" << cv.back() << ", V[1234]=" << cv[1234] << " " << std::endl;

  // iterate over vector from front to back and access each element
  vector_type::const_iterator iter = V.begin();
 
  for (size_t j = 0; j < V.size(); ++j) {
    //std::cout << *iter << " ";  // uncomment to observe scanning    
    if (*iter % 2 == 0) {  // is V's current element even?
      ++counter;
    }    
    iter++;
  }
   
  std::cout << "found " << counter << " even numbers in V" << std::endl;  

  return 0;
}
//! [example]
