from CRABClient.UserUtilities import config
import getpass

############################################################ To edit ############################################################

pset = 'Jpsi_30to50_Dstar_SPS_bbbar_3FS_4FS_2017_13TeV_AOD_cfg.py' 
out_dir_base = '/store/group/uerj/' + getpass.getuser() + '/'
output_dataset = 'CRAB_PrivateMC_RunII_UL_2017_ccbarxbbbar' # Comes after /store/user/mabarros/
storage_site = 'T2_US_Caltech'

############################################################ End editing ########################################################

config = config()
config.General.requestName = 'AOD_DATASET_DATE'
config.General.workArea = 'crab_projects'
config.General.transferOutputs = True

config.JobType.pluginName = 'Analysis' 
config.JobType.psetName = pset
config.JobType.numCores = 1
config.JobType.maxMemoryMB = 2500

config.Data.outputDatasetTag = 'DATASET'
config.Data.userInputFiles = open('paths/FILE').readlines()
config.Data.inputDBS = 'phys03'
config.Data.publishDBS = 'phys03'
config.Data.splitting = 'FileBased'
config.Data.unitsPerJob = 1
config.Data.totalUnits = config.Data.unitsPerJob * NJOBS
config.Data.outLFNDirBase = out_dir_base
config.Data.publication = True
config.Data.outputPrimaryDataset = output_dataset 
config.Site.storageSite = storage_site

