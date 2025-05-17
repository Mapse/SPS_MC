//////////////////////////// for Dmeson variables ////////////////////////////
  
  /// nanoAOD extension /// 
    
  //------------------------------ For D0 branches --------------------------//
  UInt_t nD0;  
  // track 1 from D0
  vector<Float_t> D0t1_pt;       // track 1 pt  after refit
  vector<Float_t> D0t1_eta;      // track 1 eta after refit
  vector<Float_t> D0t1_phi;      // track 1 phi after refit
  vector<Int_t> D0t1_chg;        // track 1 charge
  vector<Int_t> D0t1_tkIdx;      // *
  vector<Float_t> D0t1_Kprob;    // * temporarily misused for Strip dEdx
  vector<Float_t> D0t1_piprob;   // * temporarily misused for Strip dEdxErr
  vector<Int_t> D0t1_dEdxnmeas;  // # of dEdx measurements
  vector<Int_t> D0t1_dEdxnsat;   // # of saturated dEdx measurements
  vector<Int_t> D0t1_vtxIdx;     // track 1 primary vertex before refit 
  vector<Int_t> D0t1_muIdx;      // track 1 is muon, pointer to muon 
  vector<Float_t> D0t1_chindof;  // track 1 chi2/ndof
  vector<Int_t> D0t1_nValid;     // track 1 Valid Tracker hits
  vector<Int_t> D0t1_nPix;       // track 1 Valid Pixel Hits
  vector<uint8_t> D0t1_isHighPurity; // high purity flag
  vector<Float_t> D0t1_dxy;      // track 1 dxy w.r.t. *** what? ***
  vector<Float_t> D0t1_dz;       // track 1 dz w.r.t. *** what? ***
  vector<Int_t> D0t1_pdgId;      // * 

  // track 2 from D0
  vector<Float_t> D0t2_pt;       // track 2 pt  after refit
  vector<Float_t> D0t2_eta;      // track 2 eta after refit
  vector<Float_t> D0t2_phi;      // track 2 phi after refit
  vector<Int_t> D0t2_chg;        // track 2 charge
  vector<Int_t> D0t2_tkIdx;      // *
  vector<Float_t> D0t2_Kprob;    // * temporarily misused for Strip dEdx
  vector<Float_t> D0t2_piprob;   // * temporarily misused for Strip dEdxErr
  vector<Int_t> D0t2_dEdxnmeas;  // # of dEdx measurements
  vector<Int_t> D0t2_dEdxnsat;   // # of saturated dEdx measurements
  vector<Int_t> D0t2_vtxIdx;     // track 2 primary vertex before refit
  vector<Int_t> D0t2_muIdx;      // track 2 is muon, pointer to muon 
  vector<Float_t> D0t2_chindof;  // track 2 chi2/ndof
  vector<Int_t> D0t2_nValid;     // track 2 Valid Tracker hits
  vector<Int_t> D0t2_nPix;       // track 2 Valid Pixel hits
  vector<uint8_t> D0t2_isHighPurity; // high purity flag
  vector<Float_t> D0t2_dxy;      // track 2 dxy w.r.t. *** what? ***
  vector<Float_t> D0t2_dz;       // track 2 dz  w.r.t. *** what? ***
  vector<Int_t> D0t2_pdgId;      // *

  // D0
  vector<Float_t> D0_pt;         // D0 pt  after refit
  vector<Float_t> D0_eta;        // D0 eta after refit
  vector<Float_t> D0_phi;        // D0 phi after refit
  vector<Float_t> D0_rap;        // D0 rapidity after refit
  vector<Float_t> D0_mass12;     // D0 mass 1=K
  vector<Float_t> D0_mass21;     // D0 mass 2=K
  vector<Float_t> D0_massKK;     // D0 mass both=K
  vector<Int_t>   D0_simIdx;     // matched true D0 in genparticle list
  vector<Int_t>   D0_DstarIdx;   // equivalent D0 in Dstar list
  vector<uint8_t> D0_ambiPrim;   // flag indicating ambigous primary assignment
  vector<Int_t>   D0_vtxIdx;     // associated prim. vtx (can differ from 1,2)
  vector<uint8_t> D0_hasMuon;    // true if either "K" or "pi" is muon
  vector<Float_t> D0_chi2;       // chi2 of D0 vertex
  vector<Float_t> D0_dlxy;       // D0 decay length in xy
  vector<Float_t> D0_dlxyErr;    // D0 decay length uncertainty in xy
  vector<Float_t> D0_dlxySig;    // D0 decay length significance in xy
  vector<Float_t> D0_cosphixy;   // cosine of angle (momentum, decay length) xy
  vector<Float_t> D0_dl;         // D0 decay length in 3D (typically > xy)
  vector<Float_t> D0_dlErr;      // D0 decay length uncertainty in 3D (>xy)
  vector<Float_t> D0_dlSig;      // D0 decay length significance in 3D 
  vector<Float_t> D0_cosphi;     // cosine of angle (momentum, decay length) 3D
  vector<Float_t> D0_ptfrac;     // D0_pt/sum pt at vertex *** not final ***
  vector<Float_t> D0_ptfrac15;   // D0_pt/sum pt at vertex in cone 1.5
  vector<Float_t> D0_ptfrac10;   // D0_pt/sum pt at vertex in cone 1.0
  vector<Float_t> D0_ptfrac07;   // D0_pt/sum pt at vertex in cone 0.7
  vector<Float_t> D0_ptfrac04;   // D0_pt/sum pt at vertex in cone 0.4
  vector<Float_t> D0_x;          // D0 vertex x
  vector<Float_t> D0_y;          // D0 vertex y
  vector<Float_t> D0_z;          // D0 vertex z
  vector<Float_t> D0_Covxx;      // D0 vertex covariance
  vector<Float_t> D0_Covyx;
  vector<Float_t> D0_Covzx;
  vector<Float_t> D0_Covyy;
  vector<Float_t> D0_Covzy;
  vector<Float_t> D0_Covzz;
  // Josry2 prompt/nonprompt Dstar/D0 extension
  vector<Int_t>   D0_promptFlag; // matched true prompt D0 in genparticle list

  //----------------------------- For Dstar branches ------------------------//
  UInt_t nDstar;

  // Slow Pion from Dstar
  vector<Float_t> Dstarpis_pt;      // Dstar slow pion pt before refit
  vector<Float_t> Dstarpis_eta;     // Dstar slow pion eta before refit
  vector<Float_t> Dstarpis_phi;     // Dstar slow pion phi before refit
  vector<Float_t> Dstarpis_ptr;     // Dstar slow pion pt after refit
  vector<Float_t> Dstarpis_etar;    // Dstar slow pion eta after refit
  vector<Float_t> Dstarpis_phir;    // Dstar slow pion phi after refit
  vector<Int_t> Dstarpis_chg;       // Dstar slow pion charge 
                                    // (=charge of Dstar for right sign)
  vector<Int_t> Dstarpis_tkIdx;     // *
  vector<Float_t> Dstarpis_Kprob;    // * temporarily misused for Strip dEdx
  vector<Float_t> Dstarpis_piprob;   // * temporarily misused for Strip dEdxErr
  vector<Int_t> Dstarpis_dEdxnmeas;  // # of dEdx measurements
  vector<Int_t> Dstarpis_dEdxnsat;   // # of saturated dEdx measurements
  vector<Int_t> Dstarpis_vtxIdx;    // slow pion primary vertex before refit
  vector<Int_t> Dstarpis_muIdx;     // slow pion is muon, pointer to muon 
  vector<Float_t> Dstarpis_chindof; // slow pion chi2/ndof 
  vector<Float_t> Dstarpis_chir;    // slow pion refit chi2
  vector<Int_t> Dstarpis_nValid;    // slow pion Valid Tracker hits
  vector<Int_t> Dstarpis_nPix;      // slow pion Valid Pixel hits
  vector<uint8_t> Dstarpis_isHighPurity; // high purity flag
  vector<Float_t> Dstarpis_dxy;     // slow pion dx w.r.t. *** what? ***
  vector<Float_t> Dstarpis_dz;      // slow pion dz w.r.t. *** what? ***


  // D0 from Dstar
  vector<Float_t> DstarD0_pt;       // D0 from D* pt
  vector<Float_t> DstarD0_eta;      // D0 from D* eta
  vector<Float_t> DstarD0_phi;      // D0 from D* phi
  vector<Float_t> DstarD0_mass;     // D0 from D* mass
  vector<Float_t> DstarD0_massKK;   // D0 from D* mass, KK mode
  vector<Float_t> DstarD0_chi2;     // D0 from D* vertex chi2
  vector<Float_t> DstarD0_dlxy;     // D0 from D* decay length xy
  vector<Float_t> DstarD0_dlxyErr;  // D0 from D* decay length error xy
  vector<Float_t> DstarD0_dlxySig;  // D0 from D* deacy length significance xy
  vector<Float_t> DstarD0_cosphixy; // D0 from D* cosine (momentum, dl) xy
  vector<Float_t> DstarD0_dl;       // D0 from D* decay length 3D
  vector<Float_t> DstarD0_dlErr;    // D0 from D* decay length error 3D
  vector<Float_t> DstarD0_dlSig;    // D0 from D* decay length significance 3D
  vector<Float_t> DstarD0_cosphi;   // D0 from D* cosine (momentum, dl) 3D
  vector<Float_t> DstarD0_ptfrac;   // D0 pt/all had sum pt at vtx (not final)
  vector<Float_t> DstarD0_ptfrac15; // D0 pt/all had sum pt at vtx in cone 1.5
  vector<Float_t> DstarD0_ptfrac10; // D0 pt/all had sum pt at vtx in cone 1.0
  vector<Float_t> DstarD0_ptfrac07; // D0 pt/all had sum pt at vtx in cone 0.7
  vector<Float_t> DstarD0_ptfrac04; // D0 pt/all had sum pt at vtx in cone 0.4
  vector<Float_t> DstarD0_x;        // D0 vertex x
  vector<Float_t> DstarD0_y;        // D0 vertex y
  vector<Float_t> DstarD0_z;        // D0 vertex z
  vector<Int_t>   DstarD0_simIdx;   // matched true D0 in genparticle list
  vector<Int_t>   DstarD0_recIdx;   // equivalent D0 in D0 list
  vector<uint8_t> DstarD0_ambiPrim; // flag indicating ambigous prim. vtx ass. 
  vector<Int_t>   DstarD0_promptFlag;   // matched true prompt D0 in genparticle list

  // Kaon from Dstar
  vector<Float_t> DstarK_pt;        // Kaon pt
  vector<Float_t> DstarK_eta;       // Kaon eta
  vector<Float_t> DstarK_phi;       // Kaon phi
  vector<Int_t> DstarK_chg;         // Kaon charge
  vector<Int_t> DstarK_tkIdx;       // *
  vector<Float_t> DstarK_Kprob;     // * temporarily misused for Strip dEdx
  vector<Float_t> DstarK_piprob;    // * temporarily misused for Strip dEdxErr
  vector<Int_t> DstarK_dEdxnmeas;   // # of dEdx measurements
  vector<Int_t> DstarK_dEdxnsat;    // # of saturated dEdx measurements
  vector<Int_t> DstarK_vtxIdx;      // Kaon primary vertex before refit 
  vector<Int_t> DstarK_muIdx;       // Kaon is muon, pointer to muon 
  vector<Float_t> DstarK_chindof;   // Kaon chi2/ndof
  vector<Int_t> DstarK_nValid;      // Kaon Valid Tracker hits
  vector<Int_t> DstarK_nPix;        // Kaon Pixel hits
  vector<uint8_t> DstarK_isHighPurity; // high purity flag
  vector<Float_t> DstarK_dxy;       // Kaon dxy w.r.t. *** what? ***
  vector<Float_t> DstarK_dz;        // Kaon dz w.r.t. *** what? ***

  // Pion from Dstar
  vector<Float_t> Dstarpi_pt;       // Pion pt
  vector<Float_t> Dstarpi_eta;      // Pion eta 
  vector<Float_t> Dstarpi_phi;      // Pion phi
  vector<Int_t> Dstarpi_chg;        // Pion charge (wrong sign allowed)
  vector<Int_t> Dstarpi_tkIdx;      // *
  vector<Float_t> Dstarpi_Kprob;    // * temporarily misused for Strip dEdx
  vector<Float_t> Dstarpi_piprob;   // * temporarily misused for Strip dEdxErr
  vector<Int_t> Dstarpi_dEdxnmeas;  // # of dEdx measurements
  vector<Int_t> Dstarpi_dEdxnsat;   // # of saturated dEdx measurements
  vector<Int_t> Dstarpi_vtxIdx;     // Pion primary vertex before refit  
  vector<Int_t> Dstarpi_muIdx;      // Pion is muon, pointer to muon 
  vector<Float_t> Dstarpi_chindof;  // Pion chi2/ndof
  vector<Int_t> Dstarpi_nValid;     // Pion Valid Tracker hits
  vector<Int_t> Dstarpi_nPix;       // Pion Valid Pixel hits
  vector<uint8_t> Dstarpi_isHighPurity; // high purity flag
  vector<Float_t> Dstarpi_dxy;      // Pion dxy w.r.t *** what? ***
  vector<Float_t> Dstarpi_dz;       // Pion dz w.r.t *** what? ***
  
  // Dstar
  vector<Float_t> Dstar_pt;         // D* pt  after D0 refit, w/o pis refit
  vector<Float_t> Dstar_eta;        // D* eta after D0 refit, w/o pis refit
  vector<Float_t> Dstar_phi;        // D* phi after D0 refit, w/o pis refit
  vector<Float_t> Dstar_rap;        // D* rapidity after D0 refit, w/o pis refit
  vector<Float_t> Dstar_deltam;     // mD*-mD0 after D0 refit, w/o pis refit 
  vector<Float_t> Dstar_deltamr;    // mD*-mD0 after D0 refit, with pis refit 
  vector<Float_t> Dstar_deltamKK;   // mD*-mD0KK after D0 refit, w/o pis refit 
  vector<Float_t> Dstar_deltamrKK;  // mD*-mD0KK after D0 refit, with pis refit 
  vector<Int_t> Dstar_simIdx;       // associated true D* in Genpart, if any
  vector<Int_t> Dstar_vtxIdx;       // ass. prim. vtx (may differ from tracks)
  vector<uint8_t> Dstar_hasMuon;    // true if "K" or "pi" or "pislow" is muon
  vector<Float_t> Dstar_ptfrac;     // pt(D*)/pt(all hadrons at prim. vertex)
  // Josry2 prompt/nonprompt Dstar/D0 extension
  vector<Int_t>   Dstar_promptFlag; // matched true prompt Dstar in genparticle list
  
  vector<TransientTrack> Dstar_pisTransientTrack;
  vector<Int_t> Dstar_associationIdx;
  vector<Float_t> Dstar_associationProb;
  vector<Float_t> Dstar_associationchi2;
