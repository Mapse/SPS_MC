// The following code deals with the content of overlapping datasets
// auto-detection of dataset from triggers occurring in it

// Events can occur on several datasets. Therefore set everything to true to start with.
// But if no trigger of a given data set fired, the event is definitely *not* from this dataset.
// If no trigger information available (2010 MC) all flags will remain true 
MCdataset = true;
ZeroBiasdataset = true;
MinimumBiasdataset = true;
Commissioningdataset = true;
Mudataset = true;
MuHaddataset = true;
DoubleMudataset = true;
MuEGdataset = true;
Electrondataset = true;
DoubleElectrondataset = true;
Photondataset = true;
MuOniadataset = true;
Charmoniumdataset = true;
MuMonitordataset = true;
EGMonitordataset = true;
Jetdataset = true;
MultiJetdataset = true;
JetMETTauMonitordataset = true;
BTaudataset = true;
BParkingdataset = true;
METFwddataset = true;
datasetisunique = false;
dataset = "unknown";
