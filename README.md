# Patience-Sort
Implementation of Patience Sort created for my bachelor's thesis "Evaluation of Modern Patience Sort",
based on the paper ['Patience is a Virtue: Revisiting Merge and Sort on Modern Processors'](http://research.microsoft.com/apps/pubs/default.aspx?id=209622)
using several optimizations to achieve a runtime that is competitive to famous algorithms like Quicksort or Timsort

Patience Sort consists of 2 phases. The run generation phase splits the input data into sorted sequences
that are afterwards merged in the merge phase. The algorithm aims at sorting almost ordered data sets.
There it is up to 4x faster than std::sort, dependent of the level of order.

The main.cpp includes a short benchmark with Patience Sort and std::sort.

# Known issues
Because of perfomance issues the integrated memory pool is static and therefore not able to grow at the moment.
Thus memory errors can happen for particular input sizes.
