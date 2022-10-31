#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <chrono>

#include "utils.h"

// include build configuration defines
#include "../config.h"

#define DATASET_SIZE 2000
VIP_ENCINT data[DATASET_SIZE];

// total swaps executed so far
unsigned long swaps = 0;


void
print_data(VIP_ENCINT *data, unsigned size)
{
  fprintf(stdout, "DATA DUMP:\n");
  for (unsigned i=0; i < size; i++){
    fprintf(stdout, "  data[%u] = %x\n", i, VIP_DEC(data[i]));
  }
}

void
bubblesort(VIP_ENCINT *data, unsigned size)
{
  for (unsigned i=size; i > 1; i--)
  {
#ifndef VIP_DO_MODE
    bool swapped = false;
#endif /* !VIP_DO_MODE */
    for (unsigned j=0; j < i-1; j++)
    {
#ifndef VIP_DO_MODE
      if (data[j] > data[j+1])
      {
        VIP_ENCINT tmp = data[j];
        data[j] = data[j+1];
        data[j+1] = tmp;
        swapped = true;
        swaps++;
      }
#else /* VIP_DO_MODE */
      VIP_ENCBOOL do_swap = data[j] > data[j+1];
	//     std::cout<< "dataj:"<<std::hex <<data[j].decrypt_int()<<std::endl;
	//     std::cout<< "dataj+1:"<<std::hex <<data[j+1].decrypt_int()<<std::endl;
	// std::cout<< "secret key:"<<std::hex << SECRET_KEY.getUpperValue_64b() << SECRET_KEY.getLowerValue_64b()<<std::endl;

      VIP_ENCINT tmp = data[j];
      data[j] = VIP_CMOV(do_swap, data[j+1], data[j]);
      data[j+1] = VIP_CMOV(do_swap, tmp, data[j+1]);
      swaps++;
#endif /* VIP_DO_MODE */
    }
#ifndef VIP_DO_MODE
    // done?
    if (!swapped)
      break;
#endif /* !VIP_DO_MODE */
  }
}

int
main(void)
{
   init_accel();
  // initialize the privacy enhanced execution target
  // initialize the pseudo-RNG
  mysrand(42);
  // mysrand(time(NULL));

  // initialize the array to sort
  for (unsigned i=0; i < DATASET_SIZE; i++)
    data[i] = myrand();
  print_data(data, DATASET_SIZE);

    auto start = std::chrono::high_resolution_clock::now();

    bubblesort(data, DATASET_SIZE);
      // Get ending timepoint
    auto stop = std::chrono:: high_resolution_clock::now();
 
    // Get duration. Substart timepoints to
    // get duration. To cast it to proper unit
    // use duration cast method
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
 

  print_data(data, DATASET_SIZE);
    std::cout << "Time taken by function: "
         << duration.count() << " microseconds" << std::endl;
  // check the array
  for (unsigned i=0; i < DATASET_SIZE-1; i++)
  {
    if (VIP_DEC(data[i]) > VIP_DEC(data[i+1]))
    {
      fprintf(stdout, "ERROR: data is not properly sorted.\n");
      return -1;
    }
  }
  fprintf(stderr, "INFO: %lu swaps executed.\n", swaps);
  fprintf(stdout, "INFO: data is properly sorted.\n");
  poll_performance();
  close_accel();
  return 0;
}
