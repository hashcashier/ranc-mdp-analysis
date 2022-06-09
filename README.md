# [RANC: Reward-All Nakamoto Consensus](https://dl.acm.org/doi/abs/10.1145/3477314.3507056)
Block-withholding attack analysis using the C++ API of [Storm](https://www.stormchecker.org) for Markov Decision Process model checking.

## Getting Started
Before starting, make sure that Storm is installed. If not, see the [documentation](http://www.stormchecker.org/documentation/installation/installation.html) for details on how to install Storm.

First, configure and compile the project. Therefore, execute
```
mkdir build
cd build
cmake ..
make
cd ..
```

Then, run the executable using 
```
./build/ranc-mdp-analysis 
```

Distributed under the GNU General Public License Version 3, 29 June 2007.
