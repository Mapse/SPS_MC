from CRABClient.UserUtilities import config
import getpass

######################### To edit #################################

#out_pri_dataset = 'CRAB_PrivateMC_RunII_UL_2017' 
out_pri_dataset = 'CRAB_PrivateMC_RunII_UL_SPS_2017'
pset_name = 'nanoanalyzer_mc.py'
grid_path = '/store/group/uerj/'
out_file = 'Jpsi_9to30_Dstar_SPS_2017_13TeV_NanoAODPlus.root'
storage_site = 'T2_US_Caltech' # 'T2_BR_UERJ'

###################################################################

## Config
config = config()
config.General.requestName = 'NANO_MC_DATASET_DATE'
config.General.workArea = 'crab_projects'
config.General.transferOutputs = True

config.JobType.pluginName = 'Analysis' 
config.JobType.psetName = pset_name
config.JobType.numCores = 1
config.JobType.maxMemoryMB = 2000 #### I changed from 2500!!!!!

config.Data.outputDatasetTag = 'DATASET'
config.Data.userInputFiles = open('paths/FILE').readlines()
config.Data.inputDBS = 'phys03'
config.Data.publishDBS = 'phys03'
config.Data.splitting = 'FileBased'
config.Data.unitsPerJob = 1
config.Data.totalUnits = config.Data.unitsPerJob * NJOBS
config.Data.outLFNDirBase = grid_path + getpass.getuser() + '/'
config.Data.publication = True
config.Data.outputPrimaryDataset = out_pri_dataset  # Comes after /store/user/mabarros  

config.JobType.outputFiles = [out_file] 

config.Site.storageSite = storage_site

