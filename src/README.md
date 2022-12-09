# FTL Hybridization

This code contains the implementation of the input/ output FTL hybridization.

## Compiling

The code is implemented in ABC as a wildcard ext* function, which adds command 'ftl_hybrid' to the ABC framework.

To compile ABC as a binary, download and unzip the code, then simply type `make`.

## How to run

Running the binary in the command-line mode:
    
    [...] ~/abc> ./abc
    UC Berkeley, ABC 1.01 (compiled Oct  6 2012 19:05:18)
    abc 01>
    
Read in the logic network (in BLIF, AIG or any compactable format), then command `ftl_hybrid`. For example:

    abc 01> r benchmarks/C7552.blif
    abc 02> ftl_hybrid
    
## Command Usage

    abc 01> ftl_hybrid -h
    usage: ftl_hybrid [-ivh]
                          Hybridize network by replacing subcircuit with FTL blocks
            -i            : toggle input (output) hybridization [default = output]
            -v            : verbosity [default = 0]
            -h            : print the command usage

## Benchmarking

All benchmarks experimented and reported can be found in `FTL-ftl_hybrid/benchmarks`.

## Known bug

- [12/8/2022] Possible segmentation fault while updating result network to the current flame.
  - Error occur: `Abc_FrameReplaceCurrentNetwork(pAbc, pNtkRes)`
  - Workaround: Bug does not affect correctness of `ftl_brid`. Exit ABC after single execution of `ftl_hybrid`.  

