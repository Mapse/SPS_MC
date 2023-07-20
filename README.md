# Using CRAB to run Helac-Onica SPS events

In this tutorial, you will learn how to take the LHE files produced by Helac-Onia and convert them to ROOT. This is performed so that the hadronization can be done using Pythia 8. In the end, you will be able to perform the following chain of steps: LHE -> ROOT (hadronization) -> GEN-SIM -> DIGI2RAW -> HLT -> AOD -> NanoAODPlus.

## Step 0: Producing the relevant files

In this step, we are going to produce all relevant files used in the production chain. There is a shell (.sh) script for each dataset, <i>i.e.</i>, 2016-pre-VFP, 2016-pos-VFP, 2017, 2018. Note that in this tutorial, we have three different Monte Carlo samples, based on the Dimuon p<sub>T</sub>: 9 < p<sub>T</sub> < 30 GeV; 30 < p<sub>T</sub> < 50 GeV; 50 < p<sub>T</sub> < 100 GeV. The file names are:

* steps_SPS_2016-pre-VFP.sh
* steps_SPS_2016-pos-VFP.sh
* steps_SPS_2017.sh
* steps_SPS_2018.sh

We are going to use **steps_SPS_2017.sh** in our example.

This script has 3 input files:

* **Jpsi_Dstar_hadronizer_SPS_13TeV_cfi.py**: This file should be at **Configuration/GenProduction/python/**. This is a Python script to perform the hadronization of the LHE file using Pythia 8.
* **Jpsi_9to30_Dstar_SPS_13TeV_GS_cfi.py**: Also at **Configuration/GenProduction/python/**. This is a very important file. It filters the selected events, <i>i.e.</i>, $J/\psi$ $\rightarrow$ $\mu^+\mu^-$ and $D^{*\pm}$ $\rightarrow$ $D^0$ $\pi^\pm$ ($D^0$ $\rightarrow$ $K^-$ $\pi^+$ -> Also complex conjugate). Note that in this example we are considering sample 1: 9 < p<sub>T</sub> < 30 GeV (9to30). If you want to simulate the other samples, just choose the corresponding file, **Jpsi_30to50_Dstar_SPS_13TeV_GS_cfi.py** for 30-50 GeV and **Jpsi_50to100_Dstar_SPS_13TeV_GS_cfi.py** for 50-100 GeV.
* **Jpsi_9to30_Dstar_file_0.lhe**: File produced with Helac-Onia. Note that this file should be properly weighted.

The **steps_SPS_2017.sh** file will produce many **.py** and **.root** files. Don't worry, all files are automatically sent to the correct destination.

To run it, you must first do,

```
. quick_setup.sh
```
This will ask for your grid credentials. Then, after, you simply do:

```
. steps_SPS_2017.sh
```
And all relevant files to run the following steps should be produced.

## Step 1: LHE -> ROOT

Go to **SPS_MC/CMSSW_10_6_20_patch1/src/LHE/paths**. 

Now we are going to create a **.txt** file containing the paths to the **.lhe** files so we can process them using CRAB. If the files are on a Tier (ex: T2_US_Caltech) you have to use **vai.sh** (note that in this case, the script is written to a path located on the Caltech grid!!). However, if the files are on CERNBOX, you have to use **vai_cernbox.sh**. Just change the paths and do:

```
. vai.sh (or . vai_cernbox.sh)
```
Now, go back to LHE/ directory. There you will see 2 files **crab_config_LHE.py** and **submit_LHE.py**

* **crab_config_LHE.py**: edit lines 6-9 with the correct variables for your case.
* **submit_LHE.py**: change the name of the text file and the number of wanted jobs.

Then, to call CRAB, just do:

```
python submit_LHE.py
```
This will produce the hadronized root files on your storage site.

## Step 2: GEM-SIM


