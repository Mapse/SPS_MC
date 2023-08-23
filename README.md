# Using CRAB to run Helac-Onica SPS events

In this tutorial, you will learn how to take the LHE files produced by Helac-Onia and convert them to ROOT. This is performed so that the hadronization can be done using Pythia 8. In the end, you will be able to perform the following chain of steps: LHE -> ROOT (hadronization) -> GEN-SIM -> DIGI2RAW -> HLT -> AOD -> NanoAODPlus.

## Step 0: Producing the relevant files

In this step, we will produce all relevant files used in the production chain. There is a shell (.sh) script for each dataset, <i>i.e.</i>, 2016-pre-VFP, 2016-pos-VFP, 2017, 2018. Note that in this tutorial, we have three different Monte Carlo samples, based on the Dimuon p<sub>T</sub>: 9 < p<sub>T</sub> < 30 GeV; 30 < p<sub>T</sub> < 50 GeV; 50 < p<sub>T</sub> < 100 GeV. The file names are:

* steps_SPS_2016-pre-VFP.sh
* steps_SPS_2016-pos-VFP.sh
* steps_SPS_2017.sh
* steps_SPS_2018.sh

They are located at: CMSSW_10_6_20_patch1/src

We are going to use **steps_SPS_2017.sh** in our example.

This script has 3 input files:

* **Jpsi_Dstar_hadronizer_SPS_13TeV_cfi.py**: This file should be at **CMSSW_10_6_20_patch1/src/Configuration/GenProduction/python/**. This is a Python script to perform the hadronization of the LHE file using Pythia 8.
* **Jpsi_9to30_Dstar_SPS_13TeV_GS_cfi.py**: Also at **CMSSW_10_6_20_patch1/src/Configuration/GenProduction/python/**. This is a very important file. It filters the selected events, <i>i.e.</i>, $J/\psi$ $\rightarrow$ $\mu^+\mu^-$ and $D^{*\pm}$ $\rightarrow$ $D^0$ $\pi^\pm$ ($D^0$ $\rightarrow$ $K^-$ $\pi^+$ -> Also complex conjugate). Note that in this example we are considering sample 1: 9 < p<sub>T</sub> < 30 GeV (9to30). If you want to simulate the other samples, just choose the corresponding file, **Jpsi_30to50_Dstar_SPS_13TeV_GS_cfi.py** for 30-50 GeV and **Jpsi_50to100_Dstar_SPS_13TeV_GS_cfi.py** for 50-100 GeV.
* **Jpsi_9to30_Dstar_file_0.lhe**: File produced with Helac-Onia. Note that this file should be properly weighted.

The **steps_SPS_2017.sh** file will produce many **.py** and **.root** files. Don't worry, all files are automatically sent to the correct destination.

To run it, you must first do,

```
. quick_setup.sh
```
This will ask for your grid credentials. Then, after, you do:

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

Then, to call CRAB, do the following:

```
python submit_LHE.py
```
This will produce the hadronized root files on your storage site.

## Step 2: GEM-SIM

The first thing to do here is to identify what is the path of your files. Using T2_Caltec_US as an example, the command is,

```
xrdfs xrootd-redir.ultralight.org ls -l path_to_files
```
The <i>path_to_files</i> is the path where the files were produced. To identify it you refer to the variable **out_dir_base** on **crab_config_LHE.py** file (line 7). In this example, it is like this:

```
out_dir_base = '/store/group/uerj/' + getpass.getuser() + '/'
```
where in this case, **getpass.getuser()** is my CERN username, **mabarros**. Therefore, the first part of <i>path_to_files</i> is,

```
/store/group/uerj/mabarros
```
The next part of the path is given on the variable **output_dataset** (line 8)

```
output_dataset = 'CRAB_PrivateMC_RunII_UL_SPS_2017',
```
the next is given by the *.txt* file you created in the previous step (in this example, it is jpsi_ccbar_9to30_VFNS_SPS_2017_13TeV.txt), and the next is the number given by the CRAB job (in this case it is 230720_143214). Therefore, the <i>path_to_files</i> is, 

```
/store/group/uerj/mabarros/CRAB_PrivateMC_RunII_UL_SPS_2017/jpsi_ccbar_9to30_VFNS_SPS_2017_13TeV/230720_143214
```
Now that you have learned the format of the <i>path_to_files</i> we can produce the next **.txt** file that will be the input of the next step. Go to **SPS_MC/CMSSW_10_6_20_patch1/src/GS/path**
/

On the **get_files_xrootd.py** files, you will see four lists (after line 46). In this case, they will be like this:

```
mc = ['CRAB_PrivateMC_RunII_UL_SPS_2017'] -> same as output_dataset.
    
dataset = ['jpsi_ccbar_9to30_VFNS_SPS_2017_13TeV'] -> same as jpsi_ccbar_9to30_VFNS_SPS_2017_13TeV.txt

crab_folder = ['230720_143214'] -> from CRAB  

n_folders = [1] -> number of directories located on /store/group/uerj/mabarros/CRAB_PrivateMC_RunII_UL_SPS_2017/jpsi_ccbar_9to30_VFNS_SPS_2017_13TeV/230720_143214¹

¹The directories we refer to are those named 0000, 0001, 0002, ....

```
Then, you do the following:

```
 python3 get_files_xrootd.py
```

This will produce the file jpsi_ccbar_9to30_VFNS_SPS_2017_13TeV.txt

Now, go back to ****SPS_MC/CMSSW_10_6_20_patch1/src/GS**. There are two important files (just like in the LHE step) **crab_config_GS.py** and **submit_GS.py**

* **crab_config_GS.py**: edit lines 6-9 with the correct variables for your case.
* **submit_GS.py**: change the name of the text file and the number of wanted jobs.

Then, to call CRAB, do the following:

```
python submit_GS.py
```

## Step 3: DR

The steps here are the same as you performed on the GS step. Go to **SPS_MC/CMSSW_10_6_20_patch1/src/DR/paths**, edit **get_files_xrootd.py** and obtain the **.txt** file. Then, go to **SPS_MC/CMSSW_10_6_20_patch1/src/DR**, edit **crab_config_DR.py** and **submit_DR.py** and,

```
python submit_DR.py
```

## Step 4: HLT

Again, The steps here are the same as you performed on the GS step. Note that in this case, the CMSSW version is **CMSSW_8_0_33_UL (2016,)  CMSSW_9_4_14_UL_patch1 (2017), and CMSSW_10_2_16_UL (2018)**. Go to **SPS_MC/CMSSW_9_4_14_UL_patch1/src/HLT/paths** (using 2017 as an example), edit **get_files_xrootd.py** and obtain the **.txt** file. Then, go to **SPS_MC/CMSSW_9_4_14_UL_patch1/src/DR**, edit **crab_config_HLT.py** and **submit_HLT.py** and,

```
python submit_hlt.py
```

## Step 5: AOD

The last step before NanoAODPlus. Again, the steps are the same as you performed on the GS step. Go to **SPS_MC/CMSSW_10_6_20_patch1/src/AOD/paths**, edit **get_files_xrootd.py** and obtain the **.txt** file. Then, go to **SPS_MC/CMSSW_10_6_20_patch1/src/AOD**, edit **crab_config_AOD.py** and **submit_AOD.py** and,

```
python submit_AOD.py
```

## Step 6: NanoAODPlus (TDB)

To run NanoAODPlus you should access this repository: https://github.com/Mapse/NanoAOD
