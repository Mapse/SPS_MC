### 2017 Fragment

## LHE and python files

# Path to hadronize fragment.
path_gs=Configuration/GenProduction/python/hadronizer_test_SPS_13TeV_cfi.py

# Path to lhe file produce with Helac-Onia
path_lhe=HelacOnia_lhe/results_test_SPS_13TeV.lhe

# lhe file out
py_lhe=test_SPS_13TeV_cfg.py

# Number of events 
nevt=1

# GS cfg fragment 
py_gs=test_SPS_13TeV_GS_cfg.py

# For MC Gen parameters
REPORT_NAME=test_SPS_13TeV_GS_report.xml

# DR cfg fragment
py_dr=test_SPS_13TeV_DR_cfg.py

# HLT cfg fragment
py_hlt=test_SPS_13TeV_HLT_cfg.py

# AOD cfg fragment
py_aod=test_SPS_13TeV_AOD_cfg.py

## Root files

# LHE root
root_lhe=test_SPS_13TeV_lhe.root

# GS root
root_gs=test_SPS_13TeV_GS.root

# DR root
root_dr=test_SPS_13TeV_DR.root

# HLT root
root_hlt=test_SPS_13TeV_HLT.root

# AOD root
root_aod=test_SPS_13TeV_AOD.root

# CmsDriver for LHE step
cmsDriver.py $path_gs --mc --eventcontent LHE --datatier LHE --conditions 106X_mc2017_realistic_v8 --step NONE --era Run2_2017  --filein file:$path_lhe --python_filename $py_lhe --fileout file:$root_lhe -n $nevt --no_exec 

cmsRun $py_lhe

# CmsDriver for GS step
cmsDriver.py $path_gs --fileout file:$root_gs --mc --eventcontent RAWSIM --datatier GEN-SIM --conditions 106X_mc2017_realistic_v8 --beamspot Realistic25ns13TeVEarly2017Collision --step GEN,SIM --customise Configuration/DataProcessing/Utils.addMonitoring --geometry DB:Extended --era Run2_2017 --filein file:$root_lhe --python_filename $py_gs -n $nevt --no_exec 

cmsRun -e -j $REPORT_NAME $py_gs

# Cmsdriver for DR step - with pileup
cmsDriver.py --filein file:$root_gs --fileout file:$root_dr --pileup_input "dbs:/Neutrino_E-10_gun/RunIISummer20ULPrePremix-UL17_106X_mc2017_realistic_v6-v3/PREMIX" --mc --eventcontent PREMIXRAW --datatier GEN-SIM-DIGI --conditions 106X_mc2017_realistic_v6 --step DIGI,DATAMIX,L1,DIGI2RAW --procModifiers premix_stage2 --nThreads 1 --geometry DB:Extended --datamix PreMix --era Run2_2017 --python_filename $py_dr -n -1 --no_exec

cmsRun $py_dr

cp $py_dr DR/ && cp $root_dr ../../CMSSW_9_4_14_UL_patch1/src/HLT && cd ../../CMSSW_9_4_14_UL_patch1/src/HLT

cmsenv

# Cmsdriver for HLT step
cmsDriver.py --filein file:$root_dr --fileout file:$root_hlt --mc --eventcontent RAWSIM --datatier GEN-SIM-RAW --conditions 94X_mc2017_realistic_v15 --customise_commands 'process.source.bypassVersionCheck = cms.untracked.bool(True)' --step HLT:2e34v40 --nThreads 1 --geometry DB:Extended --era Run2_2017 --python_filename $py_hlt -n -1 --no_exec

cmsRun $py_hlt

cp $root_hlt ../../../CMSSW_10_6_20_patch1/src/ && cd ../../../CMSSW_10_6_20_patch1/src/

cmsenv

# Cmsdriver for AOD step
cmsDriver.py --filein file:$root_hlt --fileout file:$root_aod --mc --eventcontent AODSIM --runUnscheduled --datatier AODSIM --conditions 106X_mc2017_realistic_v7 --step RAW2DIGI,L1Reco,RECO,RECOSIM --nThreads 1 --geometry DB:Extended --era Run2_2017 --python_filename $py_aod -n -1 --no_exec

cmsRun $py_aod

cp $py_aod AOD/