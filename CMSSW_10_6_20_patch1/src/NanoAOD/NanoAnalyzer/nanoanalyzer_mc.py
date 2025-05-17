##################################################################################
# Nanoanalyzer configuration file for all years                                  #
# Use for HT Condor and VM                                                       #
# Uncomment & comment relevant lines before you run it                           #
##################################################################################

import sys
import FWCore.ParameterSet.Config as cms
import FWCore.ParameterSet.Types as CfgTypes
import FWCore.PythonUtilities.LumiList as LumiList
import FWCore.Utilities.FileUtils as FileUtils
from RecoMuon.TrackingTools.MuonServiceProxy_cff import *
from FWCore.ParameterSet.VarParsing import VarParsing


process = cms.Process("Nano")
process.load("FWCore.MessageLogger.MessageLogger_cfi")
process.load("TrackingTools/TransientTrack/TransientTrackBuilder_cfi")
process.load("Configuration.StandardSequences.MagneticField_cff")
process.load("Configuration.StandardSequences.FrontierConditions_GlobalTag_cff")
process.load("Configuration.Geometry.GeometryRecoDB_cff")
process.load('Configuration.StandardSequences.EndOfProcess_cff')

#################################################################################################################################
############################################################ To edit ############################################################
#################################################################################################################################

#global_tag = '106X_dataRun2_v32' #2016
global_tag = '106X_mc2017_realistic_v8' # 2017
#global_tag = '106X_upgrade2018_realistic_v15_L1v1' #2018
#global_tag = '106X_mc2017_realistic_v8'

#### JSON
good_JSON = '/afs/cern.ch/cms/CAF/CMSCOMM/COMM_DQM/certification/Collisions17/13TeV/Legacy_2017/Cert_294927-306462_13TeV_UL2017_Collisions17_GoldenJSON.txt'

# 2016: '/afs/cern.ch/cms/CAF/CMSCOMM/COMM_DQM/certification/Collisions16/13TeV/Legacy_2016/Cert_271036-284044_13TeV_Legacy2016_Collisions16_JSON.txt'
# 2017: '/afs/cern.ch/cms/CAF/CMSCOMM/COMM_DQM/certification/Collisions17/13TeV/Legacy_2017/Cert_294927-306462_13TeV_UL2017_Collisions17_GoldenJSON.txt'
# 2018: '/afs/cern.ch/cms/CAF/CMSCOMM/COMM_DQM/certification/Collisions18/13TeV/Legacy_2018/Cert_314472-325175_13TeV_Legacy2018_Collisions18_JSON.txt'

## Output file
#out_file = 'test_SPS_13TeV_NanoAODPlus.root'
out_file = 'Jpsi_9to30_Dstar_SPS_2017_13TeV_NanoAODPlus.root'

#################################################################################################################################
############################################################ End editing ############################################################
#################################################################################################################################

###################################### GLOBAL TAG ################################
# Change the global tag accordingly
# UL Data Run II 
#process.GlobalTag.globaltag = '106X_dataRun2_v28'

# MC 
process.GlobalTag.globaltag = global_tag

##################################################################################

# Intialize MessageLogger and output report
process.load("FWCore.MessageLogger.MessageLogger_cfi")
process.MessageLogger.cerr.threshold = 'INFO'
process.MessageLogger.categories.append('Nano')
process.MessageLogger.cerr.FwkReport.reportEvery = 1000
process.options   = cms.untracked.PSet(wantSummary = cms.untracked.bool(True))
                                        
# Set the maximum number of events to be processed here
process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(-1))
#process.maxEvents = cms.untracked.PSet(input = cms.untracked.int32(1))

#####################################  JSON FILE #################################
# Change the directory and JSON file accordingly

goodJSON = good_JSON

# Get the luminosity list from the JSON
# Only uncomment if you run in Data
#myLumis = LumiList.LumiList(filename = goodJSON).getCMSSWString().split(',')

#################################### INPUT FILE ##################################
# To test locally or submit batch job using condor, use this:
#fileinPut = FileUtils.loadListFromFile (InputDir)

#process.source = cms.Source("PoolSource",
#                            fileNames = cms.untracked.vstring(*fileinPut)
#)

# To submit batch job using CRAB or test locally (2nd option), use this:
process.source = cms.Source("PoolSource",
                            fileNames = cms.untracked.vstring(
# MC AODSIM
        #  'file://JpsiToMuMuwithDZero_13TeV_RECO.root',)
# MC MiniAOD
#           'file://JpsiToMuMuwithDZero_13TeV_MiniAOD.root',)
# Chamonium UL AOD
          #'/store/data/Run2018B/Charmonium/AOD/12Nov2019_UL2018-v1/100000/00C09940-4299-2642-8874-EE3B38991E20.root',)
# Chamonium UL miniAOD
#          '/store/data/Run2017E/Charmonium/MINIAOD/09Aug2019_UL2017-v1/60000/6CFDC26E-8628-6446-890E-AC0F2E3A330D.root',)
#                            fileNames = cms.untracked.vstring('')
))

##################################################################################

# Process the lumi
# Only uncomment if you run in Data
#process.source.lumisToProcess = CfgTypes.untracked(CfgTypes.VLuminosityBlockRange())
#process.source.lumisToProcess.extend(myLumis)

# *************************************************
# number of events to be skipped (0 by default)   *
# *************************************************
process.source.skipEvents = cms.untracked.uint32(0)

# Process the analyzer
process.nano = cms.EDAnalyzer('NanoAnalyzer',
                              # Change this:
                              # If HTC/VM:
                              #outFile = cms.string(options.outputName),
                              # If interactive:
                              #outFile = cms.string('test.root'), 
                              # If CRAB:
                              #outFile = cms.string('JpsiToMuMuwithDzero_MC_2017_NanoAODplus.root'),
                              outFile = cms.string(out_file),
                              #outFile = cms.string('JpsiToMuMuwithDstar_MC_2017_NanoAODplus.root'),
                              #outFile = cms.string('JpsiToMuMuwithDSPlus_MC_2017_NanoAODplus.root'),
                              #outFile = cms.string('JpsiToMuMuwithLambdac_MC_2017_NanoAODplus.root'),
                              #outFile = cms.string('Data18CharmoniumRunB_AOD.root'),
                              #outFile = cms.string('Data18CharmoniumRunE_AOD.root'),
                              #outFile = cms.string('Data17MuOniaRunE_MINIAOD.root'),
                              #outFile = cms.string('Data16_MinB_foroptmu.root'),
                              #outFile = cms.string('Data16_DMu_mDD.root'),
                              #outFile = cms.string('Data16_DMu_testnewGeo.root'),
                              #outFile = cms.string('Data16_0B_testmitRunC0BAOD.root'),
                              # If data not in CRAB:
                              #outFile = cms.string('JpsiToMuMuwithDZero_13TeV_NanoAOD.root'),
                              #outFile = cms.string('JpsiToMuMuwithDZero_13TeV_NanoAOD_2018.root'),
                              #outFile = cms.string('JpsiToMuMuwithDZero_13TeV_NanoAOD_Data.root'),
                              #outFile = cms.string('MC15_Dstar_foropt.root'),
                              #outFile = cms.string('MC15_MinBnPU_foropt.root'),
                              #outFile = cms.string('MC15_MinBPU_foropt.root'),
                              # make sure the name is same as the crab config
                              # Change this:
                              # If MC:
                              isData = cms.bool(False)
                              # If Data:
                              #isData = cms.bool(True)
)
process.p = cms.Path(process.nano)

