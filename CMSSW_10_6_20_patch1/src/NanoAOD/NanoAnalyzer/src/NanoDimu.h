  // store dimuon candidates 
  //------------------------------ For Dimu branches ------------------------//
  UInt_t nDimu;  
  // mu 1 from Dimu
  vector<Int_t> Dimu_t1muIdx;    // pointer to first muon
  vector<Float_t> Dimu_t1dxy;    // dxy of first muon w.r.t. allocated vertex
  vector<Float_t> Dimu_t1dz;     // dz of first muon w.r.t. allocated vertex

  // mu 2 from Dimu
  vector<Int_t> Dimu_t2muIdx;    // pointer to second muon
  vector<Float_t> Dimu_t2dxy;    // dxy of second muon w.r.t. allocated vertex
  vector<Float_t> Dimu_t2dz;     // dz of second muon w.r.t. allocated vertex

  // Dimu
  vector<Float_t> Dimu_pt;       // Dimuon pt  after refit
  vector<Float_t> Dimu_eta;      // Dimuon eta after refit
  vector<Float_t> Dimu_phi;      // Dimuon phi after refit
  vector<Float_t> Dimu_rap;      // Dimuon rapidity after refit
  vector<Float_t> Dimu_mass;     // Dimuon mass
  vector<Int_t>   Dimu_charge;   // Dimuon charge
  vector<Int_t>   Dimu_simIdx;   // matched true gamma/Z/meson in genparticle
  vector<Int_t>   Dimu_vtxIdx;   // associated prim. vtx (can differ from 1,2)
  vector<Float_t> Dimu_chi2;     // chi2 of Dimuon vertex
  vector<Float_t> Dimu_dlxy;     // Dimuon decay length in xy
  vector<Float_t> Dimu_dlxyErr;  // Dimuon decay length uncertainty in xy
  vector<Float_t> Dimu_dlxySig;  // Dimuon decay length significance in xy
  vector<Float_t> Dimu_cosphixy; // cosine of angle (momentum, decay length) xy
  vector<Float_t> Dimu_dl;       // Dimuon decay length in 3D (typically > xy)
  vector<Float_t> Dimu_dlErr;    // Dimuon decay length uncertainty in 3D (>xy)
  vector<Float_t> Dimu_dlSig;    // Dimuon decay length significance in 3D 
  vector<Float_t> Dimu_cosphi;   // cosine of angle (momentum, decay length) 3D
  vector<Float_t> Dimu_ptfrac;   // Dimuon_pt/sum pt at vertex *** not final***
  vector<Float_t> Dimu_x;        // Dimuon vertex x
  vector<Float_t> Dimu_y;        // Dimuon vertex y
  vector<Float_t> Dimu_z;        // Dimuon vertex z
  vector<Float_t> Dimu_Covxx;    // Dimuon vertex covariance
  vector<Float_t> Dimu_Covyx;
  vector<Float_t> Dimu_Covzx;
  vector<Float_t> Dimu_Covyy;
  vector<Float_t> Dimu_Covzy;
  vector<Float_t> Dimu_Covzz;

  vector<TransientTrack> Dimu_muon1TransientTrack;
  vector<TransientTrack> Dimu_muon2TransientTrack;