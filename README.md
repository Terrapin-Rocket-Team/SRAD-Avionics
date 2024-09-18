# SRAD_Avionics
Code and PCBs for the avionics subteam for the SRAD flight computer and side projects.

The avionics subteam manages this repo, as well as the `Multi-Mission-Flight-Software` and `Ground Station`
repositories. Look at the *Issues* tab for tasks that need to be done, and the *Projects* tab for the current
status of the Avionics subteam as a whole. 

## Setup
Visit the Software Training and Electronics Training modules for more information on how to start learning 
and getting started.

For installation, the `Ground Station` repository is submoduled on this repository. Submodules are not cloned by git by default. 

To clone the submodule when cloning the repo run:
```
git clone --recurse-submodules [url]
```
To clone the submodule within an existing clone of the repo run:
```
git submodule init
git submodule update
```

To get the latest changes for the submodule run:
```
git pull
git submodule update
```

