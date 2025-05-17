### 2016-pre-VFP Fragment

# Path to hadronize fragment.
path_hadronizer=Configuration/GenProduction/python/Jpsi_Dstar_hadronizer_SPS_13TeV_cfi.py

## Path to gs frameng
path_gs=Configuration/GenProduction/python/Jpsi_50to100_Dstar_SPS_13TeV_GS_cfi.py

# Path to lhe file produce with Helac-Onia
#path_lhe=/afs/cern.ch/work/m/mabarros/public/MonteCarlo/SPS/CMSSW_10_6_20_patch1/src/HelacOnia_lhe/3fs_4fs/Jpsi_50to100_Dstar_file_0.lhe
path_lhe=/afs/cern.ch/work/m/mabarros/public/MonteCarlo/SPS/CMSSW_10_6_20_patch1/src/HelacOnia_lhe/vfns/Jpsi_50to100_Dstar_vfns_file_0.lhe

# lhe file out
py_lhe=Jpsi_50to100_Dstar_SPS_2016pre_13TeV_LHE_cfg.py

# Number of events for gs step
nevt=-1

# GS cfg fragment 
py_gs=Jpsi_50to100_Dstar_SPS_2016pre_13TeV_GS_cfg.py

# For MC Gen parameters 
REPORT_NAME=Jpsi_50to100_Dstar_SPS_2016pre_13TeV_report.xml

## Root files

# GS root
root_gs=Jpsi_50to100_Dstar_SPS_2016pre_13TeV_GS.root

# LHE root
root_lhe=Jpsi_50to100_Dstar_SPS_2016pre_13TeV_LHE.root

# CmsDriver for LHE step
cmsDriver.py $path_hadronizer --mc --eventcontent LHE --datatier LHE --conditions 106X_mcRun2_asymptotic_preVFP_v9 --step NONE --era Run2_2016  --filein file:$path_lhe --python_filename $py_lhe --fileout file:$root_lhe -n $nevt --no_exec 

cmsRun $py_lhe

# CmsDriver for GS step
cmsDriver.py $path_gs --fileout file:$root_gs --mc --eventcontent RAWSIM --datatier GEN --conditions 106X_mcRun2_asymptotic_preVFP_v9 --beamspot Realistic25ns13TeV2016Collision --step GEN --customise Configuration/DataProcessing/Utils.addMonitoring --geometry DB:Extended --era Run2_2016 --filein file:$root_lhe --python_filename $py_gs -n $nevt --no_exec

cmsRun -e -j $REPORT_NAME $py_gs

