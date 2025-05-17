        // The dataset flags indicate the current knowledge about the whole 
        // current dataset. This should become unique after a few events.
        // The `Trig' flags indicate which datasets the current event may 
        // belong to (since an event can have fired triggers from several 
        // datasets, this is often not unique).
        // The other variables indicate thresholds for certain classes of 
        // triggers, which may span several datasets.

        // bit for generic "Good" JSON 
        bool GoodLumisection;

        // bits for generic "Good" triggers 
        bool GoodMinBiasTrigger;
        bool GoodJetTrigger;
        bool GoodMuTrigger;
        bool GoodETrigger;

        // bits for datasets
        bool MCdataset;
        bool ZeroBiasdataset;
        bool MinimumBiasdataset;
        bool Commissioningdataset;
        bool Mudataset;
        bool MuHaddataset;
        bool DoubleMudataset;
        bool MuOniadataset;
        bool Charmoniumdataset;
        bool MuMonitordataset;
        bool Jetdataset;
        bool MultiJetdataset;
        bool JetMETTauMonitordataset;
        bool BTaudataset;
        bool BParkingdataset;
        bool MuEGdataset;
        bool Electrondataset;
        bool DoubleElectrondataset;
        bool Photondataset;
        bool EGMonitordataset;
        bool METFwddataset;
        bool datasetisunique;
        std::string dataset;

        // bits and variables for MinimumBias and Commissioning datasets
        bool ZeroBiasTrig; 
        bool MinimumBiasTrig;
        bool CommissioningTrig;
        bool GoodMinimumBiasTrig;
        bool GoodMuMinimumBiasTrig;
        int MinBiasFlag;
        int MinBiasMult;
        int ZeroBiasFlag;

        // bits and variables for various Muon, Electron and Photon datasets
        bool MuTrig;
        bool MuHadTrig;
        bool DoubleMuTrig;
        bool MuEGTrig;
        bool ElectronTrig;
        bool DoubleElectronTrig;
        bool PhotonTrig;
        bool MuOniaTrig;
        bool CharmoniumTrig;
        bool MuMonitorTrig;
        bool EGMonitorTrig;
        bool GoodMuTrig;
        bool GoodETrig;
        int MuThresh; 
        int MuL1Thresh; 
        int MuL2Thresh; 
        int IsoMuThresh; 
        int DoubleMuThresh; 
        int JpsiThresh;
        int MuHadFlag;
        int MuEGFlag;
        int ElectronThresh;
        int DoubleElectronThresh;
        int PhotonThresh;

        // bits and variables for various Jet and MET datasets 
        bool JetTrig;
        bool MultiJetTrig;
        bool JetMETTauMonitorTrig;
        bool BTauTrig;
        bool BParkingTrig;
        bool METFwdTrig;
        bool GoodJetTrig;
        int JetThresh; 
        int DiJetThresh;
        int TriJetThresh;
        int QuadJetThresh;
        int HTThresh;
        int BThresh;
      
        int METThresh; 

  // The following is a historical intention and not (yet) used
  // should/will disappear
  //Int_t dataIsInclusive;
  //Int_t dataIsHighPtJet;
  //Int_t dataIsMuonLow;
  //Int_t dataIsMuonHigh;
  //Int_t dataIsDimuonLow;
  //Int_t dataIsDimuonHigh;
  //Int_t dataIsElectronLow;
  //Int_t dataIsElectronHigh;
  //Int_t dataIsDiElectronLow;
  //Int_t dataIsDiElectronHigh;
  //Int_t dataIsMuELow;  
  //Int_t dataIsMuEHigh;
  // useful dataset chains are
  //   inclusive chain:            ZeroBias -> MinimumBias -> HT -> JetMon 
  //                            -> Jet -> JetHT -> MultiJet -> HTMHTParked
  //   high pt jet chain:          Jet -> JetHT -> MultiJet
  //   low pt single muon chain:   ZeroBias -> MinimumBias -> MuMonitor -> Mu 
  //                            -> SingleMu -> MuHad -> MuEG
  //   high pt single muon chain:  Mu -> SingleMu -> MuHad -> MuEG
  //   low pt double muon chain:   ZeroBias -> MinimumBias -> MuMonitor -> Mu 
  //                            -> SingleMu -> DoubleMu(Parked) -> MuHad 
  //                            -> MuOnia(Parked) -> Scouting 
  //   high pt double muon chain:  Mu -> SingleMu -> DoubleMu(Parked) -> MuHad
  //   low pt single electron chain: ZeroBias -> MinimumBias -> EGmonitor 
  //                            -> Electron -> SingleElectron -> ElectronHad 
  //                            -> Photon -> PhotonHad -> MuEG
  //   high pt single isolated electron chain: Electron -> SingleElectron 
  //                            -> ElectronHad -> MuEG
  //   low pt double electron chain: ZeroBias -> MinumumBias -> EGmonitor 
  //                            -> Electron -> SingleElectron -> DoubleElectron
  //                            -> ElectronHad -> Photon -> SinglePhoton 
  //                            -> DoublePhoton -> DoublePhotonHighPt 
  //                            -> PhotonHad 
  //   high pt isolated double electron chain: Electron -> SingleElectron 
  //                            -> DoubleElectron -> ElectronHad  
  //   low/high pt muon-electron chain: "Or" of single muon and single electron
  //                                    chains  (low or high pt) 
  //   Others 2010:      Commissioning, JetMETTaumonitor, BTau, METFwd 
  //   Others 2011:      TauPlusX, BTag, MET, METBTag, Tau 
  //   Others 2012:      Commissioning, MET, BTag, MTMHTParked, HcalNZS, 
  //                     NoBPTX, TauParked, HCalNZS, BJetPlusX, VBF1Parked  
