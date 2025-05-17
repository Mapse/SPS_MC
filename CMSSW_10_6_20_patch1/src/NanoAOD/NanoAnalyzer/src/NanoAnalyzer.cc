// **********************************************************************//
// Prototype implementation of NanoAnalyzer                              //
// for the creation of Run 1 nanoAOD-like ntuple from AOD or RECO input  //
// and corresp. Run 2 reference/validation ntuples from AOD or miniAOD   //
// **********************************************************************//

// ****************************************************
// for implementation history, see testnanoreadme.txt *
// ****************************************************
// Direct contributors:  A. Anuar, A. Bermudez, A. Geiser (coordinator), 
// N.Z. Jomhari, S. Wunsch, Q. Wang, H. Yang, Y. Yang, 2018-2021. 

// ******************************************
// automatically set appropriate CMSSW flag *
// ******************************************
// recognize automatically (no user action needed): 
// CMSSW is tied to particular compiler versions
// 42X taken from 4_2_8, 53X from 5_3_32, 7XX from 7_6_4
#define GCC_VERSION ( 10000 * __GNUC__ + 100 * __GNUC_MINOR__ + __GNUC_PATCHLEVEL__ )
#if GCC_VERSION < 40305
// Early Run 1 legacy CMSSW42X (e.g. 2010, 4_2_8)
#define CMSSW42X
#elif GCC_VERSION < 40703
// Main application is Run 1 legacy CMSSW53X (e.g. 2011/2, 5_3_32)
#define CMSSW53X
#elif GCC_VERSION > 40902
// instead in case CMSSW version is for Run 2, CMSSW7 or higher
// (e.g. 2015-18), for validation purposes
#define CMSSW7plus
#if GCC_VERSION < 50000
// for CMSSW 7_6_X  (GCC_VERSION might need to be changed/sharpened)
// (not clear whether this logic will still work for CMSSW 8 and 9)
#define CMSSW7XX
#endif
#endif
// for ultra-legacy AOD (10_6_4 or higher)
#if GCC_VERSION > 70400
// this one comes on top of the previous
#define CMSSW106plus
#endif
#if GCC_VERSION > 80300
// for Run 3 studies
// this one comes on top of the previous
// GCC_VERSION preliminary, might need to be changed/sharpened
#define CMSSW11plus
#endif

// ********************************************************************
// the following are flags which can be (de)activated manually
// defaults are set such that for normal use no manual action is needed
// ********************************************************************

// activate this to achieve maximum compatibility with official Run 2 nanoAOD
// (default is off -> better performance, e.g. use of beam spot constraint)
// (mainly for Run 2 validation - not recommended for Run 1)
// only relevant for Run 2 AOD, turn on for better consistency with miniAOD 
//                        (slightly worse performance)
// nanoext flag can also be steered/overruled via configuration file
//#define Compatibility

// activation flag to check and store compatibility with Golden JSON 
// default is on for 2010 data (not MC, automatic), off otherwise
#ifdef CMSSW42X
#define JSONcheck
#endif

#ifdef CMSSW42X
// activate this to check 2010 Golden JSON setting, i.e. abort upon nonJSON event
// activate this only when Golden JSON is activated in configuration
// (protection against accidental use of wrong or no JSON in configuration,
//  for check and validation, not strictly needed)
#define JSONcheckabort
#endif

// activate this only for data sets for which "plus" part of trigger treatment 
// is already implemented; checks and aborts in case of inconsistency;
// should be activated by default if trigger is implemented for dataset
// (protection against inconsistencies in NanoTrigger implementation)
//#define trigcheckabort

#ifdef CMSSW7plus
// activate this when you read from miniAOD for validation (Run 2/3 only!)
//#define miniAOD
#endif

#ifdef miniAOD
// activate this when packedPFcandidate covariance matrix is also provided 
// without track details
// (Run 2/3 only, needs update of PatCandidates.h and Lookup tables)
//#define covwithoutdetails 
#endif

// turn this on to deactivate code related to jet corrections
//  (e.g. in case of problems with the configuration, and only if you do 
//   *not* use jets in your analysis)
//  default should be flag off
#define noJetCor 

// turn this on to activate code related to charm final states
#define charm

#ifdef charm
// turn this on to activate D meson cuts optimized for large rapidities
// default (on) is for new looser cuts
#define beauty
#endif

// system include files
#include <memory>
#include <iostream>
#include <vector>
#include <algorithm>
#include <utility>
#include <stdexcept>
#include <cmath>
#include <string>

#ifdef CMSSW42X
#include <boost/unordered_map.hpp>
using boost::unordered_map;
#else
#include <unordered_map>
using std::unordered_map;
#endif

//*****************************
// general user include files *
//*****************************
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/EDAnalyzer.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"

// for fillDescriptions
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "FWCore/ParameterSet/interface/ParameterSetDescription.h"

// ------ EXTRA HEADER FILES--------------------//
#include "FWCore/Framework/interface/EventSetup.h"
#include "FWCore/Framework/interface/ESHandle.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "FWCore/ServiceRegistry/interface/Service.h"
//#include "CommonTools/UtilAlgos/interface/TFileService.h"
#include "DataFormats/Common/interface/Ref.h"
// the following does not seem to be needed (included through dEdx), but also not to hurt
#include "DataFormats/Common/interface/ValueMap.h"
#include "DataFormats/Common/interface/View.h"
#include "DataFormats/Provenance/interface/EventAuxiliary.h"
// to get release version
#include "FWCore/Version/interface/GetReleaseVersion.h"
// header for conversion tools
#ifndef CMSSW11plus
#include "RecoEgamma/EgammaTools/interface/ConversionTools.h"
#else
#include "CommonTools/Egamma/interface/ConversionTools.h"
#endif
// effective area for rho
//https://github.com/cms-sw/cmssw/blob/CMSSW_9_4_X/RecoEgamma/EgammaTools/interface/EffectiveAreas.h
//#include "RecoEgamma/EgammaTools/interface/EffectiveAreas.h"

// for math and file handling
#include "TMath.h"
#include "TFile.h"
#include "Math/VectorUtil.h"

// for ntuple output
#include "TLorentzVector.h"
#include "TTree.h"

#ifdef charm
// for control histograms
#include "TH1.h"
#include "TH2.h"
#endif

//**************************
// for trigger information *
//**************************
#include "FWCore/Common/interface/TriggerNames.h"
// not needed?
//#include "FWCore/Common/interface/TriggerResultsByName.h"
#include "DataFormats/Common/interface/TriggerResults.h"
// for automatic trigger recognition
#include "HLTrigger/HLTcore/interface/HLTConfigProvider.h"
// for trigger objects
#include "DataFormats/HLTReco/interface/TriggerEvent.h"
#include <cassert> 
#include "FWCore/ParameterSet/interface/ConfigurationDescriptions.h"
#include "HLTrigger/HLTcore/interface/HLTEventAnalyzerAOD.h"
#ifndef CMSSW11plus
// no longer exists in CMSSW_11_2_X, not needed??
#include "HLTrigger/HLTcore/interface/TriggerSummaryAnalyzerAOD.h"
#endif

//***************************
// for tracking information *
//***************************
#include "DataFormats/TrackReco/interface/Track.h"
#include "DataFormats/TrackReco/interface/TrackFwd.h"
#include "DataFormats/TrackReco/interface/HitPattern.h"
// for dEdx
#include "DataFormats/TrackReco/interface/DeDxData.h" 	

#ifdef miniAOD
// tracks from PATCandidates
#include "DataFormats/PatCandidates/interface/PackedCandidate.h"
// there are three track collections in miniAOD (in addition to the muon 
// and electron collections), embedded into particleflow objects:
// packedPFCandidates allows to rebuild tracks (for pt>0.5 GeV) using
//                    pseudotrack() (object) or besttrack() (pointer)
// PackedPFCandidatesDiscarded presumably contains only discarded duplicate 
//                    muon candidates -> do not use
// lostTracks (high purity only) contains some of the non-vertex tracks 
#endif

/// for track parametrization 
#include "TrackingTools/TrajectoryParametrization/interface/GlobalTrajectoryParameters.h"
#include "TrackingTools/TrajectoryParametrization/interface/PerigeeTrajectoryParameters.h"

//*************************
// for vertex information *
//*************************
// reconstructed primary is typically within 0.02 cm of true primary in z
#include "DataFormats/VertexReco/interface/Vertex.h"
#include "DataFormats/VertexReco/interface/VertexFwd.h"
#include <DataFormats/VertexReco/interface/Vertex.h>

// for vertices refit:
#include "TrackingTools/TransientTrack/interface/TransientTrackBuilder.h"
#include "TrackingTools/Records/interface/TransientTrackRecord.h"
#include "RecoVertex/AdaptiveVertexFit/interface/AdaptiveVertexFitter.h"
#include "RecoVertex/AdaptiveVertexFinder/interface/AdaptiveVertexReconstructor.h"
#include "RecoVertex/KalmanVertexFit/interface/KalmanVertexFitter.h"
#include "RecoVertex/TrimmedKalmanVertexFinder/interface/KalmanTrimmedVertexFinder.h"
#include "RecoVertex/VertexPrimitives/interface/TransientVertex.h"
#include "RecoVertex/VertexPrimitives/interface/VertexException.h"
#include "TrackingTools/TransientTrack/interface/TrackTransientTrack.h"
//#include "TrackingTools/TransientTrack/interface/TransientTrack.h"
#include "RecoVertex/VertexTools/interface/VertexDistance3D.h"
#include "RecoVertex/PrimaryVertexProducer/interface/PrimaryVertexProducerAlgorithm.h"

// for beamspot information (beam pipe radius: 5.8 -> 4.3 cm, 
//                           beam spot at x~0.2, y~0.4, z~0.3 cm,
//                           width x~0.002?, y~0.002?, z~5.5 cm,
// beam-spot constrained vertices: x~0.001 , y~0.001 , z~0.004 cm) 
#include "DataFormats/BeamSpot/interface/BeamSpot.h"

//****************************
// for error matrix handling *
//****************************
#include "TMatrixT.h"
#include "Math/SMatrix.h"
#include "Math/StaticCheck.h"
//#include <MatrixRepresentationsStatic.h>
#include "Math/Expression.h"

// set namespaces
   using namespace edm;
   using namespace reco;
   using namespace std;

//***********************
// for muon information *
//***********************
#ifndef miniAOD
#include "DataFormats/MuonReco/interface/Muon.h"
#include "DataFormats/MuonReco/interface/MuonFwd.h"
#include "DataFormats/MuonReco/interface/MuonSelectors.h"
#include "DataFormats/MuonReco/interface/MuonIsolation.h"
// CHANGE, this needs to be protected by IFDEF for CMSSW 4.2.8
// or maybe it is not needed? probably it is (commented code)!
//#include "DataFormats/MuonReco/interface/MuonPFIsolation.h"
#endif
#ifdef miniAOD
// for PAT/miniAOD
#include "DataFormats/PatCandidates/interface/Muon.h"
#endif

//***************************
// for electron information *
//***************************
#ifndef miniAOD
#include "DataFormats/EgammaCandidates/interface/GsfElectron.h"
#include "DataFormats/EgammaCandidates/interface/GsfElectronFwd.h"
#include "DataFormats/GsfTrackReco/interface/GsfTrack.h"
#endif
#ifdef miniAOD
#include "DataFormats/PatCandidates/interface/Electron.h"
#endif

// for photon information
#ifndef miniAOD
#include "DataFormats/EgammaCandidates/interface/Photon.h"
#include "DataFormats/EgammaCandidates/interface/PhotonFwd.h"
#endif
#ifdef miniAOD
#include "DataFormats/PatCandidates/interface/Photon.h"
#endif

// for tau information
#ifndef miniAOD
#include "DataFormats/TauReco/interface/PFTau.h"
#include "DataFormats/TauReco/interface/PFTauFwd.h"
#endif
#ifdef miniAOD
#include "DataFormats/PatCandidates/interface/Tau.h"
#endif

//**********************
// for MET information *
//**********************
#ifndef miniAOD
#include "DataFormats/METReco/interface/PFMET.h"
#include "DataFormats/METReco/interface/PFMETFwd.h"
#include "DataFormats/METReco/interface/CaloMET.h"
#include "DataFormats/METReco/interface/CaloMETFwd.h"
#include "DataFormats/METReco/interface/MET.h"
#include "DataFormats/METReco/interface/METFwd.h"
#endif
#ifdef miniAOD
//#include "DataFormats/PatCandidates/interface/PFMET.h"
//#include "DataFormats/PatCandidates/interface/CaloMET.h"
#include "DataFormats/PatCandidates/interface/MET.h"
#endif 

//**********************
// for jet information *
//**********************
#include "DataFormats/Common/interface/Handle.h"
#include "DataFormats/Math/interface/deltaR.h"
#ifndef miniAOD
#include "DataFormats/JetReco/interface/Jet.h"
#include "DataFormats/JetReco/interface/PFJet.h"
#include "DataFormats/JetReco/interface/PFJetCollection.h"
#include "DataFormats/JetReco/interface/TrackJetCollection.h"
#include "DataFormats/JetReco/interface/GenJet.h"
#include "DataFormats/JetReco/interface/GenJetCollection.h"
#include "DataFormats/JetReco/interface/JetExtendedAssociation.h"
#include "DataFormats/JetReco/interface/JetID.h"
#include "JetMETCorrections/Objects/interface/JetCorrector.h"
//#include "CondFormats/JetMETObjects/interface/JetCorrectorParameters.h"
//#include "JetMETCorrections/Objects/interface/JetCorrectionsRecord.h"
#endif
#ifdef miniAOD
#include "DataFormats/PatCandidates/interface/Jet.h"
#endif

//*******************************
// for gen particle information *
//*******************************
#include "SimDataFormats/GeneratorProducts/interface/HepMCProduct.h"
#include "SimDataFormats/GeneratorProducts/interface/GenEventInfoProduct.h"
#include "SimDataFormats/PileupSummaryInfo/interface/PileupSummaryInfo.h"
#include "DataFormats/HepMCCandidate/interface/GenParticleFwd.h"
#include "DataFormats/HepMCCandidate/interface/GenParticle.h"

//********************************
// for particle flow information *
//********************************
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidate.h"
#include "DataFormats/ParticleFlowCandidate/interface/PFCandidateFwd.h"

//***************************
// class member declaration *
//***************************

//***********************************
// main analyzer class (EDAnalyzer) *
//***********************************

class NanoAnalyzer : public edm::EDAnalyzer
{
public:
  explicit NanoAnalyzer(const edm::ParameterSet&);
  ~NanoAnalyzer();

  static void fillDescriptions(edm::ConfigurationDescriptions & descriptions);

  // this is the place to define global variables and parameters 

        // declare global trigger variables
#include "NanoTrigger.h"

private:

  virtual void beginJob();

  virtual void beginRun(const edm::Run &iRun, const edm::EventSetup &iStp);
  virtual void analyze(const edm::Event&, const edm::EventSetup&);
  virtual void analyzeTrigger(const edm::Event&, const edm::EventSetup&, const std::string& triggerName);
  virtual void endRun(edm::Run const&, edm::EventSetup const&);

  virtual void endJob();
  
  // declare optional JSON quality method (code included via #include)
  bool providesGoodLumisection(const edm::Event& iEvent);

  // declare optional trigger methods (code included via #include)
  bool providesGoodMinBiasTrigger(const edm::Event& iEvent);
  bool providesGoodMuTrigger(const edm::Event& iEvent);
  bool providesGoodETrigger(const edm::Event& iEvent);
  bool providesGoodJetTrigger(const edm::Event& iEvent);
  HLTConfigProvider hltConfig_;  //Qun 17-10-19 TriggerObj

  // string manipulation methods
  static bool is_non_digit(const char &c) { return !std::isdigit(c); };
  static std::string remove_version(const std::string &path) { return path.substr(0, path.rfind("_v")); };

  // as it says on the tin
  void update_HLT_branch();

  // branch title creation 
  void eventDoc();
  void createTitle(const std::string &name, const std::string &title); 

  // HLT config for reading the table and its associated process name
  //HLTConfigProvider hlt_cfg;   // superseded above
  std::string hlt_proc;
  unordered_map<std::string, uint8_t> hlt_bit;
  
#ifdef CMSSW7plus
  // for Run 2, enable access via getByToken
#ifndef miniAOD
  // AOD collections
  EDGetTokenT<reco::BeamSpot> beamTkn;
  EDGetTokenT<reco::TrackCollection> trkTkn;
  EDGetTokenT<reco::TrackCollection> gmuTkn;
  EDGetTokenT<reco::VertexCollection> primvtxTkn;
  EDGetTokenT<reco::MuonCollection> muTkn;
  EDGetTokenT<reco::GsfElectronCollection> eTkn;
  EDGetTokenT<reco::PhotonCollection> photTkn;
  EDGetTokenT<reco::PFTauCollection> tauTkn;
  EDGetTokenT<reco::GenParticleCollection> genTkn;
  // why is the next special?
  //EDGetTokenT< std::vector<reco::PFMET> > pfmetTkn;
  EDGetTokenT<reco::PFMETCollection> pfmetTkn;
  // EDGetTokenT< std::vector<reco::CaloMET> > calometTkn;
  EDGetTokenT<reco::CaloMETCollection> calometTkn;
  // not yet needed
  // EDGetTokenT< std::vector<reco::CaloMET> > muCorrmetTkn;
  EDGetTokenT<reco::PFJetCollection> pfjetTkn;
  EDGetTokenT<reco::PFJetCollection> pffatjetTkn;
  EDGetTokenT<reco::GenJetCollection>  genjetTkn;//Qun
  EDGetTokenT<reco::TrackJetCollection> trackjetTkn;
  EDGetTokenT<edm::TriggerResults> trigTkn;
  // for Trigger and Flags (Afiq) *** need to sort out duplication ***
  EDGetTokenT<edm::TriggerResults> trig_tkn;
  EDGetTokenT<edm::TriggerResults> custom_tkn;
  EDGetTokenT<trigger::TriggerEvent> trigEvn; //Qun 
  // get dEdx ValueMaps (from example by M. Soares)
  edm::EDGetTokenT<edm::ValueMap<reco::DeDxData>> dedxMapStripTag_;
  edm::EDGetTokenT<edm::ValueMap<reco::DeDxData>> dedxMapPixelTag_;
  // nuha: conversion collection
  EDGetTokenT<reco::ConversionCollection> hConvTkn;
#endif
#ifdef miniAOD
  // miniAOD collections
  EDGetTokenT<reco::BeamSpot> beamTkn;
  EDGetTokenT<pat::PackedCandidateCollection> trkTkn;
  EDGetTokenT<pat::PackedCandidateCollection> trkTkndisc;
  EDGetTokenT<pat::PackedCandidateCollection> trkTknlost;
  // EDGetTokenT<reco::TrackCollection> gmuTkn;
  EDGetTokenT<reco::VertexCollection> primvtxTkn;
  EDGetTokenT<pat::MuonCollection> muTkn;
  EDGetTokenT<pat::ElectronCollection> eTkn;
  EDGetTokenT<pat::PhotonCollection> photTkn;
  EDGetTokenT<pat::TauCollection> tauTkn;
  EDGetTokenT<reco::GenParticleCollection> genTkn;
  EDGetTokenT<pat::METCollection> pfmetTkn;
  EDGetTokenT<pat::METCollection> calometTkn;
  EDGetTokenT<pat::JetCollection> pfjetTkn;
  EDGetTokenT<reco::GenJetCollection> genjetTkn;//Qun
  EDGetTokenT<pat::JetCollection> pffatjetTkn;
  // does not exist 
  //EDGetTokenT<pat::TrackJetCollection> trackjetTkn;
  EDGetTokenT<edm::TriggerResults> trigTkn;
  // need to sort out duplication
  EDGetTokenT<edm::TriggerResults> trig_tkn;
  EDGetTokenT<edm::TriggerResults> custom_tkn;
  EDGetTokenT<trigger::TriggerEvent> trigEvn; //Qun 
#endif
#endif

  std::string   processName_;  //Qun
  std::string   triggerName_;  //Qun
  // already occurs below
  edm::Handle<edm::TriggerResults> triggerResultsHandle_;
  // move below?
  edm::Handle<trigger::TriggerEvent> triggerEventHandle_;  //Qun 17-10-19 TriggerObj

  // member data
  std::string outFile;
  bool isData;  // whether data or MC
  int CMSSW;    // CMSSW version
  bool nanoext; // write out nanoAOD extensions
  bool covout;  // whether to write or not covariance matrices in nanoAOD extensions
  
/////////////////////////////////////////////////////////////////////////
////////////////////////// declare tree, file, //////////////////////////
/////////////////////////////////////////////////////////////////////////
  
  TFile *file;
  TTree *t_event;

#ifdef charm
  /// define tracking and D meson histograms (for debug/validation) ///
  /* TH1D *h_trackpt;
  TH1D *h_trackptlow;
  TH1D *h_trackptlowfine;
  TH1D *h_tracketa;
  TH1D *h_trackcov00;
  
  TH1D *h_d0pt;
  TH1D *h_dstarpt;
  TH1D *h_PS3pt;
  TH1D *h_K1pt;
  TH1D *h_P2pt;
  TH1D *h_K1eta;
  TH1D *h_P2eta;
  TH1D *h_PS3eta;
  TH1D *h_D0masscut;
  TH1D *h_deltaMassD0Dstar;
  TH1D *h_D0masscut_rightDLcut;
  TH1D *h_deltaMassD0Dstar_rightDLcut; */
#endif

//////////////////////////////////////////////////////////////////////////////
///////////////// declare variables you want to put into the tree ////////////
//////////////////////////////////////////////////////////////////////////////
// local for the moment
// to be declared global if analyzer split into semiindependent parts


////////////////////////////////// for general ///////////////////////////////

  /// original nanoAOD ///
  UInt_t run;
  ULong64_t event;
  UInt_t luminosityBlock;

  /// special nanoAOD extension ///
  // (see global part, NanoTrigger.h)


////////////////////////////// for Gen particle //////////////////////////////

  /// nanoAOD ///     
  //                   
  UInt_t nGenPart;
  vector<Float_t> GenPart_pt;
  vector<Float_t> GenPart_eta;
  vector<Float_t> GenPart_phi;
  vector<Float_t> GenPart_mass;
  vector<Float_t> GenPart_charge;
  vector<Int_t> GenPart_pdgId;
  vector<Int_t> GenPart_status;
  vector<Int_t> GenPart_statusFlags;
  vector<Int_t> GenPart_genPartIdxMother; 

  /// general nanoAOD extension ///
  vector<Int_t> GenPart_Id;
  vector<uint8_t> GenPart_isNano;  // satisfies criteria for nanoAOD shortlist 
                                   // (Bool on ntuple)
  vector<Int_t> GenPart_parpdgId;  // PDG id of parent particle
  vector<Int_t> GenPart_sparpdgId; // PDG id of parent stable particle
  vector<Int_t> GenPart_numberOfDaughters; 
                                   // number of daughter particles in decay
  vector<Int_t> GenPart_nstchgdaug; 
                    // number of stable charged daughter particles in decay
  vector<Float_t> GenPart_vx;      // particle origin vertex
  vector<Float_t> GenPart_vy;      // (agrees with GenPV if prompt)
  vector<Float_t> GenPart_vz;
  vector<Float_t> GenPart_mvx;     // mother origin vertex
  vector<Float_t> GenPart_mvy;     // (agrees with GenPV of prompt)
  vector<Float_t> GenPart_mvz;
  vector<Int_t> GenPart_recIdx;    // pointer to rec track list if matched

  // for decay and mother vertices from GenPart
  float dcyvtxx, dcyvtxy, dcyvtxz;
  float motx, moty, motz;

  // main generated vertex, filled from suitably chosen GenPart_vtx
  Float_t GenPV_x;                 // position
  Float_t GenPV_y;
  Float_t GenPV_z;
  Int_t GenPV_recIdx;              // pointer to rec vertex list if matched 
  Int_t GenPV_chmult;              // charged multiplicity
  //Josry prompt/nonprompt Dstar/D0 Flag
  vector<Int_t> GenPart_promptFlag;

  /////////////////////////// for trigger objects ////////////////////////////

  // Trigger Object
  UInt_t nTrigObj;                   // number of stored Trigger Obj
  vector<Int_t> TrigObj_id;          // ID of the object
  vector<Int_t> TrigObj_filterBits;  // filter bits - *** still to be implemented ***
  vector<Float_t> TrigObj_pt;        // pt
  vector<Float_t> TrigObj_phi;        // phi
  vector<Float_t> TrigObj_eta;        // eta



////////////////////////////// for track variables ///////////////////////////
   
  /// nanoAOD ///            *** not yet filled, not yet written out ***
  UInt_t nIsoTrack; // number of isolated tracks (from main prim. vertex only)
  vector<Float_t> IsoTrack_dxy;
  vector<Float_t> IsoTrack_dz;
  vector<Float_t> IsoTrack_ets;
  vector<uint8_t> IsoTrack_isHighPurityTrack;
  vector<uint8_t> IsoTrack_isPFcand;
  vector<Float_t> IsoTrack_miniPFreliso_all;
  vector<Float_t> IsoTrack_miniPFreliso_chg;
  vector<Int_t>   IsoTrack_pdgId;
  vector<Float_t> IsoTrack_PFreliso03_all;
  vector<Float_t> IsoTrack_PFreliso03_chg;
  vector<Float_t> IsoTrack_phi;
  vector<Float_t> IsoTrack_pt;

  /// nonstandard nanoAOD extension ///  
  UInt_t nTrk;               // number of tracks in event
  // *** from here not yet filled, not yet written out ***
  vector<Int_t> Trk_Id;      // unique track identifier
  vector<Int_t> Trk_pvIdx;   // if nonzero: primary vertex id (PVtx_id) this 
          // track is associated (but not necessarily fitted) to
          // if zero: the track is not associated to any primary vertex 
  vector<Int_t> Trk_pvaltIdx;// if nonzero: alternative primary vertex id 
          // (Pvtx_id) this track might be associated to  
          // i.e. the vertex association might be ambigous
  vector<Int_t> Trk_pvfIdx;  // if nonzero: primary vertex id (PVtx_id) this 
                             // track is fitted to    
  vector<Int_t> Trk_svfIdx;  // if nonzero: secondary vertex id (SVtx_id) this 
                             // track is fitted to
  vector<Int_t> Trk_isIsoTrack; // appears in IsoTrack list yes(1)/no(0)
  vector<Int_t> Trk_charge;  // track charge 
  vector<Float_t> Trk_pt;    // track transverse momentum
  vector<Float_t> Trk_eta;   // track pseudorapidity
  vector<Float_t> Trk_phi;   // track phi
  // Add here some track quality variables //


////////////////////////////// for vertex and beam spot variables ///////////////////////// 

  /// official nanoAOD ///
  Int_t   PV_npvs;           // number of reconstructed primary vertices
  Int_t   PV_npvsGood;       // number of good reconstructed primary vertices
          // selection: !isFake && ndof>4 && abs(z) <= 24 && position.rho <=2
  Float_t PV_chi2;           // main primary vertex chi2 
  Float_t PV_ndof;           // main primary vertex number of degr. of freedom
  Float_t PV_score;          // main primary vertex score, 
                             // i.e. sum pt2 of clustered objects
  Float_t PV_x;              // main primary vertex position x coordinate
  Float_t PV_y;              // main primary vertex position y coordinate
  Float_t PV_z;              // main primary vertex position z coordinate
  UInt_t nOtherPV;          // number of other primary vertices, 
                             // excluding the main PV
  vector<Float_t> OtherPV_z; // z position of other primary vertices, 
                             // excluding the main PV 

  /// nanoAOD extension ///
  UInt_t nPVtx;                // number of all primary vertex candidates
  vector<Int_t>   PVtx_Id;     // primary vertex identifier
  vector<uint8_t> PVtx_isMain; // corresponds to main prim. in PV yes(1)/no(0)
                               //  (highest score)
  vector<uint8_t> PVtx_isMainSim; // corresponds to main simulated primary 
         // yes(1)/no(0) (=vertex for which generator information is available)
  vector<uint8_t> PVtx_isGood; // vertex counted in PV_npvsGood yes(1)/no(0)
  vector<uint8_t> PVtx_isValid; // vertex fit converged properly? yes(1)/no(0)
  vector<uint8_t> PVtx_isFake; // vertex is made up? yes(1)/no(0)
  vector<uint8_t> PVtx_isTrigUnique; // This unambigously is the vertex of 
         // the event that fired the trigger (e.g. lepton); may contain some 
         // pileup, may not necessarily coincide with main vertex 
  vector<uint8_t> PVtx_isUnbiased; // This is a vertex which did not contribute 
         // to firing the trigger (no trigger pileup). 
         // *** Can be used as next-to-minimum bias. ***
         // In cases where several vertices might have significantly 
         // contributed to the trigger (e.g. multileptons from different 
         // vertices, 
         // pileup contribution to energies, ...) neither of the two previous 
         // flags will be set for these vertices
  vector<Int_t> PVtx_ntrk;   // number of tracks associated to this primary 
         // vertex (including secondaries from this primary)
         // will be filled in NanoDmeson! 
  vector<Int_t> PVtx_ntrkfit;// number of tracks fitted to this primary vertex
  vector<Float_t> PVtx_chi2;   // primary vertex chi2
  vector<Float_t> PVtx_ndof;   // primary vertex number of degrees of freedom
  vector<Float_t> PVtx_score;  // score (sum pt^2) for determination of PV
  vector<Float_t> PVtx_sumPt;  // hadronic sum Pt for this vertex (tracks only,
                               // excluding leptons)
  vector<Float_t> PVtx_Rho;    // primary vertex Rho (sqrt(x^2+y^2))
  vector<Float_t> PVtx_x;      // primary vertex x
  vector<Float_t> PVtx_y;      // primary vertex y
  vector<Float_t> PVtx_z;      // primary vertex z
  vector<Float_t> PVtx_Covxx;  // primary vertex covariance matrix (6 entries)
  vector<Float_t> PVtx_Covyx; 
  vector<Float_t> PVtx_Covzx; 
  vector<Float_t> PVtx_Covyy; 
  vector<Float_t> PVtx_Covzy; 
  vector<Float_t> PVtx_Covzz; 
  // In order to find tracks associated to a particular vertex PVtx_id, 
  // loop over the track list and check for Trk_pvIdx == PVtx_id   

  Float_t Bsp_x;              // Beam spot x0
  Float_t Bsp_y;              // Beam spot y0
  Float_t Bsp_z;              // Beam spot z0
  Float_t Bsp_sigmaz;         // Beam spot size in z
  Float_t Bsp_dxdz;           // Beam spot slope in xz
  Float_t Bsp_dydz;           // Beam spot slope in yz
  Float_t Bsp_widthx;         // Beam spot width in x
  Float_t Bsp_widthy;         // Beam spot width in y
  // width is of order 15-20 micron in both x and y
  // uncertainties of slope and width?
  
////////////////////////////// for Muon variables ////////////////////////////

  UInt_t b4_nMuon;  // all muon counter;  (b4 = before selection)
  UInt_t Muon_nNano; // counter for official nanoAOD muons

  /// official nanoAOD ///

  UInt_t nMuon;                   // number of stored muons
  vector<Int_t> Muon_charge;      // muon charge
  vector<Int_t> Muon_tightCharge; // muon tight charge
  // for the following, take the parameters from global track if available, 
  // from tracker or muon track otherwise 
  // seems to be always taken from inner track if available
  vector<Float_t> Muon_pt;        // muon transverse momentum (in GeV)
  vector<Float_t> Muon_ptErr;     // muon transverse momentum error
  vector<Float_t> Muon_eta;       // muon pseudorapidity
  vector<Float_t> Muon_phi;       // muon azimuth angle
  vector<Float_t> Muon_mass;      // muon mass (redundant ...)
  // the following are taken w.r.t. the main primary vertex (CMS default)
  // see also variable extensions w.r.t. associated primary
  vector<Float_t> Muon_dxy;       // distance in xy (in cm)
  vector<Float_t> Muon_dxyErr;    // error in xy
  vector<Float_t> Muon_dz;        // distance in z
  vector<Float_t> Muon_dzErr;     // error in z
  vector<Float_t> Muon_ip3d;      // 3D impact parameter
  vector<Float_t> Muon_sip3d;     // 3D impact parameter significance
  // for the following, take the information from ...
  vector<Float_t> Muon_pfRelIso03_all;   // PF isolation in cone 0.3
  vector<Float_t> Muon_pfRelIso03_chg;   // PF track isolation in cone 0.3
  vector<Float_t> Muon_pfRelIso04_all;   // PF isolation in cone 0.4
  vector<Int_t>   Muon_pfIsoId;          // PF isolation flag, (*** not yet ***) pileup corrected
  vector<Float_t> Muon_miniPFRelIso_all; // *
  vector<Float_t> Muon_miniPFRelIso_chg; // *
  vector<Int_t> Muon_jetIdx;   // *
  // all nanoAOD bool arrays are declared uint8_t (bool will not work)
  vector<uint8_t> Muon_isGlobal; // muon is global muon; 
                               // track parameters are global parameters;
                               // tracker only track parameters can be found 
                               // from TRK list, when filled   
  vector<uint8_t> Muon_isTracker;// muon is tracker muon;
  vector<uint8_t> Muon_isPFcand; // muon is PF candididate
  vector<uint8_t> Muon_softId;   // muon satisfies soft ID 
  vector<uint8_t> Muon_mediumId; // muon satisfies medium Id 
  vector<uint8_t> Muon_tightId;  // muon satisfies tight Id 
  vector<UChar_t> Muon_highPtId; // muon satisfies high pt Id, not bool!
  vector<Int_t> Muon_nStations;      // number of muon stations hit
  vector<Int_t> Muon_nTrackerLayers; // number of tracker layers hit
  vector<Float_t> Muon_segmentComp;  // muon segnent compatibility
  vector<UChar_t> Muon_cleanmask;    // *
  vector<Float_t> Muon_mvaTTH;       // *
  vector<Int_t> Muon_pdgId;          // +-13, depending on (-1)*charge
  vector<UChar_t> Muon_genPartFlav;  // *
  // clarify overlap with Muon_simIdx
  vector<Int_t> Muon_genPartIdx;     // *

  /// nanoAOD extension ///
  // store all muon candidates
  vector<Int_t> Muon_Id;       // unique muon identifier
  // add more position and impact parameter info
  vector<Float_t> Muon_x;      // muon track x position at dca to beamspot
  vector<Float_t> Muon_y;      // muon track y position at dca to beanspot
  vector<Float_t> Muon_z;      // muon track z position at dca to beamspot 
  vector<Float_t> Muon_dxyBest;   // distance in xy to Muon_vtxIdx vertex (cm)
  vector<Float_t> Muon_dzBest;    // distance in z to Muon_vtxIdx vertex (cm)
  vector<Float_t> Muon_ip3dBest;  // impact parameter to Muon_vtxIdx vertex(cm)
  vector<Float_t> Muon_sip3dBest; // impact parameter significance
  // add also full covariance matrix here?   
  vector<Float_t> Muon_gpt;      // global muon transverse momentum (in GeV)
  vector<Float_t> Muon_geta;     // global muon pseudorapidity
  vector<Float_t> Muon_gphi;     // global muon azimuth angle
  vector<uint8_t> Muon_looseId;  // muon satisfies loose Id
  vector<uint8_t> Muon_softId4;  // muon satisfies "old" (CMSSW4X) soft Id
  vector<uint8_t> Muon_softIdBest; // soft Id for best vertex (instead of main)
  vector<uint8_t> Muon_isNano;   // muon satisfies criteria for off. nanoAOD 
                               // (to restrict to original list)
  vector<uint8_t> Muon_isMini;   // muon satisfies criteria for MiniAOD;
  vector<uint8_t> Muon_isGood;   // muon satisfies TMOneStationTight;
  vector<uint8_t> Muon_isGoodLast; // muon satisfies TMLastStationTight;
  vector<uint8_t> Muon_isGoodAng; // muon satisfies TMLastStationAnyTight;
  vector<uint8_t> Muon_isArbitrated; // muon satisfies TrackerMuonArbitrated;
  vector<uint8_t> Muon_isStandAlone; // muon is standalone muon;
  vector<uint8_t> Muon_isRPCcand; // muon is RPC candididate
  vector<Int_t> Muon_nValid;   // number of valid hits;
  vector<Int_t> Muon_nPix;     // number of pixel hits;
  vector<Float_t> Muon_Chi2;   // tracker muon chi2/ndof;
  vector<Int_t> Muon_gnValid;  // number of valid global muon hits;
  vector<Int_t> Muon_gnPix;    // number of global muon pixel hits;
  vector<Float_t> Muon_gChi2;  // global muon chi2/ndof;
  vector<Int_t> Muon_gnValidMu;// number of valid muon hits in global muon;
  vector<Int_t> Muon_vtxIdx;   // index (PVtx_Id) of vertex in PVtx list,if any
  vector<Int_t> Muon_vtxFlag;  // quality flag for vertex association
  vector<Int_t> Muon_trkIdx;   // index (Trk_Id) of track in TRK list
  // clarify overlap with Muon_genPartIdx
  vector<Int_t> Muon_simIdx;   // index (GenPart_Id) of particle in GenPart
    
  // for dimuon candidates (nonstandard extension)
#include "NanoDimu.h" 

//////////////////////////// for electron variables ///////////////

  /// nanoAOD subset ///
/* 
// Electrons
  const static int max_el = 128;
  UInt_t value_el_n;
  float value_el_pt[max_el];
  float value_el_eta[max_el];
  float value_el_phi[max_el];
  float value_el_mass[max_el];
  Int_t value_el_charge[max_el];
  Int_t value_el_tightCharge[max_el];
  
  float value_el_pfreliso03all[max_el];
  float value_el_pfreliso03chg[max_el];
  float value_el_dr03TkSumPtOld[max_el];
  float value_el_dr03TkSumPt[max_el];  
  float value_el_dr03EcalRecHitSumEtOld[max_el];
  float value_el_dr03EcalRecHitSumEt[max_el];  

  float value_el_dr03HcalTowerSumEt[max_el];
  float value_el_dr03HcalDepth1TowerSumEtOld[max_el];
  float value_el_dr03HcalDepth1TowerSumEt[max_el];

  bool  value_el_isEB[max_el];
  bool  value_el_isEE[max_el];
  UChar_t value_el_lostHits[max_el];
  float value_el_convDist[max_el];
  float value_el_convDcot[max_el];
  bool  value_el_convVetoOld[max_el];
  bool  value_el_convVeto[max_el];
  
  float value_el_deltaEtaSC[max_el];
  float value_el_deltaPhiSC[max_el];
  float value_el_deltaEtaSCtr[max_el];
  float value_el_deltaPhiSCtr[max_el];
  float value_el_hoe[max_el];
  float value_el_sieieR1[max_el];
  float value_el_sieie[max_el];
  float value_el_eInvMinusPInvOld[max_el];
  float value_el_eInvMinusPInv[max_el];
  
  float value_el_SCeta[max_el];
  Int_t value_el_cutBased[max_el];

  float value_el_x[max_el];
  float value_el_y[max_el];
  float value_el_z[max_el];
  Int_t value_el_vtxIdx[max_el]; 

  float value_el_dxy[max_el];
  float value_el_dxyErr[max_el];
  float value_el_dz[max_el];
  float value_el_dzErr[max_el];

  float value_el_ip3d[max_el];
  float value_el_sip3d[max_el];
  
  bool value_el_isPFcand[max_el];   
  bool value_el_isNano[max_el];

  UInt_t Electron_nNano; // counter for official nanoAOD electrons


//////////////////////////// for other variables ///////////////

// Photons
  const static int max_ph = 100;
  UInt_t value_ph_n;
  float value_ph_pt[max_ph];
  float value_ph_eta[max_ph];
  float value_ph_phi[max_ph];
  float value_ph_mass[max_ph];
  int value_ph_charge[max_ph];
  float value_ph_pfreliso03all[max_ph];

// Taus
  const static int max_tau = 100;
  UInt_t value_tau_n;
  float value_tau_pt[max_tau];
  float value_tau_eta[max_tau];
  float value_tau_phi[max_tau];
  float value_tau_mass[max_tau];
  int value_tau_charge[max_tau];
  int value_tau_decaymode[max_tau];
  float value_tau_chargediso[max_tau];
  float value_tau_neutraliso[max_tau];

// MET
  float value_met_pt;
  float value_met_phi;
  float value_met_sumEt;
  float value_met_significance;
  float value_met_covxx;
  float value_met_covxy;
  float value_met_covyy;

// CaloMET
  float value_calomet_pt;
  float value_calomet_phi;
  float value_calomet_sumEt;

// Jets
  const static int max_jet = 300;
  UInt_t value_jet_n;
  float value_jet_pt[max_jet];
  float value_jet_ptuncor[max_jet];
  float value_jet_eta[max_jet];
  float value_jet_phi[max_jet];
  float value_jet_mass[max_jet];
//  float value_jet_ptD[max_jet];
  float value_jet_area[max_jet];
  int value_jet_nConstituents[max_jet];
  int value_jet_nElectrons[max_jet];
  int value_jet_nMuons[max_jet];
  float value_jet_chEmEF[max_jet];
  float value_jet_chHEF[max_jet];
  float value_jet_neEmEF[max_jet];
  float value_jet_neHEF[max_jet];
  float value_jet_CEMF[max_jet];
  float value_jet_MUF[max_jet];
  float value_jet_NumConst[max_jet]; 
  float value_jet_CHM[max_jet];
  int value_jet_id[max_jet];

  // jet correction label, can/should this be moved elsewhere?
  std::string mJetCorr;
//  std::string mJetCorr_ak5;

//GenJets
  const static int max_gjet = 300;
  UInt_t value_gjet_n;
  float value_gjet_pt[max_gjet];
  float value_gjet_eta[max_gjet];
  float value_gjet_phi[max_gjet];
  float value_gjet_mass[max_gjet];

// FatJets
  const static int max_fatjet = 300;
  UInt_t value_fatjet_n;
  float value_fatjet_pt[max_fatjet];
  float value_fatjet_eta[max_fatjet];
  float value_fatjet_phi[max_fatjet];
  float value_fatjet_mass[max_fatjet];
  float value_fatjet_area[max_fatjet];
  int value_fatjet_nConstituents[max_fatjet];
  int value_fatjet_nElectrons[max_fatjet];
  int value_fatjet_nMuons[max_fatjet];
  float value_fatjet_chEmEF[max_fatjet];
  float value_fatjet_chHEF[max_fatjet];
  float value_fatjet_neEmEF[max_fatjet];
  float value_fatjet_neHEF[max_fatjet];
 */
// Track Jets
  /* const static int max_trackjet = 300;
  UInt_t value_trackjet_n;
  float value_trackjet_pt[max_trackjet];
  float value_trackjet_eta[max_trackjet];
  float value_trackjet_phi[max_trackjet];
  float value_trackjet_mass[max_trackjet];
  float value_trackjet_area[max_trackjet];
  int value_trackjet_nConstituents[max_trackjet];
  int value_trackjet_nElectrons[max_trackjet];
  int value_trackjet_nMuons[max_trackjet];
 */
// Flags
  edm::InputTag custom_tag;
  std::vector<std::string> custom_flag;
  std::vector<uint8_t> custom_bit;

#ifdef charm
// for D mesons (nonstandard extension)
#include "NanoDmeson.h"
#endif
  // Josry prompt/nonprompt flag extension
  int promptFlag;

}; // end of class member

//
// constants, enums and typedefs
//
const float emass = 0.000510999; // [PDG]
const float pi = 3.141593;
const float mumass = 0.105658;
const float Kmass = 0.49367;
const float pimass = 0.13957;
const float mD0Actual = 1.86484;  // [PDG]
const float mDstarActual = 2.0103;  // [PDG]
const float dmDstarActual = 0.145426;  // [PDG]
// reserve array sizes; reconsider when considering new datasets! 
const unsigned nReserve_GenPart = 1024; 
const unsigned nReserve_Track = 2048; 
const unsigned nReserve_IsoTrack = 32; 
// at most 3 "otherPV" vertices are stored on official nanoAOD
// see PVtx structure for full list
const unsigned nReserve_OtherPV = 3; 
//const unsigned nReserve_OtherPV = 128; 
const unsigned nReserve_PVtx = 128; 
const unsigned nReserve_Muon = 128;
const unsigned nReserve_TrigObj = 1024;
const unsigned nReserve_Dimu = 256;
//const unsigned nReserve_D0 = 1024;
const unsigned nReserve_D0 = 4096;
//const unsigned nReserve_Dstar = 1024;
//const unsigned nReserve_Dstar = 8192;
const unsigned nReserve_Dstar = 16384;
const int maxnmusim = 128;
const int maxnD0sim = 128;
const int maxnDplussim = 128;
const int maxnDstarsim = 128;
const int maxnmuonlist = 128;
const int maxneleclist = 128;

// preset flag for trigger steering

#ifdef CMSSW11plus
#ifdef miniAOD
// ********************* temporary *******************
// need to recheck miniAOD trigger treatment
bool skiptrigger = true;
// ***************************************************
#else
bool skiptrigger = false;
#endif
#else
bool skiptrigger = false;
#endif
// preset first event flag 
bool firstevent = true;

//
// static data member definitions
//

//
// constructors and destructor
//
// none

//////////////////////////////////////////////////////////////////////////////
//                        set analysis loop parameters                      //
//////////////////////////////////////////////////////////////////////////////

NanoAnalyzer::NanoAnalyzer(const edm::ParameterSet& iConfig)
{
  // configure parameters
  outFile = iConfig.getParameter<std::string>("outFile");
  // check infile and set isData to true if it does not contain "SIM"
  isData = iConfig.getParameter<bool>("isData");
  hlt_proc = iConfig.getParameter<std::string>("hltProcess");
  nanoext = iConfig.getParameter<bool>("nanoExtension");
  covout = iConfig.getParameter<bool>("writeCovariance");
  custom_flag = iConfig.getParameter<std::vector<std::string> >("customFlag");
  if (!custom_flag.empty())
    custom_tag = iConfig.getParameter<edm::InputTag>("customTag");
  // use hlt_proc above
  //processName_ = iConfig.getParameter<std::string>("processName"); //Qun
  triggerName_ = iConfig.getParameter<std::string>("triggerName"); //Qun

#ifdef charm
  // Make sure uncertainty in histogram bins is calculated correctly
  TH1::SetDefaultSumw2(true);
#endif

  // https://github.com/cms-sw/cmssw/blob/CMSSW_9_4_X/PhysicsTools/NanoAOD/python/electrons_cff.py#L100
  // https://github.com/cms-sw/cmssw/blob/CMSSW_9_4_X/PhysicsTools/NanoAOD/plugins/IsoValueMapProducer.cc#L49
  //relative_ = iConfig.getParameter<bool>("relative");
  
  // get actual CMSSW version 
  // however being that the branch above saves int, strip out all non-digits from the output
  // FIXME this ignores the ifdef flag (likely not an issue)
  CMSSW = 0;

  // get the actual CMSSW that is used
  // the current approach ignores patch and pre releases
  std::string cmssw_ver = edm::getReleaseVersion();
  std::vector<size_t> usc(4, 0);
  for (uint iU = 0; iU < usc.size(); ++iU)
    usc.at(iU) = cmssw_ver.find("_", (iU == 0) ? iU : usc.at(iU - 1) + 1);
#ifdef CMSSW42X
  CMSSW = 10000 * std::atoi(cmssw_ver.substr(usc.at(0) + 1, usc.at(1) - usc.at(0) - 1).c_str()) + 
    100 * std::atoi(cmssw_ver.substr(usc.at(1) + 1, usc.at(2) - usc.at(1) - 1).c_str()) + 
    std::atoi(cmssw_ver.substr(usc.at(2) + 1, (usc.at(3) == std::string::npos) ? cmssw_ver.size() - usc.at(2) : usc.at(3) - usc.at(2) - 1).c_str());
#else
  CMSSW = 10000 * std::stoi(cmssw_ver.substr(usc.at(0) + 1, usc.at(1) - usc.at(0) - 1)) + 
    100 * std::stoi(cmssw_ver.substr(usc.at(1) + 1, usc.at(2) - usc.at(1) - 1)) + 
    std::stoi(cmssw_ver.substr(usc.at(2) + 1, (usc.at(3) == std::string::npos) ? cmssw_ver.size() - usc.at(2) : usc.at(3) - usc.at(2) - 1));
#endif

  if (CMSSW == 0) {
    std::cout << "*** ALARM ***: NanoAnalyzer: proper CMSSW ifdef flag not set" << std::endl;
    exit(1);
  }
  else {
    std::cout << "CMSSW flag set to CMSSW" << CMSSW << std::endl;   
  }

  // for debug: get and print some info on map
  //std::cout << "max_size = " << hlt_bit.max_size() << std::endl;
  //std::cout << "max_bucket_count = " << hlt_bit.max_bucket_count() << std::endl;
  //std::cout << "max_load_factor = " << hlt_bit.max_load_factor() << std::endl;


  // initialize GenParticle extensions

  GenPV_x = -999.;
  GenPV_y = -999.;
  GenPV_z = -999.;
  GenPV_recIdx = -1;
  GenPV_chmult = 0;

  dcyvtxx = 0.; dcyvtxy = 0.; dcyvtxz = 0.;
  motx = 0.; moty = 0.; motz = 0.; 
  // Josry prompt/nonprompt Flag extension
  promptFlag = -1;

  // initialize nonstandard Trigger variables
#include "NanoTriggerInit.h" 


#ifdef CMSSW7plus
// *********************************
// consumes initialization for Run 2
// *********************************
#ifndef miniAOD
  // ***************
  // AOD collections
  // ***************
  muTkn = consumes<reco::MuonCollection>(edm::InputTag("muons"));
  gmuTkn = consumes<reco::TrackCollection>(edm::InputTag("globalMuons"));
  trkTkn = consumes<reco::TrackCollection>(edm::InputTag("generalTracks"));
  beamTkn = consumes<reco::BeamSpot>(edm::InputTag("offlineBeamSpot"));
  eTkn = consumes<reco::GsfElectronCollection>(edm::InputTag("gedGsfElectrons"));
  photTkn = consumes<reco::PhotonCollection>(edm::InputTag("photons"));
  tauTkn = consumes<reco::PFTauCollection>(edm::InputTag("hpsPFTauProducer"));
  genTkn = consumes<reco::GenParticleCollection>(edm::InputTag("genParticles"));
  pfmetTkn = consumes<reco::PFMETCollection>(edm::InputTag("pfMet"));
  // pfmetTkn = consumes< std::vector<reco::PFMET> >(edm::InputTag("pfMet"));
  calometTkn = consumes<reco::CaloMETCollection>(edm::InputTag("caloMet"));
  // calometTkn = consumes< std::vector<reco::CaloMET> >(edm::InputTag("caloMet"));
  // muCorrmetTkn = consumes< std::vector<reco::CaloMET> >(edm::InputTag("corMetGlobalMuons"));
  // nuha: input tag for conversion collection
  hConvTkn = consumes<reco::ConversionCollection>(edm::InputTag("allConversions"));
  
#ifdef Compatibility
  // official nanoAOD uses primary vertex without beam spot constraint
  // (being changed for Run 3)
  primvtxTkn = consumes<reco::VertexCollection>(edm::InputTag("offlinePrimaryVertices"));
#endif
#ifndef Compatibility
  // to get primary vertices with beam spot constraint 
  primvtxTkn = consumes<reco::VertexCollection>(edm::InputTag("offlinePrimaryVerticesWithBS"));
#endif
  // all Run 2 and 3
  // should use ak4PFJetsCHS in miniAOD/nanoAOD Compatibility mode
  pfjetTkn = consumes<reco::PFJetCollection>(edm::InputTag("ak4PFJets"));
  genjetTkn = consumes<reco::GenJetCollection>(edm::InputTag("ak4GenJets"));//Qun
  // jet correction label, will this work? doesn't so far ...
  //mJetCorr = "ak4PFL1FastL2L3Residual";

#ifndef CMSSW11plus
  // not Run 3
  // should use ak8PFJetsCHS in miniAOD/nanoAOD Compatibility mode
#ifdef CMSSW106plus
  // ultra-legacy
  //pffatjetTkn = consumes<reco::PFJetCollection>(edm::InputTag("ak8PFJets"));
  //trackjetTkn = consumes<reco::TrackJetCollection>(edm::InputTag("ak5TrackJets"));
  pffatjetTkn = consumes<reco::PFJetCollection>(edm::InputTag("ak8PFJetsCHS"));
  trackjetTkn = consumes<reco::TrackJetCollection>(edm::InputTag("ak4TrackJets"));
#elif defined CMSSW7XX
  // 2015
  // ak8 jets and track jets do not yet/no longer exist in CMSSW_7_6_X (or 7_5 MC)? use ak4
  pffatjetTkn = consumes<reco::PFJetCollection>(edm::InputTag("ak4PFJets"));
#else
  // pre_UL Run 2016-2018
  // ak8 jets and track jets do not yet/no longer exist in CMSSW_10_2_X? use ak4
  pffatjetTkn = consumes<reco::PFJetCollection>(edm::InputTag("ak4PFJets"));
  //trackjetTkn = consumes<reco::TrackJetCollection>(edm::InputTag("ak5TrackJets"));
#endif
#else
  // Run 3
  // does not seem to exist on Run 3 test MC set?
  //pffatjetTkn = consumes<reco::PFJetCollection>(edm::InputTag("ak8PFJets"));
  //pffatjetTkn = consumes<reco::PFJetCollection>(edm::InputTag("ak8PFJetsCHS"));
  //trackjetTkn = consumes<reco::TrackJetCollection>(edm::InputTag("ak5TrackJets"));
  // just duplicate ak4jets for the time being 
  pffatjetTkn = consumes<reco::PFJetCollection>(edm::InputTag("ak4PFJets"));
  // and don't use track jets at all
  //trackjetTkn = consumes<reco::TrackJetCollection>(edm::InputTag("ak5TrackJets"));
#endif

  // dEdx (from example M. Soares)
  //dedxMapStripTag_ = consumes<edm::ValueMap<reco::DeDxData>> (iConfig.getParameter<edm::InputTag>("dedxHarmonic2"));
  //dedxMapPixelTag_ = consumes<edm::ValueMap<reco::DeDxData>> (iConfig.getParameter<edm::InputTag>("dedxPixelHarmonic2"));
  dedxMapStripTag_ = consumes<edm::ValueMap<reco::DeDxData>>(edm::InputTag("dedxHarmonic2"));
  dedxMapPixelTag_ = consumes<edm::ValueMap<reco::DeDxData>>(edm::InputTag("dedxPixelHarmonic2"));
  // dEdx (from example M. Soares)
  //dedxMapStripTag_(consumes<edm::ValueMap<reco::DeDxData>> (iConfig.getParameter<edm::InputTag>("dedxHarmonic2")));
  //dedxMapPixelTag_(consumes<edm::ValueMap<reco::DeDxData>> (iConfig.getParameter<edm::InputTag>("dedxPixelHarmonic2")));
#endif

#ifdef miniAOD
  // miniAOD collections
  muTkn = consumes<pat::MuonCollection>(edm::InputTag("slimmedMuons"));
  trkTkn = consumes<pat::PackedCandidateCollection>(edm::InputTag("packedPFCandidates"));
  trkTkndisc = consumes<pat::PackedCandidateCollection>(edm::InputTag("packedPFCandidatesDiscarded"));
  trkTknlost = consumes<pat::PackedCandidateCollection>(edm::InputTag("lostTracks"));
  beamTkn = consumes<reco::BeamSpot>(edm::InputTag("offlineBeamSpot"));
  eTkn = consumes<pat::ElectronCollection>(edm::InputTag("slimmedElectrons"));
  photTkn = consumes<pat::PhotonCollection>(edm::InputTag("slimmedPhotons"));
  tauTkn = consumes<pat::TauCollection>(edm::InputTag("slimmedTaus"));
  genTkn = consumes<reco::GenParticleCollection>(edm::InputTag("prunedGenParticles"));
  pfjetTkn = consumes<pat::JetCollection>(edm::InputTag("slimmedJets"));
  genjetTkn = consumes<reco::GenJetCollection>(edm::InputTag("slimmedGenJets"));//Qun
  // actually, the slimmedJets refer to AOD   
  pffatjetTkn = consumes<pat::JetCollection>(edm::InputTag("slimmedJetsAK8"));

  // need to insert something here for fatjets and trackjets
  //pffatjetTkn = consumes<pat::JetCollection>(edm::InputTag("slimmedJets"));
  //pffatjetTkn = consumes<pat::JetCollection>(edm::InputTag("slimmedJetsAK8"));
  //trackjetTkn = consumes<pat::TrackJetCollection>(edm::InputTag("slimmedTrackJets"));
  pfmetTkn = consumes<pat::METCollection>(edm::InputTag("slimmedMETs"));
  //  calo MET actually also available via slimmedMETs
  calometTkn = consumes<pat::METCollection>(edm::InputTag("slimmedMETs"));
  primvtxTkn = consumes<reco::VertexCollection>(edm::InputTag("offlineSlimmedPrimaryVertices"));
#endif

  // trigger and flags for both AOD and miniAOD  *** fix duplication ***
  trigTkn = consumes< edm::TriggerResults>(edm::InputTag("TriggerResults","","HLT"));
  trigEvn = consumes< trigger::TriggerEvent>(edm::InputTag("hltTriggerSummaryAOD", "", "HLT")); //Qun
  if (!hlt_proc.empty())
    trig_tkn = consumes<edm::TriggerResults>(edm::InputTag("TriggerResults", "", hlt_proc));
  if (!custom_flag.empty())
    custom_tkn = consumes<edm::TriggerResults>(custom_tag);

  // CMSSWplus 
#endif

  //cout << "init" << endl;

} // end of constructor

NanoAnalyzer::~NanoAnalyzer()
{
  // do anything here that needs to be done at destruction time
  // (e.g. close files, deallocate resources etc.)
}

//
// member functions
//

///////////////////////////////////////////////////////////////////////////////
////////////// main analysis loop: method called for each event ///////////////
///////////////////////////////////////////////////////////////////////////////

void
NanoAnalyzer::analyze(const edm::Event& iEvent, const edm::EventSetup& iSetup)
{
  // using namespace edm;  // already used above
  // using namespace reco;
  // using namespace std;
  using namespace muon; // for TMOneStationTight
  using namespace trigger; // for trigger
  
//////////////////////////////////////////////////////////////////////////////
///////////////////////// load relevant event information ////////////////////
//////////////////////////////////////////////////////////////////////////////

#ifndef miniAOD
  // AOD collections
  //Handle<reco::GenParticleCollection> genParticles;
  //Handle<reco::BeamSpot> beamSpotHandle;
  //Handle<reco::VertexCollection> Primvertex;
  Handle<reco::TrackCollection> tracks;
  Handle<reco::TrackCollection> gmuons;
  Handle<reco::MuonCollection> muons;
  Handle<reco::GsfElectronCollection> electrons;
  Handle<reco::PhotonCollection> photons;
  Handle<reco::PFTauCollection> taus;
  Handle<reco::CaloMETCollection> calomet;
  Handle<reco::PFMETCollection> met;
  Handle<reco::PFJetCollection> jets;
  Handle<reco::PFJetCollection> fatjets;
  // track jets are available for 4_2, 5_3 and 10_6, but not for others
#ifndef CMSSW11plus
#ifdef CMSSW106plus
  Handle<reco::TrackJetCollection> trackjets;
#endif
#ifdef CMSSW53X
  Handle<reco::TrackJetCollection> trackjets;
#endif
#ifdef CMSSW42X
  Handle<reco::TrackJetCollection> trackjets;
#endif
#endif
  // dEdx (from example Giacomo Fedi)
  //Handle<reco::DeDxDataValueMap> energyLossHandle;
  //edm:: Handle<edm::ValueMap<reco::DeDxData>> energyLossHandle;
  Handle<DeDxDataValueMap> energyLossHandle;
  // somehow none of these work
  //DeDxDataValueMap &  eloss  = *energyLossHandle;
  //const DeDxDataValueMap &  eloss  = *energyLossHandle;
  //const DeDxDataValueMap &  eloss  = *energyLossHandle.product();
  //const DeDxDataValueMap eloss  = *energyLossHandle;
  // nuha
  Handle<reco::ConversionCollection> hConversions;
#endif

#ifdef miniAOD
  // miniAOD collections
  //Handle<reco::GenParticleCollection> genParticles;
  //Handle<reco::BeamSpot> beamSpotHandle;
  //Handle<reco::VertexCollection> Primvertex;
  Handle<pat::PackedCandidateCollection> tracks;
  Handle<pat::PackedCandidateCollection> discTracks;
  Handle<pat::PackedCandidateCollection> lostTracks;
  // Handle<reco::TrackCollection> gmuons;
  Handle<pat::MuonCollection> muons;
  Handle<pat::ElectronCollection> electrons;
  Handle<pat::PhotonCollection> photons;
  Handle<pat::TauCollection> taus;
  Handle<pat::METCollection> calomet;
  Handle<pat::METCollection> met;
  Handle<pat::JetCollection> jets;  
  Handle<pat::JetCollection> fatjets;  
  //Handle<pat::TrackJetCollection> trackjets;  
#endif

  // common AOD and miniAOD collections
  edm::Handle<reco::GenParticleCollection> genParticles;
  edm::Handle<reco::BeamSpot> beamSpotHandle;
  edm::Handle<reco::VertexCollection> Primvertex;
  edm::Handle<reco::GenJetCollection> genjets;//Qun
  // for trigger and flags
  edm::Handle<edm::TriggerResults> trigger_handle;
  edm::Handle<edm::TriggerResults> custom_handle;

  // cout << "hello get event" << endl; 

#ifndef CMSSW7plus
  // for Run 1, use access via getByLabel
  iEvent.getByLabel("muons", muons);
  iEvent.getByLabel("globalMuons", gmuons);
  iEvent.getByLabel("generalTracks", tracks); 
  iEvent.getByLabel("offlineBeamSpot", beamSpotHandle);
  iEvent.getByLabel("gsfElectrons", electrons);
  iEvent.getByLabel("photons", photons);
  iEvent.getByLabel("hpsPFTauProducer", taus);
  iEvent.getByLabel("pfMet", met);
  //iEvent.getByLabel("caloMet", calomet);
  iEvent.getByLabel("met", calomet);
  iEvent.getByLabel("ak5PFJets", jets);
  iEvent.getByLabel("ak7PFJets", fatjets);
  iEvent.getByLabel("ak5TrackJets", trackjets);
  iEvent.getByLabel("ak5GenJets", genjets);//Qun

  // jet correction label
//  mJetCorr = "ak5CaloL2L3";
//  mJetCorr_ak5 = "jetCorr_ak5";

#ifdef CMSSW42X
  // for 2010
  mJetCorr = "ak5PFL2L3";
#else
  // for 2011/12
  mJetCorr = "ak5PFL1FastL2L3Residual";
#endif

  // choose primary vertices with/without beam spot
  // see https://twiki.cern.ch/twiki/bin/view/CMSPublic/SWGuideOfflinePrimaryVertexProduction
#ifndef Compatibility
  // for best performance (assumes beam spot is well simulated by MC)
  iEvent.getByLabel("offlinePrimaryVerticesWithBS",Primvertex);
#endif
#ifdef Compatibility
  // for compatibility with official nanoAOD
  iEvent.getByLabel("offlinePrimaryVertices",Primvertex);
#endif

#ifdef charm
  // dEdx (from example G. Fedi)
  int dedxexist = 1;
  // exists!
  if (!iEvent.getByLabel("dedxHarmonic2", energyLossHandle)) dedxexist=0;
  // does not exist for Run 1
  //if (!iEvent.getByLabel("dedxPixelHarmonic2", energyLossHandle)) dedxexist=0;
#endif

  // trigger and flags (Afiq)
  if (!skiptrigger)
    iEvent.getByLabel(edm::InputTag("TriggerResults", "", hlt_proc), trigger_handle);
  if (!custom_flag.empty())
    iEvent.getByLabel(custom_tag, custom_handle);
  // not CMSSW7plus
#endif

#ifdef CMSSW7plus 
  // for Run 2, use access via getByToken
#ifndef miniAOD
  // AOD collections
  iEvent.getByToken(muTkn, muons);
  iEvent.getByToken(gmuTkn, gmuons);
  iEvent.getByToken(trkTkn, tracks);
  iEvent.getByToken(eTkn, electrons);
  iEvent.getByToken(photTkn, photons);
  iEvent.getByToken(tauTkn, taus);
  iEvent.getByToken(genTkn, genParticles);
  iEvent.getByToken(beamTkn, beamSpotHandle);
  iEvent.getByToken(pfmetTkn, met);
  iEvent.getByToken(calometTkn, calomet);
  iEvent.getByToken(primvtxTkn, Primvertex);
  // iEvent.getByToken(muCorrmetTkn, muCorrmets);
  iEvent.getByToken(pfjetTkn, jets);
  iEvent.getByToken(genjetTkn, genjets);//Qun
  iEvent.getByToken(pffatjetTkn, fatjets);
#ifndef CMSSW11plus
#ifdef CMSSW106plus
  iEvent.getByToken(trackjetTkn, trackjets);
#endif
#endif
  // trigger
  iEvent.getByToken(trigTkn, triggerResultsHandle_);
  iEvent.getByToken(trigEvn, triggerEventHandle_); //Qun
  // nuha
  iEvent.getByToken(hConvTkn, hConversions);
  
#ifdef charm
  // dEdx (from example M. Soares)
  int stripmap = 1;
  edm::Handle<edm::ValueMap<reco::DeDxData>> dedxStMap;
  if (!iEvent.getByToken(dedxMapStripTag_,dedxStMap)) stripmap=0;  
  int pixmap = 1;
  edm::Handle<edm::ValueMap<reco::DeDxData>> dedxPixMap;
  if (!iEvent.getByToken(dedxMapPixelTag_,dedxPixMap)) pixmap=0;  
#endif
#endif

#ifdef miniAOD
  // miniAOD collections
  iEvent.getByToken(muTkn, muons);
  //  iEvent.getByToken(gmuTkn, gmuons);
  iEvent.getByToken(trkTkn, tracks);
  iEvent.getByToken(trkTkndisc, discTracks);
  iEvent.getByToken(trkTknlost, lostTracks);
  iEvent.getByToken(eTkn, electrons);
  iEvent.getByToken(photTkn, photons);
  iEvent.getByToken(tauTkn, taus);
  iEvent.getByToken(genTkn, genParticles);
  iEvent.getByToken(beamTkn, beamSpotHandle);
  iEvent.getByToken(pfmetTkn, met);
  iEvent.getByToken(calometTkn, calomet);
  iEvent.getByToken(primvtxTkn, Primvertex);
  // iEvent.getByToken(muCorrmetTkn, muCorrmets);
  iEvent.getByToken(pfjetTkn, jets);
  iEvent.getByToken(genjetTkn, genjets);//Qun
  // need to insert something here for fatjets and trackjets
  iEvent.getByToken(pffatjetTkn, fatjets);
  // iEvent.getByToken(trackjetTkn, trackjets);
  // trigger (*** fix duplication ***)
  iEvent.getByToken(trigTkn, triggerResultsHandle_);  
  iEvent.getByToken(trigEvn, triggerEventHandle_); //Qun
#endif

  // common collections
  if (!hlt_proc.empty())
    iEvent.getByToken(trig_tkn, trigger_handle);
  if (!custom_flag.empty())
    iEvent.getByToken(custom_tkn, custom_handle);

  // CMSSW7plus
#endif

  // cout << "hello get run/event info" << endl; 

// *************************************************************
//------------------ get run/event info ------------------------
// *************************************************************  

  run = (iEvent.id()).run();
  event = (iEvent.id()).event();
  luminosityBlock = (iEvent.id()).luminosityBlock();

// *************************************************************
//------------------ get/set JSON quality flag -----------------
// *************************************************************

  GoodLumisection = true;
#ifdef JSONcheck
  // the following automatically excludes MC
  if (run > 132000 && run < 150000) {
    // 2010 data only for the moment  
    //   (if Golden JSON is selected in configuration
    //    or #define JSON is not selected 
    //    this flag will always remain true)
    GoodLumisection = providesGoodLumisection(iEvent);
  }
#endif

#ifdef CMSSW42X
              // skip trigger for 2010 MC  (not available)
              if (run == 1) {
                if (!skiptrigger)    // on first event only 
		  std::cout << "Trigger information will be skipped" << std::endl;
                skiptrigger = true;
              }
#endif
	      // cout << "hello skip trigger " << run << " " << skiptrigger << endl; 
	      if (!skiptrigger) {
// *************************************************************
//------------------ get and check trigger info ----------------
// *************************************************************
//      sets hlt_bit.at pointers, i.e. whether trigger has fired or not

  //cout << "hello get trigger" << endl; 

  if (!hlt_proc.empty()) {
    //cout << "hello hlt_proc.empty" << endl; 
    for (unsigned iP = 0; iP < trigger_handle->size(); ++iP) {
      // get path name with version stripped off
      const std::string path = remove_version( iEvent.triggerNames(*trigger_handle).triggerName(iP) );
      // check whether path occurs in container (0 or 1)
      if (hlt_bit.count(path)) {
        // for debug 
        //cout << "hello accepted trigger path " << iP << " " << path << endl;
        // trigger_handle->accept returns 0 or 1,
        // .at refers to the value of the second element of the pair 
        hlt_bit.at(path) = trigger_handle->accept(iP);
      }
      else if (path.substr(0, 3) == "HLT")
        std::cout << "WARNING: HLT path " << path << " is not present in the bitmap, when it should be!! Check if the logic is "
          "properly implemented!" << std::endl;
      // for debug
      //else 
      //  cout << "hello *rejected* trigger path " << iP << " " << path << endl;
    }
  } // hlt_proc_empty
	      } // skiptrigger

              // initialize all dataset trigger flags to true
              // (for 2010 MC)
              // will be superseeded if trigger menue is present
              ZeroBiasTrig   = true;
              MinimumBiasTrig= true;
              MuTrig         = true;
              MuMonitorTrig  = true;
              MuOniaTrig     = true;
              ElectronTrig   = true;
              EGMonitorTrig  = true;
              BParkingTrig   = true;
              // ... add more!
              // initialize global trigger content flags to none set
              ZeroBiasFlag   = -10;
              MinBiasFlag    = -10;
              MinBiasMult    = -99;
              MuThresh       = -49;
              MuL1Thresh     = -49;
              MuL2Thresh     = -49;
              IsoMuThresh    = -49;
              DoubleMuThresh = -49;
              JpsiThresh     = -49;
              MuHadFlag      = -10;
              MuEGFlag       = -10;
              ElectronThresh = -49;
              DoubleElectronThresh = -49;
              PhotonThresh   = -49;
              JetThresh      = -199;
              DiJetThresh    = -199; 
              TriJetThresh   = -199; 
              QuadJetThresh  = -199; 
              HTThresh       = -199;
              BThresh        = -199;
              METThresh      = -199;

              if (skiptrigger) {
                // declare ZeroBias Trigger to be set (only trigger)
                ZeroBiasTrig = true;
                ZeroBiasFlag = 1;
                MCdataset = true;
                // all other flags remain unchanged ...
              }

	      // Trigger Object
	      nTrigObj = 0;                   // number of stored Trigger Obj
	      TrigObj_id.clear();
	      TrigObj_filterBits.clear();
	      TrigObj_pt.clear();
	      TrigObj_eta.clear();
	      TrigObj_phi.clear();

              //cout << "hello TrigObj" << endl;

              // if not to be skipped
              if (!skiptrigger) {
#ifndef CMSSW7plus
                // trigger, do not move up! 
                edm::InputTag trigResultsTag("TriggerResults","","HLT");
                iEvent.getByLabel(trigResultsTag,triggerResultsHandle_);
		//below  Qun 17-10-19 TriggerObj 
		//edm::InputTag triggerEventTag_;  
		//edm::InputTag("hltTriggerSummaryAOD", "", "HLT");//
		edm::InputTag triggerEventTag_("hltTriggerSummaryAOD", "", "HLT");  
		iEvent.getByLabel(triggerEventTag_,triggerEventHandle_);
#endif
		if (!triggerEventHandle_.isValid()) {
		  std::cout << "HLTEventAnalyzerAOD::analyze: Error in getting TriggerEvent product from Event!" << std::endl;
		  return;		  
		}
		if (triggerEventHandle_.isValid()) {
                  /*   reactivate this later!
		  cout << "Used Processname: " << triggerEventHandle_->usedProcessName() << endl;
		  const size_type nC(triggerEventHandle_->sizeCollections());
		  //cout << "Number of packed Collections: " << nC << endl;
		  //cout << "The Collections: #, tag, 1-past-end index" << endl;
		  for (size_type iC=0; iC!=nC; ++iC) {
     		    cout << iC << " "
	    	    << triggerEventHandle_->collectionTag(iC).encode() << " "
	            << triggerEventHandle_->collectionKey(iC) << endl;
     		  }
                  */
		  const size_type nO(triggerEventHandle_->sizeObjects());
		  //cout << "Number of TriggerObjects: " << nO << endl;
		  //cout << "The TriggerObjects: #, id, pt, eta, phi, mass" << endl;
		  const TriggerObjectCollection& TOC(triggerEventHandle_->getObjects());
		  // wrong cout << "Number of TriggerObjects ID: " << TOC.pt.size() << endl;
		  size_type nTO_QQ=0;
		  for (size_type iO=0; iO!=nO; ++iO) {
		    const TriggerObject& TO(TOC[iO]);
		    //cout << iO << " " << TO.id() << " " << TO.pt() << " " << TO.eta() << " " << TO.phi() << " " << TO.mass() << endl;
                    // Ids indicate kind of trigger object, e.g. +-13 = muon
		    if (TO.id()!=0 && fabs(TO.id())<60) {
                      if (nTO_QQ < nReserve_TrigObj) {
		        TrigObj_id.push_back(TO.id());
		        TrigObj_filterBits.push_back(-1);
		        TrigObj_pt.push_back(TO.pt());
		        TrigObj_eta.push_back(TO.eta());
		        TrigObj_phi.push_back(TO.phi());
		        nTO_QQ++; 
		        //cout << "The TriggerObjects: #, id, pt, eta, phi, mass of TrigObj " << TrigObj_id[nTO_QQ-1] <<  " " << TrigObj_pt[nTO_QQ-1] <<  " " <<TrigObj_eta[nTO_QQ-1] <<  " " << TrigObj_phi[nTO_QQ-1] << endl;
		      } 
                      else std::cout << "*** nReserve_TrigObj exceeded. n = " << nTO_QQ << std::endl;    
		    }    
		    //cout << "The TriggerObjects: #, id, pt, eta, phi, mass of TrigObj" << TrigObj_id[nTO_QQ-1] <<  " " << TrigObj_pt[nTO_QQ-1] <<  " " <<TrigObj_eta[nTO_QQ-1] <<  " " << TrigObj_phi[nTO_QQ-1] << endl;
		    //cout << "The TriggerObjects: #, id, pt, eta, phi, mass of TrigObj" << TrigObj_id[iO] <<  " " << TrigObj_pt[iO] <<  " " <<TrigObj_eta[iO] <<  " " << TrigObj_phi[iO] << endl;
		  }
		  nTrigObj=nTO_QQ;
		  //cout << "Number of TriggerObjects QQ with ID!=0 : " << nTO_QQ << endl;
                  /*
                  // for debug:
                  if (nTrigObj>0) {
		    const size_type nC(triggerEventHandle_->sizeCollections());
		    //cout << "Number of packed Collections: " << nC << endl;
		    //cout << "The Collections: #, tag, 1-past-end index" << endl;
            	    for (size_type iC=0; iC!=nC; ++iC) {
     		      cout << iC << " "
	    	      << triggerEventHandle_->collectionTag(iC).encode() << " "
	              << triggerEventHandle_->collectionKey(iC) << endl;
     		    }
                  }
                  */
                  // some relevant collection Tags,                   Key
                  // Electrons, 2011 MC:
                  //    hltL1IsoRecoEcalCandidate::HLT                1
                  //    hltL1NonIsoRecoEcalCandidate::HLT             1
                  //    hltRecoEcalSuperClusterActivityCandidate::HLT 0,2 or 4
                  //    hltPixelMatchElectronsL1Iso::HLT              3,5 or 6
                  //    hltPixelMatchElectronsL1NonIso::HLT           3,5 or 6
                  //    hltPixelMatch3HitElectronsL1Iso::HLT          4 or 5
                  //    hltPixelMatch3HitElectronsL1NonIso::HLT       4 or 5     
                  // Muons, 2011 MC:
                  //    hltL2MuonCandidates::HLT                      1 or 2
                  //    hltL3MuonCandidates::HLT                      2 or 3
                  //    hltMuTrackJpsiPixelTrackCands::HLT            2

		  //Qconst size_type nF(triggerEventHandle_->sizeFilters());
		  //Qcout << "Number of TriggerFilters: " << nF << endl;
		  //Qcout << "The Filters: #, tag, #ids/#keys, the id/key pairs" << endl;
		  //Qfor (size_type iF=0; iF!=nF; ++iF) {
		  //Q  const Vids& VIDS (triggerEventHandle_->filterIds(iF));
		  //Q  const Keys& KEYS(triggerEventHandle_->filterKeys(iF));
		  //Q  const size_type nI(VIDS.size());
		  //Q  const size_type nK(KEYS.size());
		  //Q  cout << iF << " " << triggerEventHandle_->filterTag(iF).encode()
		  //Q       << " " << nI << "/" << nK
		  //Q       << " the pairs: ";
		  //Q  const size_type n(max(nI,nK));
		  //Q  for (size_type i=0; i!=n; ++i) {
		  //Q    cout << " " << VIDS[i] << "/" << KEYS[i];
		  //Q  }
		  //Q  cout << endl;
		  //Q  assert (nI==nK);
		  //Q}
		}
	        //const TriggerObjectCollection& TOC(triggerEventHandle_->getObjects());

                const edm::TriggerNames& trigNames = iEvent.triggerNames(*triggerResultsHandle_); 
                //cout << "test hlt QQQQQQQQQQQQQQQQ " << hltConfig_.size() <<endl; //Qun

                // check whether trigger info was obtained successfully
                if (!triggerResultsHandle_.isValid()) {
		  std::cout << "Nano::analyze: Error in getting TriggerResults product from Event!" << std::endl;
                  // should always be available for data, but not necessarily 
                  // for e.g. 2010 MC
                  if (run > 1) {
                    // stop the program
                    exit (1);
                  }
                  else {
                    // do not ask for trigger info from here onwards
                    skiptrigger = true;
                  }
                }

		// below Qun Trigger Object
		using namespace reco;
		using namespace trigger;

		assert(triggerResultsHandle_->size()==hltConfig_.size()); //Qun
		//cout << " triggerResultsHandle_->size()==hltConfig_.size() QQQQ" << endl;
		//const unsigned int n(hltConfig_.size());
		//Get the trigger index for the current trigger
		const unsigned int triggerIndex(hltConfig_.triggerIndex(triggerName_));
		//cout << "before QQQQQ n: " << n << " QQQQQ triggerIndex: " << triggerIndex << endl;
		//check that the trigger in the event and in the configuration agree
		assert(triggerIndex==iEvent.triggerNames(*triggerResultsHandle_).triggerIndex(triggerName_));
		//cout << "after  QQQQQ n: " << n << " QQQQQ triggerIndex: " << triggerIndex << endl;
		//Qif (triggerIndex>=n) {
		//Q  cout << "HLTEventAnalyzerAOD::analyzeTrigger: path "
		//Q       << triggerName_ << " - not found!" << endl;
		//Q  return;
		//Q}
		//Qelse {cout << "HLTEventAnalyzerAOD QQQQQ " << triggerName_ << endl;}
		//Qif (triggerName_=="@") {
		//Q  const unsigned int n(hltConfig_.size());
		//Q  cout << "analyzerTrigger testA  " << n << endl;
		//Q  for (unsigned int i=0; i!=n; ++i) {
		//Q    analyzeTrigger(iEvent,iSetup,hltConfig_.triggerName(i));
		//Q  //cout << "analyzerTrigger testB  " << i << endl;
		//Q  }
		//Q} else {
		//Q  analyzeTrigger(iEvent,iSetup,triggerName_);
		//Q  //cout << "analyzerTrigger testC  " << n << endl;
		//Q}
		// Qun above Trgger Object

               // if not to be skipped (duplicate on purpose!)
               if (!skiptrigger) {

//  Here: check for "good" minimum bias, jet, or muon or electron trigger
//  separate calls from subsequent if statement in order not to make them order-dependent
//  always check all triggers on all datasets
                GoodMinBiasTrigger = providesGoodMinBiasTrigger(iEvent);
                GoodJetTrigger     = providesGoodJetTrigger(iEvent);
                GoodMuTrigger      = providesGoodMuTrigger(iEvent);
                GoodETrigger       = providesGoodETrigger(iEvent);

                //// for debug, dump all triggers of menu
                //      for (unsigned i = 0; i<trigNames.size(); i++) {
                //      // dump only accepted triggers 
		//	//                        if (triggerResultsHandle_->accept(i)==1){
                //          std::cout<<" Trigger name "<<trigNames.triggerName(i)<<", Accepted = ";
                //          std::cout<<triggerResultsHandle_->accept(i)<<std::endl;
		//	  //}
                //      }


// datasets from which special trigger variables are being treated are so far
//   (* = still to be implemented)
//
//   Commissioning10: (7 TeV and 900 GeV): Zerobias and MinimumBias 
//
//      2010A       2010B         2011A (add B*)    2012B/C
//      ZeroBias   (Zerobias)
//      MinimumBias MinimumBias   MinimumBias       MinimumBias*     <-- next
//   Commissioning* Commissioning                   Commissioning*
//      Mu          Mu          ( SingleMu        ( SingleMu
//                              ( DoubleMu        ( DoubleMuParked
//                              ( MuHad           ( MuHad*
//                              ( MuEG            ( MuEG*
//      MuMonitor   MuMonitor
//      MuOnia      MuOnia        MuOnia* <-- next  MuoniaParked*
//      EG          Electron    ( SingleElectron* ( SingleElectron*
//                              ( DoubleElectron  ( DoubleElectron*  
//		                ( ElectronHad*    ( ElectronHad*
//      EGmonitor   EGMonitor
//          next--> Photon*     ( Photon*        (( SinglePhoton*
//                                               (( DoublePhoton*
//                                               (( DoublePhotonHighPt*
//                              ( PhotonHad*      ( PhotonHad*
//      JeTMETTau*
//      BTau*       BTau        ( BTag*          (( BTag*
//                                               (( BJetPlusX*
//                              ( Tau*            ( TauParked*
//                              ( TauPlusX*       ( TauPlusX
//      JetMET*     Jet         ( Jet            (( Jet*
//      next--^                                  (( JetHT*
//                                               (( JetMon* 
//                              ( HT*             ( HTMHTParked*
//JetMETTauMonitor* JetMETTauMonitor
//                  MultiJet      MultiJet*
//                  METFwd*     ( MET*            ( MET*
//                              ( METBTag* 
//                                                  HcalNZS*
//                                                  NoBPTX*
//                                                  VBF1Parked*
//
//     also: 2015E MinimumBias (5 TeV)
//           2016  ZeroBias (13 TeV)
//           2018  B Parking* (13 TeV)
//
                  // sharpen/check dataset info if not yet unique
                  if (!datasetisunique) {
                    // dataset is not yet unique 
		    std::cout<<" dataset is not yet unique, Triggers:"<<std::endl;
                    // dump the trigger info
                      for (unsigned i = 0; i<trigNames.size(); ++i) {
                      // dump only accepted triggers 
                        if (triggerResultsHandle_->accept(i)==1){
                          std::cout<<" Trigger name "<<trigNames.triggerName(i)<<", Accepted = ";
                          std::cout<<triggerResultsHandle_->accept(i)<<std::endl;
	                }
                      }
                    if (isData) MCdataset = false;
                    if (!ZeroBiasTrig) ZeroBiasdataset = false;
                    if (!MinimumBiasTrig) MinimumBiasdataset = false;
                    if (!CommissioningTrig) Commissioningdataset = false;
                    if (!MuTrig) Mudataset = false;
                    if (!MuHadTrig) MuHaddataset = false;
                    if (!DoubleMuTrig) DoubleMudataset = false;
                    if (!MuEGTrig) MuEGdataset = false;
                    if (!ElectronTrig) Electrondataset = false;
                    if (!DoubleElectronTrig) DoubleElectrondataset = false;
                    if (!PhotonTrig) Photondataset = false;
                    if (!MuOniaTrig) MuOniadataset = false;
                    if (!CharmoniumTrig) Charmoniumdataset = false;
		    if (!MuMonitorTrig) MuMonitordataset = false;
                    if (!EGMonitorTrig) EGMonitordataset = false;
                    if (!JetTrig) Jetdataset = false;
                    if (!MultiJetTrig) MultiJetdataset = false;
                    if (!JetMETTauMonitorTrig) JetMETTauMonitordataset = false;
                    if (!BTauTrig) BTaudataset = false;
                    if (!BParkingTrig) BParkingdataset = false;
                    if (!METFwdTrig) METFwddataset = false;
                    int ndataset = 0;
                    if (MCdataset) ++ndataset;
                    if (ZeroBiasdataset) ++ndataset;
                    if (MinimumBiasdataset) ++ndataset;
                    if (Commissioningdataset) ++ndataset;
                    if (Mudataset) ++ndataset;
                    if (MuHaddataset) ++ndataset;
                    if (DoubleMudataset) ++ndataset;
                    if (MuEGdataset) ++ndataset;
                    if (Electrondataset) ++ndataset;
                    if (DoubleElectrondataset) ++ndataset;
                    if (Photondataset) ++ndataset;
                    if (MuOniadataset) ++ndataset;
                    if (Charmoniumdataset) ++ndataset;
                    if (MuMonitordataset) ++ndataset;
                    if (EGMonitordataset) ++ndataset;
                    if (Jetdataset) ++ndataset;
                    if (MultiJetdataset) ++ndataset;
                    if (JetMETTauMonitordataset) ++ndataset;
                    if (BTaudataset) ++ndataset;
                    if (BParkingdataset) ++ndataset;
                    if (METFwddataset) ++ndataset;
		    std::cout<<" dataset not yet unique, "<<ndataset<<" choices."<<std::endl;
		    std::cout<<" MC= "<<MCdataset<<std::endl;
		    std::cout<<" MinimumBias= "<<MinimumBiasdataset<<std::endl;
		    std::cout<<" ZeroBias= "<<ZeroBiasdataset<<std::endl;
		    std::cout<<" Commissioning= "<<Commissioningdataset<<std::endl;
		    std::cout<<" Mu= "<<Mudataset<<std::endl;
		    std::cout<<" MuHad= "<<MuHaddataset<<std::endl;
		    std::cout<<" DoubleMu= "<<DoubleMudataset<<std::endl;
		    std::cout<<" MuEG= "<<MuEGdataset<<std::endl;
		    std::cout<<" Electron= "<<Electrondataset<<std::endl;
		    std::cout<<" DoubleElectron= "<<DoubleElectrondataset<<std::endl;
		    std::cout<<" Photon= "<<Photondataset<<std::endl;
		    std::cout<<" MuOnia= "<<MuOniadataset<<std::endl;
		    std::cout<<" Charmonium= "<<Charmoniumdataset<<std::endl;
		    std::cout<<" MuMonitor= "<<MuMonitordataset<<std::endl;
		    std::cout<<" EGMonitor= "<<EGMonitordataset<<std::endl;
		    std::cout<<" Jet= "<<Jetdataset<<std::endl;
		    std::cout<<" MultiJet= "<<MultiJetdataset<<std::endl;
		    std::cout<<" JetMETTauMonitor= "<<JetMETTauMonitordataset<<std::endl;
		    std::cout<<" BTau= "<<BTaudataset<<std::endl;
		    std::cout<<" BParking= "<<BParkingdataset<<std::endl;
		    std::cout<<" METFwd= "<<METFwddataset<<std::endl;

                    if (MCdataset) {
                      datasetisunique = true;
                      dataset = "MC";
         	      std::cout<<" dataset now unique: "<<dataset<<std::endl;
                    }
                    else if (ndataset==1) {
		      // dataset is now unique!
                      datasetisunique = true;
                      if (ZeroBiasdataset) dataset = "ZeroBias";
                      if (MinimumBiasdataset) dataset = "MinimumBias";
                      if (Commissioningdataset) dataset = "Commissioning";
                      if (Mudataset) dataset = "Mu";
                      if (MuHaddataset) dataset = "MuHad";
                      if (DoubleMudataset) dataset = "DoubleMu";
                      if (MuEGdataset) dataset = "MuEG";
                      if (Electrondataset) dataset = "Electron";
                      if (DoubleElectrondataset) dataset = "DoubleElectron";
                      if (Photondataset) dataset = "Photon";
                      if (MuOniadataset) dataset = "MuOnia";
                      if (Charmoniumdataset) dataset = "Charmonium";
                      if (MuMonitordataset) dataset = "MuMonitor";
                      if (EGMonitordataset) dataset = "EGMonitor";
                      if (Jetdataset) dataset = "Jet";
                      if (MultiJetdataset) dataset = "MultiJet";
                      if (JetMETTauMonitordataset) dataset = "JetMETTauMonitor";
                      if (BTaudataset) dataset = "BTau";
                      if (BParkingdataset) dataset = "BParking";
                      if (METFwddataset) dataset = "METFwd";
         	      std::cout<<" dataset now unique: "<<dataset<<std::endl;
                    }
#ifndef trigcheckabort
                    else if (ndataset==0) {
                      std::cout<<" ******* no candidate dataset recognized for these triggers ******"<<std::endl;
                      // set dataset unique although it is not
                      datasetisunique = true;
                    }
#endif
#ifdef trigcheckabort
                    else if (ndataset==0) {
  	              // trigger info doesn't match any known dataset, dump trigger and abort
                      std::cout<<" ******* no candidate dataset recognized for these triggers ******"<<std::endl;
                      for (unsigned i = 0; i<trigNames.size(); ++i) {
                      // dump only accepted triggers 
                        if (triggerResultsHandle_->accept(i)==1){
                          std::cout<<" Trigger name "<<trigNames.triggerName(i)<<", Accepted = ";
                          std::cout<<triggerResultsHandle_->accept(i)<<std::endl;
	                }
                      }
                      // abort the job. This should never happen if trigger information from respective dataset
                      // is treated properly
                      // (trigger so far implemented for parts of Run 1 only)
                      exit(1);
                    }
#endif
                  } // !datasetisunique
		  else {
                    // data set is already unique. check consistency of trigger and dataset 
                    if ( dataset == "MC"       // no trigger requirement!
                     || (dataset == "ZeroBias" && ZeroBiasTrig)
                     || (dataset == "MinimumBias" && MinimumBiasTrig)
		     || (dataset == "Mu" && MuTrig)
		     || (dataset == "MuHad" && MuHadTrig)
		     || (dataset == "DoubleMu" && DoubleMuTrig)
		     || (dataset == "MuEG" && MuEGTrig)
		     || (dataset == "Electron" && ElectronTrig)
		     || (dataset == "DoubleElectron" && DoubleElectronTrig)
		     || (dataset == "Photon" && PhotonTrig)
		     || (dataset == "MuOnia" && MuOniaTrig)
		     || (dataset == "Charmonium" && CharmoniumTrig)
		     || (dataset == "MuMonitor" && MuMonitorTrig)
		     || (dataset == "EGMonitor" && EGMonitorTrig)
		     || (dataset == "Jet" && JetTrig)
		     || (dataset == "MultiJet" && MultiJetTrig)
		     || (dataset == "JetMETTauMonitor" && JetMETTauMonitorTrig)
		     || (dataset == "BTau" && BTauTrig)
		     || (dataset == "BParking" && BParkingTrig)
		     || (dataset == "METFwd" && METFwdTrig)
	             || (dataset == "Commissioning" && CommissioningTrig)) {} // do nothing
                    else {
  	              // trigger and dataset are not consistent, dump trigger and abort
                      if (firstevent == true) { 
		        std::cout<<" ****** Trigger not consistent with dataset "<<dataset<<" ****** "<<std::endl;
		        std::cout<<ZeroBiasTrig<<MinimumBiasTrig<<MuTrig<<MuHadTrig<<DoubleMuTrig<<MuEGTrig<<ElectronTrig<<DoubleElectronTrig<<PhotonTrig<<MuOniaTrig<<CharmoniumTrig<<MuMonitorTrig<<EGMonitorTrig<<JetTrig<<MultiJetTrig<<JetMETTauMonitorTrig<<BTauTrig<<BParkingTrig<<METFwdTrig<<CommissioningTrig<<std::endl;
                        for (unsigned i = 0; i<trigNames.size(); ++i) {
                        // dump only accepted triggers 
                          if (triggerResultsHandle_->accept(i)==1){
                            std::cout<<" Trigger name "<<trigNames.triggerName(i)<<", Accepted = ";
                            std::cout<<triggerResultsHandle_->accept(i)<<std::endl;
	                  }
                        }
                        firstevent = false;
#ifdef trigcheckabort
                        // abort the job. This should never happen if trigger information from respective dataset
                        // is treated properly
                        // trigger so far implemented for Run 1 only
                        exit(1);
#endif
                      }
                    } // dataset
                  } // datasetisunique  
                 } // skiptrigger
                } // skiptrigger



/////////////////////////////////////////////////////////////////////////////
////////////// and now proceed to analysis of physics content ///////////////
/////////////////////////////////////////////////////////////////////////////

	      // cout << "hello physics" << endl; 

//////////////////////////////////////////////////////////////////////////////
////////////////////////////// Gen Particle Start ////////////////////////////
//////////////////////////////////////////////////////////////////////////////

  GenPart_pt.clear();
  GenPart_eta.clear();
  GenPart_phi.clear();
  GenPart_mass.clear();
  GenPart_charge.clear();
  GenPart_pdgId.clear();
  GenPart_status.clear();
  GenPart_statusFlags.clear();
  GenPart_genPartIdxMother.clear();

  GenPart_Id.clear();
  GenPart_isNano.clear();
  GenPart_parpdgId.clear();
  GenPart_sparpdgId.clear();
  GenPart_numberOfDaughters.clear();
  GenPart_nstchgdaug.clear();
  //Josry prompt/nonprompt Flag extension
  GenPart_promptFlag.clear();
  GenPart_vx.clear();
  GenPart_vy.clear();
  GenPart_vz.clear();
  GenPart_mvx.clear();
  GenPart_mvy.clear();
  GenPart_mvz.clear();
  GenPart_recIdx.clear();

  TLorentzVector p4Kt, p4pit, p4D0t;
  p4Kt.SetPtEtaPhiE(0., 0., 0., 0.);
  p4pit.SetPtEtaPhiE(0., 0., 0., 0.);
  p4D0t.SetPtEtaPhiE(0., 0., 0., 0.);

/*   int nmusim =0;
  int idmusim[maxnmusim];
  float chgmusim[maxnmusim];
  float ptmusim[maxnmusim];
  float etamusim[maxnmusim];
  float phimusim[maxnmusim]; */

#ifdef charm
  int nD0sim =0;
  int idD0sim[maxnD0sim];
  float ptD0sim[maxnD0sim];
  float etaD0sim[maxnD0sim];
  float phiD0sim[maxnD0sim];
  int pdgIdD0t1sim[maxnD0sim];
  int chgD0t1sim[maxnD0sim];
  float ptD0t1sim[maxnD0sim];
  float etaD0t1sim[maxnD0sim];
  float phiD0t1sim[maxnD0sim];
  int pdgIdD0t2sim[maxnD0sim];
  int chgD0t2sim[maxnD0sim];
  float ptD0t2sim[maxnD0sim];
  float etaD0t2sim[maxnD0sim];
  float phiD0t2sim[maxnD0sim];
  // Josry2 prompt/nonprompt Dstar/D0 extension
  int pD0Flagsim[maxnD0sim];

  int nDstarsim =0;
  int idDstarsim[maxnDstarsim];
  float ptDstarsim[maxnDstarsim];
  float etaDstarsim[maxnDstarsim];
  float phiDstarsim[maxnDstarsim];
  // Josry2 prompt/nonprompt Dstar/D0 extension
  int pDstarFlagsim[maxnDstarsim];

  int nDplussim =0;
  int idDplussim[maxnDplussim];
  float ptDplussim[maxnDplussim];
  float etaDplussim[maxnDplussim];
  float phiDplussim[maxnDplussim];
  // Josry2 prompt/nonprompt Dstar/D0 extension
  int pDplusFlagsim[maxnDplussim];
#endif


  // clear GenPV variables for each event; somehow needed for GenPV_chmult?
  GenPV_x = -999.;
  GenPV_y = -999.;
  GenPV_z = -999.;
  GenPV_recIdx = -1;
  GenPV_chmult = 0;

  if (!isData) {

#ifndef CMSSW7plus   
    iEvent.getByLabel("genParticles", genParticles);
#endif

    bool vertexfilled = false;
    for (unsigned int ee = 0; ee < genParticles->size(); ++ee) {
    
      const GenParticle & genp = (*genParticles)[ee];

      // store true vertex = vertex of first nonzero entry
      if (!vertexfilled && genp.vz() != 0){
        GenPV_x = genp.vx();
        GenPV_y = genp.vy();
        GenPV_z = genp.vz();
        GenPV_recIdx = -1;  // will be filled later
        vertexfilled = true;
      }

      // count charged particle multiplicity
      // somehow doesn't work? Beware, this is the CMS status, not the generator status!
      if (genp.status()==1 && genp.charge()!=0) ++GenPV_chmult; 
      

      // documentation for miniAOD slimmed genparticles?
      // from https://github.com/cms-sw/cmssw/blob/CMSSW_9_2_4/PhysicsTools/PatAlgos/python/slimming/prunedGenParticles_cfi.py
      // (see also https://twiki.cern.ch/twiki/bin/view/CMSPublic/SWGuideGenParticlePruner 
      //  and https://github.com/cms-sw/cmssw/blob/master/PhysicsTools/HepMCCandAlgos/plugins/GenParticlePruner.cc  )
      //  and for nanoAOD: https://github.com/cms-sw/cmssw/blob/master/PhysicsTools/HepMCCandAlgos/plugins/GenParticlePruner.cc
      //  https://github.com/cms-sw/cmssw/blob/master/PhysicsTools/NanoAOD/python/genparticles_cff.py)
      //  miniAOD:
      //  "++keep abs(pdgId) == 11 || abs(pdgId) == 13 || abs(pdgId) == 15", # keep leptons, with history
      //  "drop   status == 2",                                              # drop the shower part of the history
      //  "keep++ (400 < abs(pdgId) < 600) || (4000 < abs(pdgId) < 6000)",   # keep decays for BPH studies
      //  "drop status == 1",                                                # drop the status=1 from BPH
      //  "keep+ (400 < abs(pdgId) < 600) || (4000 < abs(pdgId) < 6000)",    # but keep first daughter, to allow lifetime determinations
      //  "keep abs(pdgId) == 11 || abs(pdgId) == 13 || abs(pdgId) == 15",   # keep leptons (also status1)
      //  "keep abs(pdgId) == 12 || abs(pdgId) == 14 || abs(pdgId) == 16",   # keep neutrinos
      //  "+keep pdgId == 22 && status == 1 && (pt > 10 || isPromptFinalState())", # keep gamma above 10 GeV (or all prompt) and its first parent
      //  "+keep abs(pdgId) == 11 && status == 1 && (pt > 3 || isPromptFinalState())", # keep first parent of electrons above 3 GeV (or prompt)
      //  "keep++ abs(pdgId) == 15",                                         # but keep keep taus with decays
      //  "drop  status > 30 && status < 70 ", 				   # remove pythia8 garbage
      //  "drop  pdgId == 21 && pt < 5",                                     # remove pythia8 garbage
      //  "drop   status == 2 && abs(pdgId) == 21",                          # but remove again gluons in the inheritance chain
      //  "keep abs(pdgId) == 23 || abs(pdgId) == 24 || abs(pdgId) == 25 || abs(pdgId) == 6 || abs(pdgId) == 37 ",   # keep VIP(articles)s
      //  "keep abs(pdgId) == 310 && abs(eta) < 2.5 && pt > 1 ",                                                     # keep K0
      //  "+keep abs(pdgId) == 13 && status == 1", # keep muon parents
      //# keep heavy flavour quarks for parton-based jet flavour
      //  "keep (4 <= abs(pdgId) <= 5)",
      //# keep light-flavour quarks and gluons for parton-based jet flavour
      //  "keep (1 <= abs(pdgId) <= 3 || pdgId = 21) & (status = 2 || status = 11 || status = 71 || status = 72) && pt>5", 
      //# keep onia states, phi, X(3872), Z(4430)+ and psi(4040)
      //  "keep+ abs(pdgId) == 333",
      //  "keep+ abs(pdgId) == 9920443 || abs(pdgId) == 9042413 || abs(pdgId) == 9000443",
      //  "keep+ abs(pdgId) == 443 || abs(pdgId) == 100443 || abs(pdgId) == 10441 || abs(pdgId) == 20443 || abs(pdgId) == 445 || abs(pdgId) == 30443",
      //  "keep+ abs(pdgId) == 553 || abs(pdgId) == 100553 || abs(pdgId) == 200553 || abs(pdgId) == 10551 || abs(pdgId) == 20553 || abs(pdgId) == 555",
      //# additional c hadrons for jet fragmentation studies
      //  "keep abs(pdgId) = 10411 || abs(pdgId) = 10421 || abs(pdgId) = 10413 || abs(pdgId) = 10423 || abs(pdgId) = 20413 || abs(pdgId) = 20423 || abs(pdgId) = 10431 || abs(pdgId) = 10433 || abs(pdgId) = 20433", 
      //# additional b hadrons for jet fragmentation studies
      //  "keep abs(pdgId) = 10511 || abs(pdgId) = 10521 || abs(pdgId) = 10513 || abs(pdgId) = 10523 || abs(pdgId) = 20513 || abs(pdgId) = 20523 || abs(pdgId) = 10531 || abs(pdgId) = 10533 || abs(pdgId) = 20533 || abs(pdgId) = 10541 || abs(pdgId) = 10543 || abs(pdgId) = 20543", 
      //#keep SUSY particles
      //  "keep (1000001 <= abs(pdgId) <= 1000039 ) || ( 2000001 <= abs(pdgId) <= 2000015)",
      //# keep protons 
      //  "keep pdgId = 2212",
      //  "keep status == 3 || ( 21 <= status <= 29) || ( 11 <= status <= 19)",  #keep event summary (status=3 for pythia6, 21 <= status <= 29 for pythia8)
      //  "keep isHardProcess() || fromHardProcessFinalState() || fromHardProcessDecayed() || fromHardProcessBeforeFSR() || (statusFlags().fromHardProcess() && statusFlags().isLastCopy())",  #keep event summary based on status flags

      // the nanoAOD configuration (also for the statusFlag) can be found in 
      // https://github.com/cms-sw/cmssw/blob/master/PhysicsTools/NanoAOD/python/genparticles_cff.py

      // cout << "genparticle" << endl; 

      // keep matrix element summary (nano)
      if ( (genp.status() == 3 || (genp.status()>20 && genp.status()<30))
#ifdef CMSSW7plus
      // keep hard process up to last copy
	|| (genp.isHardProcess() || genp.fromHardProcessDecayed() || genp.fromHardProcessFinalState() || (genp.statusFlags().fromHardProcess() && genp.statusFlags().isLastCopy()))
#endif 
      // keep high pT partons right before hadronization (nano)
        || (genp.status()>70 && genp.status()<80 && genp.pt()>15) 
      // keep all heavy quarks (plus!) already included in previous? maybe not all
        || (std::abs(genp.pdgId()) == 4 || std::abs(genp.pdgId()) == 5 || std::abs(genp.pdgId()) == 6) 
      // keep all charged leptons and neutrinos (nano)
        || (std::abs(genp.pdgId()) > 10 && std::abs(genp.pdgId()) < 17)
      // keep also one particle back in the lepton history (nano)  (in nano for charged only, but neutrino is implicit)
      //   up to four-body decays (or higher if early in list)
           || (genp.numberOfDaughters()>0 && std::abs(genp.daughter(0)->pdgId()) > 10 && std::abs(genp.daughter(0)->pdgId()) < 17)
	   || (genp.numberOfDaughters()>1 && std::abs(genp.daughter(1)->pdgId()) > 10 && std::abs(genp.daughter(1)->pdgId()) < 17)
           || (genp.numberOfDaughters()>2 && std::abs(genp.daughter(2)->pdgId()) > 10 && std::abs(genp.daughter(2)->pdgId()) < 17)
           || (genp.numberOfDaughters()>3 && std::abs(genp.daughter(3)->pdgId()) > 10 && std::abs(genp.daughter(3)->pdgId()) < 17)
      // keep tau decay products (nano)
	   || (genp.mother() != nullptr && std::abs(genp.mother()->pdgId()) == 15)
      // keep prompt tau decay decay products if high pT (nano)
#ifdef CMSSWplus
	   || (genp.mother() != nullptr && genp.mother()->mother() != nullptr && std::abs(genp.mother()->mother()->pdgId()) == 15 && genp.isPromptDecayed() )  
#endif
      // keep photons if prompt or pt>10 (nano)
#ifdef CMSSW7plus
	   || (std::abs(genp.pdgId()) == 22 && genp.status() == 1 && (genp.isPromptFinalState() || genp.pt() > 10.))
#else
	   || (std::abs(genp.pdgId()) == 22 && genp.status() == 1 && (genp.pt() > 10.))
#endif
      // keep photon parents if prompt or pt>10 (nano) (up to 3-body decays)
#ifdef CMSSW7plus
	   || (genp.numberOfDaughters()>0 && std::abs(genp.daughter(0)->pdgId()) == 22 && genp.daughter(0)->status() == 1 && (dynamic_cast<const GenParticle *>(genp.daughter(0))->isPromptFinalState() || (genp.daughter(0)->pt() > 10.)))
	   || (genp.numberOfDaughters()>1 && std::abs(genp.daughter(1)->pdgId()) == 22 && genp.daughter(1)->status() == 1 && (dynamic_cast<const GenParticle *>(genp.daughter(1))->isPromptFinalState() || genp.daughter(1)->pt() > 10.))
	   || (genp.numberOfDaughters()>2 && std::abs(genp.daughter(2)->pdgId()) == 22 && genp.daughter(2)->status() == 1 && (dynamic_cast<const GenParticle *>(genp.daughter(2))->isPromptFinalState() || genp.daughter(2)->pt() > 10.))
#else
	   || (genp.numberOfDaughters()>0 && std::abs(genp.daughter(0)->pdgId()) == 22 && genp.daughter(0)->status() == 1 && (genp.daughter(0)->pt() > 10.))
	   || (genp.numberOfDaughters()>1 && std::abs(genp.daughter(1)->pdgId()) == 22 && genp.daughter(1)->status() == 1 && (genp.daughter(1)->pt() > 10.))
	   || (genp.numberOfDaughters()>2 && std::abs(genp.daughter(2)->pdgId()) == 22 && genp.daughter(2)->status() == 1 && (genp.daughter(2)->pt() > 10.))
#endif
      // keep VIP(article)s, i.e. Z, W, H, H+ (nano)
        || (std::abs(genp.pdgId()) == 23 || std::abs(genp.pdgId()) == 24 || std::abs(genp.pdgId()) == 25 || std::abs(genp.pdgId()) == 37)
      // keep all ground state heavy flavour hadrons, mesons and baryons, open and hidden (nano) 
        || ((std::abs(genp.pdgId()) > 400 && std::abs(genp.pdgId()) < 600) || (std::abs(genp.pdgId()) > 4000 && std::abs(genp.pdgId()) < 6000)) 

        || (std::abs(genp.pdgId()) == 211 && std::abs(genp.pdgId()) == 321)
      // keep also a selected subset of lower lying excited heavy flavour hadrons (plus!)
        || ((std::abs(genp.pdgId()) > 10400 && std::abs(genp.pdgId()) < 10600) || (std::abs(genp.pdgId()) > 20400 && std::abs(genp.pdgId()) < 20600)
        || (std::abs(genp.pdgId()) > 30400 && std::abs(genp.pdgId()) < 30600))  
        || ((std::abs(genp.pdgId()) >100400 && std::abs(genp.pdgId()) <100600) || (std::abs(genp.pdgId()) >200400 && std::abs(genp.pdgId()) <200600))  
      // keep SUSY fiction particles (nano)
        || ((std::abs(genp.pdgId()) > 1000000 && std::abs(genp.pdgId()) < 1000040) || (std::abs(genp.pdgId()) > 2000000 && std::abs(genp.pdgId()) < 2000016)) 
        ){


        // cout << "genparticle 2, pdgId " << genp.pdgId() << endl;

        // all these particle types are also stored in standard nanoAOD
        bool isNano = true;
        // put exceptions here 
        // heavy quarks with status different from those kept in default nanoAOD
#ifdef CMSSW7plus        
        if ((std::abs(genp.pdgId()) == 4 || std::abs(genp.pdgId()) == 5 || std::abs(genp.pdgId()) == 6) && !(genp.status() ==3 || (genp.status() >20 && genp.status() < 30) || (genp.status()>70 && genp.status()<80 && genp.pt()>15) ) && !(genp.isHardProcess() || genp.fromHardProcessDecayed() || genp.fromHardProcessFinalState() || (genp.statusFlags().fromHardProcess() && genp.statusFlags().isLastCopy())) ) isNano = false;
#endif

        // Onia wich do not directly decay to leptons
        if (((std::abs(genp.pdgId()) > 10400 && std::abs(genp.pdgId()) < 10600) || (std::abs(genp.pdgId()) > 20400 && std::abs(genp.pdgId()) < 20600))  
	     || ((std::abs(genp.pdgId()) >100400 && std::abs(genp.pdgId()) <100600) || (std::abs(genp.pdgId()) >200400 && std::abs(genp.pdgId()) <200600))) {
	  if (genp.numberOfDaughters()<2) isNano=false;
          else if ((std::abs(genp.daughter(0)->pdgId())>10 && std::abs(genp.daughter(0)->pdgId())<17 ) 
	    || (std::abs(genp.daughter(1)->pdgId())>10 && std::abs(genp.daughter(1)->pdgId())<17)) isNano = false;
         }

        int parpdgId = 0;
        int sparpdgId = 0;

        if ( genp.mother() != nullptr) {
          // get pointer to mother (reco::Candidate type!):
          const Candidate * mom = genp.mother();
          // and store type and index 
	  //parpdgId = genp.mother()->pdgId();
          parpdgId = mom->pdgId();
          sparpdgId = mom->pdgId();
          // if nonstable should add further loops here 
          // until stable parent is found ... not yet implemented
        }
          
        // cout << parpdgId << endl; 

        // get id of mother (always higher in list than daughter)
	// didn't get pointer comparison to work, so use pt for match 
        int genpmid = -999;
        if (parpdgId!=0) {
          for (unsigned int eee = 0; eee < GenPart_pt.size(); ++eee) {
            if ((GenPart_pt.at(eee) == genp.mother()->pt()) && (GenPart_eta.at(eee) == genp.mother()->eta())) {
              genpmid = eee;
              continue;
            }
          }  
        } 

        // cout << "hello daughters" << endl; 

        // get no. of daughters:
        size_t n = genp.numberOfDaughters();
        int nstchgdaug = 0;

        // initialize vertex variables for this event
	dcyvtxx=0; dcyvtxy=0; dcyvtxz=0; 
        // loop over daughters:
        for(size_t j = 0; j < n; ++j) {

          // get pointer d to a daughter  (reco::Candidate type!):
          const Candidate * d = genp.daughter( j );
          // if want to access extended  GenParticle options: 
          //const GenParticle *d = dynamic_cast<const GenParticle *>( genp.daughter( j ) );
  
          // fill position of decay vertex (creation vertex of first daughter)
          if (j==0) {
            dcyvtxx = d->vx();
            dcyvtxy = d->vy();
            dcyvtxz = d->vz();
          }

          // check daughter's stability (status 1) and charge, 
          // and increment counter:
          if (d->status() == 1 && d->charge() != 0) ++nstchgdaug; 
        }
        // creation point of D0 is decay point of mother
        motx = genp.vx();
        moty = genp.vy();
        motz = genp.vz();

        // Josry prompt/nonprompt Dstar/D Flag extension 
        float sqrtQuadSum = -9999;
        //taken from MuDhistos:
        //sqrtQuadSum = sqrt( std::abs(GenPart_mvx[gg]-GenPV_x) * std::abs(GenPart_mvx[gg]-GenPV_x) + std::abs(GenPart_mvy[gg]-GenPV_y) * std::abs(GenPart_mvy[gg]-GenPV_y) + std::abs(GenPart_mvz[gg]-GenPV_z) * std::abs(GenPart_mvz[gg]-GenPV_z) );
        // GenPart_mvx  is here    motx
        // GenPV_x      is here    genp.vx()
        promptFlag = -1;
        sqrtQuadSum = sqrt( std::abs(motx-GenPV_x) * std::abs(motx-GenPV_x) + std::abs(moty-GenPV_y) * std::abs(moty-GenPV_y) + std::abs(motz-GenPV_z) * std::abs(motz-GenPV_z) );
        if ( sqrtQuadSum == 0 )  promptFlag = 1;
        else promptFlag = 0;

        // cout << "hello interesting" << endl;

        // muons
/*         if (abs(genp.pdgId())==13 && genp.status()==1 && nmusim < maxnmusim) {
          idmusim[nmusim]=ee; 
          chgmusim[nmusim]=genp.charge();
          ptmusim[nmusim]=genp.pt();
          etamusim[nmusim]=genp.eta();
          phimusim[nmusim]=genp.phi();
	        ++nmusim;
        }
        if (nmusim >= maxnmusim) std::cout << "!!! maxnmusim exceeded !!!" << std::endl; */

#ifdef charm
        // cout << "hello muons" << endl;
        // D0 mesons decaying to Kpi, KK, or pipi
        if (abs(genp.pdgId())==421 && n==2 && nstchgdaug ==2 && nD0sim < maxnD0sim) {
          idD0sim[nD0sim]=ee; 
          ptD0sim[nD0sim]=genp.pt();
          etaD0sim[nD0sim]=genp.eta();
          phiD0sim[nD0sim]=genp.phi(); 
          pdgIdD0t1sim[nD0sim]=(genp.daughter(0))->pdgId();
          chgD0t1sim[nD0sim]=(genp.daughter(0))->charge();
          ptD0t1sim[nD0sim]=(genp.daughter(0))->pt();
          etaD0t1sim[nD0sim]=(genp.daughter(0))->eta();
          phiD0t1sim[nD0sim]=(genp.daughter(0))->phi(); 
          pdgIdD0t2sim[nD0sim]=(genp.daughter(1))->pdgId();
          chgD0t2sim[nD0sim]=(genp.daughter(1))->charge();
          ptD0t2sim[nD0sim]=(genp.daughter(1))->pt();
          etaD0t2sim[nD0sim]=(genp.daughter(1))->eta();
          phiD0t2sim[nD0sim]=(genp.daughter(1))->phi();
          // Josry2 prompt/nonprompt Dstar/D0 extension
          pD0Flagsim[nD0sim] = promptFlag; 
	  ++nD0sim;
        } 
        if (nD0sim >= maxnD0sim) std::cout << "!!! maxnD0sim exceeded !!!" << std::endl;

        // all charged Dstar mesons
        if (abs(genp.pdgId())==413 && nDstarsim < maxnDstarsim) {
          idDstarsim[nDstarsim]=ee; 
          ptDstarsim[nDstarsim]=genp.pt();
          etaDstarsim[nDstarsim]=genp.eta();
          phiDstarsim[nDstarsim]=genp.phi();
          // Josry2 prompt/nonprompt Dstar/D0 extension
          pDstarFlagsim[nDstarsim]=promptFlag;
          ++nDstarsim;
        } 
        if (nDstarsim >= maxnDstarsim) std::cout << "!!! maxnDstarsim exceeded !!!" << std::endl;

        // all charged Dplus mesons
        if (abs(genp.pdgId())==411 && nDplussim < maxnDplussim) {
          idDplussim[nDplussim]=ee; 
          ptDplussim[nDplussim]=genp.pt();
          etaDplussim[nDplussim]=genp.eta();
          phiDplussim[nDplussim]=genp.phi();
          // Josry2 prompt/nonprompt Dstar/D0 extension
          pDplusFlagsim[nDplussim]=promptFlag;
          ++nDplussim;
        } 
        if (nDplussim >= maxnDplussim) {
           std::cout << "!!! maxnDplussim exceeded !!!" << std::endl;
           // need to touch so far unused variables 
           // in order to make compiler happy
           cout << nDplussim << idDplussim << ptDplussim << etaDplussim << phiDplussim << pDplusFlagsim << endl;
        }  
#endif

	if (GenPart_pt.size() < nReserve_GenPart) {
	  GenPart_pt.push_back(genp.pt());
	  GenPart_eta.push_back(genp.eta());
	  GenPart_phi.push_back(genp.phi());
	  GenPart_mass.push_back(genp.mass());
	  GenPart_charge.push_back(genp.charge());
	  GenPart_pdgId.push_back(genp.pdgId());
          // status = 1: final state stable particles
          // status = 2: hadron level, unstable
          // status = 3: parton level  (may differ according to generator)
          // see also https://twiki.cern.ch/twiki/bin/view/CMSPublic/WorkBookGenParticleCandidate
	  GenPart_status.push_back(genp.status());

// gen status flags stored bitwise, bits are: 0 : isPrompt, 1 : isDecayedLeptonHadron, 
//     2 : isTauDecayProduct, 3 : isPromptTauDecayProduct, 4 : isDirectTauDecayProduct, 
//     5 : isDirectPromptTauDecayProduct, 6 : isDirectHadronDecayProduct, 7 : isHardProcess, 
//     8 : fromHardProcess, 9 : isHardProcessTauDecayProduct, 10 : isDirectHardProcessTauDecayProduct, 
//    11 : fromHardProcessBeforeFSR, 12 : isFirstCopy, 13 : isLastCopy, 14 : isLastCopyBeforeFSR,
#ifdef CMMSW7plus
          uint statusflags = genp.statusFlags().isPrompt()*pow(2,0) 
            + genp.statusFlags().isDecayedLeptonHadron()*pow(2,1)
            + genp.statusFlags().isTauDecayProduct()*pow(2,2) 
            + genp.statusFlags().isPromptTauDecayProduct()*pow(2,3) 
            + genp.statusFlags().isDirectTauDecayProduct()*pow(2,4) 
            + genp.statusFlags().isDirectPromptTauDecayProduct()*pow(2,5) 
            + genp.statusFlags().isDirectHadronDecayProduct()*pow(2,6) 
            + genp.statusFlags().isHardProcess()*pow(2,7) 
            + genp.statusFlags().fromHardProcess()*pow(2,8) 
            + genp.statusFlags().isHardProcessTauDecayProduct()*pow(2,9)
            + genp.statusFlags().isDirectHardProcessTauDecayProduct()*pow(2,10) 
            + genp.statusFlags().fromHardProcessBeforeFSR()*pow(2,11)
            + genp.statusFlags().isFirstCopy()*pow(2,12)
            + genp.statusFlags().isLastCopy()*pow(2,13) 
            + genp.statusFlags().isLastCopyBeforeFSR()*pow(2,14);
#else 
          // does not yet exist
          uint statusflags = 0;
#endif
          //cout << "hello status" << endl;  
          GenPart_statusFlags.push_back(statusflags);
          // The following does not work, do something else
	  //          GenPart_genPartIdxMother.push_back(genp.mother());
          // Rather loop over previous entries in *this* list, find parent
          // (if stored), and store index of that! 
	        GenPart_genPartIdxMother.push_back(genpmid);
          GenPart_Id.push_back(ee);
          GenPart_isNano.push_back(isNano);
          GenPart_parpdgId.push_back(parpdgId);
          GenPart_sparpdgId.push_back(sparpdgId);
          GenPart_numberOfDaughters.push_back(genp.numberOfDaughters());
          GenPart_nstchgdaug.push_back(nstchgdaug);
          GenPart_promptFlag.push_back(promptFlag);
          GenPart_vx.push_back(dcyvtxx);
          GenPart_vy.push_back(dcyvtxy);
          GenPart_vz.push_back(dcyvtxz);
          GenPart_mvx.push_back(motx);
          GenPart_mvy.push_back(moty);
          GenPart_mvz.push_back(motz);
          GenPart_recIdx.push_back(-1);
	}
	else{std::cout << "WARNING!!!!! NO. OF GENPART IS MORE THAN YOUR RESERVED NO.!!!!!!" << std::endl;}
      }
    }
  } // end of event is not data
   nGenPart = GenPart_pt.size();  // truncated to maximum size
    
//////////////////////////////////////////////////////////////////////////////
/////////////////////////////// Gen Particle End /////////////////////////////
//////////////////////////////////////////////////////////////////////////////

   // cout << "hello beam spot" << endl;

/////////////////////////////////////////////////////////////////////////////
///////////////////////////// Beam spot /////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

  reco::BeamSpot vertexBeamSpot= *beamSpotHandle;  

  // store beam spot info 
  Bsp_x = vertexBeamSpot.x0();
  Bsp_y = vertexBeamSpot.y0();
  Bsp_z = vertexBeamSpot.z0();
  Bsp_sigmaz = vertexBeamSpot.sigmaZ();
  Bsp_dxdz = vertexBeamSpot.dxdz();
  Bsp_dydz = vertexBeamSpot.dydz();
  Bsp_widthx = vertexBeamSpot.BeamWidthX();
  Bsp_widthy = vertexBeamSpot.BeamWidthY();

          
  // cout << "hello tracks and vertices" << endl;

//////////////////////////////////////////////////////////////////////////////
///////////////////////////// Tracks and vertices Start //////////////////////
//////////////////////////////////////////////////////////////////////////////


  // reimplement relevant track and vertex reference histograms
  // use includes (can be commented in and out by hand or by IFDEF)

  // nanoAOD

  // fill basic track information (all tracks in event)
  nTrk = tracks->size();
  PV_npvs = Primvertex->size();

  // nanoAOD
  nOtherPV = 0;

  // provide list of isotracks
  nIsoTrack =0;

  // nanoAOD-like extension
  nPVtx=0;

  // cout << "hello vertices" << endl;

//////////////////////////////////////////////////////////////////////////////
//////////////////////////// Tools for Vertices //////////////////////////////
//////////////////////////////////////////////////////////////////////////////
    
  // load the tools to work with vertices: 
  // declare new track builder for new Transient track collection
  ESHandle<TransientTrackBuilder> theB;
  iSetup.get<TransientTrackRecord>().get("TransientTrackBuilder",theB);

  // clear the storage containers for the objects in this event
  OtherPV_z.clear();

  PVtx_Id.clear();
  PVtx_isMain.clear();
  PVtx_isMainSim.clear();
  PVtx_isGood.clear();
  PVtx_isValid.clear();
  PVtx_isFake.clear();
  PVtx_isTrigUnique.clear();
  PVtx_isUnbiased.clear();
  PVtx_ntrk.clear();
  PVtx_ntrkfit.clear();
  PVtx_chi2.clear();
  PVtx_ndof.clear();
  PVtx_score.clear();
  PVtx_sumPt.clear();
  PVtx_Rho.clear();
  PVtx_x.clear();
  PVtx_y.clear();
  PVtx_z.clear();
  PVtx_Covxx.clear();
  PVtx_Covyx.clear();
  PVtx_Covzx.clear();
  PVtx_Covyy.clear();
  PVtx_Covzy.clear();
  PVtx_Covzz.clear();

  int nmuonlist = 0;
  int muid[maxnmuonlist];
  int muvtx[maxnmuonlist];
  double mupvx[maxnmuonlist];
  double mupvy[maxnmuonlist];
  double mupvz[maxnmuonlist];

  /* int neleclist = 0;
  int elid[maxneleclist];
  int elvtx[maxneleclist];
  double elpvx[maxneleclist];
  double elpvy[maxneleclist];
  double elpvz[maxneleclist]; */

  PV_npvsGood = 0;
  nOtherPV = 0;
  float PVSimMindist = 999.;
  int PVSimMinId = -1;
  //  for (unsigned int t = 0; t<hVtx->size(); ++t) {
  int vtxid =-1;
  int null=0; 

  // set primary vertex reference point pv
  math::XYZPoint pv(Primvertex->begin()->position());
  // to be filled later
  math::XYZPoint bestpv;

  reco::VertexCollection::const_iterator PV_ite;

  for (reco::VertexCollection::const_iterator vite = Primvertex->begin(); 
       vite != Primvertex->end(); ++vite) {
    if (PVtx_z.size() < nReserve_PVtx) {
      // primVertex_tmp = hVtx->at(t);
      // PVtx_Id.push_back(t);
      ++vtxid;
      // fill nanaoAOD vertex structure
      // by convention, first vertex is main primary *** check ***
      if (vtxid == 0) {
        PV_ite = vite;
        PV_chi2 = vite->chi2()/vite->ndof();
        PV_ndof = vite->ndof();
        PV_x = vite->x();
        PV_y = vite->y();
        PV_z = vite->z();
        PVtx_isMain.push_back(1);
      }  
      else if (OtherPV_z.size() < nReserve_OtherPV) {
	PVtx_isMain.push_back(null);
        // should this be sorted according to score first?
        ++nOtherPV;
        OtherPV_z.push_back(vite->z());
      }
      else {        
	PVtx_isMain.push_back(null);
      }
      // fill extended vertex structure
      // just in case not all vertices are stored (otherwise redundant)
      PVtx_Id.push_back(vtxid);
      //    will be refilled in NanoDmeson later (when activated)
      PVtx_ntrk.push_back(-1);
      PVtx_ntrkfit.push_back(vite->nTracks());
      PVtx_chi2.push_back(vite->chi2());
      PVtx_ndof.push_back(vite->ndof());
      float score=0., sumpt=0.;
      // flags to indicate whether vertex has muon or electron
      //   *** should this be changed to dz/dxy requirement? ***
      //       (will also work for miniAOD) 
      bool hasmuon = false;
      float hasmuonpt = 0.;
      /* bool haselec = false;
      float haselecpt = 0.; */
      // loop over all tracks from this vertex
      // (In miniAOD the vertices have "lost" their tracks -> loop is dummy:
      //  *** should loop over all PackedCandidates and check vertexRef and
      //      muon property *** )
      for (reco::Vertex::trackRef_iterator iTrack = vite->tracks_begin(); iTrack != vite->tracks_end(); ++iTrack) {
	// get track reference to full track structure
        const reco::TrackRef trackRef = iTrack->castTo<reco::TrackRef>();
        // somehow store track-vertex relation here?
        // ...
        // get and sum track pt
        float trackpt = trackRef->pt();
        score += trackpt*trackpt;

        //
        // check whether track is muon candidate *** apply cuts??? ***
        // and store cross-correlation information for muons and primary vertices
        //
        bool ismuon=false;
        int muonid=-1;
#ifndef miniAOD
        for (reco::MuonCollection::const_iterator itMuon = muons->begin(); itMuon != muons->end(); ++itMuon) {
#endif
#ifdef miniAOD
	for (pat::MuonCollection::const_iterator itMuon = muons->begin(); itMuon != muons->end(); ++itMuon) {
#endif
          // indicates position in reco::muon structure 
          ++muonid;
          if((itMuon->track()).isNonnull()){
            if (trackRef == itMuon->track()) {
              ismuon=true;
              hasmuon=true;
              // *** is the muon-vertex association loose enough? ***
              if ((itMuon->track())->pt() > hasmuonpt) hasmuonpt = (itMuon->track())->pt();
              //cout << "has muon! " << (itMuon->track())->pt() << " track " << trackRef->pt() << endl;
              // store muon-vertex association for later use
              if (nmuonlist < maxnmuonlist) {
                muid[nmuonlist]=muonid;
                muvtx[nmuonlist]=vtxid;
                mupvx[nmuonlist]=vite->x();
                mupvy[nmuonlist]=vite->y();
                mupvz[nmuonlist]=vite->z();
                ++nmuonlist;
              }
              else { std::cout << "Warning !!!! maxnmuonlist too small" << std::endl;}
              break;
            } // if trackref
          } // if itMuon
        } // for MuonCollection

        //
        // check whether track is electron candidate *** apply cuts??? ***
        // and store cross-correlation information for electrons and primary vertices
        // *** the following compiles but the association does not fully work yet ***
        //
/*         bool iselec=false;
        int elecid=-1;
#ifndef miniAOD
        for (reco::GsfElectronCollection::const_iterator itElec = electrons->begin(); itElec != electrons->end(); ++itElec) {
#endif
#ifdef miniAOD
	for (pat::ElectronCollection::const_iterator itElec = electrons->begin(); itElec != electrons->end(); ++itElec) {
#endif
          // indicates position in reco::electron structure 
          ++elecid;
          // compare track parameters
          //if (trackRef->pt()>4.) {
          //  cout << "gsf pt " << itElec->gsfTrack()->pt() << endl;
          //  cout << "track pt " << trackRef->pt() << endl;
          //}
          // doesn't exist
          //cout << "pt " << itElec->gsfTrack()->track()->pt() << endl;
	  // get track reference to full track structure
          // the following doesn't compile
          //const reco::TrackRef trackRefEle = itElec->castTo<reco::TrackRef>();
          // the following compiles, but doesn't work
          //if((itElec->track()).isNonnull()){
	  //if((itElec->gsfTrack()).isNonnull()){
            //cout << "pt " << itElec->track()->pt() << endl;
            //cout << "pt " << trackRefEle->pt() << endl;
            //if (trackRef == trackRefEle) {
	    //if (trackRef == itElec->track()) {
	    //the following comparison seems to be illegal 
            //if (trackRef == itElec->gsfTrack()) {
          // do kinematic matching: *** readjust cuts? ***
          // might not always work, and double matching is not excluded
          // pt within 5O%, eta within 0.1, phi within 0.1, same charge, x,y,z within 1 cm
          // (combine pt and charge to 1/pt?) 
          // so far this is CPU-inefficent, should make some precuts 
	  if (abs(itElec->gsfTrack()->pt()-trackRef->pt())/itElec->gsfTrack()->pt()<0.5) {
	    if (abs(itElec->gsfTrack()->eta()-trackRef->eta())<0.1) {
              float eledphi = abs(itElec->gsfTrack()->phi()-trackRef->phi());
              if (eledphi > 3.1415) eledphi = eledphi - 2.*3.1415;
	      if (abs(eledphi) <0.1) { 
                if (itElec->gsfTrack()->charge() == trackRef->charge()) {
		  if (abs(itElec->gsfTrack()->vx()-vite->x())<1. && abs(itElec->gsfTrack()->vy()-vite->y())<1. && abs(itElec->gsfTrack()->vz()-vite->z())<1.) {
              iselec=true;
              haselec=true;
              // the next dummy line is only introduced to avoid warning 
              // for unused variables on some compiler versions
              if (!iselec && !haselec) continue;
              // use gsfTrack for pt evaluation
              // *** is the track electron-vertex association loose enough? ***
              if ((itElec->gsfTrack())->pt() > haselecpt) haselecpt = (itElec->gsfTrack())->pt();
              //cout << "has electron! " << (itElec->gsfTrack())->pt()*itElec->gsfTrack()->charge() << " track " << trackRef->pt()*trackRef->charge() << endl;
              //cout << "elect eta " << (itElec->gsfTrack())->eta() << " phi " << (itElec->gsfTrack())->phi() << " z " << (itElec->gsfTrack())->vz() << endl;
              //cout << "track eta " << trackRef->eta() << " phi " << trackRef->phi() << " z " << vite->z() << endl;
              // store electron-vertex association for later use
              if (neleclist < maxneleclist) {
                elid[neleclist]=elecid;
                elvtx[neleclist]=vtxid;
                elpvx[neleclist]=vite->x();
                elpvy[neleclist]=vite->y();
                elpvz[neleclist]=vite->z();
                ++neleclist;
              }
              else { std::cout << "Warning !!!! maxneleclist too small" << std::endl;}
              // break;  // not here, the later one is often the better one
                  } // if z
		} // if charge
	      } // if phi
            } // if trackref/eta
          } // if itElec/pt
        } // for ElectronCollection
 */
        // sum track pt excluding muons (not exluding electrons for the time being)
        if (!ismuon) sumpt += trackpt;         
      } // for VertexTracks
      // vite->score() does not seem to exist
      if (vtxid==0) PV_score = score;
      PVtx_score.push_back(score);
      PVtx_sumPt.push_back(sumpt);
      PVtx_Rho.push_back(vite->position().Rho());
      PVtx_x.push_back(vite->position().x());
      PVtx_y.push_back(vite->position().y());
      PVtx_z.push_back(vite->position().z());
      // is it possible that vite->z and vite->position.z differ?
      if (vite->z() != vite->position().z())
	std::cout << "*** vertex Alarm *** " << vite->z() << " " 
		  << vite->position().z() << std::endl;
      PVtx_Covxx.push_back(vite->covariance(0,0));
      PVtx_Covyx.push_back(vite->covariance(1,0));
      PVtx_Covzx.push_back(vite->covariance(2,0));
      PVtx_Covyy.push_back(vite->covariance(1,1));
      PVtx_Covzy.push_back(vite->covariance(2,1));
      PVtx_Covzz.push_back(vite->covariance(2,2));

      // find good vertices
      // for QCD0-5 MC, about 10% of main simulated vertices are not 
      // reconstructed at all (no tracks in acceptance?) and 
      // a further 10% is not GOOD: most with |z|>10, a few with ndof<=4, 
      // all seem to be Valid, so Valid flag not needed?
      // increase "good" criterion from 10->24 cm in z 
      // and add "Rho" cut (according to doc) (radius?)
      // *** should position be absolute or relative to beam spot? ***
      int vtxGood=0;
      if (!vite->isFake() && vite->isValid() &&
          vite->ndof()>4 && fabs(vite->z()-Bsp_z)<24. &&
          vite->position().Rho() < 2.){ 
        vtxGood=1;
        ++PV_npvsGood;
      }
      float rhotest = sqrt(vite->position().x()*vite->position().x() + 
                           vite->position().y()*vite->position().y());
      // to check that the two are the same up to float precision 
      if (rhotest != float(vite->position().Rho()))
	std::cout << "*** Alarm Rho *** " << rhotest << " " 
		  << vite->position().Rho() << std::endl;
      PVtx_isGood.push_back(vtxGood);
      PVtx_isValid.push_back(vite->isValid());
      PVtx_isFake.push_back(vite->isFake());

      // check vertices against simulated vertex
      //   ... should treat the case that there is more than one ...
      // The following sets the flag to true for the first vertex with
      // distance < 0.1 cm, and then for all subsequent vertices with 
      // consecutively smaller distance
      // GenPV_recIdx will either contain the first or the closest good vertex
      if (fabs(vite->z()-GenPV_z)<0.2 && fabs(vite->z()-GenPV_z)<PVSimMindist) {
        PVSimMindist = abs(vite->z()-GenPV_z);
        // always take the first, supersede if closer good vertex
        if (PVSimMinId==-1 || vtxGood==1) PVSimMinId = vtxid;
        PVtx_isMainSim.push_back(1);
        GenPV_recIdx = PVSimMinId;
        // to get unique simulated vertex, can check offline: 
        // PVtx_isMainSim && PVtx_Id == GenPV_recIdx
      }
      else {
        PVtx_isMainSim.push_back(0);
        // GenPV_recIdx = -1 has been preset elsewhere, don't overwrite here!;
      }

      // check whether this vertex has uniquely triggered a non-minimum-bias 
      // trigger (not yet fully working)
      PVtx_isTrigUnique.push_back(null);
      // For data: 
      // (technically not disabled for MC, which always has a ZeroBias Trigger)
      // Declare a vertex `unbiased' if a genuine MinimumBias trigger has fired
      if (GoodMinimumBiasTrig) PVtx_isUnbiased.push_back(1);
      // or a pure muon trigger has fired, good muons have been found, 
      // and the vertex does not have any muon candidate 
      // (*** should be changed to `does not have a triggered muon', and be 
      //  expanded to Electron datasets ***)
      else if (GoodMuTrig && (((MuThresh>-1 || IsoMuThresh>-1) && nMuon>0) || (DoubleMuThresh >-1 && nMuon>1)) && !hasmuon) PVtx_isUnbiased.push_back(1);
      // or a single muon trigger has fired, there are at least two muons, 
      // and the highest pt vertex muon has less than half the pt of the single 
      // muon trigger threshold
      else if (GoodMuTrig && (MuThresh>2.*hasmuonpt || IsoMuThresh>2.*hasmuonpt) && muons->size()>1) PVtx_isUnbiased.push_back(1);
      // note that MuHad and MuOnia triggers do not yield unbiased vertices 
      else PVtx_isUnbiased.push_back(0);
      // net effect: MB events will always be accepted; 
      // NMB events will be accepted if they do not have muon
    }
      else {std::cout << "WARNING!!!!! NO. OF vertices IS MORE THAN YOUR RESERVED NO.!!!!!!" << std::endl;}
  }
  nPVtx = PVtx_z.size();

//////////////////////////////////////////////////////////////////////////////
///////////////////////////// Tracks and vertices End ////////////////////////
//////////////////////////////////////////////////////////////////////////////


  // cout << "hello muon" << endl;

    
//////////////////////////////////////////////////////////////////////////////
///////////////////////////// Muon Collection Start //////////////////////////
//////////////////////////////////////////////////////////////////////////////

  // define and initialize all relevant temporary muon variables
  Float_t mu_relPFIsoR03 = -9999.;
  Float_t mu_relPFIsoR04 = -9999.;
  // add isolation variables used in CMSSW 4-2-8
  Float_t mu_relIsoR03 = -9999.;
  Float_t mu_relIsoR05 = -9999.;
  // add newer variables
  Int_t   mu_pfIso = -1;

  Float_t mu_ip3d = -9999.;
  Float_t mu_ip3dBest = -9999.;
  Float_t mu_Errip3d = -9999.;
  Float_t mu_sip3d = -9999.;
  Float_t mu_sip3dBest = -9999.;

  // tlorentzvector
  TLorentzVector p4Mu;  
  p4Mu.SetPtEtaPhiE(0., 0., 0., 0.);

  // clear the storage containers for the objects in this event
  Muon_charge.clear();
  Muon_tightCharge.clear(); 
  Muon_pt.clear();
  Muon_ptErr.clear();
  Muon_eta.clear();
  Muon_phi.clear();  
  Muon_mass.clear();
  Muon_dxy.clear();
  Muon_dxyErr.clear();
  Muon_dz.clear();
  Muon_dzErr.clear();
  Muon_ip3d.clear();
  Muon_sip3d.clear();
  Muon_pfRelIso03_all.clear();
  Muon_pfRelIso03_chg.clear();
  Muon_pfRelIso04_all.clear();
  Muon_pfIsoId.clear();
  Muon_miniPFRelIso_all.clear(); 
  Muon_miniPFRelIso_chg.clear(); 
  Muon_jetIdx.clear(); 
  Muon_isGlobal.clear();
  Muon_isTracker.clear();
  Muon_isPFcand.clear();
  Muon_softId.clear();
  Muon_mediumId.clear(); 
  Muon_tightId.clear(); 
  Muon_highPtId.clear(); 
  Muon_nStations.clear();
  Muon_nTrackerLayers.clear(); 
  Muon_segmentComp.clear(); 
  Muon_cleanmask.clear(); 
  Muon_mvaTTH.clear(); 
  Muon_pdgId.clear(); 
  Muon_genPartFlav.clear(); 
  Muon_genPartIdx.clear(); 

  Muon_Id.clear(); 
  Muon_x.clear(); 
  Muon_y.clear(); 
  Muon_z.clear(); 
  Muon_dxyBest.clear();
  Muon_dzBest.clear();
  Muon_ip3dBest.clear();
  Muon_sip3dBest.clear();
  Muon_gpt.clear();
  Muon_geta.clear();
  Muon_gphi.clear();  
  Muon_looseId.clear();
  Muon_softId4.clear();
  Muon_softIdBest.clear();
  Muon_isNano.clear();
  Muon_isMini.clear();
  Muon_isGood.clear();
  Muon_isGoodLast.clear();
  Muon_isGoodAng.clear();
  Muon_isArbitrated.clear();
  Muon_isStandAlone.clear();
  Muon_isRPCcand.clear();
  Muon_nValid.clear(); 
  Muon_nPix.clear(); 
  Muon_Chi2.clear(); 
  Muon_gnValid.clear(); 
  Muon_gnPix.clear(); 
  Muon_gChi2.clear(); 
  Muon_gnValidMu.clear(); 
  Muon_vtxIdx.clear(); 
  Muon_vtxFlag.clear(); 
  Muon_trkIdx.clear();
  Muon_simIdx.clear(); 
  
  b4_nMuon = muons->size();
  Muon_nNano = 0;
  
  int muonid = -1;

  // set cuts *** to be tuned ***
  // set maximum dz for muon to be considered for vertex association 
  float mindzvtxcut = 1.;

#ifndef miniAOD
  for (reco::MuonCollection::const_iterator recoMuon = muons->begin(); recoMuon != muons->end(); ++recoMuon) {
#endif
#ifdef miniAOD
  for (pat::MuonCollection::const_iterator recoMuon = muons->begin(); recoMuon != muons->end(); ++recoMuon) {
#endif
    ++muonid;
    
    // preselection: make "or" of 
    // - Run 2 miniAOD preselection (CMSSW 7X and above, works for 53X):
    //   pT>5 or (pt>3 and (PF or global or tracker or standal. or RPC)) or PF
    //   -> isMini
    // - Run 1 Loose selection (CMSSW 52X and above):
    //      isPFMuon and (isGlobalMuon or is TrackerMuon)
    //   -> isLoose 
    // - Run 1 Soft ID (CMSSW 44X or below):
    //      OneStation Tight, >5 hits, >0 pixel hits, high purity,
    //      dxy <0.3, dz <20.
    //      also works for CMSSW 53X and Run 2
    //   -> isSoft
    // - 
    // 

    // prepare variables:
    bool mu_isGlobal = recoMuon->isGlobalMuon();
    bool mu_isTracker = recoMuon->isTrackerMuon();
    bool mu_isStandAlone = recoMuon->isStandAloneMuon();
    // n/a for 4_2_8:
#ifndef CMSSW42X
    bool mu_isPFMuon = recoMuon->isPFMuon();
    bool mu_isRPCMuon = recoMuon->isRPCMuon();
#endif
#ifdef CMSSW42X
    // set true or false?
    bool mu_isPFMuon = true;
    bool mu_isRPCMuon = false;
#endif

    // Loose ID, see
    // https://twiki.cern.ch/twiki/bin/view/CMSPublic/SWGuideMuonID
    // https://twiki.cern.ch/twiki/bin/view/CMSPublic/SWGuideMuonIDRun2
    bool mu_looseId = (mu_isGlobal || mu_isTracker) && mu_isPFMuon;

    // prepare variables:
    bool mu_isGood = muon::isGoodMuon(*recoMuon, TMOneStationTight);
    bool mu_isGoodLast = muon::isGoodMuon(*recoMuon, TMLastStationTight);
    bool mu_isGoodAng = muon::isGoodMuon(*recoMuon, TMLastStationAngTight);
    bool mu_isArbitrated = muon::isGoodMuon(*recoMuon, TrackerMuonArbitrated);
    int mu_nStations = recoMuon->numberOfMatchedStations(); 
    // will the following work for Standalone muons?
    int mu_nTrackerLayers = 0;
    int mu_nValidHits = 0;
    int mu_gnValidHits = 0;
    int mu_nPixelLayers = 0; 
    bool mu_highPurity = false;
    float mu_dxy = 0.;
    float mu_dxyBest = 0.;
    float mu_dxyError = -1.;
    float mu_dz = 0.;
    float mu_dzBest = 0.;
    float mu_dzError = -1.;
    int mu_vtxId = -1.;

    // set quality variables
    int  mu_bestflag = -1;
    if (mu_isGlobal || mu_isTracker) {
      mu_nTrackerLayers = recoMuon->innerTrack()->hitPattern().trackerLayersWithMeasurement();
      mu_nValidHits = recoMuon->innerTrack()->hitPattern().numberOfValidTrackerHits();
      mu_nPixelLayers = recoMuon->innerTrack()->hitPattern().pixelLayersWithMeasurement();
      mu_highPurity = recoMuon->innerTrack()->quality(reco::TrackBase::highPurity);
      if (recoMuon->globalTrack().isNonnull()) {
        mu_gnValidHits = recoMuon->globalTrack()->hitPattern().numberOfValidTrackerHits();
      }
      
      // w.r.t. main primary vertex
      // somehow often gives wrong sign 

      // global does not seem to give best result -> commented
      // if (mu_isGlobal) {
      //  mu_dxy = recoMuon->globalTrack()->dxy(pv);
      //  // take this from Stefan Wunsch setup 
      //  mu_dxyError = recoMuon->globalTrack()->d0Error();
      //  mu_dz = recoMuon->globalTrack()->dz(pv);
      //  mu_dzError = recoMuon->globalTrack()->dzError();
      // }
      //else {

#ifdef CMSSW42X
      // innertrack seems to work better than globaltrack
      // does it always exist??
      mu_dxy = recoMuon->innerTrack()->dxy(pv);
      // the following does not give better results
      // mu_dxy = recoMuon->innerTrack()->dxy(pv) * recoMuon->charge();
      // if (recoMuon->phi()<0) mu_dxy = -mu_dxy; 
      // dxy or d0??  *** investigate!
      //mu_dxyError = recoMuon->innerTrack()->dxyError();
      //   d0Error seems to work better than dxyError, 
      //   neither innertrack nor globaltrack work well,
      //   slightly better for innertrack 
      mu_dxyError = recoMuon->innerTrack()->d0Error();
      //   both globaltrack and inner track are reasonable, 
      //   slightly better for innertrack? 
      mu_dz = recoMuon->innerTrack()->dz(pv);
      // neither is good, but global track is somewhat better??
      mu_dzError = recoMuon->innerTrack()->dzError();
#else
      // use bestTrack when available in CMSSW
      mu_dxy = recoMuon->bestTrack()->dxy(pv);
      mu_dxyError = recoMuon->bestTrack()->d0Error();
      mu_dz = recoMuon->bestTrack()->dz(pv);
      mu_dzError = recoMuon->bestTrack()->dzError();
#endif
      // } // else

      // w.r.t. associated primary vertex, preset to beamspot
      //   beware, this will not work for muons close to, 
      //   but not associated to, vertex!
      // preset w.r.t. beamspot
      mu_dxyBest = recoMuon->innerTrack()->dxy(vertexBeamSpot);
      // does not work for z -> keep 0.
      // mu_dzBest = recoMuon->innerTrack()->dz(vertexBeamSpot);
      for (int mm = 0; mm<nmuonlist; ++mm) {
        if (muonid == muid[mm]) {
          // set primary vertex reference point bestpv
          bestpv.SetXYZ(mupvx[mm],mupvy[mm],mupvz[mm]);
          // fill distance
          mu_dxyBest = recoMuon->innerTrack()->dxy(bestpv);
          mu_dzBest = recoMuon->innerTrack()->dz(bestpv);
          mu_vtxId = muvtx[mm];
          mu_bestflag = 0;
          break;    // candidate found -> stop loop
        } //if  muonid
      } // for mm
    } // if global or tracker

    else if (mu_isStandAlone) {
      // standalone only muons
      mu_dxy = recoMuon->outerTrack()->dxy(pv);
      //mu_dxy = recoMuon->outerTrack()->dxy(pv) * recoMuon->charge();
      //if (recoMuon->phi()<0) mu_dxy = -mu_dxy; 
      mu_dxyBest = recoMuon->outerTrack()->dxy(vertexBeamSpot);
      mu_dxyError = recoMuon->outerTrack()->dxyError();
      mu_dz = recoMuon->outerTrack()->dz(pv);
      // somehow beamspot does not work for z (and does not make much sense)
      mu_dzBest = recoMuon->outerTrack()->dz();
      mu_dzError = recoMuon->outerTrack()->dzError();
      mu_bestflag = -2;
    } // else if standalone    

    // treat nonvertex-fitted muons 
    // if not directly associated to any vertex, find closest PV
    if (mu_bestflag<0) {
      float mindzvtx = 99.;
      int idmindzvtx = -1;
      // loop over PVs
      for (uint pvtx=0; pvtx<nPVtx; ++pvtx) {
        float dzvtx = recoMuon->vz()-PVtx_z[pvtx];
        if (abs(dzvtx)<abs(mindzvtx)) {
          mindzvtx = dzvtx;
          idmindzvtx = pvtx;
        } // if
      } // for
      // and store it if closer than some cut value
      if (abs(mindzvtx) < mindzvtxcut) {
        mu_bestflag = 1;
        mu_dzBest = mindzvtx;
        if (abs(mu_dzBest)>0.1) mu_bestflag=2;
        mu_vtxId = idmindzvtx;
      } // if
    } // if bestflag


    // Soft ID, see
    // https://twiki.cern.ch/twiki/bin/view/CMSPublic/SWGuideMuonID
    // https://twiki.cern.ch/twiki/bin/view/CMSPublic/SWGuideMuonIDRun2
    // for 53X and Run 2:
    bool mu_softId = mu_isGood && mu_nTrackerLayers > 5 && mu_nPixelLayers >0 && mu_highPurity && fabs(mu_dxy) < 0.3 && fabs(mu_dz)<20.;
    // for 42X:  (both defined for both)
    bool mu_softId4 = mu_isGood && mu_nValidHits > 10 && mu_nPixelLayers >1 && fabs(mu_dxy) < 3. && fabs(mu_dz)<30.;
    // relative to best (not main) pv; probably makes little difference
    bool mu_softIdBest = mu_isGood && mu_nValidHits > 10 && mu_nPixelLayers >1 && fabs(mu_dxyBest) < 3. && fabs(mu_dzBest)<30.;

    // MiniAOD preselection
    // to recover MiniAOD content, where available, use isMini
    bool mu_isMini = false;
#ifndef CMSSW42X
    // for 53X and higher
    if (recoMuon->pt() > 5. or (recoMuon->pt() > 3. and (recoMuon->isPFMuon() or recoMuon->isGlobalMuon() or recoMuon->isTrackerMuon() or recoMuon->isStandAloneMuon() or recoMuon->isRPCMuon())) or recoMuon->isPFMuon()) {
      mu_isMini = true;
    } // Mini
#endif
#ifdef CMSSW42X
    // for 42X: isPFMuon and isRPCMuon do not exist yet
    // *** need to add something to accept low pt muons? ***
    if (recoMuon->pt() > 5. or (recoMuon->pt() > 3. and (recoMuon->isGlobalMuon() or recoMuon->isTrackerMuon() or recoMuon->isStandAloneMuon())) ) {
      mu_isMini = true;
    } // Mini
#endif

    // ************************************************************
    // make preselection for ntuple storage: Mini or Loose or Soft:
    // ************************************************************
    // actually, removes almost nothing -> do not make cut, i.e. keep all AOD muons  
    //if (!mu_isMini && !mu_looseId && !mu_softId && !mu_softId4 && !mu_softIdBest) continue;

    // to recover nanoAOD content, where available, use isNano
    bool mu_isNano = false;
    if (mu_isMini && mu_looseId && recoMuon->pt() > 3.) {
      mu_isNano = true;
      ++Muon_nNano;
    } // Nano

    // medium ID
    // https://twiki.cern.ch/twiki/bin/view/CMSPublic/SWGuideMuonIDRun2
    float mu_validFraction = 0;
    if (recoMuon->innerTrack().isNonnull()) {
      mu_validFraction = recoMuon->innerTrack()->validFraction();
    }
    float mu_Chi2 = 0;
    if (recoMuon->innerTrack().isNonnull()) {
      mu_Chi2 = recoMuon->innerTrack()->normalizedChi2();
    }
    float mu_globalChi2 = 0;
    if (recoMuon->globalTrack().isNonnull()) {
      mu_globalChi2 = recoMuon->globalTrack()->normalizedChi2();
    }
    float mu_standPosMatch = recoMuon->combinedQuality().chi2LocalPosition;
    float mu_kink = recoMuon->combinedQuality().trkKink;  
    float mu_segmentComp = muon::segmentCompatibility(*recoMuon);
    bool mu_mediumId = false;
    if (mu_looseId && mu_validFraction>0.8 && ((mu_isGlobal && mu_globalChi2 < 3 && mu_standPosMatch<12 && mu_kink<20 && mu_segmentComp > 0.303) || mu_segmentComp >0.451)) {
      mu_mediumId = true;
    } // medium id

    // tight ID (with respect to main primary?), see
    // https://twiki.cern.ch/twiki/bin/view/CMSPublic/SWGuideMuonID
    // https://twiki.cern.ch/twiki/bin/view/CMSPublic/SWGuideMuonIDRun2
    int mu_gnValidMuHits = 0;
    if (recoMuon->globalTrack().isNonnull()) {
      mu_gnValidMuHits = recoMuon->globalTrack()->hitPattern().numberOfValidMuonHits();
    }

    // *** fill this properly ***
    float mu_dB = mu_dxy;

    int mu_nValidPixel=0;
    if (recoMuon->innerTrack().isNonnull()) {
      mu_nValidPixel = recoMuon->innerTrack()->hitPattern().numberOfValidPixelHits();
    } 
    int mu_gnValidPixel=0;
    if (recoMuon->globalTrack().isNonnull()) {
      mu_gnValidPixel = recoMuon->globalTrack()->hitPattern().numberOfValidPixelHits();
    } 
#ifndef CMSSW42X
    // 53X or higher:
    bool mu_tightId = mu_isGlobal && mu_isPFMuon && mu_globalChi2<10 && mu_gnValidMuHits>0 && mu_nStations>1 && (fabs(mu_dxy)<0.2 || fabs(mu_dB)<0.2)
      && fabs(mu_dz)<0.5 && mu_nValidPixel>0 && mu_nTrackerLayers>5;     
#endif
#ifdef CMSSW42X
    // 42X:
    bool mu_tightId = mu_isGlobal && mu_globalChi2<10 && mu_gnValidMuHits>0 && mu_nStations>1 && (fabs(mu_dxy)<0.2 || fabs(mu_dB)<0.2)
    && mu_nValidPixel > 0 && mu_nTrackerLayers>8;
    // the latter is roughly equivalent to mu_nValidHits>10     
#endif
    // compare to muon::isTightMuon(*recomuon,*vertices->begin());
    // compare to muon::isSoftMuon(*recomuon,*vertices->begin());

    // high pt ID
    // choose one of the following:
    // https://twiki.cern.ch/twiki/bin/view/CMSPublic/SWGuideMuonID
    // use main vertex vertex
    //bool mu_highPtId = muon::isHighPtMuon(*recoMuon,*PV_ite, reco::TunePType = muon::improvedTuneP);
#ifdef CMSSW7plus
    UChar_t mu_highPtId = muon::isHighPtMuon(*recoMuon,*PV_ite);
#endif
#ifdef CMSSW53X
    UChar_t mu_highPtId = muon::isHighPtMuon(*recoMuon,*PV_ite, improvedTuneP);
    //bool mu_highPtId = muon::isHighPtMuon(*recoMuon,*PV_ite, muon::improvedTuneP);
#endif
#ifdef CMSSW42X
    // possibly for 42X only:
    //bool mu_highPtId = muon::isHighPtMuon(*recoMuon,*PV_ite);
    UChar_t mu_highPtId = false;
    //bool mu_highPtId = muon::isHighPtMuon(*recoMuon,*PV_ite, muon::TunePType= muon::defaultTuneP);
#endif

    // calculate relative isolation from PF
    // n/a in 4-2-8, *** need to IFDEF! ***  see commented include!
    // (search for CHANGE)
    // need to add determination from PAT::
#ifndef CMSSW42X
    // should we check recoMuon->isPFisolationValid?
    mu_relPFIsoR03 = ((recoMuon->pfIsolationR03()).sumChargedHadronPt +
    		      (recoMuon->pfIsolationR03()).sumNeutralHadronEt + 
    		      (recoMuon->pfIsolationR03()).sumPhotonEt) / recoMuon->pt();	
    mu_relPFIsoR04 = ((recoMuon->pfIsolationR04()).sumChargedHadronPt +
    		      (recoMuon->pfIsolationR04()).sumNeutralHadronEt + 
    		      (recoMuon->pfIsolationR04()).sumPhotonEt) / recoMuon->pt();
#endif

    // add isolation without PF instead 
    mu_relIsoR03 = ((recoMuon->isolationR03()).sumPt +
    		    (recoMuon->isolationR03()).hadEt + 
    		    (recoMuon->isolationR03()).emEt) / recoMuon->pt();	

    mu_relIsoR05 = ((recoMuon->isolationR05()).sumPt +
    		    (recoMuon->isolationR05()).hadEt + 
    		    (recoMuon->isolationR05()).emEt) / recoMuon->pt();
    
    //cout << "Muon pt " << recoMuon->pt() << " best " << recoMuon->bestTrack()->pt() << endl;
    //if (mu_isGlobal) { cout << "global " << recoMuon->globalTrack()->pt() << endl;} 
    //if (mu_isGlobal || mu_isTracker) { cout << "inner " << recoMuon->innerTrack()->pt() << endl;} 

    // calculate impact parameter and its significance
    mu_sip3d = 0;
    if (recoMuon->isGlobalMuon()) {  
      mu_ip3d = sqrt((recoMuon->globalTrack()->dxy(pv) * recoMuon->globalTrack()->dxy(pv)) + (recoMuon->globalTrack()->dz(pv) * recoMuon->globalTrack()->dz(pv)));
      if (mu_bestflag>-1) mu_ip3dBest = sqrt((recoMuon->globalTrack()->dxy(bestpv) * recoMuon->globalTrack()->dxy(bestpv)) + (recoMuon->globalTrack()->dz(bestpv) * recoMuon->globalTrack()->dz(bestpv))); 
      mu_Errip3d = sqrt((recoMuon->globalTrack()->d0Error() * recoMuon->globalTrack()->d0Error()) + (recoMuon->globalTrack()->dzError() * recoMuon->globalTrack()->dzError()));
      mu_sip3d = mu_ip3d / mu_Errip3d;
      if (mu_bestflag>-1) mu_sip3dBest = mu_ip3dBest / mu_Errip3d;
    } // global muon
    else if (recoMuon->isTrackerMuon()) {
      mu_ip3d = sqrt((recoMuon->innerTrack()->dxy(pv) * recoMuon->innerTrack()->dxy(pv)) + (recoMuon->innerTrack()->dz(pv) * recoMuon->innerTrack()->dz(pv)));
      if (mu_bestflag>-1) mu_ip3dBest = sqrt((recoMuon->innerTrack()->dxy(bestpv) * recoMuon->innerTrack()->dxy(bestpv)) + (recoMuon->innerTrack()->dz(bestpv) * recoMuon->innerTrack()->dz(bestpv)));
      mu_Errip3d = sqrt((recoMuon->innerTrack()->d0Error() * recoMuon->innerTrack()->d0Error()) + (recoMuon->innerTrack()->dzError() * recoMuon->innerTrack()->dzError()));
      mu_sip3d = mu_ip3d / mu_Errip3d;
      if (mu_bestflag>-1) mu_sip3dBest = mu_ip3dBest / mu_Errip3d;
    } // tracker muon
 
      // now store the muon candidate 
      if (Muon_pt.size() < nReserve_Muon) {

       // store nanoAOD extensions only if flag set
       if (nanoext || mu_isNano) {
         
        // official nanoAOD variables

        // kinematic quantities, default is tracker muon quantities, 
        // when available, best other otherwise 
	Muon_charge.push_back(recoMuon->charge());
	Muon_pt.push_back(recoMuon->pt());
        float mu_ptErr = -1.;
#ifndef CMSSW42X
	mu_ptErr = recoMuon->muonBestTrack()->ptError();
#endif
#ifdef CMSSW42X	
        if (recoMuon->innerTrack().isNonnull()) {
	  mu_ptErr = recoMuon->innerTrack()->ptError();
        }
        else if (recoMuon->globalTrack().isNonnull()) {
	  mu_ptErr = recoMuon->globalTrack()->ptError();
        }
#endif	
	Muon_ptErr.push_back(mu_ptErr);
        // ?(muonBestTrack().ptError()/muonBestTrack().pt() < 0.2)?2:0
        if (mu_ptErr/recoMuon->pt()<0.2) Muon_tightCharge.push_back(2);
        else Muon_tightCharge.push_back(0);
	Muon_eta.push_back(recoMuon->eta());
	Muon_phi.push_back(recoMuon->phi());
	Muon_mass.push_back(mumass);
        //cout << "Muon pt, eta, phi " << recoMuon->charge()*recoMuon->pt() << " " << recoMuon->eta() << " " << recoMuon->phi() << endl; 

	// dxy, dz, ip
	Muon_dxy.push_back(mu_dxy);
	Muon_dxyBest.push_back(mu_dxyBest);
	Muon_dxyErr.push_back(mu_dxyError);
	Muon_dz.push_back(mu_dz);
	Muon_dzBest.push_back(mu_dzBest);
	Muon_dzErr.push_back(mu_dzError);	
	Muon_ip3d.push_back(mu_ip3d);
	Muon_sip3d.push_back(mu_sip3d);
	Muon_ip3dBest.push_back(mu_ip3dBest);
	Muon_sip3dBest.push_back(mu_sip3dBest);

        // isolation
        if (mu_relPFIsoR04 != -9999.) {
	  Muon_pfRelIso03_all.push_back(mu_relPFIsoR03);
	  Muon_pfRelIso04_all.push_back(mu_relPFIsoR04);
          // temporarily, until have figured out pileup correction 
          mu_pfIso = int(mu_relPFIsoR03<0.3)+int(mu_relPFIsoR03<0.25)+int(mu_relPFIsoR03<0.2)+int(mu_relPFIsoR03<0.15)+int(mu_relPFIsoR03<0.1)+int(mu_relPFIsoR03<0.05);
        } // if
	else { // for CMSSW 4-2-8
	  Muon_pfRelIso03_all.push_back(mu_relIsoR03);
	  Muon_pfRelIso04_all.push_back(mu_relIsoR05);
          mu_pfIso = int(mu_relIsoR03<0.3)+int(mu_relIsoR03<0.25)+int(mu_relIsoR03<0.2)+int(mu_relIsoR03<0.15)+int(mu_relIsoR03<0.1)+int(mu_relIsoR03<0.05);
        } // else
#ifndef CMSSW42X     
	Muon_pfRelIso03_chg.push_back((recoMuon->pfIsolationR03()).sumChargedHadronPt / recoMuon->pt());
#endif
#ifdef CMSSW42X
        // not really appropriate (hadEt is presumably HCAL component) 
	// Muon_pfRelIso03_chg.push_back((recoMuon->isolationR03()).hadEt / recoMuon->pt());
	Muon_pfRelIso03_chg.push_back((recoMuon->isolationR03()).sumPt / recoMuon->pt());
#endif
        Muon_pfIsoId.push_back(mu_pfIso);
	Muon_miniPFRelIso_all.push_back(999.); // *
	Muon_miniPFRelIso_chg.push_back(999.); // *

	Muon_jetIdx.push_back(999);

        Muon_isGlobal.push_back(mu_isGlobal); 
        Muon_isTracker.push_back(mu_isTracker); 
	Muon_isPFcand.push_back(mu_isPFMuon);

	// IDs (not directly available in CMSSW53X)
	// Muon_mediumId.push_back(recoMuon->passed(recoMuon->CutBasedIdMedium));
	// Muon_softId.push_back(recoMuon->passed(recoMuon->SoftCutBasedId));
	// Muon_tightId.push_back(recoMuon->passed(recoMuon->CutBasedIdTight));
        // calculated by hand above //
	Muon_softId.push_back(mu_softId);
	Muon_mediumId.push_back(mu_mediumId); 
	Muon_tightId.push_back(mu_tightId);
	Muon_highPtId.push_back(mu_highPtId);

	Muon_nStations.push_back(mu_nStations);

	// unsigned should be int & can't be (-)
	// Fill these variables with something nonsense atm
        // not yet being sorted!
	Muon_cleanmask.push_back('9'); // *
	Muon_genPartFlav.push_back('9'); // *
	Muon_genPartIdx.push_back(999); // *
	Muon_mvaTTH.push_back(999.); // *
	Muon_nTrackerLayers.push_back(mu_nTrackerLayers);
	// this needs to be changed to id after truth matching 
	Muon_pdgId.push_back(-13*recoMuon->charge());
	Muon_segmentComp.push_back(mu_segmentComp);

        ///////////////////////////////
        // nanoAOD-like extensions
        ///////////////////////////////

        // this is relevant when not all muon candidates are stored, 
        // redundant otherwise
        Muon_Id.push_back(muonid);

        Muon_x.push_back(recoMuon->vx());
        Muon_y.push_back(recoMuon->vy());
        Muon_z.push_back(recoMuon->vz());

        if (recoMuon->globalTrack().isNonnull()) {
	  Muon_gpt.push_back(recoMuon->globalTrack()->pt());
          Muon_geta.push_back(recoMuon->globalTrack()->eta());
          Muon_gphi.push_back(recoMuon->globalTrack()->phi());
        }
        else {
	  Muon_gpt.push_back(-1.);
          Muon_geta.push_back(0.);
          Muon_gphi.push_back(0.);
        }
        Muon_looseId.push_back(mu_looseId);
	Muon_softId4.push_back(mu_softId4);
	Muon_softIdBest.push_back(mu_softIdBest);
        Muon_isMini.push_back(mu_isMini); 
        Muon_isNano.push_back(mu_isNano); 
        Muon_isGood.push_back(mu_isGood); 
        Muon_isGoodLast.push_back(mu_isGoodLast); 
        Muon_isGoodAng.push_back(mu_isGoodAng); 
        Muon_isArbitrated.push_back(mu_isArbitrated); 
        Muon_isStandAlone.push_back(mu_isStandAlone); 
	Muon_isRPCcand.push_back(mu_isRPCMuon);
	Muon_nValid.push_back(mu_nValidHits);
	Muon_nPix.push_back(mu_nValidPixel);
	Muon_Chi2.push_back(mu_Chi2);
	Muon_gnValid.push_back(mu_gnValidHits);
	Muon_gnPix.push_back(mu_gnValidPixel);
	Muon_gChi2.push_back(mu_globalChi2);
        Muon_gnValidMu.push_back(mu_gnValidMuHits);

        Muon_vtxIdx.push_back(mu_vtxId);
        Muon_vtxFlag.push_back(mu_bestflag);

#ifndef miniAOD
	// find muon in generaltracks list
        int itcount = -1;
        int mu_trkIdx = -1;
        if ((recoMuon->innerTrack()).isNonnull()) {
          for (reco::TrackCollection::const_iterator itrack = tracks->begin(); itrack != tracks->end(); ++itrack) {
            ++itcount;
            //if (*(itrack) == *(recoMuon->track())) {
	    //if (itrack->track() == recoMuon->track()) {
	    //if (itrack->get<TrackRef>() == recoMuon->track()) {
	    //if (*(itrack)->get<TrackRef>() == recoMuon->track()) {
	    //if (*(itrack).get<TrackRef>() == recoMuon->track()) {
	    //if (itrack == (recoMuon->track())) {
	    //if (itrack == (recoMuon->track())->get()) {
	    //if (itrack == (recoMuon->track()).get()) {
	    //if (itrack == recoMuon->track().get()) {
	    //if (itrack->ref() == (recoMuon->track())) {
	    //if (*(itrack) == (recoMuon->track())) {          
	    //if (itrack->pt() == (recoMuon->track())->pt()) {
            // capitulate: (is this safe?)
	    if (itrack->pt() == recoMuon->innerTrack()->pt()) {
              // muon found
              mu_trkIdx = itcount;
              break; 
	    } // if
          } // for
        } // nonnull
        Muon_trkIdx.push_back(mu_trkIdx);
#endif 
#ifdef miniAOD
        // not yet treated
        Muon_trkIdx.push_back(-1); // *
#endif

        // check for associated simulated muon from prestored list
        // bool musimfound = false;
        int genp_size = GenPart_eta.size();
        int best_muon_idx = -1;
        float best_delta_r = 99;
        for (int im = 0; im < genp_size; ++im) {
          if (abs(GenPart_pdgId.at(im)) != 13) continue;
          if (recoMuon->charge() != GenPart_charge.at(im)) continue;
          float delta_r = deltaR(GenPart_eta.at(im), GenPart_phi.at(im), recoMuon->eta(), recoMuon->phi());
          //cout << "Calculated deltaR: " << delta_r << endl;
          if ((delta_r < 0.03) && (delta_r < best_delta_r)) {
            best_muon_idx = im;
            best_delta_r = delta_r;
          }
        }

        Muon_simIdx.push_back(best_muon_idx);
        /* cout << "best id: " << best_muon_idx << " best deltaR " << best_delta_r << endl;
        if (best_muon_idx > -1) {
          cout << "Reco: Muon pt: " << recoMuon->pt() << " Muon eta: " << recoMuon->eta() << " Muon phi: " << recoMuon->phi() << endl;
          cout << "Gen : Muon pt: " << GenPart_pt.at(best_muon_idx) << " Muon eta: " << GenPart_eta.at(best_muon_idx) << " Muon phi: " << GenPart_phi.at(best_muon_idx) << endl;
        } */
        /* for (int im=0; im<genp_size; ++im) {
          if (abs(GenPart_pdgId.at(im)) != 13) continue;
          if (recoMuon->charge()==GenPart_charge.at(im) && abs(recoMuon->pt()-GenPart_pt.at(im))/GenPart_pt.at(im) < 0.1 && abs(recoMuon->eta()-GenPart_eta.at(im))<0.1 && fmod(abs(recoMuon->phi()-GenPart_phi.at(im)),2.*pi)<0.1) {
            musimfound = true;
            Muon_simIdx.push_back(im);
            break;
          } // if
	} // im
        if (!musimfound) {
          Muon_simIdx.push_back(-1);
        } // !musimfound  */   

       } // nanoext
      } // muon size
      else {
	std::cout << "WARNING!!!!! NO. OF Mu IS MORE THAN YOUR RESERVED NO.!!!!!!" << std::endl;
      } // else

  } // end of loop recoMuon

  // set actual size
  nMuon = Muon_pt.size();

  // Resort muons: isNano, ordered according to pt, first! 
  // then the others
  for (int i1=0; i1<int(nMuon)-1; ++i1) {
    for (int i2= i1+1; i2<int(nMuon); ++i2) {
      if (Muon_isNano[i2] && (!Muon_isNano[i1] || 
			      Muon_pt[i2] > Muon_pt[i1])) {

        Int_t tempi = Muon_charge[i1];
        Muon_charge[i1] = Muon_charge[i2];
        Muon_charge[i2] = tempi;    
        tempi = Muon_tightCharge[i1];
        Muon_tightCharge[i1] = Muon_tightCharge[i2];
        Muon_tightCharge[i2] = tempi;    
        Float_t temp = Muon_pt[i1];
        Muon_pt[i1] = Muon_pt[i2];
        Muon_pt[i2] = temp;    
        temp = Muon_ptErr[i1];
        Muon_ptErr[i1] = Muon_ptErr[i2];
        Muon_ptErr[i2] = temp;    
        temp = Muon_eta[i1];
        Muon_eta[i1] = Muon_eta[i2];
        Muon_eta[i2] = temp;    
        temp = Muon_phi[i1];
        Muon_phi[i1] = Muon_phi[i2];
        Muon_phi[i2] = temp;    
        temp = Muon_mass[i1];
        Muon_mass[i1] = Muon_mass[i2];
        Muon_mass[i2] = temp;    

        temp = Muon_dxy[i1];
        Muon_dxy[i1] = Muon_dxy[i2];
        Muon_dxy[i2] = temp;    
        temp = Muon_dxyBest[i1];
        Muon_dxyBest[i1] = Muon_dxyBest[i2];
        Muon_dxyBest[i2] = temp;    
        temp = Muon_dxyErr[i1];
        Muon_dxyErr[i1] = Muon_dxyErr[i2];
        Muon_dxyErr[i2] = temp;    
        temp = Muon_dz[i1];
        Muon_dz[i1] = Muon_dz[i2];
        Muon_dz[i2] = temp;    
        temp = Muon_dzBest[i1];
        Muon_dzBest[i1] = Muon_dzBest[i2];
        Muon_dzBest[i2] = temp;    
        temp = Muon_dzErr[i1];
        Muon_dzErr[i1] = Muon_dzErr[i2];
        Muon_dzErr[i2] = temp;    
        temp = Muon_ip3d[i1];
        Muon_ip3d[i1] = Muon_ip3d[i2];
        Muon_ip3d[i2] = temp;    
        temp = Muon_sip3d[i1];
        Muon_sip3d[i1] = Muon_sip3d[i2];
        Muon_sip3d[i2] = temp;    
        temp = Muon_ip3dBest[i1];
        Muon_ip3dBest[i1] = Muon_ip3dBest[i2];
        Muon_ip3dBest[i2] = temp;    
        temp = Muon_sip3dBest[i1];
        Muon_sip3dBest[i1] = Muon_sip3dBest[i2];
        Muon_sip3dBest[i2] = temp;    

        temp = Muon_pfRelIso03_all[i1];
        Muon_pfRelIso03_all[i1] = Muon_pfRelIso03_all[i2];
        Muon_pfRelIso03_all[i2] = temp;
        temp = Muon_pfRelIso03_chg[i1];
        Muon_pfRelIso03_chg[i1] = Muon_pfRelIso03_chg[i2];
        Muon_pfRelIso03_chg[i2] = temp;
        temp = Muon_pfRelIso04_all[i1];
        Muon_pfRelIso04_all[i1] = Muon_pfRelIso04_all[i2];
        Muon_pfRelIso04_all[i2] = temp;
        tempi = Muon_pfIsoId[i1];
        Muon_pfIsoId[i1] = Muon_pfIsoId[i2];
        Muon_pfIsoId[i2] = tempi;
        temp = Muon_miniPFRelIso_all[i1];
        Muon_miniPFRelIso_all[i1] = Muon_miniPFRelIso_all[i2];
        Muon_miniPFRelIso_all[i2] = temp;
        temp = Muon_miniPFRelIso_chg[i1];
        Muon_miniPFRelIso_chg[i1] = Muon_miniPFRelIso_chg[i2];
        Muon_miniPFRelIso_chg[i2] = temp;

        tempi = Muon_jetIdx[i1];
        Muon_jetIdx[i1] = Muon_jetIdx[i2];
        Muon_jetIdx[i2] = tempi;    

        uint8_t tempi8 = Muon_isGlobal[i1];
        Muon_isGlobal[i1] = Muon_isGlobal[i2];
        Muon_isGlobal[i2] = tempi8;    
        tempi8 = Muon_isTracker[i1];
        Muon_isTracker[i1] = Muon_isTracker[i2];
        Muon_isTracker[i2] = tempi8;    
        tempi8 = Muon_isPFcand[i1];
        Muon_isPFcand[i1] = Muon_isPFcand[i2];
        Muon_isPFcand[i2] = tempi8;    

        tempi8 = Muon_softId[i1];
        Muon_softId[i1] = Muon_softId[i2];
        Muon_softId[i2] = tempi8;    
        tempi8 = Muon_mediumId[i1];
        Muon_mediumId[i1] = Muon_mediumId[i2];
        Muon_mediumId[i2] = tempi8;    
        tempi8 = Muon_tightId[i1];
        Muon_tightId[i1] = Muon_tightId[i2];
        Muon_tightId[i2] = tempi8;    
        UChar_t tempU = Muon_highPtId[i1];
        Muon_highPtId[i1] = Muon_highPtId[i2];
        Muon_highPtId[i2] = tempU;    

        tempi = Muon_nStations[i1];
        Muon_nStations[i1] = Muon_nStations[i2];
        Muon_nStations[i2] = tempi;    
        tempi = Muon_nTrackerLayers[i1];
        Muon_nTrackerLayers[i1] = Muon_nTrackerLayers[i2];
        Muon_nTrackerLayers[i2] = tempi;    
        temp = Muon_segmentComp[i1];
        Muon_segmentComp[i1] = Muon_segmentComp[i2];
        Muon_segmentComp[i2] = temp;    

        tempi = Muon_pdgId[i1];
        Muon_pdgId[i1] = Muon_pdgId[i2];
        Muon_pdgId[i2] = tempi;    

        tempU = Muon_cleanmask[i1];
        Muon_cleanmask[i1] = Muon_cleanmask[i2];
        Muon_cleanmask[i2] = tempU;    
        temp = Muon_mvaTTH[i1];
        Muon_mvaTTH[i1] = Muon_mvaTTH[i2];
        Muon_mvaTTH[i2] = temp;    
        tempU = Muon_genPartFlav[i1];
        Muon_genPartFlav[i1] = Muon_genPartFlav[i2];
        Muon_genPartFlav[i2] = tempU;    
        tempi = Muon_genPartIdx[i1];
        Muon_genPartIdx[i1] = Muon_genPartIdx[i2];
        Muon_genPartIdx[i2] = tempi;    

        tempi = Muon_Id[i1];
        Muon_Id[i1] = Muon_Id[i2];
        Muon_Id[i2] = tempi;    

        temp = Muon_x[i1];
        Muon_x[i1] = Muon_x[i2];
        Muon_x[i2] = temp;    
        temp = Muon_y[i1];
        Muon_y[i1] = Muon_y[i2];
        Muon_y[i2] = temp;    
        temp = Muon_z[i1];
        Muon_z[i1] = Muon_z[i2];
        Muon_z[i2] = temp;    

        temp = Muon_gpt[i1];
        Muon_gpt[i1] = Muon_gpt[i2];
        Muon_gpt[i2] = temp;    
        temp = Muon_geta[i1];
        Muon_geta[i1] = Muon_geta[i2];
        Muon_geta[i2] = temp;    
        temp = Muon_gphi[i1];
        Muon_gphi[i1] = Muon_gphi[i2];
        Muon_gphi[i2] = temp;    

        tempi8 = Muon_looseId[i1];
        Muon_looseId[i1] = Muon_looseId[i2];
        Muon_looseId[i2] = tempi8;    
        tempi8 = Muon_softId4[i1];
        Muon_softId4[i1] = Muon_softId4[i2];
        Muon_softId4[i2] = tempi8;    
        tempi8 = Muon_softIdBest[i1];
        Muon_softIdBest[i1] = Muon_softIdBest[i2];
        Muon_softIdBest[i2] = tempi8;    
        tempi8 = Muon_isMini[i1];
        Muon_isMini[i1] = Muon_isMini[i2];
        Muon_isMini[i2] = tempi8;    
        tempi8 = Muon_isNano[i1];
        Muon_isNano[i1] = Muon_isNano[i2];
        Muon_isNano[i2] = tempi8;    
        tempi8 = Muon_isGood[i1];
        Muon_isGood[i1] = Muon_isGood[i2];
        Muon_isGood[i2] = tempi8;    
        tempi8 = Muon_isGoodLast[i1];
        Muon_isGoodLast[i1] = Muon_isGoodLast[i2];
        Muon_isGoodLast[i2] = tempi8;    
        tempi8 = Muon_isGoodAng[i1];
        Muon_isGoodAng[i1] = Muon_isGoodAng[i2];
        Muon_isGoodAng[i2] = tempi8;    
        tempi8 = Muon_isArbitrated[i1];
        Muon_isArbitrated[i1] = Muon_isArbitrated[i2];
        Muon_isArbitrated[i2] = tempi8;    
        tempi8 = Muon_isStandAlone[i1];
        Muon_isStandAlone[i1] = Muon_isStandAlone[i2];
        Muon_isStandAlone[i2] = tempi8;    
        tempi8 = Muon_isRPCcand[i1];
        Muon_isRPCcand[i1] = Muon_isRPCcand[i2];
        Muon_isRPCcand[i2] = tempi8;    
        tempi = Muon_nValid[i1];
        Muon_nValid[i1] = Muon_nValid[i2];
        Muon_nValid[i2] = tempi;    
        tempi = Muon_nPix[i1];
        Muon_nPix[i1] = Muon_nPix[i2];
        Muon_nPix[i2] = tempi;    
        temp = Muon_Chi2[i1];
        Muon_Chi2[i1] = Muon_Chi2[i2];
        Muon_Chi2[i2] = temp;    
        tempi = Muon_gnValid[i1];
        Muon_gnValid[i1] = Muon_gnValid[i2];
        Muon_gnValid[i2] = tempi;    
        tempi = Muon_gnPix[i1];
        Muon_gnPix[i1] = Muon_gnPix[i2];
        Muon_gnPix[i2] = tempi;    
        temp = Muon_gChi2[i1];
        Muon_gChi2[i1] = Muon_gChi2[i2];
        Muon_gChi2[i2] = temp;    
        tempi = Muon_gnValidMu[i1];
        Muon_gnValidMu[i1] = Muon_gnValidMu[i2];
        Muon_gnValidMu[i2] = tempi;    

        tempi = Muon_vtxIdx[i1];
        Muon_vtxIdx[i1] = Muon_vtxIdx[i2];
        Muon_vtxIdx[i2] = tempi;    
        tempi = Muon_vtxFlag[i1];
        Muon_vtxFlag[i1] = Muon_vtxFlag[i2];
        Muon_vtxFlag[i2] = tempi;    
        tempi = Muon_trkIdx[i1];
        Muon_trkIdx[i1] = Muon_trkIdx[i2];
        Muon_trkIdx[i2] = tempi;    
        tempi = Muon_simIdx[i1];
        Muon_simIdx[i1] = Muon_simIdx[i2];
        Muon_simIdx[i2] = tempi;    
      } // isNano
    } // i2
  } // i1

  // cout << "IS MUON LOOP OK? " << nMuon << endl;
  
  // cout << "hello dimuon" << endl;
 
// add refitted dimuon candidates (nonstandard extension)
#include "NanoDimu.cc.forinclude"

//////////////////////////////////////////////////////////////////////////////
////////////////////////////// Muon Collection End ///////////////////////////
//////////////////////////////////////////////////////////////////////////////

  // cout << "hello electron" << endl; 

//////////////////////////////////////////////////////////////////////////////
//////////// Electron, Photon, Tau, MET and Jet collections //////////////////
//////////////////////////////////////////////////////////////////////////////

// Electrons
/* 
// number of electrons already Ok, but order seemingly not -> reorder
  value_el_n = 0;
  Electron_nNano = 0;

  //int misshits = -9999;
  double IP3d_e = -9999.;
  double ErrIP3d_e = -9999.;
  double SIP3d_e = -9999.;
  //double relPFIso_e = -9999.;
  int elecid = -1;
  
  //"official" cut
  float el_min_pt = 5;
  // NanoAODplus cut
  if (nanoext) el_min_pt = 0;

  // for (auto it = electrons->begin(); it != electrons->end(); ++it) {
#ifndef miniAOD
  for (reco::GsfElectronCollection::const_iterator it = electrons->begin(); it != electrons->end(); ++it) {
#endif
#ifdef miniAOD
  for (pat::ElectronCollection::const_iterator it = electrons->begin(); it != electrons->end(); ++it) {
#endif
    ++elecid;

    // calculate variables

#ifndef CMSSW42X 
    // satisfies isPFcand   
    if (it->passingPflowPreselection())
      {
#endif
#ifndef CMSSW7plus
	// I need to define some vars before cut
        // see https://twiki.cern.ch/twiki/bin/view/CMSPublic/EgammaPublicData
	//misshits = ((it->gsfTrack())->trackerExpectedHitsInner()).numberOfHits();
#endif
	IP3d_e = sqrt ( (it->gsfTrack()->dxy(pv) * it->gsfTrack()->dxy(pv)) + (it->gsfTrack()->dz(pv) * it->gsfTrack()->dz(pv)) );
	ErrIP3d_e = sqrt ( (it->gsfTrack()->d0Error() * it->gsfTrack()->d0Error()) + (it->gsfTrack()->dzError() * it->gsfTrack()->dzError()) );
	SIP3d_e = IP3d_e / ErrIP3d_e;
    
#ifdef CMSSW53X    
	//relPFIso_e = ((it->pfIsolationVariables()).chargedHadronIso +
	//	      (it->pfIsolationVariables()).neutralHadronIso +
	//	      (it->pfIsolationVariables()).photonIso) /it->pt();
#endif
#ifndef CMSSW42X    
      }
#endif

    value_el_isNano[value_el_n] = false;
    if (it->pt() > el_min_pt) {
      if (it->pt() > 5) {
        value_el_isNano[value_el_n] = true;
        ++Electron_nNano;
      }
      value_el_pt[value_el_n] = it->pt();
      value_el_eta[value_el_n] = it->eta();
      value_el_phi[value_el_n] = it->phi();
      value_el_charge[value_el_n] = it->charge();
      // nuha
      value_el_tightCharge[value_el_n] = it->isGsfCtfScPixChargeConsistent() + it->isGsfScPixChargeConsistent();
      
      // the following was validated by comparison to the Run 2 nanoAOD
      value_el_mass[value_el_n] = it->mass();
      //value_el_mass[value_el_n] = emass;

      // need to define this for other CMSSW versions
#ifdef CMSSW42X
      // non pf based approximation, from 
      // https://twiki.cern.ch/twiki/bin/view/CMSPublic/EgammaPublicData
      // improve?
      if (it->isEB()) {
        // better to cut explicitly on eta of supercluster?
        value_el_pfreliso03all[value_el_n] =
	  (it->dr03TkSumPt() + max(0.,it->dr03EcalRecHitSumEt() -1.)
           + it->dr03HcalTowerSumEt() )/it->pt();
      }
      else {
        value_el_pfreliso03all[value_el_n] =
	  (it->dr03TkSumPt() + it->dr03EcalRecHitSumEt()
           + it->dr03HcalTowerSumEt() )/it->pt();
      }
      value_el_pfreliso03chg[value_el_n] = it->dr03TkSumPt()/it->pt();
#endif

#ifdef CMSSW53X
      // see https://twiki.cern.ch/twiki/bin/view/CMS/EgammaPFBasedIsolation
      // verify whether the isPFcand condition is needed here (in Run 2 it is not)
      if (it->passingPflowPreselection()) {
        auto iso03 = it->pfIsolationVariables();
        value_el_pfreliso03all[value_el_n] =
            (iso03.chargedHadronIso + iso03.neutralHadronIso + iso03.photonIso)/it->pt();
        value_el_pfreliso03chg[value_el_n] = iso03.chargedHadronIso/it->pt();
      } 
      else {
        value_el_pfreliso03all[value_el_n] = -999.;
        value_el_pfreliso03chg[value_el_n] = -999.;
      }
#endif

#ifdef CMSSW7plus
      // *** to be fixed!!! ***
      // from https://twiki.cern.ch/twiki/bin/view/CMSPublic/WorkBookMiniAOD2017
      // value_el_pfreliso03all[value_el_n] = (it->ecalPFClusterIso()+it->hcalPFClusterIso())/it->pt();
      // do *not* require isPFcand at this stage (validated with official nanoAOD)
      auto iso03 = it->pfIsolationVariables();
      value_el_pfreliso03all[value_el_n] = (iso03.sumChargedHadronPt + iso03.sumNeutralHadronEt + iso03.sumPhotonEt) / it->pt();
      value_el_pfreliso03chg[value_el_n] = iso03.sumChargedHadronPt / it->pt();
#endif

      // https://twiki.cern.ch/twiki/bin/view/CMS/EgammaCutBasedIdentification
      // nuha
      value_el_dr03TkSumPtOld[value_el_n] = it->dr03TkSumPt();
      value_el_dr03TkSumPt[value_el_n] = (it->pt() > 35.) ? it->dr03TkSumPt() : 0.;      
      value_el_dr03EcalRecHitSumEtOld[value_el_n] = it->dr03EcalRecHitSumEt();
      value_el_dr03EcalRecHitSumEt[value_el_n] = (it->pt() > 35.) ? it->dr03EcalRecHitSumEt() : 0.;
      
      // combine the next two variables?
      value_el_dr03HcalTowerSumEt[value_el_n] = it->dr03HcalTowerSumEt();
      // for the moment fill with same input (4_2_8)
     // nuha  *** A.G., the next might need to be reactivated ***
      //value_el_dr03HcalDepth1TowerSumEt[value_el_n] = it->dr03HcalTowerSumEt();
      //#ifdef CMSSW7plus     
      value_el_dr03HcalDepth1TowerSumEtOld[value_el_n] = it->dr03HcalDepth1TowerSumEt();
      value_el_dr03HcalDepth1TowerSumEt[value_el_n] = (it->pt() > 35.) ? it->dr03HcalDepth1TowerSumEt() : 0;      
      //#endif
            
      // do the next three contain redundancy?
      value_el_isEB[value_el_n] = it->isEB();
      value_el_isEE[value_el_n] = it->isEE();
      value_el_SCeta[value_el_n] = (it->superCluster())->eta();

      value_el_isPFcand[value_el_n] = 0;
#ifndef CMSSW42X
      value_el_isPFcand[value_el_n] = it->passingPflowPreselection();
#endif    
      // *** need to fix for Run 2 ***
#ifndef CMSSW7plus
      // https://twiki.cern.ch/twiki/bin/view/CMS/EgammaCutBasedIdentification
      value_el_lostHits[value_el_n] = ((it->gsfTrack())->trackerExpectedHitsInner()).numberOfHits();
      // nuha  (seems to yield same result, although not documented)
      //value_el_lostHits[value_el_n] = ((it->gsfTrack())->trackerExpectedHitsInner()).numberOfLostHits();
#endif
#ifdef CMSSW7plus
      value_el_lostHits[value_el_n] = ((it->gsfTrack())->hitPattern()).numberOfLostHits(reco::HitPattern::MISSING_INNER_HITS);
#endif
#ifdef CMSSW7plus
      //this is how it was before
      //value_el_lostHits[value_el_n] = 0;

      // only available from 94X onwards (do we need further ifdef?)
      // https://github.com/cms-sw/cmssw/blob/CMSSW_9_4_X/RecoEgamma/ElectronIdentification/plugins/cuts/GsfEleMissingHitsCut.cc
      value_el_lostHits[value_el_n] = ((it->gsfTrack())->hitPattern()).numberOfLostHits(reco::HitPattern::MISSING_INNER_HITS);
      // for 80X, they use this:
      // value_el_lostHits[value_el_n] = ((it->gsfTrack())->hitPattern()).numberOfHits(reco::HitPattern::MISSING_INNER_HITS);
      // https://github.com/cms-sw/cmssw/blob/CMSSW_8_0_X/RecoEgamma/ElectronIdentification/plugins/cuts/GsfEleMissingHitsCut.cc
      // not sure which one to use, need to discuss/check, see also 
      // https://github.com/cms-sw/cmssw/blob/CMSSW_7_6_X/RecoEgamma/ElectronIdentification/plugins/cuts/GsfEleMissingHitsCut.cc
      // https://github.com/cms-sw/cmssw/blob/CMSSW_10_2_X/RecoEgamma/ElectronIdentification/plugins/cuts/GsfEleMissingHitsCut.cc
#endif

      // are the next two redundant with convVeto?
      value_el_convDist[value_el_n] = it->convDist();
      value_el_convDcot[value_el_n] = it->convDcot();
      // flag to *pass* veto (i.e. accept if 1)
      // deal with -10000 entries (not treated so far)

      // nuha
      value_el_convVetoOld[value_el_n] = it->convDist()<0.02 && it->convDcot()<0.02;
      // conversion veto definition for Run 2 and Run1
      // Copy paste the def in: https://github.com/cms-sw/cmssw/blob/CMSSW_9_4_X/PhysicsTools/PatAlgos/plugins/PATElectronProducer.cc
      // set conversion veto selection
      bool passconversionveto = false;
#ifndef miniAOD
      if (hConversions.isValid()) {
	// this is recommended method
#ifdef CMSSW106plus
        // argument type has changed
	passconversionveto = !ConversionTools::hasMatchedConversion( *it, *hConversions, beamSpotHandle->position());
#else
	passconversionveto = !ConversionTools::hasMatchedConversion( *it, hConversions, beamSpotHandle->position());
#endif
      } else {
	// use missing hits without vertex fit method
#ifndef CMSSW7plus
	passconversionveto = it->gsfTrack()->trackerExpectedHitsInner().numberOfLostHits() < 1;
#endif
#ifdef CMSSW7plus
	passconversionveto = it->gsfTrack()->hitPattern().numberOfLostHits(reco::HitPattern::MISSING_INNER_HITS) < 1;
#endif	
      }
#endif
      value_el_convVeto[value_el_n] = passconversionveto;
	
      // *** recheck the following ***
      // see https://github.com/cms-sw/cmssw/blob/82e3ed4ea0b8756c412d1f56758b7698d717104e/PhysicsTools/NanoAOD/python/electrons_cff.py
      value_el_deltaEtaSC[value_el_n] = (it->superCluster()->eta()) - it->eta();
      value_el_deltaPhiSC[value_el_n] = (it->superCluster()->phi()) - it->phi();
      if (value_el_deltaPhiSC[value_el_n] > 3.1415) value_el_deltaPhiSC[value_el_n] = value_el_deltaPhiSC[value_el_n]-2.*3.1415;  
      if (value_el_deltaPhiSC[value_el_n] < -3.1415) value_el_deltaPhiSC[value_el_n] = value_el_deltaPhiSC[value_el_n]+2.*3.1415;  
      // nanoAODplus variables for Run 1
      value_el_deltaEtaSCtr[value_el_n] = it->deltaEtaSuperClusterTrackAtVtx();
      value_el_deltaPhiSCtr[value_el_n] = it->deltaPhiSuperClusterTrackAtVtx();
      // what about it->hadronicOverEM() ? see
      // https://twiki.cern.ch/twiki/bin/view/CMS/EgammaCutBasedIdentification
      value_el_hoe[value_el_n] = it->hcalOverEcal();
#ifdef CMSSW7plus
      // https://github.com/cms-sw/cmssw/blob/CMSSW_9_4_X/PhysicsTools/NanoAOD/python/electrons_cff.py#L268
      value_el_sieie[value_el_n] = it->full5x5_sigmaIetaIeta();
#endif
#ifndef CMSSW7plus
      value_el_sieie[value_el_n] = -9999.;
#endif
      // nanoAODplus variable for Run 1 (exists also for Run 2)
      value_el_sieieR1[value_el_n] = it->sigmaIetaIeta();
      // what about it->ecalEnergy()  and it->trackMomentumAtVtx().p() ? see
      // https://twiki.cern.ch/twiki/bin/view/CMS/EgammaCutBasedIdentification
      // what about it->ecalEnergy()  and it->trackMomentumAtVtx().p() ? see
      // https://twiki.cern.ch/twiki/bin/view/CMS/EgammaCutBasedIdentification
      // nuha
      value_el_eInvMinusPInvOld[value_el_n] = 1/it->p4().E() - 1/it->p4().P();
      value_el_eInvMinusPInv[value_el_n] = (1-(it->eSuperClusterOverP()) ) / it->ecalEnergy();

      // auto trk = it->gsfTrack();
      // see https://twiki.cern.ch/twiki/bin/view/CMSPublic/EgammaPublicData
      value_el_x[value_el_n] = it->gsfTrack()->vx();
      value_el_y[value_el_n] = it->gsfTrack()->vy();
      value_el_z[value_el_n] = it->gsfTrack()->vz();
      value_el_vtxIdx[value_el_n] = -1;
      for (int ele = 0; ele<neleclist; ++ele) {
        if (elecid == elid[ele]) {
          // set primary vertex reference point bestpv
          bestpv.SetXYZ(elpvx[ele],elpvy[ele],elpvz[ele]);
          // fill distance
          //el_dxyBest = it->gsfTrack()->dxy(bestpv);
          //el_dzBest = it->gsfTrack()->dz(bestpv);
          value_el_vtxIdx[value_el_n] = elvtx[ele];
          //el_bestflag = 0;
          //break;    // candidate found -> stop loop
        } //if  elecid
      } // for ee

      value_el_dxy[value_el_n] = it->gsfTrack()->dxy(pv);
      // value_el_dxy[value_el_n] = trk->dxy(pv) * it->charge();
      // if (it->phi()<0) value_el_dxy[value_el_n] = -value_el_dxy[value_el_n];
      if (it->phi()<0) value_el_dxy[value_el_n] = -value_el_dxy[value_el_n];
      value_el_dz[value_el_n] = it->gsfTrack()->dz(pv);
      value_el_dxyErr[value_el_n] = it->gsfTrack()->d0Error();
      value_el_dzErr[value_el_n] = it->gsfTrack()->dzError();
  
      value_el_ip3d[value_el_n] = IP3d_e;
      value_el_sip3d[value_el_n] = SIP3d_e;
      
      // for cut based selection cuts (2011 data and higher), see
      // https://twiki.cern.ch/twiki/bin/view/CMS/EgammaCutBasedIdentification
      // for 2010 data, see
      // https://twiki.cern.ch/twiki/bin/view/CMSPublic/EgammaPublicData
      bool eisVeto = true;
      bool eisLoose = true;
      bool eisMedium = true;
      bool eisTight = true;
      if (fabs(value_el_SCeta[value_el_n])<=1.479) {
      // barrel (are SCeta and isEB/isEE redundant?)
      // verify which of the two variable variants (with or without tr) 
      // should be used!
        // dEtaIn   *** recheck whether EtaSc or EtaSCtr! ***
        if (fabs(value_el_deltaEtaSC[value_el_n])>0.004) {
	  eisMedium = false;  
	  eisTight = false;  
        }
        if (fabs(value_el_deltaEtaSC[value_el_n])>0.007) {
	  eisVeto = false;  
	  eisLoose = false;  
        }
        // dPhiIn  *** recheck whether PhiSc or PhiSCtr! ***
        if (fabs(value_el_deltaPhiSC[value_el_n])>0.03) {
	  eisTight = false;
        } 
        if (fabs(value_el_deltaPhiSC[value_el_n])>0.06) {
	  eisMedium = false;
        } 
        if (fabs(value_el_deltaPhiSC[value_el_n])>0.15) {
	eisLoose = false;
        } 
        if (fabs(value_el_deltaPhiSC[value_el_n])>0.8) {
	  eisVeto = false;
        } 
        // sigmaIEtaIEta  *** recheck whether sieie or sieieR1! ***
        if (value_el_sieie[value_el_n]>0.01) {
	  eisTight = false;
	  eisMedium = false;
	  eisLoose = false;
	  eisVeto = false;
        }
        // H/E
        if (value_el_hoe[value_el_n]>0.12) {
	  eisTight = false;
	  eisMedium = false;
	  eisLoose = false;
        }
        if (value_el_hoe[value_el_n]>0.15) {
	  eisVeto = false;
        }
        // d0(vtx)
        if (fabs(value_el_dxy[value_el_n])>0.02) {
	  eisTight = false;
	  eisMedium = false;
	  eisLoose = false;
        }
        if (fabs(value_el_dxy[value_el_n])>0.04) {
	  eisVeto = false;
        }
        // dz(vtx)
        if (fabs(value_el_dz[value_el_n])>0.1) {
	  eisTight = false;
	  eisMedium = false;
        }
        if (fabs(value_el_dz[value_el_n])>0.2) {
	  eisLoose = false;
	  eisVeto = false;
        }
        // 1/E-1/p
        if (fabs(value_el_eInvMinusPInvOld[value_el_n])>0.05) { // nuha
	  eisTight = false;
	  eisMedium = false;
	  eisLoose = false;
        }
        // pf isolation/pt 
        if (value_el_pfreliso03all[value_el_n]>0.10) {
	  eisTight = false;
        }
        if (value_el_pfreliso03all[value_el_n]>0.15) {
	  eisMedium = false;
	  eisLoose = false;
	  eisVeto = false;
        }
        // conversion rejection
        // implement properly!
        if (!value_el_convVetoOld[value_el_n]) {
	  eisTight = false;
	  eisMedium = false;
	  eisLoose = false;
        }
        // missing hits
        if (value_el_lostHits[value_el_n]>0) {
	  eisTight = false;
        }
        if (value_el_lostHits[value_el_n]>1) {
	  eisMedium = false;
	  eisLoose = false;       
        }
      }
      else if (fabs(value_el_SCeta[value_el_n])<=2.5) {
      // end cap (are SCeta and isEB/isEE redundant?)
        // dEtaIn
        if (fabs(value_el_deltaEtaSC[value_el_n])>0.005) {
	  eisTight = false;  
        }
        if (fabs(value_el_deltaEtaSC[value_el_n])>0.007) {
	  eisMedium = false;  
        }
        if (fabs(value_el_deltaEtaSC[value_el_n])>0.009) {
	  eisLoose = false;  
        }
        if (fabs(value_el_deltaEtaSC[value_el_n])>0.010) {
	  eisVeto = false;  
        }
        // dPhiIn
        if (fabs(value_el_deltaPhiSC[value_el_n])>0.02) {
	  eisTight = false;
        } 
        if (fabs(value_el_deltaPhiSC[value_el_n])>0.03) {
	  eisMedium = false;
        } 
        if (fabs(value_el_deltaPhiSC[value_el_n])>0.10) {
	eisLoose = false;
        } 
        if (fabs(value_el_deltaPhiSC[value_el_n])>0.7) {
	  eisVeto = false;
        } 
        // sigmaIEtaIEta
        if (value_el_sieie[value_el_n]>0.03) {
	  eisTight = false;
	  eisMedium = false;
	  eisLoose = false;
	  eisVeto = false;
        }
        // H/E
        if (value_el_hoe[value_el_n]>0.10) {
	  eisTight = false;
	  eisMedium = false;
	  eisLoose = false;
        }
        // d0(vtx)
        if (fabs(value_el_dxy[value_el_n])>0.02) {
	  eisTight = false;
	  eisMedium = false;
	  eisLoose = false;
        }
        if (fabs(value_el_dxy[value_el_n])>0.04) {
	  eisVeto = false;
        }
        // dz(vtx)
        if (fabs(value_el_dz[value_el_n])>0.1) {
	  eisTight = false;
	  eisMedium = false;
        }
        if (fabs(value_el_dz[value_el_n])>0.2) {
	  eisLoose = false;
	  eisVeto = false;
        }
        // 1/E-1/p
        if (fabs(value_el_eInvMinusPInvOld[value_el_n])>0.05) { // nuha
	  eisTight = false;
	  eisMedium = false;
	  eisLoose = false;
        }
        // pf isolation/pt 
        if (value_el_pt[value_el_n]>20.) {
          if (value_el_pfreliso03all[value_el_n]>0.10) {
	    eisTight = false;
          }
          if (value_el_pfreliso03all[value_el_n]>0.15) {
	    eisMedium = false;
	    eisLoose = false;
	    eisVeto = false;
          }
        }
	else {
          if (value_el_pfreliso03all[value_el_n]>0.07) {
	    eisTight = false;
          }
          if (value_el_pfreliso03all[value_el_n]>0.10) {
	    eisMedium = false;
	    eisLoose = false;
          }
          if (value_el_pfreliso03all[value_el_n]>0.15) {
	    eisVeto = false;
          }
        }
        // conversion rejection
        // implement properly!
        if (value_el_convVetoOld[value_el_n]) {
	  eisTight = false;
	  eisMedium = false;
	  eisLoose = false;
        }
        // missing hits
        if (value_el_lostHits[value_el_n]>0) {
	  eisTight = false;
        }
        if (value_el_lostHits[value_el_n]>1) {
	  eisMedium = false;
	  eisLoose = false;       
        }
      }
      else {
        // this should not happen
	std::cout << "NanoAnalyzer: Electron cluster outside bound" << std::endl; 
	eisTight = false;
	eisMedium = false;
	eisLoose = false;
	eisVeto = false;
      }
      // set variable
      if (eisTight) value_el_cutBased[value_el_n] = 4;
      else if (eisMedium) value_el_cutBased[value_el_n] = 3;
      else if (eisLoose) value_el_cutBased[value_el_n] = 2;
      else if (eisVeto) value_el_cutBased[value_el_n] = 1;
      else value_el_cutBased[value_el_n] = 0;

      ++value_el_n;
      if (int(value_el_n) >= max_el) {
	std::cout << "NanoAnalyzer: max_el exceeded" << std::endl;
        continue;
      }

    } // pt 
  } // for 

  //  for 2010 data and nanoAOD, implement
  // Electron_dr03TkSumPt
  // Electron_dr03EcalRecHitSumEt
  // Electron_dr03HcalTowerSumEt  (2010)
  // Electron_dr03HcalDepth1TowerSumEt (nanoAOD)
  // Electron_isEB (2010)
  // Electron_isEE (2010)
  // Electron_lostHits (=misshits)
  // Electron_convDist (2010)
  // Electron_convDcot (2010)
  // Electron_convVeto 
  // Electron_cutBased (nanoAOD)
  // Electron_deltaEtaSC (it->superCluster()->eta()) - it->eta() (nanoAOD)
  // Electron_deltaPhiSC (it->superCluster()->phi()) - it->phi() (nanoAOD)
  // Electron_deltaEtaSCtr (it->deltaEtaSuperClusterTrackAtVtx())  (2010)
  // Electron_deltaPhiSCtr (it->deltaPhiSuperClusterTrackAtVtx())  (2010)
  // Electron_hoe (it->hcalOverEcal())
  // Electron_sieie (it->sigmaIetaIeta())
  // Electron_SCeta
  // Electron_eInvMinusPInv

  //  for 2011
  // Electron_pfRelIso03_chg

  //  for Run 2 
  // Electron_pfRelIso03_all
  // Electron_pfRelIso03_chg

  //  Sort electrons according to pt
  for (int i1=0; i1<int(value_el_n)-1; ++i1) {
    for (int i2= i1+1; i2<int(value_el_n); ++i2) {
      if (value_el_pt[i1]<value_el_pt[i2]) {

        Float_t temp = value_el_pt[i1];
        value_el_pt[i1] = value_el_pt[i2];
        value_el_pt[i2] = temp;    
        temp = value_el_eta[i1];
        value_el_eta[i1] = value_el_eta[i2];
        value_el_eta[i2] = temp;    
        temp = value_el_phi[i1];
        value_el_phi[i1] = value_el_phi[i2];
        value_el_phi[i2] = temp;    
        Int_t tempi = value_el_charge[i1];
        value_el_charge[i1] = value_el_charge[i2];
        value_el_charge[i2] = tempi;    
        temp = value_el_mass[i1];
        value_el_mass[i1] = value_el_mass[i2];
        value_el_mass[i2] = temp;
	// nuha
        tempi = value_el_tightCharge[i1];
        value_el_tightCharge[i1] = value_el_tightCharge[i2];
        value_el_tightCharge[i2] = tempi;

        temp = value_el_pfreliso03all[i1];
        value_el_pfreliso03all[i1] = value_el_pfreliso03all[i2];
        value_el_pfreliso03all[i2] = temp;    
        temp = value_el_pfreliso03chg[i1];
        value_el_pfreliso03chg[i1] = value_el_pfreliso03chg[i2];
        value_el_pfreliso03chg[i2] = temp;

	// nuha
        temp = value_el_dr03TkSumPtOld[i1];
        value_el_dr03TkSumPtOld[i1] = value_el_dr03TkSumPtOld[i2];
        value_el_dr03TkSumPtOld[i2] = temp;
        temp = value_el_dr03TkSumPt[i1];
        value_el_dr03TkSumPt[i1] = value_el_dr03TkSumPt[i2];
        value_el_dr03TkSumPt[i2] = temp;	
        temp = value_el_dr03EcalRecHitSumEtOld[i1];
        value_el_dr03EcalRecHitSumEtOld[i1] = value_el_dr03EcalRecHitSumEtOld[i2];
        value_el_dr03EcalRecHitSumEtOld[i2] = temp;
        temp = value_el_dr03EcalRecHitSumEt[i1];
        value_el_dr03EcalRecHitSumEt[i1] = value_el_dr03EcalRecHitSumEt[i2];
        value_el_dr03EcalRecHitSumEt[i2] = temp;
		
        temp = value_el_dr03HcalTowerSumEt[i1];
        value_el_dr03HcalTowerSumEt[i1] = value_el_dr03HcalTowerSumEt[i2];
        value_el_dr03HcalTowerSumEt[i2] = temp;
	// nuha
        temp = value_el_dr03HcalDepth1TowerSumEtOld[i1];
        value_el_dr03HcalDepth1TowerSumEtOld[i1] = value_el_dr03HcalDepth1TowerSumEtOld[i2];
        value_el_dr03HcalDepth1TowerSumEtOld[i2] = temp;    
        temp = value_el_dr03HcalDepth1TowerSumEt[i1];
        value_el_dr03HcalDepth1TowerSumEt[i1] = value_el_dr03HcalDepth1TowerSumEt[i2];
        value_el_dr03HcalDepth1TowerSumEt[i2] = temp;
	
        bool tempb = value_el_isEB[i1];
        value_el_isEB[i1] = value_el_isEB[i2];
        value_el_isEB[i2] = tempb;    
        tempb = value_el_isEE[i1];
        value_el_isEE[i1] = value_el_isEE[i2];
        value_el_isEE[i2] = tempb;
	tempb = value_el_isPFcand[i1];
        value_el_isPFcand[i1] = value_el_isPFcand[i2];
        value_el_isPFcand[i2] = tempb;
	tempb = value_el_isNano[i1];
        value_el_isNano[i1] = value_el_isNano[i2];
        value_el_isNano[i2] = tempb;
        UChar_t tempU = value_el_lostHits[i1];
        value_el_lostHits[i1] = value_el_lostHits[i2];
        value_el_lostHits[i2] = tempU;    
        temp = value_el_convDist[i1];
        value_el_convDist[i1] = value_el_convDist[i2];
        value_el_convDist[i2] = temp;    
        temp = value_el_convDcot[i1];
        value_el_convDcot[i1] = value_el_convDcot[i2];
        value_el_convDcot[i2] = temp;
	// nuha
        tempb = value_el_convVetoOld[i1];
        value_el_convVetoOld[i1] = value_el_convVetoOld[i2];
        value_el_convVetoOld[i2] = tempb;
        tempb = value_el_convVeto[i1];
        value_el_convVeto[i1] = value_el_convVeto[i2];
        value_el_convVeto[i2] = tempb;
	
        temp = value_el_deltaEtaSC[i1];
        value_el_deltaEtaSC[i1] = value_el_deltaEtaSC[i2];
        value_el_deltaEtaSC[i2] = temp;    
        temp = value_el_deltaPhiSC[i1];
        value_el_deltaPhiSC[i1] = value_el_deltaPhiSC[i2];
        value_el_deltaPhiSC[i2] = temp;    
        temp = value_el_deltaEtaSCtr[i1];
        value_el_deltaEtaSCtr[i1] = value_el_deltaEtaSCtr[i2];
        value_el_deltaEtaSCtr[i2] = temp;    
        temp = value_el_deltaPhiSCtr[i1];
        value_el_deltaPhiSCtr[i1] = value_el_deltaPhiSCtr[i2];
        value_el_deltaPhiSCtr[i2] = temp;    
        temp = value_el_hoe[i1];
        value_el_hoe[i1] = value_el_hoe[i2];
        value_el_hoe[i2] = temp;    
        temp = value_el_sieie[i1];
        value_el_sieie[i1] = value_el_sieie[i2];
        value_el_sieie[i2] = temp; 
        temp = value_el_sieieR1[i1];
        value_el_sieieR1[i1] = value_el_sieieR1[i2];
        value_el_sieieR1[i2] = temp;       
        temp = value_el_SCeta[i1];
        value_el_SCeta[i1] = value_el_SCeta[i2];
        value_el_SCeta[i2] = temp;
	// nuha
        temp = value_el_eInvMinusPInvOld[i1];
        value_el_eInvMinusPInvOld[i1] = value_el_eInvMinusPInvOld[i2];
        value_el_eInvMinusPInvOld[i2] = temp;
        temp = value_el_eInvMinusPInv[i1];
        value_el_eInvMinusPInv[i1] = value_el_eInvMinusPInv[i2];
        value_el_eInvMinusPInv[i2] = temp;
	
        tempi = value_el_cutBased[i1];
        value_el_cutBased[i1] = value_el_cutBased[i2];
        value_el_cutBased[i2] = tempi;    
        //cout << value_el_cutBased[i1] << " " << value_el_cutBased[i2] << endl;

        temp = value_el_x[i1];
        value_el_x[i1] = value_el_x[i2];
        value_el_x[i2] = temp;    
        temp = value_el_y[i1];
        value_el_y[i1] = value_el_y[i2];
        value_el_y[i2] = temp;    
        temp = value_el_z[i1];
        value_el_z[i1] = value_el_z[i2];
        value_el_z[i2] = temp;    
        tempi = value_el_vtxIdx[i1];
        value_el_vtxIdx[i1] = value_el_vtxIdx[i2];
        value_el_vtxIdx[i2] = tempi;    

        temp = value_el_dxy[i1];
        value_el_dxy[i1] = value_el_dxy[i2];
        value_el_dxy[i2] = temp;    
        temp = value_el_dxyErr[i1];
        value_el_dxyErr[i1] = value_el_dxyErr[i2];
        value_el_dxyErr[i2] = temp;    
        temp = value_el_dz[i1];
        value_el_dz[i1] = value_el_dz[i2];
        value_el_dz[i2] = temp;    
        temp = value_el_dzErr[i1];
        value_el_dzErr[i1] = value_el_dzErr[i2];
        value_el_dzErr[i2] = temp;    

        temp = value_el_ip3d[i1];
        value_el_ip3d[i1] = value_el_ip3d[i2];
        value_el_ip3d[i2] = temp;    
        temp = value_el_sip3d[i1];
        value_el_sip3d[i1] = value_el_sip3d[i2];
        value_el_sip3d[i2] = temp;
      }
    }
  }

  // cout << "hello tau" << endl; 

  // Taus

  // to be fully implemented as indicated in 
  // https://twiki.cern.ch/twiki/bin/view/CMSPublic/SWGuidePFTauID#4XX
  //                                                               53X

  const float tau_min_pt = 15;
  value_tau_n = 0;
  //  for (auto it = taus->begin(); it != taus->end(); ++it) {
#ifndef miniAOD
  for (reco::PFTauCollection::const_iterator it = taus->begin(); it != taus->end(); ++it) {
#endif
#ifdef miniAOD
  for (pat::TauCollection::const_iterator it = taus->begin(); it != taus->end(); ++it) {
#endif
    if (it->pt() > tau_min_pt) {
      value_tau_pt[value_tau_n] = it->pt();
      value_tau_eta[value_tau_n] = it->eta();
      value_tau_phi[value_tau_n] = it->phi();
      value_tau_charge[value_tau_n] = it->charge();
      value_tau_mass[value_tau_n] = it->mass();
      value_tau_decaymode[value_tau_n] = it->decayMode();
#ifndef miniAOD
      value_tau_chargediso[value_tau_n] = it->isolationPFChargedHadrCandsPtSum();
      value_tau_neutraliso[value_tau_n] = it->isolationPFGammaCandsEtSum();
#endif
      ++value_tau_n;
      if (int(value_tau_n) >= max_tau) {
	std::cout << "NanoAnalyzer: max_tau exceeded" << std::endl;
        continue;
      }
    }
  }

  // cout << "hello photon" << endl; 

  // Photons

  value_ph_n = 0;
  const float ph_min_pt = 5;
  // for (auto it = photons->begin(); it != photons->end(); ++it) {
#ifndef miniAOD
  for (reco::PhotonCollection::const_iterator it = photons->begin(); it != photons->end(); ++it) {
#endif
#ifdef miniAOD
  for (pat::PhotonCollection::const_iterator it = photons->begin(); it != photons->end(); ++it) {
#endif
    if (it->pt() > ph_min_pt) {
      value_ph_pt[value_ph_n] = it->pt();
      value_ph_eta[value_ph_n] = it->eta();
      value_ph_phi[value_ph_n] = it->phi();
      value_ph_charge[value_ph_n] = it->charge();
      value_ph_mass[value_ph_n] = it->mass();
      value_ph_pfreliso03all[value_ph_n] = it->ecalRecHitSumEtConeDR03();
      ++value_ph_n;
      if (int(value_ph_n) >= max_ph) {
	std::cout << "NanoAnalyzer: max_ph exceeded" << std::endl;
        continue;
      }
    }
  }

  // cout << "hello MET" << endl; 

  // MET

  value_met_pt = met->begin()->pt();
  value_met_phi = met->begin()->phi();
  value_met_sumEt = met->begin()->sumEt();
  // avoid occasional crash on 2010 data due to singular matrix 
  if (value_met_sumEt == 0 || value_met_sumEt > 0.001)
    value_met_significance = met->begin()->significance();
  else {
    std::cout << "missing pt/ET: " << value_met_pt << " " << value_met_sumEt << std::endl;
    std::cout << "nanoAnalyzer: MET significance set to 0 to avoid singular matrix" << std::endl; 
    value_met_significance = 0.;
  }
  // cout << "hello MET 3" << endl; 
#ifdef CMSSW53X
  auto cov = met->begin()->getSignificanceMatrix();
  value_met_covxx = cov[0][0];
  value_met_covxy = cov[0][1];
  value_met_covyy = cov[1][1];
#else
  value_met_covxx = 0;
  value_met_covxy = 0;
  value_met_covyy = 0;
#endif

  // cout << "hello caloMET" << endl; 

  // caloMET
#ifndef miniAOD
  value_calomet_pt = calomet->begin()->pt();
  value_calomet_phi = calomet->begin()->phi();
  value_calomet_sumEt = calomet->begin()->sumEt();
#endif
#ifdef miniAOD
  value_calomet_pt = calomet->front().caloMETPt();
  value_calomet_phi = calomet->front().caloMETPhi();
  value_calomet_sumEt = calomet->front().caloMETSumEt();
#endif

  // cout << "hello Jet" << endl; 

  // Jets

////////////////////////////////////////////////////////

#ifndef noJetCor
  const JetCorrector* corrector = JetCorrector::getJetCorrector(mJetCorr, iSetup);
#endif
  const float jet_min_pt = 15;
  value_jet_n = 0;
  // for (auto it = jets->begin(); it != jets->end(); ++it) {
#ifndef miniAOD
  for (reco::PFJetCollection::const_iterator it = jets->begin(); it != jets->end(); ++it) {
#endif
#ifdef miniAOD
    for (pat::JetCollection::const_iterator it = jets->begin(); it != jets->end(); ++it) {
#endif 
    if (it->pt() > jet_min_pt) {
      // note that pt cut is applied to uncorrected jets
      value_jet_ptuncor[value_jet_n] = it->pt();
      value_jet_eta[value_jet_n] = it->eta();
      value_jet_phi[value_jet_n] = it->phi();
      value_jet_mass[value_jet_n] = it->mass();

      // cout << value_jet_n << " jet pt " << value_jet_pt[value_jet_n] << endl;

#ifndef noJetCor
      // do the jet corrections (if not turned off by #define noJetCor)

#ifdef CMSSW42X
// https://github.com/cms-sw/cmssw/blob/CMSSW_4_2_X/JetMETCorrections/Objects/interface/JetCorrector.h (lines 36-39)
// https://kmishra.net/talks/2012/CMSDAS_IntroTalk_2012-1.pdf (slide 17)

      int ijet = it - jets->begin();
      edm::RefToBase<reco::Jet> jetRef(edm::Ref<reco::PFJetCollection>(jets,ijet));
      double jec = corrector->correction(*it, jetRef, iEvent, iSetup);
#else
// https://github.com/cms-sw/cmssw/blob/master/JetMETCorrections/Objects/interface/JetCorrector.h (line 33)

      double jec = corrector->correction(*it, iEvent, iSetup);
#endif
      // copy original (uncorrected) jet;
      reco::PFJet corjet = *it;
      // apply JEC
      corjet.scaleEnergy(jec);

//      cout<<jec<<" "<<it->pt()<<" "<<corjet.pt()<<endl;

      value_jet_pt[value_jet_n] = corjet.pt();

#else

      value_jet_pt[value_jet_n] = it->pt();

#endif

#ifdef miniAOD
      int pfConstituents = 0;
#else
      int pfConstituents = it->getPFConstituents().size();
#endif */
      // quark-gluon separator (not yet working)
/*      double ptD = 0, pfqt2 = 0, pfqt = 0;
      for (int ii = 0; ii < pfConstituents; ii ++) {


        double apt = 0;
        const reco::PFCandidatePtr myptr =    (it->getPFConstituent(ii));
        apt = myptr->pt();
        pfqt2 += pow(apt, 2);
        pfqt += apt;
      }
      ptD = sqrt( pfqt2/pow(pfqt, 2) );*/

/*cout<<"ptD: "<<ptD<<endl;
cout<<"area: "<<it->jetArea()<<endl;
cout<<"constituents: "<<pfConstituents<<endl;
cout<<"el mult: "<<it->electronMultiplicity()<<endl;
cout<<"mu mult: "<<it->muonMultiplicity()<<endl;

cout<<"chEmEF: "<<it->chargedEmEnergyFraction()<<endl;
cout<<"chHEF: "<<it->chargedHadronEnergyFraction()<<endl;
cout<<"neEmEF: "<<it->neutralEmEnergyFraction()<<endl;
cout<<"neHEF: "<<it->neutralHadronEnergyFraction()<<endl;

cout<<"muEF: "<<it->muonEnergyFraction()<<endl;
cout<<"---------------------------------"<<endl;*/
/* 
//
//      value_jet_ptD[value_jet_n] = ptD;
      value_jet_area[value_jet_n] = it->jetArea();
      value_jet_nConstituents[value_jet_n] = pfConstituents;
      value_jet_nElectrons[value_jet_n] = it->electronMultiplicity();
      value_jet_nMuons[value_jet_n] = it->muonMultiplicity();
      value_jet_chEmEF[value_jet_n] = it->chargedEmEnergyFraction();
      value_jet_chHEF[value_jet_n] = it->chargedHadronEnergyFraction();
      value_jet_neEmEF[value_jet_n] = it->neutralEmEnergyFraction();
      value_jet_neHEF[value_jet_n] = it->neutralHadronEnergyFraction();
      value_jet_CEMF[value_jet_n] = it->chargedEmEnergyFraction();
      value_jet_MUF[value_jet_n] = it->muonEnergyFraction();
      value_jet_NumConst[value_jet_n] = it->chargedMultiplicity()+it->neutralMultiplicity(); 
      value_jet_CHM[value_jet_n] = it->chargedMultiplicity();
      bool looseJetID = (value_jet_neHEF[value_jet_n]<0.99 && value_jet_neEmEF[value_jet_n]<0.99 && value_jet_NumConst[value_jet_n]>1) && ((abs(value_jet_eta[value_jet_n])<=2.4 && value_jet_chHEF[value_jet_n]>0 && value_jet_CHM[value_jet_n]>0 && value_jet_CEMF[value_jet_n]<0.99) || abs(value_jet_eta[value_jet_n])>2.4);
      bool looseLepVetoJetID = (value_jet_neHEF[value_jet_n]<0.99 && value_jet_neEmEF[value_jet_n]<0.99 && value_jet_NumConst[value_jet_n]>1 && value_jet_MUF[value_jet_n]<0.8) && ((abs(value_jet_eta[value_jet_n])<=2.4 && value_jet_chHEF[value_jet_n]>0 && value_jet_CHM[value_jet_n]>0 && value_jet_CEMF[value_jet_n]<0.99) || abs(value_jet_eta[value_jet_n])>2.4);
      bool tightJetID = (value_jet_neHEF[value_jet_n]<0.9 && value_jet_neEmEF[value_jet_n]<0.9 && value_jet_NumConst[value_jet_n]>1) && ((abs(value_jet_eta[value_jet_n])<=2.4 && value_jet_chHEF[value_jet_n]>0 && value_jet_CHM[value_jet_n]>0 && value_jet_CEMF[value_jet_n]<0.9) || abs(value_jet_eta[value_jet_n])>2.4);
      bool tightLepVetoJetID = (value_jet_neHEF[value_jet_n]<0.9 && value_jet_neEmEF[value_jet_n]<0.9 && value_jet_NumConst[value_jet_n]>1 && value_jet_MUF[value_jet_n]<0.8) && ((abs(value_jet_eta[value_jet_n])<=2.4 && value_jet_chHEF[value_jet_n]>0 && value_jet_CHM[value_jet_n]>0 && value_jet_CEMF[value_jet_n]<0.9) || abs(value_jet_eta[value_jet_n])>2.4);
      int jetid = looseJetID*1+tightJetID*2+tightLepVetoJetID*4+looseLepVetoJetID*8;
      //cout<<"value_jet_n = "<<value_jet_n<<endl;
      //cout<<"looseJetID = "<<looseJetID<<" tightJetID = "<<tightJetID<<" jetid = "<<jetid<<endl;
      value_jet_id[value_jet_n] = jetid;   
      //cout<<looseJetID<<endl;
      //float NHF = it->neutralHadronEnergyFraction();
 */
      /*
      float NEMF = it->neutralEmEnergyFraction();
      float CHF = it->chargedHadronEnergyFraction();
      float MUF = it->muonEnergyFraction();
      float CEMF = it->chargedEmEnergyFraction();
      float NumConst = it->chargedMultiplicity()+it->neutralMultiplicity();
      float CHM = it->chargedMultiplicity();
       
      bool looseJetID = (NHF<0.99 && NEMF<0.99 && NumConst>1 && MUF<0.8) && ((abs(eta)<=2.4 && CHF>0 && CHM>0 && CEMF<0.99) || abs(eta)>2.4);
      bool tightJetID = (NHF<0.90 && NEMF<0.90 && NumConst>1 && MUF<0.8) && ((abs(eta)<=2.4 && CHF>0 && CHM>0 && CEMF<0.90) || abs(eta)>2.4);
      
      int jetid = looseJetID*1+tightJetID*2;
      value_jet_id[value_jet_n] = jetid;
   */   
/* 
      ++value_jet_n;
      if (int(value_jet_n) >= max_jet) {
        std::cout << "NanoAnalyzer: max_jet exceeded" << std::endl;
        continue;
      }
    }
  }

  // cout << "hello FatJet" << endl; 

  // FatJets
  const float fatjet_min_pt = 15;
  value_fatjet_n = 0;
#ifndef miniAOD
  for (reco::PFJetCollection::const_iterator it = fatjets->begin(); it != fatjets->end(); ++it) {
#endif
#ifdef miniAOD
    for (pat::JetCollection::const_iterator it = fatjets->begin(); it != fatjets->end(); ++it) {
#endif 
    if (it->pt() > fatjet_min_pt) {
      value_fatjet_pt[value_fatjet_n] = it->pt();
      value_fatjet_eta[value_fatjet_n] = it->eta();
      value_fatjet_phi[value_fatjet_n] = it->phi();
      value_fatjet_mass[value_fatjet_n] = it->mass();

      // cout << value_fatjet_n << " fatjet pt " << value_fatjet_pt[value_fatjet_n] << endl;

#ifdef miniAOD
      int pfConstituents = 0;
#else
      int pfConstituents = it->getPFConstituents().size();
#endif

      value_fatjet_area[value_fatjet_n] = it->jetArea();
      value_fatjet_nConstituents[value_fatjet_n] = pfConstituents;
#ifdef miniAOD
      // code claims fatjets were not made from a PFjet
      value_fatjet_nElectrons[value_fatjet_n] = 0;
      value_fatjet_nMuons[value_fatjet_n] = 0;
      value_fatjet_chEmEF[value_fatjet_n] = 0;
      value_fatjet_chHEF[value_fatjet_n] = 0;
      value_fatjet_neHEF[value_fatjet_n] = 0;
#else
      value_fatjet_nElectrons[value_fatjet_n] = it->electronMultiplicity();
      value_fatjet_nMuons[value_fatjet_n] = it->muonMultiplicity();
      value_fatjet_chEmEF[value_fatjet_n] = it->chargedEmEnergyFraction();
      value_fatjet_chHEF[value_fatjet_n] = it->chargedHadronEnergyFraction();
      value_fatjet_neEmEF[value_fatjet_n] = it->neutralEmEnergyFraction();
      value_fatjet_neHEF[value_fatjet_n] = it->neutralHadronEnergyFraction();
#endif

      ++value_fatjet_n;
      if (int(value_fatjet_n) >= max_fatjet) {
        cout << "NanoAnalyzer: max_fatjet exceeded" << endl;
        continue;
      }
    }
  }

  // cout << "hello before TrackJets" << endl; 

  // TrackJets, available for Run 1 and UL, not in between and not Run 3
  value_trackjet_n = 0;
#ifndef CMSSW11plus
#if defined (CMSSW42X) || defined (CMSSW53X) || defined(CMSSW106plus)
  const float trackjet_min_pt = 0;
#ifndef miniAOD
  for (reco::TrackJetCollection::const_iterator it = trackjets->begin(); it != trackjets->end(); ++it) {
#endif
#ifdef miniAOD
    for (pat::TrackJetCollection::const_iterator it = trackjets->begin(); it != trackjets->end(); ++it) {
#endif 
      //cout << "hello TrackJets start" << endl; 
    if (it->pt() > trackjet_min_pt) {
      //cout << "hello TrackJets " << value_trackjet_n << endl; 
      value_trackjet_pt[value_trackjet_n] = it->pt();
      value_trackjet_eta[value_trackjet_n] = it->eta();
      value_trackjet_phi[value_trackjet_n] = it->phi();
      value_trackjet_mass[value_trackjet_n] = it->mass();

      //int pfConstituents = it->getPFConstituents().size();

      value_trackjet_area[value_trackjet_n] = it->jetArea();
      value_trackjet_nConstituents[value_trackjet_n] = it->numberOfTracks();
      //value_trackjet_nElectrons[value_trackjet_n] = it->electronMultiplicity();
      value_trackjet_nElectrons[value_trackjet_n] = 0;
      //value_trackjet_nMuons[value_trackjet_n] = it->muonMultiplicity();
      value_trackjet_nMuons[value_trackjet_n] = 0;

      ++value_trackjet_n;
      if (int(value_trackjet_n) >= max_trackjet) {
        cout << "NanoAnalyzer: max_trackjet exceeded" << endl;
        continue;
      }
    }
  }
    // for CMSSW106plus
#endif
    // for CMSSW11plus
#endif
 */
    // cout << "hello after TrackJets" << endl; 

  // GenJets  //Qun

/*   const float gjet_min_pt = 10;
  value_gjet_n = 0;

  if (!isData) {

#ifndef miniAOD
  for (reco::GenJetCollection::const_iterator it = genjets->begin(); it != genjets->end(); ++it) {
#endif
#ifdef miniAOD
  for (reco::GenJetCollection::const_iterator it = genjets->begin(); it != genjets->end(); ++it) {
#endif 
    if (it->pt() > gjet_min_pt) {
      value_gjet_pt[value_gjet_n] = it->pt();
      value_gjet_eta[value_gjet_n] = it->eta();
      value_gjet_phi[value_gjet_n] = it->phi();
      value_gjet_mass[value_gjet_n] = it->mass();
      // cout << "genJet:" << value_gjet_n << " pt:" << value_gjet_pt[value_gjet_n] << " eta: "<< value_gjet_eta[value_gjet_n] << endl;
      ++value_gjet_n;
      if (int(value_gjet_n) >= max_gjet) {
        std::cout << "NanoAnalyzer: max_gjet exceeded" << std::endl;
        continue;
      }
    }
   } 
  } // isData */

    // cout << "hello after GenJets" << endl; 

  // afiqaize
  // *************************************************************
  // ----------------- get custom filter -------------------------
  // ************************************************************* 

  if (!custom_flag.empty()) {
    const edm::TriggerResults &custom_result = *custom_handle;
    const edm::TriggerNames &custom_list = iEvent.triggerNames(custom_result);

    for (size_t iFlag = 0; iFlag < custom_flag.size(); ++iFlag) {
      size_t iPath = custom_list.triggerIndex(custom_flag.at(iFlag));

      if (iPath < custom_result.size() and custom_result.accept(iPath))
        custom_bit.at(iFlag) = 1;
      else 
        custom_bit.at(iFlag) = 0;
    }
  }

#ifdef charm
  // cout << "hello D meson" << endl;

  // do D meson reconstruction (nonstandard extension)
#include "NanoDmeson.cc.forinclude" 
#endif

  Dstar_associationIdx.clear();
  Dstar_associationProb.clear();
  Dstar_associationchi2.clear();
  //Vertex fit
  //cout << "Calculating Vertex association QQ + C" << endl;
  //cout << "Size Dimu: " << Dimu_muon1TransientTrack.size() << " " << Dimu_charge.size() << " Size Dstar " << Dstar_pisTransientTrack.size() << " " << Dstar_hasMuon.size() << endl;
  size_t Dstar_size = Dstar_pisTransientTrack.size();
  size_t Dimu_size = Dimu_muon1TransientTrack.size();
  //cout << "Size Dimu: " << Dimu_size << ", Size Dstar: " << Dstar_size << endl;
  for (UInt_t j = 0; j < Dstar_size; ++j) {
    Int_t best_dimuidx = -99;
    Float_t vertex_probability = -99.;
    Float_t best_chi2_association = 9999.;
    bool track_calculate = true;
    if (DstarK_chg[j] == Dstarpi_chg[j]) track_calculate = false;
    for (UInt_t i = 0; i < Dimu_size; ++i) {
      if (Dimu_charge[i] != 0) track_calculate = false;
      
      if (track_calculate) {
        vector<TransientTrack> tracksAssociation;
        tracksAssociation.clear();
        tracksAssociation.push_back(Dimu_muon1TransientTrack[i]);
        tracksAssociation.push_back(Dimu_muon2TransientTrack[i]);
        tracksAssociation.push_back(Dstar_pisTransientTrack[j]);

        KalmanVertexFitter  theFitterAssociation(true);
        // do the fit
        TransientVertex associationVertex = theFitterAssociation.vertex(tracksAssociation);  
        if (!associationVertex.isValid()) continue;
        // vertex parameters
        float ndofAssociation = associationVertex.degreesOfFreedom();
        // for Adaptive Vertex Fitter this sum of weights*2-3
        // for Kalman Vertex Fitter this is 1 for two tracks?
        float chi2Association = associationVertex.totalChiSquared();
        if (chi2Association < best_chi2_association) {
          best_chi2_association = chi2Association;
          best_dimuidx = (Int_t) i;

          if (ndofAssociation >= 1) {
            vertex_probability = TMath::Prob(chi2Association,(Int_t)ndofAssociation); 
          }
        }
      }

      // confidence level
      
      // cut on chisquared (Adaptive) or CL (Kalman) here?

      //cout << "ndof: " << ndofAssociation << " chi2: " << chi2Association << " CL: " << CLAssociation << endl;

    }
    Dstar_associationIdx.push_back(best_dimuidx);
    Dstar_associationProb.push_back(vertex_probability);
    Dstar_associationchi2.push_back(best_chi2_association/3); // chi2/ndof
      
  }


  // cout << "fill tree" << endl;

  // Fill the tree for muon and dmeson
  t_event->Fill();

  // cout << "hello analysis end" << endl;

}//NanoAnalyzer::analyze ends

//**************************************************
//---------------------------Actual trigger analysis-------------  Qun below
//**************************************************
void NanoAnalyzer::analyzeTrigger(const edm::Event& iEvent, const edm::EventSetup& iSetup, const std::string& triggerName)
{
  using namespace std;
  using namespace edm;
  using namespace reco;
  using namespace trigger;

  std::cout<<"Currently analyzing trigger "<<triggerName<<std::endl;
  //Check the current configuration to see how many total triggers there are
  const unsigned int n(hltConfig_.size());
  //Get the trigger index for the current trigger
  const unsigned int triggerIndex(hltConfig_.triggerIndex(triggerName));
  //check that the trigger in the event and in the configuration agree
  assert(triggerIndex==iEvent.triggerNames(*triggerResultsHandle_).triggerIndex(triggerName));
  // abort on invalid trigger name
  if (triggerIndex>=n) {
    std::cout << "HLTEventAnalyzerAOD::analyzeTrigger: path "
	      << triggerName << " - not found!" << std::endl;
    return;
  }
  //else {cout << "HLTEventAnalyzerAOD inside loop QQQQQ " << triggerName_ << endl;}

  //const std::pair<int,int> prescales(hltConfig_.prescaleValues(iEvent,iSetup,triggerName));
  //cout << "HLTEventAnalyzerAOD::analyzeTrigger: path "
  //    << triggerName << " [" << triggerIndex << "] "
  //    << "prescales L1T,HLT: " << prescales.first << "," << prescales.second
  //    << endl;

  //  One could find the list of modules in a given trigger from the HLTConfigProvider as explained somewhere else in this code.
  //  Get index (slot position) of module giving the decision of the path as described in "DataFormats/Common/interface/HLTGlobalStatus.h"
  const unsigned int m(hltConfig_.size(triggerIndex));
  const vector<string>& moduleLabels(hltConfig_.moduleLabels(triggerIndex));
  const unsigned int moduleIndex(triggerResultsHandle_->index(triggerIndex));
  //cout << " Last active module - label/type: "
  //     << moduleLabels[moduleIndex] << "/" << hltConfig_.moduleType(moduleLabels[moduleIndex])
  //     << " [" << moduleIndex << " out of 0-" << (m-1) << " on this path]"
  //     << endl;

  assert (moduleIndex<m);


  for (unsigned int j=0; j<=moduleIndex; ++j) {
    const string& moduleLabel(moduleLabels[j]);
    const string  moduleType(hltConfig_.moduleType(moduleLabel));

    const unsigned int filterIndex(triggerEventHandle_->filterIndex(InputTag(moduleLabel,"",processName_)));
    if (filterIndex<triggerEventHandle_->sizeFilters()) {
//      cout << " 'L3' filter in slot " << j << " - label/type " << moduleLabel << "/" << moduleType << endl;
      const Vids& VIDS (triggerEventHandle_->filterIds(filterIndex));
      const Keys& KEYS(triggerEventHandle_->filterKeys(filterIndex));
      const size_type nI(VIDS.size());
      const size_type nK(KEYS.size());
      std::cout << "QQQ before  " << " accepted 'L3' objects found: " << " nI " << nI << " nK " << nK << std::endl;
      assert(nI==nK);
      const size_type n(max(nI,nK));
      std::cout << "QQQ   " << n  << " accepted 'L3' objects found: " << " nI " << nI << " nK " << nK << std::endl;
      const TriggerObjectCollection& TOC(triggerEventHandle_->getObjects());
      for (size_type i=0; i!=n; ++i) {
        const TriggerObject& TO(TOC[KEYS[i]]);
	std::cout << "QQQ Obj   " << i << " " << VIDS[i] << "/" << KEYS[i] << ": id, pt, eta, phi, mass"
             << TO.id() << " " << TO.pt() << " " << TO.eta() << " "
             << TO.phi() << " " << TO.mass()
		  << std::endl;
      }



    } //end of filterIndex
  } //end of moduleIndex
}// NanoAnalyzer::analyzeTrigger ends
//above Qun


//**************************************************
//************* additional methods *****************
//**************************************************

// ------------ method called once each job just before starting event loop  ------------
void 
//NanoAnalyzer::beginJob(const edm::Event& iEvent)
NanoAnalyzer::beginJob()
{
  file = new TFile(outFile.c_str(), "recreate"); // check
  t_event = new TTree("Events", "Events");
  //t_event->SetAutoSave(-500000000);
  t_event->SetAutoSave(0);

#if ROOT_VERSION_CODE > ROOT_VERSION(6, 6, 0)
  // mutiple threads caused trouble with trigger bit treatment
  t_event->SetImplicitMT(false);
#endif

#ifdef charm
  // histograms 
  /* h_trackpt = new TH1D("h_trackpt","",500, 0.,50.);
  h_trackptlow = new TH1D("h_trackptlow","",100, 0.,1.);
  h_trackptlowfine = new TH1D("h_trackptlowfine","",600, 0.,1.5);
  h_tracketa = new TH1D("h_tracketa","",160, -4.,4.);
  h_trackcov00 = new TH1D("h_trackcov00","",1000, 0.,0.001);

  h_d0pt = new TH1D("h_d0pt","",100, 0. ,15.);
  h_dstarpt = new TH1D("h_dstarpt","",100, 0. ,15.);
  h_PS3pt = new TH1D("h_PS3pt","",100, 0. ,2.);
  h_K1pt = new TH1D("h_K1pt","",100, 0. ,10.);
  h_P2pt = new TH1D("h_P2pt","",100, 0. ,10.);
  h_PS3eta = new TH1D("h_PS3eta","",50, -3. ,3.);
  h_P2eta = new TH1D("h_P2eta","",50, -3. ,3.);
  h_K1eta = new TH1D("h_K1eta","",50, -3. ,3.);
  h_D0masscut = new TH1D("h_D0masscut", "", 60, 1.6, 2.2);
  h_deltaMassD0Dstar = new TH1D("h_deltaMassD0Dstar", "", 64, 0.138, 0.17);
  h_D0masscut_rightDLcut = new TH1D("h_D0masscut_rightDLcut", "", 60, 1.6, 2.2);
  h_deltaMassD0Dstar_rightDLcut = new TH1D("h_deltaMassD0Dstar_rightDLcut", "", 64, 0.138, 0.17); */
#endif

  //---------------------------- Gen Particle reserve --------------------------//
  
  GenPart_pt.reserve(nReserve_GenPart);
  GenPart_eta.reserve(nReserve_GenPart);
  GenPart_phi.reserve(nReserve_GenPart);
  GenPart_mass.reserve(nReserve_GenPart);
  GenPart_charge.reserve(nReserve_GenPart);
  GenPart_pdgId.reserve(nReserve_GenPart);
  GenPart_status.reserve(nReserve_GenPart);
  GenPart_statusFlags.reserve(nReserve_GenPart);
  GenPart_genPartIdxMother.reserve(nReserve_GenPart);

  GenPart_Id.reserve(nReserve_GenPart);
  GenPart_isNano.reserve(nReserve_GenPart);
  GenPart_parpdgId.reserve(nReserve_GenPart);
  GenPart_sparpdgId.reserve(nReserve_GenPart);
  GenPart_numberOfDaughters.reserve(nReserve_GenPart);
  GenPart_nstchgdaug.reserve(nReserve_GenPart);
  // Josry2 prompt/nonprompt extension
  GenPart_promptFlag.reserve(nReserve_GenPart);
  GenPart_vx.reserve(nReserve_GenPart);
  GenPart_vy.reserve(nReserve_GenPart);
  GenPart_vz.reserve(nReserve_GenPart);
  GenPart_mvx.reserve(nReserve_GenPart);
  GenPart_mvy.reserve(nReserve_GenPart);
  GenPart_mvz.reserve(nReserve_GenPart);
  GenPart_recIdx.reserve(nReserve_GenPart);

  //--------------------------------- IsoTrack reserve -----------------------------//
  
  IsoTrack_dxy.reserve(nReserve_IsoTrack);
  IsoTrack_dz.reserve(nReserve_IsoTrack);
  IsoTrack_ets.reserve(nReserve_IsoTrack);
  IsoTrack_isHighPurityTrack.reserve(nReserve_IsoTrack);
  IsoTrack_isPFcand.reserve(nReserve_IsoTrack);
  IsoTrack_miniPFreliso_all.reserve(nReserve_IsoTrack);
  IsoTrack_miniPFreliso_chg.reserve(nReserve_IsoTrack);
  IsoTrack_pdgId.reserve(nReserve_IsoTrack);
  IsoTrack_PFreliso03_all.reserve(nReserve_IsoTrack);
  IsoTrack_PFreliso03_chg.reserve(nReserve_IsoTrack);
  IsoTrack_phi.reserve(nReserve_IsoTrack);
  IsoTrack_pt.reserve(nReserve_IsoTrack);

  //--------------------------------- OtherPV reserve -----------------------------//
  
  OtherPV_z.reserve(nReserve_OtherPV);

  //--------------------------------- PVtx reserve -----------------------------//
  
  PVtx_Id.reserve(nReserve_PVtx);
  PVtx_isMain.reserve(nReserve_PVtx);
  PVtx_isMainSim.reserve(nReserve_PVtx);
  PVtx_isGood.reserve(nReserve_PVtx);
  PVtx_isValid.reserve(nReserve_PVtx);
  PVtx_isFake.reserve(nReserve_PVtx);
  PVtx_isTrigUnique.reserve(nReserve_PVtx);
  PVtx_isUnbiased.reserve(nReserve_PVtx);
  PVtx_ntrk.reserve(nReserve_PVtx);
  PVtx_ntrkfit.reserve(nReserve_PVtx);
  PVtx_chi2.reserve(nReserve_PVtx);
  PVtx_ndof.reserve(nReserve_PVtx);
  PVtx_score.reserve(nReserve_PVtx);
  PVtx_sumPt.reserve(nReserve_PVtx);
  PVtx_Rho.reserve(nReserve_PVtx);
  PVtx_x.reserve(nReserve_PVtx);
  PVtx_y.reserve(nReserve_PVtx);
  PVtx_z.reserve(nReserve_PVtx);
  PVtx_Covxx.reserve(nReserve_PVtx);
  PVtx_Covyx.reserve(nReserve_PVtx);
  PVtx_Covzx.reserve(nReserve_PVtx);
  PVtx_Covyy.reserve(nReserve_PVtx);
  PVtx_Covzy.reserve(nReserve_PVtx);
  PVtx_Covzz.reserve(nReserve_PVtx);

  //--------------------------------- Muon reserve -----------------------------//
  
  Muon_charge.reserve(nReserve_Muon);
  Muon_tightCharge.reserve(nReserve_Muon); 
  Muon_pt.reserve(nReserve_Muon);
  Muon_ptErr.reserve(nReserve_Muon);
  Muon_eta.reserve(nReserve_Muon);
  Muon_phi.reserve(nReserve_Muon);  
  Muon_mass.reserve(nReserve_Muon);
  Muon_dxy.reserve(nReserve_Muon);
  Muon_dxyBest.reserve(nReserve_Muon);
  Muon_dxyErr.reserve(nReserve_Muon);
  Muon_dz.reserve(nReserve_Muon);
  Muon_dzBest.reserve(nReserve_Muon);
  Muon_dzErr.reserve(nReserve_Muon);
  Muon_ip3d.reserve(nReserve_Muon);
  Muon_sip3d.reserve(nReserve_Muon);
  Muon_ip3dBest.reserve(nReserve_Muon);
  Muon_sip3dBest.reserve(nReserve_Muon);
  Muon_pfRelIso03_all.reserve(nReserve_Muon);
  Muon_pfRelIso03_chg.reserve(nReserve_Muon);
  Muon_pfRelIso04_all.reserve(nReserve_Muon);
  Muon_pfIsoId.reserve(nReserve_Muon);
  Muon_miniPFRelIso_all.reserve(nReserve_Muon); 
  Muon_miniPFRelIso_chg.reserve(nReserve_Muon); 
  Muon_jetIdx.reserve(nReserve_Muon); 
  Muon_isGlobal.reserve(nReserve_Muon);
  Muon_isTracker.reserve(nReserve_Muon);
  Muon_isPFcand.reserve(nReserve_Muon);
  Muon_softId.reserve(nReserve_Muon);
  Muon_mediumId.reserve(nReserve_Muon); 
  Muon_tightId.reserve(nReserve_Muon);
  Muon_highPtId.reserve(nReserve_Muon); 
  Muon_nStations.reserve(nReserve_Muon);
  Muon_nTrackerLayers.reserve(nReserve_Muon); 
  Muon_segmentComp.reserve(nReserve_Muon); 
  Muon_cleanmask.reserve(nReserve_Muon); 
  Muon_mvaTTH.reserve(nReserve_Muon); 
  Muon_pdgId.reserve(nReserve_Muon); 
  Muon_genPartFlav.reserve(nReserve_Muon); 
  Muon_genPartIdx.reserve(nReserve_Muon); 

  Muon_Id.reserve(nReserve_Muon); 
  Muon_x.reserve(nReserve_Muon); 
  Muon_y.reserve(nReserve_Muon); 
  Muon_z.reserve(nReserve_Muon); 
  Muon_gpt.reserve(nReserve_Muon);
  Muon_geta.reserve(nReserve_Muon);
  Muon_gphi.reserve(nReserve_Muon);  
  Muon_looseId.reserve(nReserve_Muon); 
  Muon_softId4.reserve(nReserve_Muon);
  Muon_softIdBest.reserve(nReserve_Muon);
  Muon_isNano.reserve(nReserve_Muon);
  Muon_isMini.reserve(nReserve_Muon);
  Muon_isGood.reserve(nReserve_Muon);
  Muon_isGoodLast.reserve(nReserve_Muon);
  Muon_isGoodAng.reserve(nReserve_Muon);
  Muon_isArbitrated.reserve(nReserve_Muon);
  Muon_isStandAlone.reserve(nReserve_Muon);
  Muon_isRPCcand.reserve(nReserve_Muon);
  Muon_nValid.reserve(nReserve_Muon); 
  Muon_nPix.reserve(nReserve_Muon); 
  Muon_Chi2.reserve(nReserve_Muon); 
  Muon_gnValid.reserve(nReserve_Muon); 
  Muon_gnPix.reserve(nReserve_Muon); 
  Muon_gChi2.reserve(nReserve_Muon); 
  Muon_gnValidMu.reserve(nReserve_Muon); 
  Muon_vtxIdx.reserve(nReserve_Muon); 
  Muon_vtxFlag.reserve(nReserve_Muon); 
  Muon_trkIdx.reserve(nReserve_Muon); 
  Muon_simIdx.reserve(nReserve_Muon); 

  //------------------------------- Dimuon reserve --------------------------//

  Dimu_t1muIdx.reserve(nReserve_Dimu);
  Dimu_t1dxy.reserve(nReserve_Dimu);
  Dimu_t1dz.reserve(nReserve_Dimu);
  Dimu_t2muIdx.reserve(nReserve_Dimu);
  Dimu_t2dxy.reserve(nReserve_Dimu);
  Dimu_t2dz.reserve(nReserve_Dimu);
  Dimu_pt.reserve(nReserve_Dimu);
  Dimu_eta.reserve(nReserve_Dimu);
  Dimu_phi.reserve(nReserve_Dimu);
  Dimu_rap.reserve(nReserve_Dimu);
  Dimu_mass.reserve(nReserve_Dimu);
  Dimu_charge.reserve(nReserve_Dimu);
  Dimu_simIdx.reserve(nReserve_Dimu);
  Dimu_vtxIdx.reserve(nReserve_Dimu);
  Dimu_chi2.reserve(nReserve_Dimu);
  Dimu_dlxy.reserve(nReserve_Dimu);
  Dimu_dlxyErr.reserve(nReserve_Dimu);
  Dimu_dlxySig.reserve(nReserve_Dimu);
  Dimu_cosphixy.reserve(nReserve_Dimu);
  Dimu_dl.reserve(nReserve_Dimu);
  Dimu_dlErr.reserve(nReserve_Dimu);
  Dimu_dlSig.reserve(nReserve_Dimu);
  Dimu_cosphi.reserve(nReserve_Dimu);
  Dimu_ptfrac.reserve(nReserve_Dimu);
  Dimu_x.reserve(nReserve_Dimu);
  Dimu_y.reserve(nReserve_Dimu);
  Dimu_z.reserve(nReserve_Dimu);
  Dimu_Covxx.reserve(nReserve_Dimu);
  Dimu_Covyx.reserve(nReserve_Dimu);
  Dimu_Covzx.reserve(nReserve_Dimu);
  Dimu_Covyy.reserve(nReserve_Dimu);
  Dimu_Covzy.reserve(nReserve_Dimu);
  Dimu_Covzz.reserve(nReserve_Dimu);

  // trigger objects 

  TrigObj_id.reserve(nReserve_TrigObj);
  TrigObj_filterBits.reserve(nReserve_TrigObj);
  TrigObj_pt.reserve(nReserve_TrigObj);
  TrigObj_eta.reserve(nReserve_TrigObj);
  TrigObj_phi.reserve(nReserve_TrigObj);

#ifdef charm
  //------------------------------- Dmeson reserve --------------------------//
#include "NanoDmesonReserve.cc.forinclude"
#endif

  //---------------------------- Create branch for tree ------------------------//
  
  // nanoAOD run/event structure 
  t_event->Branch("run", &run, "run/i");
  t_event->Branch("event", &event, "event/l");
  t_event->Branch("luminosityBlock", &luminosityBlock, "luminosityBlock/i");

  t_event->Branch("CMSSW", &CMSSW, "CMSSW/I");

    // nanoAOD extension
    // store JSON info
    t_event->Branch("GoodLumisection", &GoodLumisection, "GoodLumisection/O");
    // store dataset info
    t_event->Branch("Dataset_isMC", &MCdataset, "Dataset_isMC/O");
    t_event->Branch("Dataset_ZeroBias", &ZeroBiasdataset, "Dataset_ZeroBias/O");
    t_event->Branch("Dataset_MinimumBias", &MinimumBiasdataset, "Dataset_MinimumBias/O");
    t_event->Branch("Dataset_Jet", &Jetdataset, "Dataset_Jet/O");
    t_event->Branch("Dataset_MultiJet", &MultiJetdataset, "Dataset_MultiJet/O");
    t_event->Branch("Dataset_JetMETTauMonitor", &JetMETTauMonitordataset, "Dataset_JetMETTauMonitor/O");
    t_event->Branch("Dataset_Mu", &Mudataset, "Dataset_Mu/O");
    t_event->Branch("Dataset_MuMonitor", &MuMonitordataset, "Dataset_MuMonitor/O");
    t_event->Branch("Dataset_DoubleMu", &DoubleMudataset, "Dataset_DoubleMu/O");
    t_event->Branch("Dataset_MuHad", &MuHaddataset, "Dataset_MuHad/O");
    t_event->Branch("Dataset_MuOnia", &MuOniadataset, "Dataset_MuOnia/O");
    t_event->Branch("Dataset_Charmonium", &Charmoniumdataset, "Dataset_Charmonium/O");
    t_event->Branch("Dataset_BParking", &BParkingdataset, "Dataset_BParking/O");
    t_event->Branch("Dataset_BTau", &BTaudataset, "Dataset_BTau/O");
    t_event->Branch("Dataset_Electron", &Electrondataset, "Dataset_Electron/O");
    t_event->Branch("Dataset_DoubleElectron", &DoubleElectrondataset, "Dataset_DoubleElectron/O");
    t_event->Branch("Dataset_Photon", &Photondataset, "Dataset_Photon/O");
    t_event->Branch("Dataset_EGMonitor", &EGMonitordataset, "Dataset_EGMonitor/O");
    t_event->Branch("Dataset_MuEG", &MuEGdataset, "Dataset_MuEG/O");
    t_event->Branch("Dataset_METFwd", &METFwddataset, "Dataset_METFwd/O");
    t_event->Branch("Dataset_Commissioning", &Commissioningdataset, "Dataset_Commissioning/O");
    // and info on which other data sets this event occurs
    t_event->Branch("Alsoon_ZeroBias", &ZeroBiasTrig, "Alsoon_ZeroBias/O");
    t_event->Branch("Alsoon_MinimumBias", &MinimumBiasTrig, "Alsoon_MinimumBias/O");
    t_event->Branch("Alsoon_Jet", &JetTrig, "Alsoon_Jet/O");
    t_event->Branch("Alsoon_MultiJet", &MultiJetTrig, "Alsoon_MultiJet/O");
    t_event->Branch("Alsoon_JetMETTauMonitor", &JetMETTauMonitorTrig, "Alsoon_JetMETTauMonitor/O");
    t_event->Branch("Alsoon_Mu", &MuTrig, "Alsoon_Mu/O");
    t_event->Branch("Alsoon_MuMonitor", &MuMonitorTrig, "Alsoon_MuMonitor/O");
    t_event->Branch("Alsoon_DoubleMu", &DoubleMuTrig, "Alsoon_DoubleMu/O");
    t_event->Branch("Alsoon_MuHad", &MuHadTrig, "Alsoon_MuHad/O");
    t_event->Branch("Alsoon_MuOnia", &MuOniaTrig, "Alsoon_MuOnia/O");
    t_event->Branch("Alsoon_Charmonium", &CharmoniumTrig, "Alsoon_Charmonium/O");
    t_event->Branch("Alsoon_BParking", &BParkingTrig, "Alsoon_BParking/O");
    t_event->Branch("Alsoon_BTau", &BTauTrig, "Alsoon_BTau/O");
    t_event->Branch("Alsoon_Electron", &ElectronTrig, "Alsoon_Electron/O");
    t_event->Branch("Alsoon_DoubleElectron", &DoubleElectronTrig, "Alsoon_DoubleElectron/O");
    t_event->Branch("Alsoon_Photon", &PhotonTrig, "Alsoon_Photon/O");
    t_event->Branch("Alsoon_EGMonitor", &EGMonitorTrig, "Alsoon_EGMonitor/O");
    t_event->Branch("Alsoon_MuEG", &MuEGTrig, "Alsoon_MuEG/O");
    t_event->Branch("Alsoon_METFwd", &METFwdTrig, "Alsoon_METFwd/O");
    t_event->Branch("Alsoon_Commissioning", &CommissioningTrig, "Alsoon_Commissioning/O");

  //}

  if (!isData) {

    std::cout << "This is MC" << std::endl;
    //---------------------- Create branch of GenPart's tree -------------------//
    
    // official nanoAOD structure
    t_event->Branch("nGenPart", &nGenPart, "nGenPart/I");
    t_event->Branch("GenPart_pt", GenPart_pt.data(), "GenPart_pt[nGenPart]/F");
    t_event->Branch("GenPart_eta", GenPart_eta.data(), "GenPart_eta[nGenPart]/F");
    t_event->Branch("GenPart_phi", GenPart_phi.data(), "GenPart_phi[nGenPart]/F");
    t_event->Branch("GenPart_mass", GenPart_mass.data(), "GenPart_mass[nGenPart]/F");
    t_event->Branch("GenPart_charge", GenPart_charge.data(), "GenPart_charge[nGenPart]/F");
    t_event->Branch("GenPart_pdgId", GenPart_pdgId.data(), "GenPart_pdgId[nGenPart]/I");
    t_event->Branch("GenPart_status", GenPart_status.data(), "GenPart_status[nGenPart]/I");
    t_event->Branch("GenPart_statusFlags", GenPart_statusFlags.data(), "GenPart_statusFlags[nGenPart]/I");
    t_event->Branch("GenPart_genPartIdxMother", GenPart_genPartIdxMother.data(), "GenPart_genPartIdxMother[nGenPart]/I");

    if (nanoext) {
    // GenPart extension
      t_event->Branch("GenPart_Id", GenPart_Id.data(), "GenPart_Id[nGenPart]/I");
      t_event->Branch("GenPart_isNano", GenPart_isNano.data(), "GenPart_isNano[nGenPart]/O");
      t_event->Branch("GenPart_parpdgId", GenPart_parpdgId.data(), "GenPart_parpdgId[nGenPart]/I");
      t_event->Branch("GenPart_sparpdgId", GenPart_sparpdgId.data(), "GenPart_sparpdgId[nGenPart]/I");
      t_event->Branch("GenPart_numberOfDaughters", GenPart_numberOfDaughters.data(), "GenPart_numberOfDaughters[nGenPart]/I");
      t_event->Branch("GenPart_nstchgdaug", GenPart_nstchgdaug.data(), "GenPart_nstchgdaug[nGenPart]/I");
      t_event->Branch("GenPart_promptFlag", GenPart_promptFlag.data(), "GenPart_promptFlag[nGenPart]/I");
      t_event->Branch("GenPart_vx", GenPart_vx.data(), "GenPart_vx[nGenPart]/F");
      t_event->Branch("GenPart_vy", GenPart_vy.data(), "GenPart_vy[nGenPart]/F");
      t_event->Branch("GenPart_vz", GenPart_vz.data(), "GenPart_vz[nGenPart]/F");
      t_event->Branch("GenPart_mvx", GenPart_mvx.data(), "GenPart_mvx[nGenPart]/F");
      t_event->Branch("GenPart_mvy", GenPart_mvy.data(), "GenPart_mvy[nGenPart]/F");
      t_event->Branch("GenPart_mvz", GenPart_mvz.data(), "GenPart_mvz[nGenPart]/F");
      t_event->Branch("GenPart_recIdx", GenPart_recIdx.data(), "GenPart_recIdx[nGenPart]/I");

      // GenPV extension
      t_event->Branch("GenPV_x", &GenPV_x, "GenPV_x/F");
      t_event->Branch("GenPV_y", &GenPV_y, "GenPV_y/F");
      t_event->Branch("GenPV_z", &GenPV_z, "GenPV_z/F");
      t_event->Branch("GenPV_recIdx", &GenPV_recIdx, "GenPV_recIdx/I");
      t_event->Branch("GenPV_chmult", &GenPV_chmult, "GenPV_chmult/I");
    } // end of nanoext
    
  } // end of event !isData


  // nanoAOD track structure
  t_event->Branch("nTrk", &nTrk, "nTrk/I");

  // nanoAOD IsoTrack structure
  t_event->Branch("nIsoTrack", &nIsoTrack, "nIsoTrack/I");

  // nanoAOD PV structure
  t_event->Branch("PV_npvs", &PV_npvs, "PV_npvs/I");
  t_event->Branch("PV_npvsGood", &PV_npvsGood, "PV_npvsGood/I");
  t_event->Branch("PV_chi2", &PV_chi2, "PV_chi2/F");
  t_event->Branch("PV_ndof", &PV_ndof, "PV_ndof/F");
  t_event->Branch("PV_score", &PV_score, "PV_score/F");
  t_event->Branch("PV_x", &PV_x, "PV_x/F");
  t_event->Branch("PV_y", &PV_y, "PV_y/F");
  t_event->Branch("PV_z", &PV_z, "PV_z/F");

  // nanoAOD OtherPV structure 
  t_event->Branch("nOtherPV", &nOtherPV, "nOtherPV/I");
  t_event->Branch("OtherPV_z", OtherPV_z.data(), "OtherPV_z[nOtherPV]/F");

  if (nanoext) {

    // PVtx extension
    t_event->Branch("nPVtx", &nPVtx, "nPVtx/I");
    t_event->Branch("PVtx_Id", PVtx_Id.data(), "PVtx_Id[nPVtx]/I");
    t_event->Branch("PVtx_isMain", PVtx_isMain.data(), "PVtx_isMain[nPVtx]/O");
    t_event->Branch("PVtx_isMainSim", PVtx_isMainSim.data(), "PVtx_isMainSim[nPVtx]/O");
    t_event->Branch("PVtx_isGood", PVtx_isGood.data(), "PVtx_isGood[nPVtx]/O");
    t_event->Branch("PVtx_isValid", PVtx_isValid.data(), "PVtx_isValid[nPVtx]/O");
    t_event->Branch("PVtx_isFake", PVtx_isFake.data(), "PVtx_isFake[nPVtx]/O");
    t_event->Branch("PVtx_isTrigUnique", PVtx_isTrigUnique.data(), "PVtx_isTrigUnique[nPVtx]/O");
    t_event->Branch("PVtx_isUnbiased", PVtx_isUnbiased.data(), "PVtx_isUnbiased[nPVtx]/O");
    t_event->Branch("PVtx_ntrk", PVtx_ntrk.data(), "PVtx_ntrk[nPVtx]/I");
    t_event->Branch("PVtx_ntrkfit", PVtx_ntrkfit.data(), "PVtx_ntrkfit[nPVtx]/I");
    t_event->Branch("PVtx_chi2", PVtx_chi2.data(), "PVtx_chi2[nPVtx]/F");
    t_event->Branch("PVtx_ndof", PVtx_ndof.data(), "PVtx_ndof[nPVtx]/F");
    t_event->Branch("PVtx_score", PVtx_score.data(), "PVtx_score[nPVtx]/F");
    t_event->Branch("PVtx_sumPt", PVtx_sumPt.data(), "PVtx_sumPt[nPVtx]/F");
    t_event->Branch("PVtx_Rho", PVtx_Rho.data(), "PVtx_Rho[nPVtx]/F");
    t_event->Branch("PVtx_x", PVtx_x.data(), "PVtx_x[nPVtx]/F");
    t_event->Branch("PVtx_y", PVtx_y.data(), "PVtx_y[nPVtx]/F");
    t_event->Branch("PVtx_z", PVtx_z.data(), "PVtx_z[nPVtx]/F");
    if (covout) {
      t_event->Branch("PVtx_Covxx", PVtx_Covxx.data(), "PVtx_Covxx[nPVtx]/F");
      t_event->Branch("PVtx_Covyx", PVtx_Covyx.data(), "PVtx_Covyx[nPVtx]/F");
      t_event->Branch("PVtx_Covzx", PVtx_Covzx.data(), "PVtx_Covzx[nPVtx]/F");
      t_event->Branch("PVtx_Covyy", PVtx_Covyy.data(), "PVtx_Covyy[nPVtx]/F");
      t_event->Branch("PVtx_Covzy", PVtx_Covzy.data(), "PVtx_Covzy[nPVtx]/F");
      t_event->Branch("PVtx_Covzz", PVtx_Covzz.data(), "PVtx_Covzz[nPVtx]/F");
    }

    // beamspot extension
    t_event->Branch("Bsp_x",&Bsp_x, "Bsp_x/F");
    t_event->Branch("Bsp_y",&Bsp_y, "Bsp_y/F");
    t_event->Branch("Bsp_z",&Bsp_z, "Bsp_z/F");
    t_event->Branch("Bsp_sigmaz",&Bsp_sigmaz, "Bsp_sigmaz/F");
    t_event->Branch("Bsp_dxdz",&Bsp_dxdz, "Bsp_dxdz/F");
    t_event->Branch("Bsp_dydz",&Bsp_dydz, "Bsp_dydz/F");
    t_event->Branch("Bsp_widthx",&Bsp_widthx, "Bsp_widthx/F");
    t_event->Branch("Bsp_widthy",&Bsp_widthy, "Bsp_widthy/F");
  } // end of nanoext
  
  //-------------------------- Create branch of Muons's tree -------------------//
  // official nanoAOD
  t_event->Branch("nMuon", &nMuon, "nMuon/i");
  t_event->Branch("Muon_charge", Muon_charge.data(), "Muon_charge[nMuon]/I");
  t_event->Branch("Muon_tightCharge", Muon_tightCharge.data(), "Muon_tightCharge[nMuon]/I");
  t_event->Branch("Muon_pt", Muon_pt.data(), "Muon_pt[nMuon]/F");
  t_event->Branch("Muon_ptErr", Muon_ptErr.data(), "Muon_ptErr[nMuon]/F");
  t_event->Branch("Muon_eta", Muon_eta.data(), "Muon_eta[nMuon]/F"); // betul
  t_event->Branch("Muon_phi", Muon_phi.data(), "Muon_phi[nMuon]/F");
  t_event->Branch("Muon_mass", Muon_mass.data(), "Muon_mass[nMuon]/F");
  t_event->Branch("Muon_dxy", Muon_dxy.data(), "Muon_dxy[nMuon]/F");
  t_event->Branch("Muon_dxyBest", Muon_dxyBest.data(), "Muon_dxyBest[nMuon]/F");
  t_event->Branch("Muon_dxyErr", Muon_dxyErr.data(), "Muon_dxyErr[nMuon]/F");
  t_event->Branch("Muon_dz", Muon_dz.data(), "Muon_dz[nMuon]/F");
  t_event->Branch("Muon_dzBest", Muon_dzBest.data(), "Muon_dzBest[nMuon]/F");
  t_event->Branch("Muon_dzErr", Muon_dzErr.data(), "Muon_dzErr[nMuon]/F");
  t_event->Branch("Muon_ip3d", Muon_ip3d.data(), "Muon_ip3d[nMuon]/F");
  t_event->Branch("Muon_sip3d", Muon_sip3d.data(), "Muon_sip3d[nMuon]/F");
  t_event->Branch("Muon_ip3dBest", Muon_ip3dBest.data(), "Muon_ip3dBest[nMuon]/F");
  t_event->Branch("Muon_sip3dBest", Muon_sip3dBest.data(), "Muon_sip3dBest[nMuon]/F");
  t_event->Branch("Muon_pfRelIso03_all", Muon_pfRelIso03_all.data(), "Muon_pfRelIso03_all[nMuon]/F");
  t_event->Branch("Muon_pfRelIso03_chg", Muon_pfRelIso03_chg.data(), "Muon_pfRelIso03_chg[nMuon]/F");  
  t_event->Branch("Muon_pfRelIso04_all", Muon_pfRelIso04_all.data(), "Muon_pfRelIso04_all[nMuon]/F");
  t_event->Branch("Muon_pfIsoId", Muon_pfIsoId.data(), "Muon_pfIsoId[nMuon]/I");
  t_event->Branch("Muon_miniPFRelIso_all", Muon_miniPFRelIso_all.data(), "Muon_miniPFRelIso_all[nMuon]/F");
  t_event->Branch("Muon_miniPFRelIso_chg", Muon_miniPFRelIso_chg.data(), "Muon_miniPFRelIso_chg[nMuon]/F");
  t_event->Branch("Muon_jetIdx", Muon_jetIdx.data(), "Muon_jetIdx[nMuon]/I");
  t_event->Branch("Muon_isPFcand", Muon_isPFcand.data(), "Muon_isPFcand[nMuon]/O");
  t_event->Branch("Muon_softId", Muon_softId.data(), "Muon_softId[nMuon]/O");
  t_event->Branch("Muon_mediumId", Muon_mediumId.data(), "Muon_mediumId[nMuon]/O");
  t_event->Branch("Muon_tightId", Muon_tightId.data(), "Muon_tightId[nMuon]/O");
  t_event->Branch("Muon_highPtId", Muon_highPtId.data(), "Muon_highPtId[nMuon]/b");
  t_event->Branch("Muon_nStations", Muon_nStations.data(), "Muon_nStations[nMuon]/I");
  t_event->Branch("Muon_nTrackerLayers", Muon_nTrackerLayers.data(), "Muon_nTrackerLayers[nMuon]/I");
  t_event->Branch("Muon_segmentComp", Muon_segmentComp.data(), "Muon_segmentComp[nMuon]/F");
  t_event->Branch("Muon_cleanmask", Muon_cleanmask.data(), "Muon_cleanmask[nMuon]/b");
  t_event->Branch("Muon_mvaTTH", Muon_mvaTTH.data(), "Muon_mvaTTH[nMuon]/F");
  t_event->Branch("Muon_pdgId", Muon_pdgId.data(), "Muon_pdgId[nMuon]/I");
  t_event->Branch("Muon_genPartFlav", Muon_genPartFlav.data(), "Muon_genPartFlav[nMuon]/b");
  t_event->Branch("Muon_genPartIdx", Muon_genPartIdx.data(), "Muon_genPartIdx[nMuon]/I");
  t_event->Branch("Muon_isGlobal", Muon_isGlobal.data(), "Muon_isGlobal[nMuon]/O");
  t_event->Branch("Muon_isTracker", Muon_isTracker.data(), "Muon_isTracker[nMuon]/O");

  if (nanoext) {
    // nanoAOD extension
    t_event->Branch("Muon_Id", Muon_Id.data(), "Muon_Id[nMuon]/I");
    t_event->Branch("Muon_x", Muon_x.data(), "Muon_x[nMuon]/F");
    t_event->Branch("Muon_y", Muon_y.data(), "Muon_y[nMuon]/F");
    t_event->Branch("Muon_z", Muon_z.data(), "Muon_z[nMuon]/F");
    t_event->Branch("Muon_gpt", Muon_gpt.data(), "Muon_gpt[nMuon]/F");
    t_event->Branch("Muon_geta", Muon_geta.data(), "Muon_geta[nMuon]/F");
    t_event->Branch("Muon_gphi", Muon_gphi.data(), "Muon_gphi[nMuon]/F");
    t_event->Branch("Muon_looseId", Muon_looseId.data(), "Muon_looseId[nMuon]/O");
    t_event->Branch("Muon_softId4", Muon_softId4.data(), "Muon_softId4[nMuon]/O");
    t_event->Branch("Muon_softIdBest", Muon_softIdBest.data(), "Muon_softIdBest[nMuon]/O");
    t_event->Branch("Muon_isNano", Muon_isNano.data(), "Muon_isNano[nMuon]/O");
    t_event->Branch("Muon_isMini", Muon_isMini.data(), "Muon_isMini[nMuon]/O");
    t_event->Branch("Muon_isGood", Muon_isGood.data(), "Muon_isGood[nMuon]/O");
    t_event->Branch("Muon_isGoodLast", Muon_isGoodLast.data(), "Muon_isGoodLast[nMuon]/O");
    t_event->Branch("Muon_isGoodAng", Muon_isGoodAng.data(), "Muon_isGoodAng[nMuon]/O");
    t_event->Branch("Muon_isArbitrated", Muon_isArbitrated.data(), "Muon_isArbitrated[nMuon]/O");
    t_event->Branch("Muon_isStandAlone", Muon_isStandAlone.data(), "Muon_isStandAlone[nMuon]/O");
    t_event->Branch("Muon_isRPCcand", Muon_isRPCcand.data(), "Muon_isRPCcand[nMuon]/O");
    t_event->Branch("Muon_nValid", Muon_nValid.data(), "Muon_nValid[nMuon]/I");
    t_event->Branch("Muon_nPix", Muon_nPix.data(), "Muon_nPix[nMuon]/I");
    t_event->Branch("Muon_Chi2", Muon_Chi2.data(), "Muon_Chi2[nMuon]/F");
    t_event->Branch("Muon_gnValid", Muon_gnValid.data(), "Muon_gnValid[nMuon]/I");
    t_event->Branch("Muon_gnPix", Muon_gnPix.data(), "Muon_gnPix[nMuon]/I");
    t_event->Branch("Muon_gChi2", Muon_gChi2.data(), "Muon_gChi2[nMuon]/F");
    t_event->Branch("Muon_gnValidMu", Muon_gnValidMu.data(), "Muon_gnValidMu[nMuon]/I");
    t_event->Branch("Muon_vtxIdx", Muon_vtxIdx.data(), "Muon_vtxIdx[nMuon]/I");
    t_event->Branch("Muon_vtxFlag", Muon_vtxFlag.data(), "Muon_vtxFlag[nMuon]/I");
    t_event->Branch("Muon_trkIdx", Muon_trkIdx.data(), "Muon_trkIdx[nMuon]/I");
    t_event->Branch("Muon_simIdx", Muon_simIdx.data(), "Muon_simIdx[nMuon]/I");
    // temporary
    //t_event->Branch("b4_nMuon", &b4_nMuon, "b4_nMuon/i");  
    t_event->Branch("Muon_nNano", &Muon_nNano, "Muon_nNano/i");  

    //
    // Dimuon branches 
    //
    t_event->Branch("nDimu", &nDimu, "nDimu/i");  
    t_event->Branch("Dimu_t1muIdx", Dimu_t1muIdx.data(), "Dimu_t1muIdx[nDimu]/I");
    t_event->Branch("Dimu_t1dxy", Dimu_t1dxy.data(), "Dimu_t1dxy[nDimu]/F");
    t_event->Branch("Dimu_t1dz", Dimu_t1dz.data(), "Dimu_t1dz[nDimu]/F");
    t_event->Branch("Dimu_t2muIdx", Dimu_t2muIdx.data(), "Dimu_t2muIdx[nDimu]/I");
    t_event->Branch("Dimu_t2dxy", Dimu_t2dxy.data(), "Dimu_t2dxy[nDimu]/F");
    t_event->Branch("Dimu_t2dz", Dimu_t2dz.data(), "Dimu_t2dz[nDimu]/F");

    t_event->Branch("Dimu_pt", Dimu_pt.data(), "Dimu_pt[nDimu]/F");
    t_event->Branch("Dimu_eta", Dimu_eta.data(), "Dimu_eta[nDimu]/F");
    t_event->Branch("Dimu_phi", Dimu_phi.data(), "Dimu_phi[nDimu]/F");
    t_event->Branch("Dimu_rap", Dimu_rap.data(), "Dimu_rap[nDimu]/F");
    t_event->Branch("Dimu_mass", Dimu_mass.data(), "Dimu_mass[nDimu]/F");
    t_event->Branch("Dimu_charge", Dimu_charge.data(), "Dimu_charge[nDimu]/I");
    t_event->Branch("Dimu_simIdx", Dimu_simIdx.data(), "Dimu_simIdx[nDimu]/I");
    t_event->Branch("Dimu_vtxIdx", Dimu_vtxIdx.data(), "Dimu_vtxIdx[nDimu]/I");
    t_event->Branch("Dimu_chi2", Dimu_chi2.data(), "Dimu_chi2[nDimu]/F");
    t_event->Branch("Dimu_dlxy", Dimu_dlxy.data(), "Dimu_dlxy[nDimu]/F");
    t_event->Branch("Dimu_dlxyErr", Dimu_dlxyErr.data(), "Dimu_dlxyErr[nDimu]/F");
    t_event->Branch("Dimu_dlxySig", Dimu_dlxySig.data(), "Dimu_dlxySig[nDimu]/F");
    t_event->Branch("Dimu_cosphixy", Dimu_cosphixy.data(), "Dimu_cosphixy[nDimu]/F");
    t_event->Branch("Dimu_dl", Dimu_dl.data(), "Dimu_dl[nDimu]/F");
    t_event->Branch("Dimu_dlErr", Dimu_dlErr.data(), "Dimu_dlErr[nDimu]/F");
    t_event->Branch("Dimu_dlSig", Dimu_dlSig.data(), "Dimu_dlSig[nDimu]/F");
    t_event->Branch("Dimu_cosphi", Dimu_cosphi.data(), "Dimu_cosphi[nDimu]/F");
    t_event->Branch("Dimu_ptfrac", Dimu_ptfrac.data(), "Dimu_ptfrac[nDimu]/F");
    t_event->Branch("Dimu_x", Dimu_x.data(), "Dimu_x[nDimu]/F");
    t_event->Branch("Dimu_y", Dimu_y.data(), "Dimu_y[nDimu]/F");
    t_event->Branch("Dimu_z", Dimu_z.data(), "Dimu_z[nDimu]/F");
    if (covout) {
      t_event->Branch("Dimu_Covxx", Dimu_Covxx.data(), "Dimu_Covxx[nDimu]/F");
      t_event->Branch("Dimu_Covyx", Dimu_Covyx.data(), "Dimu_Covyx[nDimu]/F");
      t_event->Branch("Dimu_Covzx", Dimu_Covzx.data(), "Dimu_Covzx[nDimu]/F");
      t_event->Branch("Dimu_Covyy", Dimu_Covyy.data(), "Dimu_Covyy[nDimu]/F");
      t_event->Branch("Dimu_Covzy", Dimu_Covzy.data(), "Dimu_Covzy[nDimu]/F");
      t_event->Branch("Dimu_Covzz", Dimu_Covzz.data(), "Dimu_Covzz[nDimu]/F");
    } // covout

  } // nanoext

  //----------------------- Stefan Wunsch variables + more -------------------//
  // official nanoAOD subset //
/* 
  // Electrons
  t_event->Branch("nElectron", &value_el_n, "nElectron/i");
  t_event->Branch("Electron_pt", value_el_pt, "Electron_pt[nElectron]/F");
  t_event->Branch("Electron_eta", value_el_eta, "Electron_eta[nElectron]/F");
  t_event->Branch("Electron_phi", value_el_phi, "Electron_phi[nElectron]/F");
  t_event->Branch("Electron_mass", value_el_mass, "Electron_mass[nElectron]/F");
  t_event->Branch("Electron_charge", value_el_charge, "Electron_charge[nElectron]/I");
  // nuha
  t_event->Branch("Electron_tightCharge", value_el_tightCharge, "Electron_tightCharge[nElectron]/I");
 
  t_event->Branch("Electron_pfRelIso03_all", value_el_pfreliso03all, "Electron_pfRelIso03_all[nElectron]/F");
  t_event->Branch("Electron_pfRelIso03_chg", value_el_pfreliso03chg, "Electron_pfRelIso03_chg[nElectron]/F");
  // nuha
  t_event->Branch("Electron_dr03TkSumPtOld", value_el_dr03TkSumPtOld, "Electron_dr03TkSumPtOld[nElectron]/F");
  t_event->Branch("Electron_dr03TkSumPt", value_el_dr03TkSumPt, "Electron_dr03TkSumPt[nElectron]/F");  
  t_event->Branch("Electron_dr03EcalRecHitSumEtOld", value_el_dr03EcalRecHitSumEtOld, "Electron_dr03EcalRecHitSumEtOld[nElectron]/F");
  t_event->Branch("Electron_dr03EcalRecHitSumEt", value_el_dr03EcalRecHitSumEt, "Electron_dr03EcalRecHitSumEt[nElectron]/F");
  
  // nanoAOD extension
  t_event->Branch("Electron_dr03HcalTowerSumEt", value_el_dr03HcalTowerSumEt, "Electron_dr03HcalTowerSumEt[nElectron]/F");
  // nuha
  t_event->Branch("Electron_dr03HcalDepth1TowerSumEtOld", value_el_dr03HcalDepth1TowerSumEtOld, "Electron_dr03HcalDepth1TowerSumEtOld[nElectron]/F");
  t_event->Branch("Electron_dr03HcalDepth1TowerSumEt", value_el_dr03HcalDepth1TowerSumEt, "Electron_dr03HcalDepth1TowerSumEt[nElectron]/F");

  // nanoAOD extension (next two)
  t_event->Branch("Electron_isEB", value_el_isEB, "Electron_isEB[nElectron]/O");
  t_event->Branch("Electron_isEE", value_el_isEE, "Electron_isEE[nElectron]/O");
  t_event->Branch("Electron_lostHits", value_el_lostHits, "Electron_lostHits[nElectron]/b");
  t_event->Branch("Electron_isPFcand", value_el_isPFcand, "Electron_isPFcand[nElectron]/O");
  t_event->Branch("Electron_isNano", value_el_isNano, "Electron_isNano[nElectron]/O");
    
  // nanoAOD extension (next two)
  t_event->Branch("Electron_convDist", value_el_convDist, "Electron_convDist[nElectron]/F");
  t_event->Branch("Electron_convDcot", value_el_convDcot, "Electron_convDcot[nElectron]/F");
  // nuha
  t_event->Branch("Electron_convVetoOld", value_el_convVetoOld, "Electron_convVetoOld[nElectron]/O");
  t_event->Branch("Electron_convVeto", value_el_convVeto, "Electron_convVeto[nElectron]/O");
  
  t_event->Branch("Electron_deltaEtaSC", value_el_deltaEtaSC, "Electron_deltaEtaSC[nElectron]/F");
  t_event->Branch("Electron_deltaPhiSC", value_el_deltaPhiSC, "Electron_deltaPhiSC[nElectron]/F");
  t_event->Branch("Electron_deltaEtaSCtr", value_el_deltaEtaSCtr, "Electron_deltaEtaSCtr[nElectron]/F");
  t_event->Branch("Electron_deltaPhiSCtr", value_el_deltaPhiSCtr, "Electron_deltaPhiSCtr[nElectron]/F");
  t_event->Branch("Electron_hoe", value_el_hoe, "Electron_hoe[nElectron]/F");
  t_event->Branch("Electron_sieie", value_el_sieie, "Electron_sieie[nElectron]/F");
  t_event->Branch("Electron_sieieR1", value_el_sieieR1, "Electron_sieieR1[nElectron]/F");
  // nuha
  t_event->Branch("Electron_eInvMinusPInvOld", value_el_eInvMinusPInvOld, "Electron_eInvMinusPInvOld[nElectron]/F");
  t_event->Branch("Electron_eInvMinusPInv", value_el_eInvMinusPInv, "Electron_eInvMinusPInv[nElectron]/F");

  // nanoAOD extension
  t_event->Branch("Electron_SCeta", value_el_SCeta, "Electron_SCeta[nElectron]/F");
  t_event->Branch("Electron_cutBased", value_el_cutBased, "Electron_cutBased[nElectron]/I");

  t_event->Branch("Electron_x", value_el_x, "Electron_x[nElectron]/F");
  t_event->Branch("Electron_y", value_el_y, "Electron_y[nElectron]/F");
  t_event->Branch("Electron_z", value_el_z, "Electron_z[nElectron]/F");
  t_event->Branch("Electron_vtxIdx", value_el_vtxIdx, "Electron_vtxIdx[nElectron]/I");

  t_event->Branch("Electron_dxy", value_el_dxy, "Electron_dxy[nElectron]/F");
  t_event->Branch("Electron_dxyErr", value_el_dxyErr, "Electron_dxyErr[nElectron]/F");
  t_event->Branch("Electron_dz", value_el_dz, "Electron_dz[nElectron]/F");
  t_event->Branch("Electron_dzErr", value_el_dzErr, "Electron_dzErr[nElectron]/F");
  t_event->Branch("Electron_ip3d", value_el_ip3d, "Electron_ip3d[nElectron]/F");
  t_event->Branch("Electron_sip3d", value_el_sip3d, "Electron_sip3d[nElectron]/F");
  t_event->Branch("Electron_nNano", &Electron_nNano, "Electron_nNano/i");  

  // Taus
  t_event->Branch("nTau", &value_tau_n, "nTau/i");
  t_event->Branch("Tau_pt", value_tau_pt, "Tau_pt[nTau]/F");
  t_event->Branch("Tau_eta", value_tau_eta, "Tau_eta[nTau]/F");
  t_event->Branch("Tau_phi", value_tau_phi, "Tau_phi[nTau]/F");
  t_event->Branch("Tau_mass", value_tau_mass, "Tau_mass[nTau]/F");
  t_event->Branch("Tau_charge", value_tau_charge, "Tau_charge[nTau]/I");
  t_event->Branch("Tau_decayMode", value_tau_decaymode, "Tau_decayMode[nTau]/I");
  t_event->Branch("Tau_chargedIso", value_tau_chargediso, "Tau_chargedIso[nTau]/F");
  t_event->Branch("Tau_neutralIso", value_tau_neutraliso, "Tau_neutralIso[nTau]/F");

  // Photons
  t_event->Branch("nPhoton", &value_ph_n, "nPhoton/i");
  t_event->Branch("Photon_pt", value_ph_pt, "Photon_pt[nPhoton]/F");
  t_event->Branch("Photon_eta", value_ph_eta, "Photon_eta[nPhoton]/F");
  t_event->Branch("Photon_phi", value_ph_phi, "Photon_phi[nPhoton]/F");
  t_event->Branch("Photon_mass", value_ph_mass, "Photon_mass[nPhoton]/F");
  t_event->Branch("Photon_charge", value_ph_charge, "Photon_charge[nPhoton]/I");
  t_event->Branch("Photon_pfRelIso03_all", value_ph_pfreliso03all, "Photon_pfRelIso03_all[nPhoton]/F");

  // MET
  t_event->Branch("MET_pt", &value_met_pt, "MET_pt/F");
  t_event->Branch("MET_phi", &value_met_phi, "MET_phi/F");
  t_event->Branch("MET_sumEt", &value_met_sumEt, "MET_sumEt/F");
  t_event->Branch("MET_significance", &value_met_significance, "MET_significance/F");
  t_event->Branch("MET_covXX", &value_met_covxx, "MET_covXX/F");
  t_event->Branch("MET_covXY", &value_met_covxy, "MET_covXY/F");
  t_event->Branch("MET_covYY", &value_met_covyy, "MET_covYY/F");

  // caloMET
  t_event->Branch("CaloMET_pt", &value_calomet_pt, "CaloMET_pt/F");
  t_event->Branch("CaloMET_phi", &value_calomet_phi, "CaloMET_phi/F");
  t_event->Branch("CaloMET_sumEt", &value_calomet_sumEt, "CaloMET_sumEt/F");

  // Jets
  t_event->Branch("nJet", &value_jet_n, "nJet/i");
  t_event->Branch("Jet_pt", value_jet_pt, "Jet_pt[nJet]/F");
  t_event->Branch("Jet_ptuncor", value_jet_ptuncor, "Jet_ptuncor[nJet]/F");
  t_event->Branch("Jet_eta", value_jet_eta, "Jet_eta[nJet]/F");
  t_event->Branch("Jet_phi", value_jet_phi, "Jet_phi[nJet]/F");
  t_event->Branch("Jet_mass", value_jet_mass, "Jet_mass[nJet]/F");
//  t_event->Branch("Jet_ptD", value_jet_ptD, "Jet_ptD[nJet]/F");
  t_event->Branch("Jet_area", value_jet_area, "Jet_area[nJet]/F");
  t_event->Branch("Jet_nConstituents", value_jet_nConstituents, "Jet_nConstituents[nJet]/i");
  t_event->Branch("Jet_nElectrons", value_jet_nElectrons, "Jet_nElectrons[nJet]/i");
  t_event->Branch("Jet_nMuons", value_jet_nMuons, "Jet_nMuons[nJet]/i");
  t_event->Branch("Jet_chEmEF", value_jet_chEmEF, "Jet_chEmEF[nJet]/F");
  t_event->Branch("Jet_chHEF", value_jet_chHEF, "Jet_chHEF[nJet]/F");
  t_event->Branch("Jet_neEmEF", value_jet_neEmEF, "Jet_neEmEF[nJet]/F");
  t_event->Branch("Jet_neHEF", value_jet_neHEF, "Jet_neHEF[nJet]/F");
  // input values to jetId not stored (for the time being)
  t_event->Branch("Jet_jetId",value_jet_id, "Jet_jetId[nJet]/i");

 if (!isData) {
  // GenJets //Qun
  t_event->Branch("nGenJet", &value_gjet_n, "nGenJet/i");
  t_event->Branch("GenJet_pt", value_gjet_pt, "GenJet_pt[nGenJet]/F");
  t_event->Branch("GenJet_eta", value_gjet_eta, "GenJet_eta[nGenJet]/F");
  t_event->Branch("GenJet_phi", value_gjet_phi, "GenJet_phi[nGenJet]/F");
  t_event->Branch("GenJet_mass", value_gjet_mass, "GenJet_mass[nGenJet]/F");
 }

  // FatJets
  t_event->Branch("nFatJet", &value_fatjet_n, "nFatJet/i");
  t_event->Branch("FatJet_pt", value_fatjet_pt, "FatJet_pt[nFatJet]/F");
  t_event->Branch("FatJet_eta", value_fatjet_eta, "FatJet_eta[nFatJet]/F");
  t_event->Branch("FatJet_phi", value_fatjet_phi, "FatJet_phi[nFatJet]/F");
  t_event->Branch("FatJet_mass", value_fatjet_mass, "FatJet_mass[nFatJet]/F");
  t_event->Branch("FatJet_area", value_fatjet_area, "FatJet_area[nFatJet]/F");
  t_event->Branch("FatJet_nConstituents", value_fatjet_nConstituents, "FatJet_nConstituents[nFatJet]/i");
  t_event->Branch("FatJet_nElectrons", value_fatjet_nElectrons, "FatJet_nElectrons[nFatJet]/i");
  t_event->Branch("FatJet_nMuons", value_fatjet_nMuons, "FatJet_nMuons[nFatJet]/i");
  t_event->Branch("FatJet_chEmEF", value_fatjet_chEmEF, "FatJet_chEmEF[nFatJet]/F");
  t_event->Branch("FatJet_chHEF", value_fatjet_chHEF, "FatJet_chHEF[nFatJet]/F");
  t_event->Branch("FatJet_neEmEF", value_fatjet_neEmEF, "FatJet_neEmEF[nFatJet]/F");
  t_event->Branch("FatJet_neHEF", value_fatjet_neHEF, "FatJet_neHEF[nFatJet]/F");

  // TrackJets
  t_event->Branch("nTrackJet", &value_trackjet_n, "nTrackJet/i");
  t_event->Branch("TrackJet_pt", value_trackjet_pt, "TrackJet_pt[nTrackJet]/F");
  t_event->Branch("TrackJet_eta", value_trackjet_eta, "TrackJet_eta[nTrackJet]/F");
  t_event->Branch("TrackJet_phi", value_trackjet_phi, "TrackJet_phi[nTrackJet]/F");
  t_event->Branch("TrackJet_mass", value_trackjet_mass, "TrackJet_mass[nTrackJet]/F");
  t_event->Branch("TrackJet_area", value_trackjet_area, "TrackJet_area[nTrackJet]/F");
  t_event->Branch("TrackJet_nConstituents", value_trackjet_nConstituents, "TrackJet_nConstituents[nTrackJet]/i");
  t_event->Branch("TrackJet_nElectrons", value_trackjet_nElectrons, "TrackJet_nElectrons[nTrackJet]/i");
  t_event->Branch("TrackJet_nMuons", value_trackjet_nMuons, "TrackJet_nMuons[nTrackJet]/i"); */


  // Flags
  if (!custom_flag.empty())
    custom_bit.reserve(custom_flag.size());
  for (size_t iFlag = 0; iFlag < custom_flag.size(); ++iFlag) {
    custom_bit.push_back(0);
    t_event->Branch(("Flag_" + custom_flag.at(iFlag)).c_str(), &custom_bit.at(iFlag), ("Flag_" + custom_flag.at(iFlag) + "/O").c_str());
  }

#ifdef charm
  //----------------------- Create branches of Dmeson's tree ---------------------//
#include "NanoDmesonBranch.cc.forinclude"
#endif

// trigger objects
  t_event->Branch("nTrigObj", &nTrigObj, "nTrigObj/I");    
  t_event->Branch("TrigObj_id", TrigObj_id.data(), "TrigObj_id[nTrigObj]/I");
  t_event->Branch("TrigObj_filterBits", TrigObj_filterBits.data(), "TrigObj_filterBits[nTrigObj]/I");
  t_event->Branch("TrigObj_pt", TrigObj_pt.data(), "TrigObj_pt[nTrigObj]/F");
  t_event->Branch("TrigObj_phi", TrigObj_phi.data(), "TrigObj_phi[nTrigObj]/F");
  t_event->Branch("TrigObj_eta", TrigObj_eta.data(), "TrigObj_eta[nTrigObj]/F");

  // store cross-dataset "good" trigger Flags //
  // the "GoodxTrig" and "GoodxTrigger" variables are the same 
  t_event->Branch("Trig_goodMinBiasTrigger", &GoodMinBiasTrigger, "Trig_goodMinBiasTrigger/O");
  t_event->Branch("Trig_goodJetTrigger", &GoodJetTrigger, "Trig_goodJetTrigger/O");
  t_event->Branch("Trig_goodMuTrigger", &GoodMuTrigger, "Trig_goodMuTrigger/O");
  t_event->Branch("Trig_goodETrigger", &GoodETrigger, "Trig_goodETrigger/O");

  // cross-dataset trigger flags and thresholds 
  t_event->Branch("Trig_ZeroBiasFlag", &ZeroBiasFlag, "Trig_ZeroBiasFlag/I");
  t_event->Branch("Trig_MinBiasFlag", &MinBiasFlag, "Trig_MinBiasFlag/I");
  t_event->Branch("Trig_MinBiasMult", &MinBiasMult, "Trig_MinBiasMult/I");
  t_event->Branch("Trig_MuThresh", &MuThresh, "Trig_MuThresh/I");
  t_event->Branch("Trig_MuL1Thresh", &MuL1Thresh, "Trig_MuL1Thresh/I");
  t_event->Branch("Trig_MuL2Thresh", &MuL2Thresh, "Trig_MuL2Thresh/I");
  t_event->Branch("Trig_IsoMuThresh", &IsoMuThresh, "Trig_IsoMuThresh/I");
  t_event->Branch("Trig_DoubleMuThresh", &DoubleMuThresh, "Trig_DoubleMuThresh/I");
  t_event->Branch("Trig_JpsiThresh", &JpsiThresh, "Trig_JpsiThresh/I");
  t_event->Branch("Trig_MuHadFlag", &MuHadFlag, "Trig_MuHadFlag/I");
  t_event->Branch("Trig_MuEGFlag", &MuEGFlag, "Trig_MuEGFlag/I");
  t_event->Branch("Trig_ElectronThresh", &ElectronThresh, "Trig_ElectronThresh/I");
  t_event->Branch("Trig_DoubleElectronThresh", &DoubleElectronThresh, "Trig_DoubleElectronThresh/I");
  t_event->Branch("Trig_PhotonThresh", &PhotonThresh, "Trig_PhotonThresh/I");
  t_event->Branch("Trig_JetThresh", &JetThresh, "Trig_JetThresh/I");
  t_event->Branch("Trig_DiJetThresh", &DiJetThresh, "Trig_DiJetThresh/I");
  t_event->Branch("Trig_TriJetThresh", &TriJetThresh, "Trig_TriJetThresh/I");
  t_event->Branch("Trig_QuadJetThresh", &QuadJetThresh, "Trig_QuadJetThresh/I");
  t_event->Branch("Trig_HTThresh", &HTThresh, "Trig_HTThresh/I");
  t_event->Branch("Trig_BThresh", &BThresh, "Trig_BThresh/I");
  t_event->Branch("Trig_METThresh", &METThresh, "Trig_METThresh/I");

  // branch title creation
  eventDoc();

} // end of beginJob

// ------------ method called once each job just after ending the event loop  ------------
void 
NanoAnalyzer::endJob()
{
  file->cd();
  t_event->Write();

#ifdef charm
  if (!nanoext) return;

  // write histograms
  /* h_trackpt->Write();
  h_trackptlow->Write();
  h_trackptlowfine->Write();
  h_tracketa->Write();
  h_trackcov00->Write();
  
  h_d0pt->Write();
  h_dstarpt->Write();
  h_PS3pt->Write();
  h_K1pt->Write();
  h_P2pt->Write();
  h_K1eta->Write();
  h_P2eta->Write();
  h_PS3eta->Write();
  h_D0masscut->Write();
  h_deltaMassD0Dstar->Write();
  h_D0masscut_rightDLcut->Write();
  h_deltaMassD0Dstar_rightDLcut->Write(); */
#endif
}

// ------------ method called once each job just before starting run  ------------
void NanoAnalyzer::beginRun(const edm::Run &iRun, const edm::EventSetup &iStp)
{
  //If the hltConfig can be initialized, then the below is an example of how to extract the config information for the trigger from the so-called provenance.
  //The trigger configuration can change from run to run (during the run is the same), so it needs to be called here.
  //"init" return value indicates whether intitialisation has succeeded
  //"changed" parameter indicates whether the config has actually changed
  bool changed = false;
  std::cout << "beginRun" << std::endl;
  //if (!hlt_cfg.init(iRun, iStp, hlt_proc, changed)) {
  if (!hltConfig_.init(iRun, iStp, hlt_proc, changed)) {
    std::cout << "Initialization of HLTConfigProvider failed!!" << std::endl;
    std::cout << "This is normal on 2010 MC only" << std::endl;
    return;
  }
  if (changed) {
    // std::cout << "HLT menu used in run " << iRun.run() << " is " << hlt_cfg.tableName() << std::endl << std::endl;
    std::cout << "HLT menu used in run " << iRun.run() << " is " << hltConfig_.tableName() << std::endl << std::endl;
    /*
    std::cout << "The trigger paths in this menu are: " << std::endl;
    for (auto &p : hlt_cfg.triggerNames())
      std::cout << remove_version(p) << std::endl;

    std::cout << "There are " << hlt_cfg.triggerNames().size() << " paths in total." << std::endl << std::endl;
    */
    update_HLT_branch();
  }

// for trigger objects
	if (triggerName_!="@") { // "@" means: analyze all triggers in config
	  const unsigned int n(hltConfig_.size());
	  const unsigned int triggerIndex(hltConfig_.triggerIndex(triggerName_));
	  if (triggerIndex>=n) {
            std::cout << "HLTEventAnalyzerAOD::analyze:"
               << " TriggerName " << triggerName_
	       << " not available in (new) config!" << std::endl;
            std::cout << "Available TriggerNames are: " << std::endl;
            hltConfig_.dump("Triggers");
          }
	  std::cout << "Available TriggerNames are:QQQQQQ " << std::endl;
	}

  //cout << "hello beginRun end" << endl; 
}

// ------------ method for creating and updating HLT branches as necessary run by run ------------
void NanoAnalyzer::update_HLT_branch()
{
  // zero out all existing bits between runs, so that the triggers that stop existing is set to 0
  // instead of whatever value it has in the last event
  for (unordered_map<std::string, uint8_t>::iterator it = hlt_bit.begin(); it != hlt_bit.end(); ++it)
    it->second = 0;

  // for (uint iP = 0; iP < hlt_cfg.triggerNames().size(); ++iP) {
  //  std::string path = remove_version(hlt_cfg.triggerNames().at(iP));
  for (uint iP = 0; iP < hltConfig_.triggerNames().size(); ++iP) {
    std::string path = remove_version(hltConfig_.triggerNames().at(iP));

    // check if the path already exists
    // if not, create a new branch and pad it with 0 for previous events
    // only allow paths that start with HLT (presumably AlCa etc are not needed)
    if (!hlt_bit.count(path) and path.substr(0, 3) == "HLT") {
      hlt_bit.insert( std::make_pair(path, 0) );
      TBranch *hlt_path = t_event->Branch(path.c_str(), &hlt_bit.at(path), (path + "/O").c_str());  
      for (uint iE = 0; iE < t_event->GetEntries(); ++iE)
        hlt_path->Fill(); 
    }
  }
}

void NanoAnalyzer::fillDescriptions(edm::ConfigurationDescriptions & descriptions) 
{ // set defaults for parameters which can be overruled by configuration
  edm::ParameterSetDescription desc;
  desc.add<std::string>("outFile", "test.root");
  desc.add<bool>("isData", true);
  desc.add<std::string>("hltProcess", "HLT");
  desc.add<std::string>("hlt_proc", "HLT");
#ifdef Compatibility
  desc.add<bool>("nanoExtension", false);
#else 
  desc.add<bool>("nanoExtension", true);
#endif
  desc.add<bool>("writeCovariance", false);
  desc.add<std::vector<std::string> >("customFlag", std::vector<std::string>());
  desc.add<edm::InputTag>("customTag", edm::InputTag());
  desc.add<std::string>("triggerName_", "@");
  desc.add<std::string>("triggerName", "@");
  descriptions.add("nanoAnalyzer", desc);
}

// ------------ method called when ending the processing of a run  ------------  Qun below
void NanoAnalyzer::endRun(edm::Run const&, edm::EventSetup const&)
{
  // for test: (currently saves almost empty file)
  // t_event->SaveAs("test.json");
}

#ifdef charm
//include JSON quality check
#include "NanoJSON.cc.forinclude"
#endif

//include methods for special trigger variables
#include "NanoTrigger.cc.forinclude"

//define this as a plug-in
DEFINE_FWK_MODULE(NanoAnalyzer);

// branch title creation
void NanoAnalyzer::eventDoc() { 

  createTitle("run", "run/i"); 
  createTitle("event", "event/l"); 
  createTitle("luminosityBlock", "luminosityBlock/i");  
  createTitle("CMSSW",                   "CMSSW version used for the processing"); 
  createTitle("GoodLumisection",         "flag: good lumi section, from Golden JSON (not all datasets)"); 
  
  createTitle("Dataset_isMC",            "flag: event is from MC dataset");
  // flags from trigger-based auto-recognition 
  createTitle("Dataset_ZeroBias",        "flag: event from ZeroBias dataset"); 
  createTitle("Dataset_MinimumBias",     "flag: event from MinimumBias dataset"); 
  createTitle("Dataset_Jet",             "flag: event from Jet dataset"); 
  createTitle("Dataset_MultiJet",        "flag: event from MultiJet dataset");
  createTitle("Dataset_JetMETTauMonitor","flag: event from JetMETTauMonitor dataset"); 
  createTitle("Dataset_Mu",              "flag: event from Mu or SingleMuon dataset");
  createTitle("Dataset_MuMonitor",       "flag: event from MuMonitor dataset");
  createTitle("Dataset_DoubleMu",        "flag: event from DoubleMu(Parked) dataset"); 
  createTitle("Dataset_MuHad",           "flag: event from MuHad dataset"); 
  createTitle("Dataset_MuOnia",          "flag: event from MuOnia dataset"); 
  createTitle("Dataset_Charmonium",      "flag: event from Charmonium dataset"); 
  createTitle("Dataset_BParking",        "flag: event from BParking dataset"); 
  createTitle("Dataset_BTau",            "flag: event from BTau dataset"); 
  createTitle("Dataset_Electron",        "flag: event from EG or Electron dataset"); 
  createTitle("Dataset_DoubleElectron",  "flag: event from DoubleElectron dataset"); 
  createTitle("Dataset_Photon",          "flag: event from Photon dataset"); 
  createTitle("Dataset_EGMonitor",       "flag: event from EGMonitor dataset");
  createTitle("Dataset_METFwd",          "flag: event from METFwd dataset");
  createTitle("Dataset_MuEG",            "flag: event from MuEG dataset");
  createTitle("Dataset_Commissioning",   "flag: event from Commissioning dataset");
  
  createTitle("Alsoon_ZeroBias",         "flag: event also on ZeroBias dataset"); 
  createTitle("Alsoon_MinimumBias",      "flag: event also on MinimumBias dataset");
  createTitle("Alsoon_Jet",              "flag: event also on Jet dataset"); 
  createTitle("Alsoon_MultiJet",         "flag: event also on MultiJet dataset"); 
  createTitle("Alsoon_JetMETTauMonitor", "flag: event also on JetMETTauMonitor dataset"); 
  createTitle("Alsoon_Mu",               "flag: event also on Mu or SingleMuon dataset");
  createTitle("Alsoon_MuMonitor",        "flag: event also on MuMonitor dataset"); 
  createTitle("Alsoon_DoubleMu",         "flag: event also on DoubleMu(Parked) dataset");
  createTitle("Alsoon_MuHad",            "flag: event also on MuHad dataset"); 
  createTitle("Alsoon_MuOnia",           "flag: event also on MuOnia dataset"); 
  createTitle("Alsoon_Charmonium",       "flag: event also on Charmonium dataset"); 
  createTitle("Alsoon_BParking",         "flag: event also on BParking dataset");
  createTitle("Alsoon_BTau",             "flag: event also on BTau dataset"); 
  createTitle("Alsoon_Electron",         "flag: event also on EG or Electron dataset"); 
  createTitle("Alsoon_DoubleElectron",   "flag: event also on DoubleElectron dataset");
  createTitle("Alsoon_Photon",           "flag: event also on Photon datset");
  createTitle("Alsoon_EGMonitor",        "flag: event also on EGMonitor dataset");
  createTitle("Alsoon_MuEG",             "flag: event also on MuEG dataset");
  createTitle("Alsoon_METFwd",           "flag: event also on METFwd dataset"); 
  createTitle("Alsoon_Commissioning",    "flag: event also on Commissioning dataset"); 

  if (!isData) {
    
  createTitle("nGenPart", "interesting gen particles"); 
  createTitle("GenPart_pt", "pt"); 
  createTitle("GenPart_eta", "eta"); 
  createTitle("GenPart_phi", "phi");
  createTitle("GenPart_mass", "mass (not for all particles); if 0 when it should not be: look up from PDG");
  createTitle("GenPart_charge", "charge");
  createTitle("GenPart_pdgId", "PDG id"); 
  createTitle("GenPart_status", "Particle status. 1=stable"); 
  createTitle("GenPart_statusFlags", "gen status flags stored bitwise, bits are: 0 : isPrompt, 1 : isDecayedLeptonHadron, 2 : isTauDecayProduct, 3 : isPromptTauDecayProduct, 4 : isDirectTauDecayProduct, 5 : isDirectPromptTauDecayProduct, 6 : isDirectHadronDecayProduct, 7 : isHardProcess, 8 : fromHardProcess, 9 : isHardProcessTauDecayProduct, 10 : isDirectHardProcessTauDecayProduct, 11 : fromHardProcessBeforeFSR, 12 : isFirstCopy, 13 : isLastCopy, 14 : isLastCopyBeforeFSR, "); 
  createTitle("GenPart_genPartIdxMother", "index of the mother particle");
  
  if (nanoext) {
      
  createTitle("GenPart_Id", "unique genparticle identifier for list-independent cross referencing"); 
  createTitle("GenPart_isNano", "will also be found on official nanoAOD"); 
  createTitle("GenPart_parpdgId", "pdg id of immediate parent"); 
  createTitle("GenPart_sparpdgId", "pdg id of remote parent"); 
  createTitle("GenPart_numberOfDaughters", "number of daughters in decay");
  createTitle("GenPart_nstchgdaug", "number of stable charged daughters in decay"); 
  createTitle("GenPart_promptFlag", "GenPart_promptFlag"); 
  createTitle("GenPart_vx", "x position of decay vertex");
  createTitle("GenPart_vy", "y position of decay vertex");
  createTitle("GenPart_vz", "z position of decay vertex");
  createTitle("GenPart_mvx", "x position of mother origin vertex");
  createTitle("GenPart_mvy", "y position of mother origin vertex");
  createTitle("GenPart_mvz", "z position of mother origin vertex");
  createTitle("GenPart_recIdx", "identifier of associated reconstructed track (-1 if none)"); 
  
  createTitle("GenPV_x", "x position of generated main vertex"); 
  createTitle("GenPV_y", "y position of generated main vertex");
  createTitle("GenPV_z", "z position of generated main vertex"); 
  createTitle("GenPV_recIdx", "identifyer of corresponding reconstructed vertex");
  createTitle("GenPV_chmult", "charged multiplicity at this vertex"); 
  
  }
  
  }
  
  createTitle("nTrk", "number of tracks in input track collection"); 
  
  createTitle("nIsoTrack", "isolated tracks after basic selection (((pt>5 && (abs(pdgId) == 11 || abs(pdgId) == 13)) || pt > 10) && (abs(pdgId) < 15 || abs(eta) < 2.5) && abs(dxy) < 0.2 && abs(dz) < 0.1 && ((pfIsolationDR03().chargedHadronIso < 5 && pt < 25) || pfIsolationDR03().chargedHadronIso/pt < 0.2)) and lepton veto"); 
  
  createTitle("PV_npvs", "total number of reconstructed primary vertices");
  createTitle("PV_npvsGood", "number of good reconstructed primary vertices. selection:!isFake && ndof > 4 && abs(z) <= 24 && position.Rho <= 2"); 
  createTitle("PV_chi2", "main primary vertex reduced chi2");
  createTitle("PV_ndof", "main primary vertex number of degree of freedom");
  createTitle("PV_score", "main primary vertex score, i.e. sum pt2 of clustered objects"); 
  createTitle("PV_x", "main primary vertex position x coordinate"); 
  createTitle("PV_y", "main primary vertex position y coordinate"); 
  createTitle("PV_z", "main primary vertex position z coordinate");
  
  createTitle("nOtherPV", "other primary vertices"); 
  createTitle("OtherPV_z", "Z position of other primary vertices, excluding the main PV"); 
  
  if (nanoext) {  
  
  createTitle("nPVtx", "primary vertices (AOD, with or w/o cuts)");
  createTitle("PVtx_Id", "unique id of reconstructed primary vertex for list-independent cross-referencing");
  createTitle("PVtx_isMain", "is the main primary vertex, PV");
  createTitle("PVtx_isMainSim", "is the main simulated vertex"); 
  createTitle("PVtx_isGood", "satisfies good vertex cuts"); 
  createTitle("PVtx_isValid", "PVtx_isValid"); 
  createTitle("PVtx_isFake", "PVtx_isFake"); 
  createTitle("PVtx_isTrigUnique", "is the unique Trigger vertex; only set if no other vertices contribute to trigger"); 
  createTitle("PVtx_isUnbiased", "is unbiased by the main relevant trigger (e.g. inclusive muon trigger on muon dataset)");
  createTitle("PVtx_ntrk", "number of tracks associated to this vertex (including nonfitted close secondaries)");
  createTitle("PVtx_ntrkfit", "number of tracks used in the vertex fit");
  createTitle("PVtx_chi2", "primary vertex reduced chi2");
  createTitle("PVtx_ndof", "number of degrees of freedom (including weights)");
  createTitle("PVtx_score", "sum pt2 score"); 
  createTitle("PVtx_sumPt", "sum pt of all tracks"); 
  createTitle("PVtx_Rho", "radial distance to beam spot");
  createTitle("PVtx_x", "x position of vertex");
  createTitle("PVtx_y", "y position of vertex"); 
  createTitle("PVtx_z", "z position of vertex");
  
  if (covout) {
      
  createTitle("PVtx_Covxx", "PVtx_Covxx, covariance matrix of primary vertex"); 
  createTitle("PVtx_Covyx", "PVtx_Covyx");
  createTitle("PVtx_Covzx", "PVtx_Covzx");
  createTitle("PVtx_Covyy", "PVtx_Covyy");
  createTitle("PVtx_Covzy", "PVtx_Covzy"); 
  createTitle("PVtx_Covzz", "PVtx_Covzz"); 
  
  }
  
  createTitle("Bsp_x", "beam spot position x coordinate"); 
  createTitle("Bsp_y", "beam spot position y coordinate");
  createTitle("Bsp_z", "beam spot position z coordinate"); 
  createTitle("Bsp_sigmaz", "beam spot z width");
  createTitle("Bsp_dxdz", "beam spot x slope"); 
  createTitle("Bsp_dydz", "beam spot y slope"); 
  createTitle("Bsp_widthx", "beam spot x width");
  createTitle("Bsp_widthy", "beam spot y width");
  
  }
  
  createTitle("nMuon", "(AOD) muons after basic nanoAODplus selection");
  createTitle("Muon_charge", "electric charge");
  createTitle("Muon_tightCharge", "Tight charge criterion using pterr/pt of muonBestTrack (0:fail, 2:pass)"); 
  createTitle("Muon_pt", "pt"); 
  createTitle("Muon_ptErr", "ptError of the muon track"); 
  createTitle("Muon_eta", "eta"); 
  createTitle("Muon_phi", "phi");
  createTitle("Muon_mass", "mass"); 
  createTitle("Muon_dxy", "dxy (with sign) wrt first PV, in cm");
  createTitle("Muon_dxyBest", "dxy (with sign) wrt best PV, in cm");
  createTitle("Muon_dxyErr", "dxy uncertainty, in cm");
  createTitle("Muon_dz", "dz (with sign) wrt first PV, in cm"); 
  createTitle("Muon_dzBest", "dz (with sign) wrt best PV, in cm"); 
  createTitle("Muon_dzErr", "dz uncertainty, in cm"); 
  createTitle("Muon_ip3d", "3D impact parameter wrt first PV, in cm");
  createTitle("Muon_sip3d", "3D impact parameter significance wrt first PV"); 
  createTitle("Muon_ip3dBest", "3D impact parameter wrt best PV, in cm"); 
  createTitle("Muon_sip3dBest", "3D impact parameter significance wrt best PV"); 
  createTitle("Muon_pfRelIso03_all", "PF relative isolation dR=0.3, total (deltaBeta corrections)"); 
  createTitle("Muon_pfRelIso03_chg", "PF relative isolation dR=0.3, charged component");
  createTitle("Muon_pfRelIso04_all", "PF relative isolation dR=0.4, total (deltaBeta corrections)"); 
  createTitle("Muon_pfIsoId", "Muon_pfIsoId");
  createTitle("Muon_miniPFRelIso_all", "mini PF relative isolation, total (with scaled rho*EA PU corrections)"); 
  createTitle("Muon_miniPFRelIso_chg", "mini PF relative isolation, charged component"); 
  createTitle("Muon_jetIdx", "index of the associated jet (-1 if none)");
  createTitle("Muon_isPFcand", "muon is PF candidate");
  createTitle("Muon_softId", "soft cut-based ID");
  createTitle("Muon_mediumId", "cut-based ID, medium WP"); 
  createTitle("Muon_tightId", "cut-based ID, tight WP"); 
  createTitle("Muon_highPtId", "high-pT cut-based ID (1 = tracker high pT, 2 = global high pT, which includes tracker high pT)"); 
  createTitle("Muon_nStations", "number of matched stations with default arbitration (segment & track)");
  createTitle("Muon_nTrackerLayers", "number of layers in the tracker"); 
  createTitle("Muon_segmentComp", "muon segment compatibility");
  createTitle("Muon_cleanmask", "simple cleaning mask with priority to leptons"); 
  createTitle("Muon_mvaTTH", "TTH MVA lepton ID score"); 
  createTitle("Muon_pdgId", "PDG code assigned by the event reconstruction (not by MC truth)"); 
  createTitle("Muon_genPartFlav", "Flavour of genParticle for MC matching to status==1 muons: 1 = prompt muon (including gamma*->mu mu), 15 = muon from prompt tau, 5 = muon from b, 4 = muon from c, 3 = muon from light or unknown, 0 = unmatched"); 
  createTitle("Muon_genPartIdx", "Index into genParticle list for MC matching to status==1 muons");
  createTitle("Muon_isGlobal", "muon is global muon"); 
  createTitle("Muon_isTracker", "muon is tracker muon");
  
  if (nanoext) {
    
  createTitle("Muon_Id", "Unique identifier of muon, for list-independent cross-referencing"); 
  createTitle("Muon_x", "x point of closest approach to beam line, in cm"); 
  createTitle("Muon_y", "y point of closest approach to beam line, in cm"); 
  createTitle("Muon_z", "z point of closest approach to beam line, in cm"); 
  createTitle("Muon_gpt", "global muon pt"); 
  createTitle("Muon_geta", "global muon eta"); 
  createTitle("Muon_gphi", "global muon phi"); 
  createTitle("Muon_looseId", "loose cut-based ID"); 
  createTitle("Muon_softId4", "soft cut-based ID, 2010 version"); 
  createTitle("Muon_softIdBest", "soft cut-based ID, w.r.t. best PV"); 
  createTitle("Muon_isNano", "can be found on NanoAOD"); 
  createTitle("Muon_isMini", "can be found on MiniAOD"); 
  createTitle("Muon_isGood", "isGood(TMAnyStationTight)"); 
  createTitle("Muon_isGoodLast", "isGood(TMLastStationTight)"); 
  createTitle("Muon_isGoodAng", "isGood(TMLastStationAngTight)"); 
  createTitle("Muon_isArbitrated", "isGood(TrackerMuonArbitrated)"); 
  createTitle("Muon_isStandAlone", "StandAlone Muon"); 
  createTitle("Muon_isRPCcand", "RPC Muon"); 
  createTitle("Muon_nValid", "number of valid tracker hits"); 
  createTitle("Muon_nPix", "number of valid pixel hits"); 
  createTitle("Muon_Chi2", "tracker muon chi2/ndof"); 
  createTitle("Muon_gnValid", "number of valid tracker hits, global muon"); 
  createTitle("Muon_gnPix", "number of valid pixel hits, global muon"); 
  createTitle("Muon_gChi2", "global muon chi2/ndof"); 
  createTitle("Muon_gnValidMu", "Muon_gnValidMu"); 
  createTitle("Muon_vtxIdx", "index of the associated primary vertex (-1 if none)");
  createTitle("Muon_vtxFlag", "quality flag for vertex association"); 
  createTitle("Muon_trkIdx", "index of the associated track (-1 if none)"); 
  createTitle("Muon_simIdx", "index of the associated gen object (-1 if none)"); 
  createTitle("Muon_nNano", "number of muons satisfying isNano"); 
  
  createTitle("nDimu", "number of dimuons after vertex refit"); 
  createTitle("Dimu_t1muIdx", "cross reference to unique Id of first muon"); 
  createTitle("Dimu_t1dxy", "dxy of first muon (needed?)"); 
  createTitle("Dimu_t1dz", "dz of first muon (needed?)"); 
  createTitle("Dimu_t2muIdx", "cross reference to unique Id of 2nd muon"); 
  createTitle("Dimu_t2dxy", "dxy of 2nd muon (needed?)"); 
  createTitle("Dimu_t2dz", "dz of 2nd muon (needed?)"); 
  createTitle("Dimu_pt", "dimuon pt after vertex fit"); 
  createTitle("Dimu_eta", "dimuon eta after vertex fit"); 
  createTitle("Dimu_phi", "dimuon phi after vertex fit"); 
  createTitle("Dimu_rap", "dimuon rapidity after vertex fit"); 
  createTitle("Dimu_mass", "dimuon mass after vertex fit"); 
  createTitle("Dimu_charge", "dimuon charge"); 
  createTitle("Dimu_simIdx", "cross reference to parent meson or boson in GenPart"); 
  createTitle("Dimu_vtxIdx", "cross reference to associated primary vertex"); 
  createTitle("Dimu_chi2", "dimuon vertex chi2/ndof"); 
  createTitle("Dimu_dlxy", "dimuon decay length in xy"); 
  createTitle("Dimu_dlxyErr", "dimuon decay length error in xy"); 
  createTitle("Dimu_dlxySig", "dimuon decay length significance in xy"); 
  createTitle("Dimu_cosphixy", "cosine of vertex alignment angle in xy"); 
  createTitle("Dimu_dl", "dimuon decay length in 3D"); 
  createTitle("Dimu_dlErr", "dimuon decay length error in 3D"); 
  createTitle("Dimu_dlSig", "dimuon decay length significance in 3D"); 
  createTitle("Dimu_cosphi", "cosine of vertex alignment angle in 3D"); 
  createTitle("Dimu_ptfrac", "transverse momentum fraction w.r.t. total of associated primary vertex"); 
  createTitle("Dimu_x", "x position of dimuon vertex"); 
  createTitle("Dimu_y", "y position of dimuon vertex"); 
  createTitle("Dimu_z", "z position of dimuon vertex"); 
  
  if (covout) {    
      
  createTitle("Dimu_Covxx", "Dimu_Covxx (covariance matrix of vertex refit)"); 
  createTitle("Dimu_Covyx", "Dimu_Covyx"); 
  createTitle("Dimu_Covzx", "Dimu_Covzx"); 
  createTitle("Dimu_Covyy", "Dimu_Covyy"); 
  createTitle("Dimu_Covzy", "Dimu_Covzy"); 
  createTitle("Dimu_Covzz", "Dimu_Covzz"); 
  
  }
  
  }

/*   createTitle("nElectron", "number of Electrons"); 
  createTitle("Electron_x", "x point of closest approach to beam line, in cm"); 
  createTitle("Electron_y", "y point of closest approach to beam line, in cm"); 
  createTitle("Electron_z", "z point of closest approach to beam line, in cm"); 
  createTitle("Electron_pt", "pt");
  createTitle("Electron_eta", "eta"); 
  createTitle("Electron_phi", "phi");
  createTitle("Electron_mass", "mass");
  createTitle("Electron_charge", "electric charge");
  createTitle("Electron_tightCharge", "tight charge criteria"); 
  createTitle("Electron_pfRelIso03_all", "PF relative isolation dR=0.3"); 
  createTitle("Electron_pfRelIso03_chg", "PF relative isolation dR=0.3, charged component"); 
  createTitle("Electron_dr03TkSumPtOld", "old: Non-PF track isolation within a delta R cone of 0.3"); 
  createTitle("Electron_dr03TkSumPt", "Non-PF track isolation within a delta R cone of 0.3 with electron pt > 35 GeV"); 
  createTitle("Electron_dr03EcalRecHitSumEtOld", "Non-PF Ecal isolation within a delta R cone of 0.3"); 
  createTitle("Electron_dr03EcalRecHitSumEt", "Non-PF Ecal isolation within a delta R cone of 0.3 with electron pt > 35 GeV"); 
  createTitle("Electron_dr03HcalTowerSumEt", "NanoAODplus extension"); 
  createTitle("Electron_dr03HcalDepth1TowerSumEtOld", "Non-PF Hcal isolation within a delta R cone of 0.3"); 
  createTitle("Electron_dr03HcalDepth1TowerSumEt", "Non-PF Hcal isolation within a delta R cone of 0.3 with electron pt > 35 GeV"); 
  createTitle("Electron_isEB", "NanoAODplus extension"); 
  createTitle("Electron_isEE", "NanoAODplus extension"); 
  createTitle("Electron_lostHits", "NanoAODplus extension"); 
  createTitle("Electron_isPFcand", "NanoAODplus extension"); 
  createTitle("Electron_isNano", "NanoAODplus extension"); 
  createTitle("Electron_convDist", "NanoAODplus extension"); 
  createTitle("Electron_convDcot", "NanoAODplus extension"); 
  createTitle("Electron_convVetoOld", "pass conversion veto");
  createTitle("Electron_convVeto", "pass conversion veto"); 
  createTitle("Electron_deltaEtaSC", "delta eta (SC,ele) with sign"); 
  createTitle("Electron_deltaPhiSC", "delta phi (SC,ele) with sign"); 
  createTitle("Electron_deltaEtaSCtr", "NanoAODplus extension"); 
  createTitle("Electron_deltaPhiSCtr", "NanoAODplus extension"); 
  createTitle("Electron_hoe", "H over E");
  createTitle("Electron_sieie", "sigma_IetaIeta of the supercluster, calculated with full 5x5 region");
  createTitle("Electron_sieieR1", "NanoAODplus Run1"); 
  createTitle("Electron_eInvMinusPInvOld", "NanoAODplus Run1"); 
  createTitle("Electron_eInvMinusPInv", "1/E_SC - 1/p_trk"); 
  createTitle("Electron_SCeta", "Super Cluster eta");
  createTitle("Electron_cutBased", "cut-based ID Fall17 V2 (0:fail, 1:veto, 2:loose, 3:medium, 4:tight)"); 
  createTitle("Electron_dxy", "dxy (with sign) wrt first PV, in cm"); 
  createTitle("Electron_dxyErr", "dxy uncertainty, in cm"); 
  createTitle("Electron_dz", "dz (with sign) wrt first PV, in cm"); 
  createTitle("Electron_dzErr", "dz uncertainty, in cm"); 
  createTitle("Electron_ip3d", "3D impact parameter wrt first PV, in cm");
  createTitle("Electron_sip3d", "3D impact parameter significance wrt first PV, in cm");
  createTitle("Electron_nNano", "electron that follows nanoAOD cut (pT < 5 GeV)"); 
  createTitle("Electron_vtxIdx", "index of the associated primary vertex (-1 if none)");
 
  createTitle("nTau", "Taus after basic selection (pt > 18 && tauID('decayModeFindingNewDMs') && (tauID('byLooseCombinedIsolationDeltaBetaCorr3Hits') || tauID('byVLooseIsolationMVArun2v1DBoldDMwLT2015') || tauID('byVLooseIsolationMVArun2v1DBnewDMwLT') || tauID('byVLooseIsolationMVArun2v1DBdR03oldDMwLT') || tauID('byVVLooseIsolationMVArun2v1DBoldDMwLT') || tauID('byVVLooseIsolationMVArun2v1DBoldDMwLT2017v2') || tauID('byVVLooseIsolationMVArun2v1DBnewDMwLT2017v2') || tauID('byVVLooseIsolationMVArun2v1DBdR03oldDMwLT2017v2')))"); 
  createTitle("Tau_pt", "pt"); 
  createTitle("Tau_eta", "eta"); 
  createTitle("Tau_phi", "phi"); 
  createTitle("Tau_mass", "mass"); 
  createTitle("Tau_charge", "electric charge"); 
  createTitle("Tau_decayMode", "decayMode()"); 
  createTitle("Tau_chargedIso", "charged isolation"); 
  createTitle("Tau_neutralIso", "neutral (photon) isolation"); 
  
  createTitle("nPhoton", "Photons after basic selection (pt > 5 )"); 
  createTitle("Photon_pt", "pt"); 
  createTitle("Photon_eta", "eta"); 
  createTitle("Photon_phi", "phi"); 
  createTitle("Photon_mass", "mass"); 
  createTitle("Photon_charge", "electric charge"); 
  createTitle("Photon_pfRelIso03_all", "PF relative isolation dR=0.3, total (with rho*EA PU corrections)"); 
  
  createTitle("MET_pt", "pt"); 
  createTitle("MET_phi", "phi"); 
  createTitle("MET_sumEt", "scalar sum of Et"); 
  createTitle("MET_significance", "MET significance"); 
  createTitle("MET_covXX", "xx element of met covariance matrix"); 
  createTitle("MET_covXY", "xy element of met covariance matrix"); 
  createTitle("MET_covYY", "yy element of met covariance matrix"); 
  
  createTitle("CaloMET_pt", "pt"); 
  createTitle("CaloMET_phi", "phi"); 
  createTitle("CaloMET_sumEt", "scalar sum of Et"); 
  
  createTitle("nJet", "PFJets, ak5 for Run 1, ak4 for Run2, after basic selection (pt > 15)"); 
  createTitle("Jet_pt", "pt with JEC applied"); 
  createTitle("Jet_ptuncor", "pt before application of JEC"); 
  createTitle("Jet_eta", "eta"); 
  createTitle("Jet_phi", "phi"); 
  createTitle("Jet_mass", "mass"); 
  createTitle("Jet_area", "jet catchment area, for JECs"); 
  createTitle("Jet_nConstituents", "number of particles in the jet"); 
  createTitle("Jet_nElectrons", "number of electrons in the jet"); 
  createTitle("Jet_nMuons", "number of muons in the jet"); 
  createTitle("Jet_chEmEF", "charged Electromagnetic Energy Fraction"); 
  createTitle("Jet_chHEF", "charged Hadron Energy Fraction"); 
  createTitle("Jet_neEmEF", "neutral Electromagnetic Energy Fraction"); 
  createTitle("Jet_neHEF", "neutral Hadron Energy Fraction"); 
  createTitle("Jet_jetId", "Jet ID flags bit1 is loose (does not always exist), bit2 is tight, bit3 is tightLepVeto"); 
  
  if (!isData) {
   
  createTitle("nGenJet", "GenJets, i.e. ak5 or ak4 Jets made with visible genparticles"); 
  createTitle("GenJet_pt", "pt"); 
  createTitle("GenJet_eta", "eta"); 
  createTitle("GenJet_phi", "phi"); 
  createTitle("GenJet_mass", "mass"); 
  
  }
  
  createTitle("nFatJet", "ak7 (Run 1) or ak8 (Run 2) PFjets for wide jet or boosted analysis"); 
  createTitle("FatJet_pt", "pt"); 
  createTitle("FatJet_eta", "eta"); 
  createTitle("FatJet_phi", "phi"); 
  createTitle("FatJet_mass", "mass"); 
  createTitle("FatJet_area", "jet catchment area, for JECs"); 
  createTitle("FatJet_nConstituents", "number of PF constituents"); 
  createTitle("FatJet_nElectrons", "number of Electrons in jet"); 
  createTitle("FatJet_nMuons", "number of Muons in jet"); 
  createTitle("FatJet_chEmEF", "FatJet_chEmEF"); 
  createTitle("FatJet_chHEF", "FatJet_chHEF"); 
  createTitle("FatJet_neEmEF", "FatJet_neEmEF"); 
  createTitle("FatJet_neHEF", "FatJet_neHEF"); 
  
  createTitle("nTrackJet", "ak5 (Run 1) or ak4 (Run 2) track jets"); 
  createTitle("TrackJet_pt", "pt"); 
  createTitle("TrackJet_eta", "eta"); 
  createTitle("TrackJet_phi", "phi"); 
  createTitle("TrackJet_mass", "mass"); 
  createTitle("TrackJet_area", "area"); 
  createTitle("TrackJet_nConstituents", "number of track constituents"); 
  createTitle("TrackJet_nElectrons", "number of Electrons in jet"); 
  createTitle("TrackJet_nMuons", "number of Muons in jet");  */
  
  createTitle("nTrigObj", "trigger objects, only partially filled so far"); 
  createTitle("TrigObj_id", "ID of the object: 11 = Electron (PixelMatched e/gamma), 22 = Photon (PixelMatch-vetoed e/gamma), 13 = Muon, 15 = Tau, 1 = Jet, 6 = FatJet, 2 = MET, 3 = HT, 4 = MHT"); 
  createTitle("TrigObj_filterBits", "extra bits of associated information: 1 = CaloIdL_TrackIdL_IsoVL, 2 = 1e (WPTight), 4 = 1e (WPLoose), 8 = OverlapFilter PFTau, 16 = 2e, 32 = 1e-1mu, 64 = 1e-1tau, 128 = 3e, 256 = 2e-1mu, 512 = 1e-2mu for Electron (PixelMatched e/gamma); 1 = TrkIsoVVL, 2 = Iso, 4 = OverlapFilter PFTau, 8 = 1mu, 16 = 2mu, 32 = 1mu-1e, 64 = 1mu-1tau, 128 = 3mu, 256 = 2mu-1e, 512 =1mu-2e for Muon; 1 = LooseChargedIso, 2 = MediumChargedIso, 4 = TightChargedIso, 8 = TightID OOSC photons, 16 = HPS, 32 = single-tau + tau+MET, 64 = di-tau, 128 = e-tau, 256 = mu-tau, 512 = VBF+di-tau for Tau; 1 = VBF cross-cleaned from loose iso PFTau for Jet; "); 
  createTitle("TrigObj_pt", "pt"); 
  createTitle("TrigObj_phi", "phi"); 
  createTitle("TrigObj_eta", "eta"); 
  
  createTitle("Trig_goodMinBiasTrigger", "generic flag: event trigger suited for Minimum Bias analysis"); 
  createTitle("Trig_goodJetTrigger",     "generic flag: event trigger suited for inclusive Jet analysis"); 
  createTitle("Trig_goodMuTrigger",      "generic flag: event trigger suited for inclusive Muon analysis"); 
  createTitle("Trig_goodETrigger",       "generic flag: event trigger suited for inclusive Electron analysis"); 
  createTitle("Trig_ZeroBiasFlag",       "flag for kind of ZeroBias trigger"); 
  createTitle("Trig_MinBiasFlag",        "flag for kind of MinimumBias irigger"); 
  createTitle("Trig_MinBiasMult",        "flag for multiplicity in Minimum Bias Multiplicity triggers"); 
  createTitle("Trig_MuThresh",           "muon trigger pt threshold"); 
  createTitle("Trig_MuL1Thresh",         "L1 muon trigger pt threshold"); 
  createTitle("Trig_MuL2Thresh",         "L2 muon trigger pt threshold"); 
  createTitle("Trig_IsoMuThresh",        "isolated muon trigger pt threshold"); 
  createTitle("Trig_DoubleMuThresh",     "double muon trigger, lower pt threshold"); 
  createTitle("Trig_JpsiThresh",         "Jpsi trigger, Jpsi pt threshold"); 
  createTitle("Trig_MuHadFlag",          "flag for kind of Muon+Hadron trigger"); 
  createTitle("Trig_MuEGFlag",           "flag for kind of Muon+Electron or Photon trigger"); 
  createTitle("Trig_ElectronThresh",     "electron trigger pt threshold"); 
  createTitle("Trig_DoubleElectronThresh","double electron trigger lower pt threshold"); 
  createTitle("Trig_PhotonThresh",       "photon trigger pt threshold"); 
  createTitle("Trig_JetThresh",          "jet trigger Et threshold"); 
  createTitle("Trig_DiJetThresh",        "diJet trigger Et threshold"); 
  createTitle("Trig_TriJetThresh",       "triJet trigger Et threshold"); 
  createTitle("Trig_QuadJetThresh",      "four-Jet trigger Et threshold"); 
  createTitle("Trig_HTThresh",           "HT trigger HT threshold"); 
  createTitle("Trig_BThresh",            "threshold of B trigger object"); 
  createTitle("Trig_METThresh",          "MET trigger threshold"); 

#ifdef charm
  if (covout) {

  createTitle("D0_Covxx", "D0_Covxx, covariance matrix of D0 vertex"); 
  createTitle("D0_Covyx", "D0_Covyx"); 
  createTitle("D0_Covyy", "D0_Covyy"); 
  createTitle("D0_Covzx", "D0_Covzx"); 
  createTitle("D0_Covzy", "D0_Covzy"); 
  createTitle("D0_Covzz", "D0_Covzz");           
 
  }

  createTitle("D0_DstarIdx", "cross reference to D* this D0 is part of, if any"); 
  createTitle("D0_ambiPrim", "flag: primary vertex association is ambigous"); 
  createTitle("D0_chi2", "chi2/ndof of D0 vertex fit");   
  createTitle("D0_cosphi", "D0 cosine of vertex alignment angle in 3D"); 
  createTitle("D0_cosphixy", "D0 cosine of vertex alignment angle in xy"); 
  createTitle("D0_dl", "D0 decay length in 3D"); 
  createTitle("D0_dlErr", "D0 decay length error in 3D"); 
  createTitle("D0_dlSig", "D0 decay length significance in 3D"); 
  createTitle("D0_dlxy", "D0 decay length in xy"); 
  createTitle("D0_dlxyErr", "D0 decay length error in xy"); 
  createTitle("D0_dlxySig", "D0 decay length significance in xy"); 
  createTitle("D0_eta", "D0 eta"); 
  createTitle("D0_hasMuon", "flag: one of decay 'hadrons' is identified as muon"); 
  createTitle("D0_mass12", "D0 mass, first track is K"); 
  createTitle("D0_mass21", "D0 mass, 2nd track is K"); 
  createTitle("D0_massKK", "D0 mass, both tracks are K"); 
  createTitle("D0_phi", "D0 phi"); 
  createTitle("D0_promptFlag", "1=prompt, 0=nonprompt, -1= unknown"); 
  createTitle("D0_pt", "D0 pt"); 
  createTitle("D0_ptfrac", "transverse momentum fraction of D0 w.r.t. total of associated primary vertex, excluding muons"); 
  createTitle("D0_ptfrac04", "transverse momentum fraction of D0 w.r.t. cone of DeltaR=0.4, excluding muons"); 
  createTitle("D0_ptfrac07", "transverse momentum fraction of D0 w.r.t. cone of DeltaR=0.7, excluding muons"); 
  createTitle("D0_ptfrac10", "transverse momentum fraction of D0 w.r.t. cone of DeltaR=1.0, excluding muons"); 
  createTitle("D0_ptfrac15", "transverse momentum fraction of D0 w.r.t. cone of DeltaR=1.5, excluding muons"); 
  createTitle("D0_rap", "D0 rapidity"); 
  createTitle("D0_simIdx", "cross reference to D0 in GenPart (loose matching), -1 if not matched"); 
  createTitle("D0_t1Kprob", "Kaon probability of first D0 decay track (from dEdx)"); 
  createTitle("D0_t1chg", "charge of first D0 decay track"); 
  createTitle("D0_t1chindof", "chi2/ndof of first track"); 
  createTitle("D0_t1dEdxnmeas", "number of measured dEdx hits, first track"); 
  createTitle("D0_t1dEdxnsat", "number of saturated dEdx hits, first track"); 
  createTitle("D0_t1dxy", "dxy of first track from D0"); 
  createTitle("D0_t1dz", "dz of first track from D0"); 
  createTitle("D0_t1eta", "eta of first D0 decay track"); 
  createTitle("D0_t1isHighPurity", "flag: first track satisfies high purity criteria"); 
  createTitle("D0_t1muIdx", "muon index of firts track, if any"); 
  createTitle("D0_t1nPix", "number of pixel hits of first track"); 
  createTitle("D0_t1nValid", "number of valid hits of firat track"); 
  createTitle("D0_t1pdgId", "pdgId of first track"); 
  createTitle("D0_t1phi", "phi of first D0 decay track"); 
  createTitle("D0_t1piprob", "pion probability of first D0 decay track (from dEdx)"); 
  createTitle("D0_t1pt", "pt of first D0 decay track"); 
  createTitle("D0_t1tkIdx", "track unique Id of firts D0 deacy track"); 
  createTitle("D0_t1vtxIdx", "Id of vertex to which firts D0 decay track was originally associated"); 
  createTitle("D0_t2Kprob", "Kaon probability of 2nd D0 deacy track (from dEdx)"); 
  createTitle("D0_t2chg", "charge of 2nd D0 decay track"); 
  createTitle("D0_t2chindof", "chi2/ndof of 2nd track"); 
  createTitle("D0_t2dEdxnmeas", "numbre of measured dEdx hits, 2nd track"); 
  createTitle("D0_t2dEdxnsat", "number of saturated dEdx hits, 2nd track"); 
  createTitle("D0_t2dxy", "dxy of 2nd D0 decay track"); 
  createTitle("D0_t2dz", "dz of 2nd D0 decay track"); 
  createTitle("D0_t2eta", "eta of 2nd D0 decay track"); 
  createTitle("D0_t2isHighPurity", "flag: 2nd track satisfies high purity criteria"); 
  createTitle("D0_t2muIdx", "muon index of 2nd track, if any"); 
  createTitle("D0_t2nPix", "number of pixel hits of 2nd track"); 
  createTitle("D0_t2nValid", "number of valid hits of 2nd track"); 
  createTitle("D0_t2pdgId", "pdgId of 2nd track"); 
  createTitle("D0_t2phi", "phi of 2nd D0 decay track"); 
  createTitle("D0_t2piprob", "pion probability of 2nd D0 decay track (from dEdx)"); 
  createTitle("D0_t2pt", "pt of 2nd D0 deacy track"); 
  createTitle("D0_t2tkIdx", "pttrack unique Id of 2nd D0 deacy track"); 
  createTitle("D0_t2vtxIdx", "Id of vertex to which 2nd D0 deacy track was originally associated"); 
  createTitle("D0_vtxIdx", "cross reference to associated primary vertex"); 
  createTitle("D0_x", "x position of D0 vertex"); 
  createTitle("D0_y", "y position of D0 vertex"); 
  createTitle("D0_z", "z position of D0 vertex"); 
  createTitle("nD0", "reconstructed D0 candidates"); 
  
  createTitle("Dstar_D0ambiPrim", "flag: primary vertex association is ambigous"); 
  createTitle("Dstar_D0chi2", "chi2/ndof of D0 vertex fit"); 
  createTitle("Dstar_D0cosphi", "D0 from D*, cosine of vertex alignment angle in 3D"); 
  createTitle("Dstar_D0cosphixy", "D0 from D*, cosine of vertex alignment angle in xy"); 
  createTitle("Dstar_D0dl", "D0 from D*, decay length in 3D"); 
  createTitle("Dstar_D0dlErr", "D0 from D*, decay length error in 3D"); 
  createTitle("Dstar_D0dlSig", "D0 from D*, decay length significance in 3D"); 
  createTitle("Dstar_D0dlxy", "D0 from D*, decay length in xy"); 
  createTitle("Dstar_D0dlxyErr", "D0 from D*, decay length error in xy"); 
  createTitle("Dstar_D0dlxySig", "D0 from D*, decay length significance in xy"); 
  createTitle("Dstar_D0eta", "eta of D0 from D*"); 
  createTitle("Dstar_D0mass", "mass of D0 from D*"); 
  createTitle("Dstar_D0massKK", "mass for D0->KK hypothesis"); 
  createTitle("Dstar_D0phi", "phi of D0 from D*"); 
  createTitle("Dstar_D0promptFlag", "1=prompt, 0=nonprompt, -1= unknown"); 
  createTitle("Dstar_D0pt", "pt of D0 from D*"); 
  createTitle("Dstar_D0ptfrac", "transverse momentum fraction of D* w.r.t. total of associated primary vertex, excluding muons"); 
  createTitle("Dstar_D0ptfrac04", "transverse momentum fraction of D* w.r.t. cone of DeltaR=0.4, excluding muons"); 
  createTitle("Dstar_D0ptfrac07", "transverse momentum fraction of D* w.r.t. cone of DeltaR=0.7, excluding muons"); 
  createTitle("Dstar_D0ptfrac10", "transverse momentum fraction of D* w.r.t. cone of DeltaR=1.0, excluding muons"); 
  createTitle("Dstar_D0ptfrac15", "transverse momentum fraction of D* w.r.t. cone of DeltaR=1.5, excluding muons"); 
  createTitle("Dstar_D0recIdx", "cross reference to associated primary vertex"); 
  createTitle("Dstar_D0simIdx", "cross reference to D0 in GenPart (loose matching), -1 if not matched"); 
  createTitle("Dstar_D0x", "x position of D0 (from D*) vertex"); 
  createTitle("Dstar_D0y", "y position of D0 (from D*) vertex"); 
  createTitle("Dstar_D0z", "z position of D0 (from D*) vertex"); 
  createTitle("Dstar_KKprob", "Kaon probability of K from D0 from D* (from dEdx)"); 
  createTitle("Dstar_Kchg", "charge of K from D0 from D*"); 
  createTitle("Dstar_Kchindof", "chi2/ndof of K track"); 
  createTitle("Dstar_KdEdxnmeas", "number of measured dEdx hits, K track"); 
  createTitle("Dstar_KdEdxnsat", "number of saturated dEdx hits, K track"); 
  createTitle("Dstar_Kdxy", "dxy of K from D0 from D*"); 
  createTitle("Dstar_Kdz", "dz of K from D0 from D*"); 
  createTitle("Dstar_Keta", "eta of K from D0 from D*"); 
  createTitle("Dstar_KisHighPurity", "flag: K track satisfies high purity criteria"); 
  createTitle("Dstar_KmuIdx", "index to muon, K track"); 
  createTitle("Dstar_KnPix", "number of pixel hits of K track"); 
  createTitle("Dstar_KnValid", "number of valid hits of K track"); 
  createTitle("Dstar_Kphi", "phi of K from D0 from D*"); 
  createTitle("Dstar_Kpiprob", "pion probability of K from D0 from D* (from dEdx)"); 
  createTitle("Dstar_Kpt", "pt of K from D0 from D*"); 
  createTitle("Dstar_KtkIdx", "track unique Id of K from D0 from D*"); 
  createTitle("Dstar_KvtxIdx", "Id of vertex to which K from D0 from D* was originally associated"); 
  createTitle("Dstar_deltam", "D*-D0 mass difference (add D0 mass for D* mass)"); 
  createTitle("Dstar_deltamKK", "deltam for D0->KK hypothesis"); 
  createTitle("Dstar_deltamr", "D*-D0 mass difference with slow pion refit"); 
  createTitle("Dstar_deltamrKK", "refitted deltam for D0->KK hypothesis"); 
  createTitle("Dstar_eta", "D* eta"); 
  createTitle("Dstar_hasMuon", "flag: one of decay 'hadrons' is identified as muon"); 
  createTitle("Dstar_phi", "D* phi"); 
  createTitle("Dstar_piKprob", "Kaon probability of pi from D0 from D* (from dEdx)"); 
  createTitle("Dstar_pichg", "charge of pi from D0 from D*"); 
  createTitle("Dstar_pichindof", "chi2/ndof of pi track"); 
  createTitle("Dstar_pidEdxnmeas", "number of measured dEdx hits"); 
  createTitle("Dstar_pidEdxnsat", "number of saturated dEdx hits"); 
  createTitle("Dstar_pidxy", "dxy of pi from D0 from D*"); 
  createTitle("Dstar_pidz", "dz of pi from D0 from D*"); 
  createTitle("Dstar_pieta", "eta of pi from D0 from D*"); 
  createTitle("Dstar_piisHighPurity", "flag: pi track satisfies high purity criteria"); 
  createTitle("Dstar_pimuIdx", "muon index for pi track"); 
  createTitle("Dstar_pinPix", "number of pixel hits of pi track"); 
  createTitle("Dstar_pinValid", "number of valid hits of pi track"); 
  createTitle("Dstar_piphi", "phi of pi from D0 from D*"); 
  createTitle("Dstar_pipiprob", "pion probability of pi from D0 from D* (from dEdx)"); 
  createTitle("Dstar_pipt", "pt of pi from D0 from D*"); 
  createTitle("Dstar_pitkIdx", "track unique Id of pi from D0 from D*"); 
  createTitle("Dstar_pivtxIdx", "Id of vertex to which pi from D0 from D* was originally associated"); 
  createTitle("Dstar_pisKprob", "Kaon probability of pis from D* (from dEdx)"); 
  createTitle("Dstar_pischg", "charge of pis from D*"); 
  createTitle("Dstar_pischindof", "chi2/ndof of pis track"); 
  createTitle("Dstar_pischir", "chi2/ndof of pis track"); 
  createTitle("Dstar_pisdEdxnmeas", "number of measured dEdx hits, pis"); 
  createTitle("Dstar_pisdEdxnsat", "number of saturated dEdx hits, pis"); 
  createTitle("Dstar_pisdxy", "dxy of pis from D*"); 
  createTitle("Dstar_pisdz", "dz of pis from D*"); 
  createTitle("Dstar_piseta", "eta of pis from D*"); 
  createTitle("Dstar_pisetar", "eta of pis from D*"); 
  createTitle("Dstar_pisisHighPurity", "pis is high purity track"); 
  createTitle("Dstar_pismuIdx", "index to muon, if any"); 
  createTitle("Dstar_pisnPix", "number of pixel hits of pis track"); 
  createTitle("Dstar_pisnValid", "number of valid hits of pis track"); 
  createTitle("Dstar_pisphi", "phi of pis from D*"); 
  createTitle("Dstar_pisphir", "phi of pis from D*"); 
  createTitle("Dstar_pispiprob", "pion probability of pis from D* (from dEdx)"); 
  createTitle("Dstar_pispt", "pt of pis from D*"); 
  createTitle("Dstar_pisptr", "pt of pis from D*"); 
  createTitle("Dstar_pistkIdx", "track unique Id of pis from D*"); 
  createTitle("Dstar_pisvtxIdx", "Id of vertex to which pis from D* was originally associated"); 
  createTitle("Dstar_promptFlag", "1=prompt, 0=nonprompt, -1= unknown"); 
  createTitle("Dstar_pt", "D* pt"); 
  createTitle("Dstar_ptfrac", "transverse momentum fraction of D* w.r.t. total of associated primary vertex"); 
  createTitle("Dstar_rap", "D* rapidity"); 
  createTitle("Dstar_simIdx", "cross reference to D* in GenPart (loose matching), -1 if not matched"); 
  createTitle("Dstar_vtxIdx", "cross reference to associated primary vertex"); 
  createTitle("nDstar", "D* -> K pi pis candidates"); 
#endif

}

void NanoAnalyzer::createTitle(const std::string &name, const std::string &title) { 
 TBranch *branch = t_event->GetBranch(name.c_str()); branch->SetTitle(title.c_str());
} 
