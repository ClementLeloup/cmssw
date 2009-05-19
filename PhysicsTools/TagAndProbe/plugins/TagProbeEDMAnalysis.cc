// -*- C++ -*-
//
// Package:    TagProbeEDMAnalysis
// Class:      TagProbeEDMAnalysis
// 
/**\class TagProbeEDMAnalysis TagProbeEDMAnalysis.cc PhysicsTools/TagProbeEDMAnalysis/src/TagProbeEDMAnalysis.cc

 Description: <one line class summary>

 Implementation:
     <Notes on implementation>
*/
//
// Original Author:  "Nadia Adam"
//         Created:  Sun Apr 20 10:35:25 CDT 2008
//
// Kalanand Mishra: July 1, 2008 
// Added a configurable option "useRecoVarsForTruthMatchedCands" 
// (default == 'false') to use reconstructed or detector values 
// (instead of MC generated values) for var1 and var2 when doing MC truth efficiencies.
//
// Kalanand Mishra: October 7, 2008 
// Removed duplication of code in the fitting machinery. 
// Also, fixed the problem with RooDataSet declaration.
//
// Jason Haupt: October 9th, 2008 
// Added demoninator histograms
// UPdated binning on the SBS method
//

#include "PhysicsTools/TagAndProbe/interface/TagProbeEDMAnalysis.h"

#include "PhysicsTools/TagAndProbe/interface/EffTableLoader.h"

#include "PhysicsTools/TagAndProbe/interface/ZLineShape.hh"
#include "PhysicsTools/TagAndProbe/interface/CBLineShape.hh"
#include "PhysicsTools/TagAndProbe/interface/GaussianLineShape.hh"
#include "PhysicsTools/TagAndProbe/interface/PolynomialLineShape.hh"
#include "PhysicsTools/TagAndProbe/interface/CMSBkgLineShape.hh"

#include "FWCore/Framework/interface/MakerMacros.h"
#include "FWCore/Framework/interface/Frameworkfwd.h"
#include "FWCore/Framework/interface/Event.h"
#include "FWCore/ParameterSet/interface/ParameterSet.h"
#include "PhysicsTools/TagAndProbe/interface/RooCMSShapePdf.h"

// ROOT
#include <TROOT.h>
#include <TChain.h>
#include <TCanvas.h>
#include <TH1F.h>
#include <TH2F.h>
#include <TTree.h>
#include <TCanvas.h>
#include <TFile.h>
#include <TGraphAsymmErrors.h>
#include <TStyle.h>

// RooFit headers
#include <RooAbsData.h>
#include <RooDataSet.h>
#include <RooAddPdf.h>
#include <RooCategory.h>
#include <RooCatType.h>
#include <RooChi2Var.h>
#include <RooDataHist.h>
#include <RooFitResult.h>
#include <RooGenericPdf.h>
#include <RooGlobalFunc.h>
#include <RooLandau.h>
#include <RooMinuit.h>
#include <RooNLLVar.h>
#include <RooPlot.h>
#include <RooRealVar.h>
#include <RooSimultaneous.h>
#include <RooTreeData.h>

// system include files
#include <vector>
#include <string>

TagProbeEDMAnalysis::TagProbeEDMAnalysis(const edm::ParameterSet& iConfig): zLineShape_(NULL),
									    cbLineShape_(NULL),
									    gaussLineShape_(NULL),
									    polyBkgLineShape_(NULL),
									    cmsBkgLineShape_(NULL),
									    signalShapePdf_(NULL),
									    var1Pass_(NULL),
									    var1All_(NULL),
									    var2Pass_(NULL),
									    var2All_(NULL),
									    var1var2Pass_(NULL),
									    var1var2All_(NULL),
									    var1BiasPass_(NULL),
									    var1BiasAll_(NULL),
									    var2BiasPass_(NULL),
									    var2BiasAll_(NULL),
									    var1var2BiasPass_(NULL),
									    var1var2BiasAll_(NULL)
									    
{
   // TagProbeEDMAnalysis variables
   std::vector<string>       dEmptyStringVec;
   std::vector<unsigned int> dEmptyUIntVec;
   std::vector<double>       dEmptyDoubleVec;
   quantities_      = iConfig.getUntrackedParameter< std::vector<string> >("quantities",dEmptyStringVec); 
   conditions_      = iConfig.getUntrackedParameter< std::vector<string> >("conditions",dEmptyStringVec); 
   outputFileNames_ = iConfig.getUntrackedParameter< std::vector<string> >("outputFileNames",dEmptyStringVec);
   XBins_           = iConfig.getUntrackedParameter< std::vector<unsigned int> >("XBins",dEmptyUIntVec);
   XMin_            = iConfig.getUntrackedParameter< std::vector<double> >("XMin",dEmptyDoubleVec);
   XMax_            = iConfig.getUntrackedParameter< std::vector<double> >("XMax",dEmptyDoubleVec);
   logY_            = iConfig.getUntrackedParameter< std::vector<unsigned int> >("logY",dEmptyUIntVec);

   // Efficiency input variables
   tagProbeType_   = iConfig.getUntrackedParameter< int >("TagProbeType",0);

   calcEffsSB_     = iConfig.getUntrackedParameter< bool >("CalculateEffSideBand",false);
   calcEffsFitter_ = iConfig.getUntrackedParameter< bool >("CalculateEffFitter",false);
   calcEffsTruth_  = iConfig.getUntrackedParameter< bool >("CalculateEffTruth",false);
   
   truthParentId_  = iConfig.getUntrackedParameter< int >("MCTruthParentId",23);

   // Type of fit
   unbinnedFit_    = iConfig.getUntrackedParameter< bool >("UnbinnedFit",false);
   do2DFit_        = iConfig.getUntrackedParameter< bool >("Do2DFit",false);

   massNbins_      = iConfig.getUntrackedParameter< int >("NumBinsMass",20);
   massLow_        = iConfig.getUntrackedParameter< double >("MassLow",0.0);
   massHigh_       = iConfig.getUntrackedParameter< double >("MassHigh",100.0);

   inweight_         = iConfig.getUntrackedParameter< double >("Weight",1.0);

   // The efficiency binning variables (default pt and eta)
   std::vector< double > dBins;
   var1Name_       = iConfig.getUntrackedParameter< string >("NameVar1","pt");
   var1Nbins_      = iConfig.getUntrackedParameter< int >("NumBinsVar1",20);
   var1Low_        = iConfig.getUntrackedParameter< double >("Var1Low",0.0);
   var1High_       = iConfig.getUntrackedParameter< double >("Var1High",100.0);
   var1Bins_       = iConfig.getUntrackedParameter< std::vector<double> >("Var1BinBoundaries",dBins);

   var2Name_       = iConfig.getUntrackedParameter< string >("NameVar2","eta");
   var2Nbins_      = iConfig.getUntrackedParameter< int >("NumBinsVar2",20);
   var2Low_        = iConfig.getUntrackedParameter< double >("Var2Low",-2.4);
   var2High_       = iConfig.getUntrackedParameter< double >("Var2High",2.4);
   var2Bins_       = iConfig.getUntrackedParameter< std::vector<double> >("Var2BinBoundaries",dBins);

   // This gives the option to use bins from a file. 
   doTextDefinedBins_ = iConfig.getUntrackedParameter< bool >("DoBinsFromTxt",false);
   if (doTextDefinedBins_)
   {
      textBinsFile_      = iConfig.getUntrackedParameter< std::string >("EffBinsFile","EffBinsFile.txt");
      effBinsFromTxt_    = new EffTableLoader(textBinsFile_);
      var1Bins_.clear();
      for (int bin = 0; bin <= effBinsFromTxt_->size(); ++bin) 
        {
           var1Bins_.push_back(bin);
        }
   }


   // If want to use reconstructed or detector values (instead of MC generated values) 
   // of var1 and var2 when doing MC truth efficiencies (i.e., when "calcEffsTruth==true").
   useRecoVarsForTruthMatchedCands_ = 
     iConfig.getUntrackedParameter< bool >("useRecoVarsForTruthMatchedCands",true);


   // Check that the names of the variables are okay ...
   if( !( var1Name_ == "pt" || var1Name_ == "p"   || var1Name_ == "px" ||
	  var1Name_ == "py" || var1Name_ == "pz"  || var1Name_ == "e"  ||
	  var1Name_ == "et" || var1Name_ == "eta" || var1Name_ == "phi" || 
	  var1Name_ == "ptDet" || var1Name_ == "pDet"   || var1Name_ == "pxDet" ||
	  var1Name_ == "pyDet" || var1Name_ == "pzDet"  || var1Name_ == "eDet"  ||
	  var1Name_ == "etDet" || var1Name_ == "etaDet" || var1Name_ == "phiDet" ||
	  var1Name_ == "jetDeltaR" || var1Name_ == "totJets" ) )
   {
      edm::LogWarning("TagAndProbe") << "Warning: Var1 name invalid, setting var1 name to pt!";
      var1Name_ = "pt";
   }

   if( !( var2Name_ == "pt" || var2Name_ == "p"   || var2Name_ == "px" ||
	  var2Name_ == "py" || var2Name_ == "pz"  || var2Name_ == "e"  ||
	  var2Name_ == "et" || var2Name_ == "eta" || var2Name_ == "phi" ||
	  var2Name_ == "ptDet" || var2Name_ == "pDet"   || var2Name_ == "pxDet" ||
	  var2Name_ == "pyDet" || var2Name_ == "pzDet"  || var2Name_ == "eDet"  ||
	  var2Name_ == "etDet" || var2Name_ == "etaDet" || var2Name_ == "phiDet" ||
	  var2Name_ == "jetDeltaR" || var2Name_ == "totJets" ) )
   {
      edm::LogWarning("TagAndProbe") << "Warning: Var2 name invalid, setting var2 name to eta!";
      var2Name_ = "eta";
   }

   // Make the uppercase names ...
   var1NameUp_ = var1Name_;
   var1NameUp_.at(0) -= 32;

   var2NameUp_ = var2Name_;
   var2NameUp_.at(0) -= 32;

   // Set up the bins for the eff histograms ...
   
   if( var1Bins_.size() == 0 ) 
   {
      // User didn't set bin boundaries, so use even binning
      double bwidth = (var1High_-var1Low_)/(double)var1Nbins_;
      for( int i=0; i<=var1Nbins_; ++i )
      {
	 double low_edge = var1Low_+(double)i*bwidth;
	 var1Bins_.push_back(low_edge);
      }
   }
   if( var2Bins_.size() == 0 ) 
   {
      // User didn't set bin boundaries, so use even binning
      double bwidth = (var2High_-var2Low_)/(double)var2Nbins_;
      for( int i=0; i<=var2Nbins_; ++i )
      {
	 double low_edge = var2Low_+(double)i*bwidth;
	 var2Bins_.push_back(low_edge);
      }
   }

   // SBS
   SBSPeak_     = iConfig.getUntrackedParameter< double >("SBSPeak",90);
   SBSStanDev_  = iConfig.getUntrackedParameter< double >("SBSStanDev",2);

   

   // Fitter variables
   rooMass_ = new RooRealVar("Mass","Invariant Di-Lepton Mass", massLow_, massHigh_, "GeV/c^{2}");
   rooMass_->setBins(massNbins_);
   
   edm::ParameterSet dZLineShape;
   edm::ParameterSet ZLineShapePSet = iConfig.getUntrackedParameter< edm::ParameterSet >("ZLineShape",dZLineShape);
   if (!ZLineShapePSet.empty()) {
     zLineShape_ = new ZLineShape(ZLineShapePSet, rooMass_);
   }

   edm::ParameterSet dCBLineShape;
   edm::ParameterSet CBLineShapePSet = iConfig.getUntrackedParameter< edm::ParameterSet >("CBLineShape",dCBLineShape);
   if (!CBLineShapePSet.empty()) {
     cbLineShape_ = new CBLineShape(CBLineShapePSet, rooMass_);
   }

   edm::ParameterSet dGaussLineShape;
   edm::ParameterSet GaussLineShapePSet = iConfig.getUntrackedParameter< edm::ParameterSet >("GaussLineShape",dGaussLineShape);
   if (!GaussLineShapePSet.empty()) {
     gaussLineShape_ = new GaussianLineShape(GaussLineShapePSet, rooMass_);
   }

   edm::ParameterSet dPolyLineShape;
   edm::ParameterSet PolyLineShapePSet = iConfig.getUntrackedParameter< edm::ParameterSet >("PolynomialLineShape",dPolyLineShape);
   if (!PolyLineShapePSet.empty()) {
     polyBkgLineShape_ = new PolynomialLineShape(PolyLineShapePSet, rooMass_);
   }

   edm::ParameterSet dCMSBkgLineShape;
   edm::ParameterSet CMSBkgLineShapePSet = iConfig.getUntrackedParameter< edm::ParameterSet >("CMSBkgLineShape",dCMSBkgLineShape);
   if (!CMSBkgLineShapePSet.empty()) {
     cmsBkgLineShape_ = new CMSBkgLineShape(CMSBkgLineShapePSet, rooMass_);
   }

   std::vector<double> dEff;
   dEff.push_back(0.98);
   dEff.push_back(0.0);
   dEff.push_back(1.1);
   efficiency_      = iConfig.getUntrackedParameter< std::vector<double> >("Efficiency",dEff);
   std::vector<double> dNSig;
   dNSig.push_back(1000.0);
   dNSig.push_back(-10.0);
   dNSig.push_back(1000000.0);
   numSignal_       = iConfig.getUntrackedParameter< std::vector<double> >("NumSignal",dNSig);
   std::vector<double> dNBPs;
   dNBPs.push_back(1000.0);
   dNBPs.push_back(-10.0);
   dNBPs.push_back(1000000.0);
   numBkgPass_      = iConfig.getUntrackedParameter< std::vector<double> >("NumBkgPass",dNBPs);
   std::vector<double> dNBFl;
   dNBFl.push_back(1000.0);
   dNBFl.push_back(-10.0);
   dNBFl.push_back(1000000.0);
   numBkgFail_      = iConfig.getUntrackedParameter< std::vector<double> >("NumBkgFail",dNBFl);

   // Get the mode of operation variables
   mode_        = iConfig.getUntrackedParameter< string >("Mode","Normal");
   fitFileName_ = iConfig.getUntrackedParameter< string >("FitFileName","fitfile.root");

   std::vector< std::string > dReadFiles;
   readFiles_   = iConfig.getUntrackedParameter< std::vector<string> >("ReadFromFiles",dReadFiles);
   
   // Allocate space for the simple plot histograms
   numQuantities_ = quantities_.size();
   Histograms_ = new TH1F[numQuantities_];

   for(unsigned int i = 0; i < numQuantities_; i++)
   {
      Histograms_[i] =  TH1F(("h"+quantities_[i]).c_str(), "", 
			     XBins_[i], XMin_[i], XMax_[i]);
   }

   // Verify correct use of input variables for making simple plots 
   doAnalyze_ = true;
   if( numQuantities_ == 0 ) doAnalyze_ = false;
   else if(outputFileNames_.size() != numQuantities_){
      doAnalyze_ = false;
      edm::LogInfo("TagProbeEDMAnalysis") << "outputFileNames is not the same size as quantities";    
   }else if(conditions_.size() != numQuantities_){
      doAnalyze_ = false;
      edm::LogInfo("TagProbeEDMAnalysis") << "conditions is not the same size as quantities";    
   }else if(XBins_.size() != numQuantities_){
      doAnalyze_ = false;
      edm::LogInfo("TagProbeEDMAnalysis") << "XBins is not the same size as quantities";    
   }else if(XMax_.size() != numQuantities_){
      doAnalyze_ = false;
      edm::LogInfo("TagProbeEDMAnalysis") << "XMax is not the same size as quantities";    
   }else if(XMin_.size() != numQuantities_){
      doAnalyze_ = false;
      edm::LogInfo("TagProbeEDMAnalysis") << "XMin is not the same size as quantities";    
   }else if(logY_.size() != numQuantities_){
      doAnalyze_ = false;
      edm::LogInfo("TagProbeEDMAnalysis") << "logY is not the same size as quantities";    
   }   

   // Make the simple fit tree
   if( mode_ != "Read" )
   {
      string fmode = "RECREATE";
      outRootFile_ = new TFile(fitFileName_.c_str(),fmode.c_str());
      outRootFile_->cd();

      fitTree_ = new TTree("fitter_tree","Tree For Fitting",1);
      fitTree_->Branch("ProbePass",         &ProbePass_,"ProbePass/I");
      fitTree_->Branch("Mass",              &Mass_,     "Mass/D");
      fitTree_->Branch("Weight",            &Weight_,   "Weight/D");
      fitTree_->Branch(var1NameUp_.c_str(), &Var1_,     
		       (var1NameUp_+"/D").c_str());
      fitTree_->Branch(var2NameUp_.c_str(), &Var2_,     
		       (var2NameUp_+"/D").c_str());   
   }

   InitializeMCHistograms();
}


void TagProbeEDMAnalysis::InitializeMCHistograms(){

   // MC Truth Histograms
   var1Pass_ = new TH1F("hvar1pass","Var1 Pass",
			var1Bins_.size()-1,&var1Bins_[0]);
   var1All_  = new TH1F("hvar1all","Var1 All",
			var1Bins_.size()-1,&var1Bins_[0]);
   
   var2Pass_ = new TH1F("hvar2pass","Var2 Pass",
			var2Bins_.size()-1,&var2Bins_[0]);
   var2All_  = new TH1F("hvar2all","Var2 All",
			var2Bins_.size()-1,&var2Bins_[0]);
   
   var1var2Pass_ = new TH2F("hvar1var2pass","Var1:Var2 Pass",
			    var1Bins_.size()-1,&var1Bins_[0],
			    var2Bins_.size()-1,&var2Bins_[0]);
   var1var2All_  = new TH2F("hvar1var2all","Var1:Var2 All",
			    var1Bins_.size()-1,&var1Bins_[0],
			    var2Bins_.size()-1,&var2Bins_[0]);

   // MC Truth Tag-Probe Biased Histograms
   var1BiasPass_ = new TH1F("hvar1biaspass","Var1 Pass",
                             var1Bins_.size()-1,&var1Bins_[0]);
   var1BiasAll_  = new TH1F("hvar1biasall","Var1 All",
		             var1Bins_.size()-1,&var1Bins_[0]);
   
   var2BiasPass_ = new TH1F("hvar2biaspass","Var2 Pass",
			     var2Bins_.size()-1,&var2Bins_[0]);
   var2BiasAll_  = new TH1F("hvar2biasall","Var2 All",
			     var2Bins_.size()-1,&var2Bins_[0]);
   
   var1var2BiasPass_ = new TH2F("hvar1var2biaspass","Var1:Var2 Pass",
			         var1Bins_.size()-1,&var1Bins_[0],
			         var2Bins_.size()-1,&var2Bins_[0]);
   var1var2BiasAll_  = new TH2F("hvar1var2biasall","Var1:Var2 All",
			         var1Bins_.size()-1,&var1Bins_[0],
			         var2Bins_.size()-1,&var2Bins_[0]);

}

void TagProbeEDMAnalysis::CleanUpMCHistograms() {

  if (var1Pass_  ) delete var1Pass_;
  if (var1All_   ) delete var1All_;
  if( var2Pass_ ) delete var2Pass_;
  if( var2All_  ) delete var2All_;
  
  if( var1BiasPass_  ) delete var1BiasPass_;
  if( var1BiasAll_   ) delete var1BiasAll_;
  if( var2BiasPass_ ) delete var2BiasPass_;
  if( var2BiasAll_  ) delete var2BiasAll_;

}

void TagProbeEDMAnalysis::ReadMCHistograms(){

      // Now read in the MC truth histograms and add the results
      for( int iFile=0; iFile<(int)readFiles_.size(); ++iFile )
      {
	 TFile inputFile(readFiles_[iFile].c_str());

	 var1Pass_->Add( (TH1F*)inputFile.Get("hvar1pass") );
	 var1All_->Add( (TH1F*)inputFile.Get("hvar1all") );  
	 
	 var2Pass_->Add( (TH1F*)inputFile.Get("hvar2pass") );
	 var2All_->Add( (TH1F*)inputFile.Get("hvar2all") );  
	 
	 var1var2Pass_->Add( (TH2F*)inputFile.Get("hvar1var2pass") );
	 var1var2All_->Add( (TH2F*)inputFile.Get("hvar1var2all") );

	 var1BiasPass_->Add( (TH1F*)inputFile.Get("hvar1biaspass") );
	 var1BiasAll_->Add( (TH1F*)inputFile.Get("hvar1biasall") );  

	 var2BiasPass_->Add( (TH1F*)inputFile.Get("hvar2biaspass") );
	 var2BiasAll_->Add( (TH1F*)inputFile.Get("hvar2biasall") );  
	 
	 var1var2BiasPass_->Add( (TH2F*)inputFile.Get("hvar1var2biaspass") );
	 var1var2BiasAll_->Add( (TH2F*)inputFile.Get("hvar1var2biasall") );

      }
}


void TagProbeEDMAnalysis::WriteMCHistograms() {
  var1Pass_->Write();
  var1All_->Write();  
  
  var2Pass_->Write();
  var2All_->Write();  
  
  var1var2Pass_->Write();
  var1var2All_->Write();
  
  var1BiasPass_->Write();
  var1BiasAll_->Write();  
  
  var2BiasPass_->Write();
  var2BiasAll_->Write();  
  
  var1var2BiasPass_->Write();
  var1var2BiasAll_->Write();
}

TagProbeEDMAnalysis::~TagProbeEDMAnalysis()
{
   // Clean up

  CleanUpMCHistograms();
  
   if (outRootFile_) 
   {
      outRootFile_->Close();
      if( mode_ != "Read" ) delete outRootFile_;
   }
  
   if(Histograms_)
   {
      delete [] Histograms_; 
      Histograms_ = 0;
   }

   cleanFitVariables();
}

void TagProbeEDMAnalysis::FillFitTree(const edm::Event& iEvent){

      edm::Handle< std::vector<int> > tp_type;
      if ( !iEvent.getByLabel("TPEdm","TPtype",tp_type) ) {
	edm::LogInfo("TagProbeEDMAnalysis") << "No TPtype in Tree!"; 
      }

      edm::Handle< std::vector<int> > tp_true;
      if ( !iEvent.getByLabel("TPEdm","TPtrue",tp_true) ) {
         edm::LogInfo("TagProbeEDMAnalysis") << "No TPtrue in Tree!"; 
      }
      
      edm::Handle< std::vector<int> > tp_ppass;
      if ( !iEvent.getByLabel("TPEdm","TPppass",tp_ppass) ) {
         edm::LogInfo("TagProbeEDMAnalysis") << "No TPppass in Tree!"; 
      }
      
      edm::Handle< std::vector<float> > tp_mass;
      if ( !iEvent.getByLabel("TPEdm","TPmass",tp_mass) ) {
         edm::LogInfo("TagProbeEDMAnalysis") << "No TPmass in Tree!"; 
      }

      edm::Handle< std::vector<float> > tp_probe_var1;
      if ( !iEvent.getByLabel("TPEdm",("TPProbe"+var1Name_).c_str(),tp_probe_var1) ) {
         edm::LogInfo("TagProbeEDMAnalysis") << "No TPProbe"+var1Name_+" in Tree!"; 
      }
      
      edm::Handle< std::vector<float> > tp_probe_var2;
      if ( !iEvent.getByLabel("TPEdm",("TPProbe"+var2Name_).c_str(),tp_probe_var2) ) {
         edm::LogInfo("TagProbeEDMAnalysis") << "No TPProbe"+var2Name_+" in Tree!"; 
      }
      
      outRootFile_->cd();

      if( tp_type.isValid() ) {
	int nrTP = tp_type->size();
	for( int i=0; i<nrTP; ++i ) {
	  if( (*tp_type)[i] != tagProbeType_ ) continue;
	 
	  Weight_ = inweight_;
	  ProbePass_ = (*tp_ppass)[i];
	  Mass_ = (double)(*tp_mass)[i];
	  Var1_ = (double)(*tp_probe_var1)[i];
	  Var2_ = (double)(*tp_probe_var2)[i];	 
	  fitTree_->Fill();
	}
      }      
}


// ------------ method called to for each event  ------------
void TagProbeEDMAnalysis::analyze(const edm::Event& iEvent, 
				  const edm::EventSetup& iSetup)
{

   // Safety check .. if mode = read, the user should use an empty source.
   if( mode_ == "Read" ) return;

   // Fill the fit tree if we are fitting or doing SB subtraction
   if( calcEffsSB_ || calcEffsFitter_ || mode_ == "Write" )
   {
      // Get the TP variables to fill the fitter tree ...
     FillFitTree(iEvent);
   }

   // Fill the MC truth information if required
   if( calcEffsTruth_ || mode_ == "Write" )
   {
      // Get the Cnd variable for the MC truth ...
      edm::Handle< std::vector<int> > cnd_type;
      if ( !iEvent.getByLabel("TPEdm","Cndtype",cnd_type) ) {
         edm::LogInfo("TagProbeEDMAnalysis") << "No Cndtype in Tree!"; 
      }

      edm::Handle< std::vector<int> > cnd_tag;
      if ( !iEvent.getByLabel("TPEdm","Cndtag",cnd_tag) ) {
         edm::LogInfo("TagProbeEDMAnalysis") << "No Cndtag in Tree!"; 
      }

      edm::Handle< std::vector<int> > cnd_aprobe;
      if ( !iEvent.getByLabel("TPEdm","Cndaprobe",cnd_aprobe) ) {
         edm::LogInfo("TagProbeEDMAnalysis") << "No Cndaprobe in Tree!"; 
      }

      edm::Handle< std::vector<int> > cnd_pprobe;
      if ( !iEvent.getByLabel("TPEdm","Cndpprobe",cnd_pprobe) ) {
         edm::LogInfo("TagProbeEDMAnalysis") << "No Cndpprobe in Tree!"; 
      }

      edm::Handle< std::vector<int> > cnd_moid;
      if ( !iEvent.getByLabel("TPEdm","Cndmoid",cnd_moid) ) {
         edm::LogInfo("TagProbeEDMAnalysis") << "No Cndmoid in Tree!"; 
      }

      edm::Handle< std::vector<int> > cnd_gmid;
      if ( !iEvent.getByLabel("TPEdm","Cndgmid",cnd_gmid) ) {
         edm::LogInfo("TagProbeEDMAnalysis") << "No Cndgmid in Tree!"; 
      }

      std::string truthVar1 = var1Name_.c_str();
      std::string truthVar2 = var2Name_.c_str();

      if(!useRecoVarsForTruthMatchedCands_) {
	truthVar1.insert(0, "Cnd");
	truthVar2.insert(0, "Cnd");
      }
      else
      {
	truthVar1.insert(0, "Cndr");
	truthVar2.insert(0, "Cndr");
      }

      edm::Handle< std::vector<float> > cnd_var1;
      if ( !iEvent.getByLabel("TPEdm",truthVar1.c_str(),cnd_var1) ) {
         edm::LogInfo("TagProbeEDMAnalysis") << "No Cnd"+var1Name_+" in Tree!"; 
      }

      edm::Handle< std::vector<float> > cnd_var2;
      if ( !iEvent.getByLabel("TPEdm",truthVar2.c_str(),cnd_var2) ) {
         edm::LogInfo("TagProbeEDMAnalysis") << "No Cnd"+var2Name_+" in Tree!"; 
      }

      edm::Handle< std::vector<float> > cnd_rpx;
      if ( !iEvent.getByLabel("TPEdm","Cndrpx",cnd_rpx) ) {
         edm::LogInfo("TagProbeEDMAnalysis") << "No Cndrpx in Tree!"; 
      }

      edm::Handle< std::vector<float> > cnd_rpy;
      if ( !iEvent.getByLabel("TPEdm","Cndrpy",cnd_rpy) ) {
         edm::LogInfo("TagProbeEDMAnalysis") << "No Cndrpy in Tree!"; 
      }

      edm::Handle< std::vector<float> > cnd_rpz;
      if ( !iEvent.getByLabel("TPEdm","Cndrpz",cnd_rpz) ) {
         edm::LogInfo("TagProbeEDMAnalysis") << "No Cndrpz in Tree!"; 
      }

      edm::Handle< std::vector<float> > cnd_re;
      if ( !iEvent.getByLabel("TPEdm","Cndre",cnd_re) ) {
         edm::LogInfo("TagProbeEDMAnalysis") << "No Cndre in Tree!"; 
      }

      if( cnd_type.isValid() )
      {
	 int nTag = 0;
	 int nMatch = 0;
	 float px = 0;
	 float py = 0;
	 float pz = 0;
	 float e = 0;
	 unsigned int nCnd = cnd_type->size();
	 for( unsigned int iCnd=0; iCnd<nCnd; ++iCnd )
	 {
	    if( (*cnd_type)[iCnd] != tagProbeType_ ) continue;
	    if( truthParentId_ != 0 && 
	    !( fabs((*cnd_gmid)[iCnd]) == truthParentId_ || fabs((*cnd_moid)[iCnd]) == truthParentId_ ) ) continue;

	    if( (*cnd_tag)[iCnd] == 1 ) ++nTag;
	    if( (*cnd_tag)[iCnd] == 1 || (*cnd_aprobe)[iCnd] == 1 )
            {
	       ++nMatch;
	       px += (*cnd_rpx)[iCnd];
	       py += (*cnd_rpy)[iCnd];
	       pz += (*cnd_rpz)[iCnd];
	       e += (*cnd_re)[iCnd];
	    }
	 }
	 if( nTag >= 1 && nMatch == 2 )
	 {
	    float invMass = sqrt(e*e - px*px - py*py - pz*pz);
	    if( invMass > massLow_ && invMass < massHigh_ )
	    {
	       for( unsigned int iCnd=0; iCnd<nCnd; ++iCnd )
	       {
		  if( (*cnd_type)[iCnd] != tagProbeType_ ) continue;
	    
		  if( truthParentId_ != 0 && 
		  !( fabs((*cnd_gmid)[iCnd]) == truthParentId_ || fabs((*cnd_moid)[iCnd]) == truthParentId_ ) ) continue;
	    
		  // If there is only one tag, only count the probe
		  bool FillBiasHists = true;
		  if( nTag==1 && (*cnd_tag)[iCnd]==1 ) FillBiasHists = false;

		  bool inVar1Range = false;
		  if( (*cnd_var1)[iCnd] > var1Bins_[0] &&
		  (*cnd_var1)[iCnd] < var1Bins_[var1Bins_.size()-1] )
		     inVar1Range = true;
		  bool inVar2Range = false;
		  if( (*cnd_var2)[iCnd] > var2Bins_[0] &&
		  (*cnd_var2)[iCnd] < var2Bins_[var2Bins_.size()-1] )
		     inVar2Range = true;

		  if( (*cnd_aprobe)[iCnd] == 1 && (*cnd_pprobe)[iCnd] == 1 )
		  {
		     if( inVar2Range ) var1Pass_->Fill((*cnd_var1)[iCnd]);
		     if( inVar1Range ) var2Pass_->Fill((*cnd_var2)[iCnd]);
		     var1var2Pass_->Fill((*cnd_var1)[iCnd],(*cnd_var2)[iCnd]);

		     if( FillBiasHists )
		     {
			if( inVar2Range ) var1BiasPass_->Fill((*cnd_var1)[iCnd]);
			if( inVar1Range ) var2BiasPass_->Fill((*cnd_var2)[iCnd]);
			var1var2BiasPass_->Fill((*cnd_var1)[iCnd],(*cnd_var2)[iCnd]);
		     }
		  }
		  if( (*cnd_aprobe)[iCnd] == 1 )
		  {
		     if( inVar2Range ) var1All_->Fill((*cnd_var1)[iCnd]);
		     if( inVar1Range ) var2All_->Fill((*cnd_var2)[iCnd]);
		     var1var2All_->Fill((*cnd_var1)[iCnd],(*cnd_var2)[iCnd]);

		     if( FillBiasHists )
		     {
			if( inVar2Range ) var1BiasAll_->Fill((*cnd_var1)[iCnd]);
			if( inVar1Range ) var2BiasAll_->Fill((*cnd_var2)[iCnd]);
			var1var2BiasAll_->Fill((*cnd_var1)[iCnd],(*cnd_var2)[iCnd]);
		     }
		  }
	       }
	    }
	 }
      }
   }

   // Do plot making etc ...
   if( doAnalyze_ )
   {
      for(unsigned int iQuant = 0; iQuant < numQuantities_; ++iQuant)
      {
	 // int
	 if( quantities_[iQuant].find("n") == 0 || 
	     quantities_[iQuant] == "Run"       || 
	     quantities_[iQuant] == "Event" )
	 {
	    edm::Handle< int > h;
	    if ( !iEvent.getByLabel("TPEdm",quantities_[iQuant].c_str(),h) ) { 
	       edm::LogInfo("TagProbeEDMAnalysis") << "No "+quantities_[iQuant]+" in Tree!"; 
            }
	    if( h.isValid() ) Histograms_[iQuant].Fill(*h);
	 }
	 // std::vector<int>
	 else if( quantities_[iQuant].find("type") != string::npos ||
		  quantities_[iQuant].find("true") != string::npos || 
		  quantities_[iQuant].find("ppass") != string::npos || 
		  quantities_[iQuant].find("l1") != string::npos || 
		  quantities_[iQuant].find("hlt") != string::npos || 
		  quantities_[iQuant].find("tag") != string::npos || 
		  quantities_[iQuant].find("aprobe") != string::npos || 
		  quantities_[iQuant].find("pprobe") != string::npos || 
		  quantities_[iQuant].find("moid") != string::npos || 
		  quantities_[iQuant].find("gmid") != string::npos || 
		  quantities_[iQuant].find("pid") != string::npos || 
		  quantities_[iQuant].find("bc") != string::npos )
	 {
	    edm::Handle< std::vector<int> > h;
	    if ( !iEvent.getByLabel("TPEdm",quantities_[iQuant].c_str(),h) ) {
	        edm::LogInfo("TagProbeEDMAnalysis") << "No "+quantities_[iQuant]+" in Tree!"; 
            }
	    if( h.isValid() ) 
	      for( int n=0; n<(int)h->size(); ++n ) 
		Histograms_[iQuant].Fill((*h)[n]);
	 }
	 // std::vector< float >
	 else
	 {
	    edm::Handle< std::vector<float> > h;
	    if ( !iEvent.getByLabel("TPEdm",quantities_[iQuant].c_str(),h) ) {
	        edm::LogInfo("TagProbeEDMAnalysis") << "No "+quantities_[iQuant]+" in Tree!"; 
            }
	    if( h.isValid() ) 
	      for( int n=0; n<(int)h->size(); ++n ) 
		Histograms_[iQuant].Fill((*h)[n]);
	 }
      }
   }
}


// ********* Save the user requested histograms ******** //
int TagProbeEDMAnalysis::SaveHistogram(TH1F& Histo, std::string outFileName, Int_t LogY = 0)
{
  
   TCanvas* c1 = new TCanvas("c1","c1",700,500);
   c1->GetPad(0)->SetTicks(1,1);
   c1->SetLogy(LogY);
  
   Histo.Draw();
  
   c1->SaveAs(outFileName.c_str());
  
   if(c1) delete c1;

   return 0;
}
// ***************************************************** //

// ****************** TP Eff Side band subtraction *************
void TagProbeEDMAnalysis::TPEffSBS( string &fileName, string &bvar, 
				     std::vector< double > bins, string &bvar2, 
				     double bvar2Lo, double bvar2Hi )
{

   outRootFile_->cd();
   fitTree_->SetDirectory(outRootFile_);
  
   //return;
   edm::LogInfo("TagProbeEDMAnalysis") << "***** Here in TP sideband subtraction ******";
   edm::LogInfo("TagProbeEDMAnalysis") << "Number of entries " << fitTree_->GetEntries();
   
   // Here I will just change the names if we are using 1D regions by reading 
   // in the efficiencies from a file.
 
   string bvard = bvar;
   if ( doTextDefinedBins_ ) bvard = bvar + "_" + bvar2;

   string hname = "sbs_eff_" + bvard;
   string htitle = "SBS Efficiency vs " + bvard;

   string hdname = "sbs_den_" + bvard; 
   string hdtitle = "SBS Denominator vs " + bvard; 


   stringstream condition;
   stringstream histoName;
   stringstream histoTitle;;

   int bnbins = bins.size()-1;
   edm::LogInfo("TagProbeEDMAnalysis") << "There are " << bnbins << " bins ";
   TH1F effhist(hname.c_str(),htitle.c_str(),bnbins,&bins[0]);
   TH1F denhist(hdname.c_str(),hdtitle.c_str(),bnbins,&bins[0]);
   for( int bin=0; bin<bnbins; ++bin ) edm::LogInfo("TagProbeEDMAnalysis") << "Bin low edge " << 
					 effhist.GetBinLowEdge(bin+1);

   TH1F* PassProbes(0);
   TH1F* FailProbes(0);

   TH1F* SBSPassProbes(0);
   TH1F* SBSFailProbes(0);

   const int XBinsSBS = massNbins_;
   double XMinSBS = massLow_;
   double XMaxSBS = massHigh_;


   if( XMinSBS > ( SBSPeak_ - 13*SBSStanDev_ ) )
     XMinSBS = SBSPeak_ - 13*SBSStanDev_;

   if( XMaxSBS < ( SBSPeak_ + 13*SBSStanDev_ ) )
     XMaxSBS = SBSPeak_ + 13*SBSStanDev_;

   for( int bin=0; bin<bnbins; ++bin )
   {
 
      // The binning variable
      string bunits = "";
      double lowEdge = bins[bin];
      double highEdge = bins[bin+1];
      if( bvar == "Pt" ) bunits = "GeV";
      if ( doTextDefinedBins_ ) 
      {
         bunits = "";
         std::vector<std::pair<float, float> > bininfo = effBinsFromTxt_->GetCellInfo(bin);
         lowEdge  = bininfo[0].first;
         highEdge = bininfo[0].second;
         bvar2Lo  = bininfo[1].first;
         bvar2Hi  = bininfo[1].second;
         edm::LogInfo("TagProbeEDMAnalysis") << " Bin " << bin << ", lowEdge " << lowEdge << ", highEdge " << highEdge << ", bvar2Lo " << bvar2Lo << ", bvar2Hi " << bvar2Hi << std::endl;
      }
///////////// HERE I NEED TO CHANGE THESE VALUES
      // Print out the pass/fail condition
      stringstream DisplayCondition;
      DisplayCondition.str(std::string());
      DisplayCondition << "(ProbePass==1(0) && " << bvar << ">" <<  
	lowEdge << " && " << bvar << "<" << highEdge << " && " << 
	bvar2 << ">" << bvar2Lo << " && " << bvar2 << "<" << 
	bvar2Hi <<")*Weight";
      edm::LogInfo("TagProbeEDMAnalysis") << "Pass(Fail) condition[" << bvar<< "]: " << 
	DisplayCondition.str() << std::endl;

      // Passing Probes
      condition.str(std::string());
      condition  << "(ProbePass==1 && " << bvar << ">" <<  lowEdge << 
	" && " << bvar << "<" << highEdge << " && " << bvar2 << ">" << 
	bvar2Lo << " && " << bvar2 << "<" << bvar2Hi <<")*Weight";
      histoName.str(std::string());
      histoName << "sbs_pass_" << bvard << "_" << bin;
      histoTitle.str(std::string());
      histoTitle << "Passing Probes - " << lowEdge << "<" << bvar << 
	"<" << highEdge;
      PassProbes = new TH1F(histoName.str().c_str(), 
			    histoTitle.str().c_str(), XBinsSBS, 
			    XMinSBS, XMaxSBS); 
      PassProbes->Sumw2();
      PassProbes->SetDirectory(outRootFile_);
      fitTree_->Draw(("Mass >> " + histoName.str()).c_str(), 
		     condition.str().c_str() );

      // Failing Probes
      condition.str(std::string());
      condition  << "(ProbePass==0 && " << bvar << ">" <<  lowEdge << 
	" && " << bvar << "<" << highEdge << " && " << bvar2 << ">" << 
	bvar2Lo << " && " << bvar2 << "<" << bvar2Hi <<")*Weight";
      histoName.str(std::string());
      histoName << "sbs_fail_" <<  bvard << "_" << bin;
      histoTitle.str(std::string());
      histoTitle << "Failing Probes - " << lowEdge << "<" << bvar << 
	"<" << highEdge;
      FailProbes = new TH1F(histoName.str().c_str(), 
			    histoTitle.str().c_str(), 
			    XBinsSBS, XMinSBS, XMaxSBS); 
      FailProbes->Sumw2();
      FailProbes->SetDirectory(outRootFile_);
      fitTree_->Draw(("Mass >> " + histoName.str()).c_str(), 
		     condition.str().c_str());

      // SBS Passing  Probes
      histoName.str(std::string());
      histoName << "sbs_pass_subtracted_" << bvard << "_" << bin;
      histoTitle.str(std::string());
      histoTitle << "Passing Probes SBS - "  << lowEdge << "<" << 
	bvar << "<" << highEdge;
      SBSPassProbes = new TH1F(histoName.str().c_str(), 
			       histoTitle.str().c_str(), 
			       XBinsSBS, XMinSBS, XMaxSBS); 
      SBSPassProbes->Sumw2();

      // SBS Failing Probes
      histoName.str(std::string());
      histoName << "sbs_fail_subtracted_" << bvard << "_" << bin; 
      histoTitle.str(std::string());
      histoTitle << "Failing Probes SBS - "  << lowEdge << "<" << 
	bvar << "<" << highEdge;
      SBSFailProbes = new TH1F(histoName.str().c_str(), 
			       histoTitle.str().c_str(), 
			       XBinsSBS, XMinSBS, XMaxSBS); 
      SBSFailProbes->Sumw2();

      // Perform side band subtraction
      SideBandSubtraction(*PassProbes, *SBSPassProbes, SBSPeak_, SBSStanDev_);
      SideBandSubtraction(*FailProbes, *SBSFailProbes, SBSPeak_, SBSStanDev_);

      // Count the number of passing and failing probes in the region
      double npassR = SBSPassProbes->Integral("");
      double nfailR = SBSFailProbes->Integral("");

      if((npassR + nfailR) != 0){
	Double_t eff = npassR/(npassR + nfailR);
	Double_t effErr = sqrt(npassR * nfailR / 
			       (npassR + nfailR))/(npassR + nfailR);
	
	edm::LogInfo("TagProbeEDMAnalysis") << "Num pass " << npassR << ",  Num fail " << 
	  nfailR << ". Eff " << eff << " +/- " << effErr;
	
	// Fill the efficiency hist
	effhist.SetBinContent(bin+1,eff);
	effhist.SetBinError(bin+1,effErr);

	//Fill the denominator hist
	denhist.SetBinContent(bin+1,npassR+nfailR);
	denhist.SetBinError(bin+1,pow(npassR+nfailR,0.5));

      }else {
	edm::LogInfo("TagProbeEDMAnalysis") << " no probes ";
      }

      // ********** Make and save Canvas for the plots ********** //
      outRootFile_->cd();

      PassProbes->Write();
      FailProbes->Write();
      SBSPassProbes->Write();
      SBSFailProbes->Write();
   }
   
   outRootFile_->cd();
   effhist.Write();
   denhist.Write();

   if(PassProbes)    delete PassProbes;
   if(FailProbes)    delete FailProbes;
   if(SBSPassProbes) delete SBSPassProbes;
   if(SBSFailProbes) delete SBSFailProbes;

   return;

}





// ****************** TP Eff Side band subtraction *************
void TagProbeEDMAnalysis::TPEffSBS2D( string &fileName, string &bvar1, 
				       std::vector< double > bins1,
				       string &bvar2, std::vector< double > bins2 )
{

  outRootFile_->cd();
  fitTree_->SetDirectory(outRootFile_);
  
  //return;
  edm::LogInfo("TagProbeEDMAnalysis") << "***** Here in TP sideband subtraction 2D ******";
  edm::LogInfo("TagProbeEDMAnalysis") << "Number of entries " << fitTree_->GetEntries();
   
  string hname = "sbs_eff_" + bvar1 + "_" + bvar2;
  string htitle = "SBS Efficiency: " + bvar1 + " vs " + bvar2;
  
  string hdname = "sbs_den_" + bvar1 + "_" + bvar2; 
  string hdtitle = "SBS Denominator vs " + bvar1 + " vs " + bvar2; 
  
  stringstream condition;
  stringstream histoName;
  stringstream histoTitle;;
  
   int bnbins1 = bins1.size()-1;
   int bnbins2 = bins2.size()-1;
   edm::LogInfo("TagProbeEDMAnalysis") << "There are " << bnbins1 << " bins for var1 ";

   TH2F effhist(hname.c_str(),htitle.c_str(),bnbins1,&bins1[0],
		bnbins2,&bins2[0]);
   TH2F denhist(hdname.c_str(),hdtitle.c_str(),bnbins1,&bins1[0],
		bnbins2,&bins2[0]);

   TH1F* PassProbes(0);
   TH1F* FailProbes(0);

   TH1F* SBSPassProbes(0);
   TH1F* SBSFailProbes(0);

   const int XBinsSBS = massNbins_;
   const double XMinSBS = massLow_;
   const double XMaxSBS = massHigh_;

   for( int bin1=0; bin1<bnbins1; ++bin1 )
   {
      double lowEdge1 = bins1[bin1];
      double highEdge1 = bins1[bin1+1];

      for( int bin2=0; bin2<bnbins2; ++bin2 )
      {
	 // The binning variables
	 double lowEdge2 = bins2[bin2];
	 double highEdge2 = bins2[bin2+1];

	 // Print out the pass/fail condition
	 stringstream DisplayCondition;
	 DisplayCondition.str(std::string());
	 DisplayCondition << "(ProbePass==1(0) && " << bvar1 << 
	   ">" <<  lowEdge1 << " && " << bvar1 << "<" << 
	   highEdge1 << " && " << bvar2 << ">" << lowEdge2
			  << " && " << bvar2 << "<" << highEdge2 <<")*Weight";
	 edm::LogInfo("TagProbeEDMAnalysis") << "Pass(Fail) condition[" << bvar1 << ":" << 
	   bvar2 << "]: " << DisplayCondition.str() <<  std::endl;
	 
	 // Passing Probes
	 condition.str(std::string());
	 condition  << "(ProbePass==1 && " << bvar1 << ">" <<  
	   lowEdge1 << " && " << bvar1 << "<" << highEdge1 << 
	   " && " << bvar2 << ">" << lowEdge2 << " && " << 
	   bvar2 << "<" << highEdge2  << ")*Weight";

	 histoName.str(std::string());
	 histoName << "sbs_pass_" << bvar1 << "_" << bin1 << 
	   "_" << bvar2 << "_" << bin2;
	 histoTitle.str(std::string());
	 histoTitle << "Passing Probes - " << lowEdge1 << 
	   "<" << bvar1 << "<" << highEdge1 << " & " << lowEdge2 << 
	   "<" << bvar2 << "<" << highEdge2;
	 PassProbes = new TH1F(histoName.str().c_str(), 
			       histoTitle.str().c_str(), 
			       XBinsSBS, XMinSBS, XMaxSBS); 
	 PassProbes->Sumw2();
	 PassProbes->SetDirectory(outRootFile_);
	 fitTree_->Draw(("Mass >> " + histoName.str()).c_str(), 
			condition.str().c_str() );

	 // Failing Probes
	 condition.str(std::string());
	 condition  << "(ProbePass==0 && " << bvar1 << ">" <<  
	   lowEdge1 << " && " << bvar1 << "<" << highEdge1 << 
	   " && " << bvar2 << ">" << lowEdge2 << " && " << 
	   bvar2 << "<" << highEdge2 << ")*Weight";
	 histoName.str(std::string());
	 histoName << "sbs_fail_" << bvar1 << "_" << bin1 << 
	   "_" << bvar2 << "_" << bin2;
	 histoTitle.str(std::string());
	 histoTitle << "Failing Probes - " << lowEdge1 << 
	   "<" << bvar1 << "<" << highEdge1 << " & " << lowEdge2 << 
	   "<" << bvar2 << "<" << highEdge2;
	 FailProbes = new TH1F(histoName.str().c_str(), 
			       histoTitle.str().c_str(), 
			       XBinsSBS, XMinSBS, XMaxSBS); 
	 FailProbes->Sumw2();
	 FailProbes->SetDirectory(outRootFile_);
	 fitTree_->Draw(("Mass >> " + histoName.str()).c_str(), 
			condition.str().c_str());

	 // SBS Passing  Probes
	 histoName.str(std::string());
	 histoName << "sbs_pass_subtracted_" << bvar1 << 
	   "_" << bin1 << "_" << bvar2 << "_" << bin2;
	 histoTitle.str(std::string());
	 histoTitle << "Passing Probes SBS - " << lowEdge1 << 
	   "<" << bvar1 << "<" << highEdge1 << " & " << lowEdge2 << 
	   "<" << bvar2 << "<" << highEdge2;
	 SBSPassProbes = new TH1F(histoName.str().c_str(), 
				  histoTitle.str().c_str(), 
				  XBinsSBS, XMinSBS, XMaxSBS); 
	 SBSPassProbes->Sumw2();

	 // SBS Failing Probes
	 histoName.str(std::string());
	 histoName << "sbs_fail_subtracted_" << bvar1 << "_" << 
	   bin1 << "_" << bvar2 << "_" << bin2;
	 histoTitle.str(std::string());
	 histoTitle << "Failing Probes SBS - " << lowEdge1 << "<" << 
	   bvar1 << "<" << highEdge1 << " & " << lowEdge2 << "<" << 
	   bvar2 << "<" << highEdge2;
	 SBSFailProbes = new TH1F(histoName.str().c_str(), 
				  histoTitle.str().c_str(), 
				  XBinsSBS, XMinSBS, XMaxSBS); 
	 SBSFailProbes->Sumw2();

	 // Perform side band subtraction
	 SideBandSubtraction(*PassProbes, *SBSPassProbes, 
			     SBSPeak_, SBSStanDev_);
	 SideBandSubtraction(*FailProbes, *SBSFailProbes, 
			     SBSPeak_, SBSStanDev_);

	 // Count the number of passing and failing probes in the region
	 double npassR = SBSPassProbes->Integral("");
	 double nfailR = SBSFailProbes->Integral("");

	 if((npassR + nfailR) != 0){
	    Double_t eff = npassR/(npassR + nfailR);
	    Double_t effErr = sqrt(npassR * nfailR / 
				   (npassR + nfailR))/(npassR + nfailR);

	    edm::LogInfo("TagProbeEDMAnalysis") << "Num pass " << npassR << ",  Num fail " << 
	      nfailR << ". Eff " << eff << " +/- " << effErr;
	    
	    // Fill the efficiency hist
	    effhist.SetBinContent(bin1+1,bin2+1,eff);
	    effhist.SetBinError(bin1+1,bin2+1,effErr);
	    
            denhist.SetBinContent(bin1+1,bin2+1,(npassR + nfailR));
	    denhist.SetBinError(bin1+1,bin2+1,pow(npassR + nfailR,0.5));
	      
	 }else {
	    edm::LogInfo("TagProbeEDMAnalysis") << " no probes ";
	 }

	 // ********** Make and save Canvas for the plots ********** //
	 outRootFile_->cd();

	 PassProbes->Write();
	 FailProbes->Write();
	 SBSPassProbes->Write();
	 SBSFailProbes->Write();
	 edm::LogInfo("TagProbeEDMAnalysis") << "Wrote probes.";
      }
   }
   
   outRootFile_->cd();
   effhist.Write();
   denhist.Write();
   
   edm::LogInfo("TagProbeEDMAnalysis") << "Wrote eff hist!";

   if(PassProbes)    delete PassProbes;
   if(FailProbes)    delete FailProbes;
   if(SBSPassProbes) delete SBSPassProbes;
   if(SBSFailProbes) delete SBSFailProbes;

   return;

}





// ********* Do sideband subtraction on the requested histogram ********* //
void TagProbeEDMAnalysis::SideBandSubtraction( const TH1F& Total, TH1F& Result,
                                               Double_t Peak, Double_t SD)
{
  // Total Means signal plus background

  const Double_t BinWidth  = Total.GetXaxis()->GetBinWidth(1);
  const Int_t nbins = Total.GetNbinsX();
  const Double_t xmin = Total.GetXaxis()->GetXmin();

  const Int_t PeakBin = (Int_t)((Peak - xmin)/BinWidth + 1); // Peak
  const Double_t SDBin = (SD/BinWidth); // Standard deviation
  const Int_t I = (Int_t)((3.0*SDBin > 1.0)  ?  3.0*SDBin  : 1 ); // Interval
  const Int_t D = (Int_t)((10.0*SDBin > 1.0) ?  10.0*SDBin : 1 );  // Distance from peak

  const Double_t IntegralRight = Total.Integral(PeakBin + D, PeakBin + D + I);
  const Double_t IntegralLeft = Total.Integral(PeakBin - D - I, PeakBin - D);

  double SubValue = 0.0;
  double NewValue = 0.0;

  const double Slope     = (IntegralRight - IntegralLeft)/
    (double)((2*D + I )*(I+1));
  const double Intercept = IntegralLeft/(double)(I+1) - 
    ((double)PeakBin - (double)D - (double)I/2.0)*Slope;

  for(Int_t bin = 1; bin < (nbins + 1); bin++){
    SubValue = Slope*bin + Intercept;
    if(SubValue < 0)
      SubValue = 0;

    NewValue = Total.GetBinContent(bin)-SubValue;
    if(NewValue > 0){
      Result.SetBinContent(bin, NewValue);
    }
  }
  Result.SetEntries(Result.Integral());
}
// ********************************************************************** //



// ********** Z -> l+l- Fitter ********** //
void TagProbeEDMAnalysis::TPEffFitter( string &fileName, string &bvar, 
					std::vector< double > bins, string &bvar2, 
					double bvar2Lo, double bvar2Hi )
{
   edm::LogInfo("TagProbeEDMAnalysis") << "Here in TP fitter";
    
   outRootFile_->cd();
   fitTree_ = (TTree*)outRootFile_->Get("fitter_tree");

   string hname = "fit_eff_" + bvar;
   string htitle = "Efficiency vs " + bvar;
   int bnbins = bins.size()-1;
   edm::LogInfo("TagProbeEDMAnalysis") << "TPEffFitter: The number of bins is " << bnbins;
   TH1F effhist(hname.c_str(),htitle.c_str(),bnbins,&bins[0]);
    
   std::vector< double > bins2;
   bins2.push_back( bvar2Lo );
   bins2.push_back( bvar2Hi );
   double eff = 0.0, err = 0.0;

   for( int bin=0; bin<bnbins; ++bin ) 
   {
      eff = 0.0;
      err = 0.0;
    
      doFit( bvar, bins, bin, bvar2, bins2, 0, eff, err, false );

      // Fill the efficiency hist
      effhist.SetBinContent(bin+1, eff);
      effhist.SetBinError(bin+1, err);

   }

   outRootFile_->cd();
   effhist.Write();

   return;
}


// ********** Z -> l+l- Fitter ********** //
void TagProbeEDMAnalysis::TPEffFitter2D( string &fileName, string &bvar1, std::vector< double > bins1,
					  string &bvar2, std::vector<double> bins2 )
{

   outRootFile_->cd();
   fitTree_->SetDirectory(outRootFile_);
  
   edm::LogInfo("TagProbeEDMAnalysis") << "Here in TP fitter 2D";
  
   string hname = "fit_eff_" + bvar1 + "_" + bvar2;
   string htitle = "Efficiency: " + bvar1 + " vs " + bvar2;
   int bnbins1 = bins1.size()-1;
   int bnbins2 = bins2.size()-1;
   edm::LogInfo("TagProbeEDMAnalysis") << "TPEffFitter2D: The number of bins is " << bnbins1 << ":" << bnbins2;
   TH2F effhist(hname.c_str(),htitle.c_str(),bnbins1,&bins1[0],bnbins2,&bins2[0]);
   double eff = 0.0, err = 0.0;

   for( int bin1=0; bin1<bnbins1; ++bin1 )
   {
      for( int bin2=0; bin2<bnbins2; ++bin2 )
      {
	 eff = 0.0;
	 err = 0.0;

	 doFit( bvar1, bins1, bin1, bvar2, bins2, bin2, eff, err, true );
	 // Fill the efficiency hist
	 effhist.SetBinContent(bin1+1,bin2+1,eff);
	 effhist.SetBinError(bin1+1,bin2+1,err);
      }
   }

   outRootFile_->cd();
   effhist.Write();

   return;
}
// ************************************** //


// ***** Function to return the signal Pdf depending on the users choice of fit func ******* //
void TagProbeEDMAnalysis::makeSignalPdf( )
{

  
   if (cbLineShape_) {
     cbLineShape_->CreatePDF(signalShapePdf_);
     signalShapeFailPdf_  = signalShapePdf_;
   } else if (zLineShape_) {
     zLineShape_->CreatePDF(signalShapePdf_);
     signalShapeFailPdf_  = signalShapePdf_;
   } else if (gaussLineShape_) {
     gaussLineShape_->CreatePDF(signalShapePdf_);
     signalShapeFailPdf_  = signalShapePdf_;
   } else {
     edm::LogError("TagAndProbe") << "No signal PDF specified";
     exit(1);
   }
}

// ***** Function to return the background Pdf depending on the users choice of fit func ******* //
void TagProbeEDMAnalysis::makeBkgPdf( )
{
   if (polyBkgLineShape_) {
     polyBkgLineShape_->CreatePDF(bkgShapePdf_);
   } else if(cmsBkgLineShape_) {
     cmsBkgLineShape_->CreatePDF(bkgShapePdf_);
   } else {
     edm::LogError("TagAndProbe") << "No signal PDF specified";
     exit(1);
   }
}

// ****************** Function to perform the efficiency fit ************ //
void TagProbeEDMAnalysis::doFit( std::string &bvar1, std::vector< double > bins1, int bin1, 
				 std::string &bvar2, std::vector<double> bins2, int bin2,  
                                 double &eff, double &err, bool is2D )
{

  using namespace RooFit;

   // The fit variable - lepton invariant mass
   RooRealVar Mass = *rooMass_;

   // The binning variables
   string bunits = "GeV";
   double lowEdge1 = bins1[bin1];
   double highEdge1 = bins1[bin1+1];
   if( bvar1 == "Eta" || bvar1 == "Phi" ) bunits = "";
   RooRealVar Var1(bvar1.c_str(),bvar1.c_str(),lowEdge1,highEdge1,bunits.c_str());

   bunits = "GeV";
   double lowEdge2 = bins2[bin2];
   double highEdge2 = bins2[bin2+1];
   if( bvar2 == "Eta" || bvar2 == "Phi" ) bunits = "";
   RooRealVar Var2(bvar2.c_str(),bvar2.c_str(),lowEdge2,highEdge2,bunits.c_str());
  
   // The weighting
   RooRealVar Weight("Weight","Weight",1.0);
  
   // Make the category variable that defines the two fits,
   // namely whether the probe passes or fails the eff criteria.
   RooCategory ProbePass("ProbePass","sample");
   ProbePass.defineType("pass",1);
   ProbePass.defineType("fail",0);  

   gROOT->cd();

   RooDataSet* data = new RooDataSet("fitData","fitData",fitTree_,
   RooArgSet(ProbePass,Mass,Var1,Var2,Weight));

   data->setWeightVar("Weight");

#if ROOT_VERSION_CODE <= ROOT_VERSION(5,19,0)
   data->defaultStream(&roofitstream;)
#else
   data->defaultPrintStream(&roofitstream);
#endif
   data->get()->Print();
   edm::LogInfo("RooFit") << roofitstream.str();
   roofitstream.str(std::string());

   edm::LogInfo("TagProbeEDMAnalysis") << "Made dataset";
   RooDataHist *bdata = new RooDataHist("bdata","Binned Data",
   RooArgList(Mass,ProbePass),*data);
   edm::LogInfo("TagProbeEDMAnalysis") << "Made binned data: Weighted = " << bdata->isWeighted();

   // ********** Construct signal shape PDF ********** //

   makeSignalPdf();

   // ********** Construct background shape PDF ********** //

   makeBkgPdf();

   // Now define some efficiency/yield variables  
   RooRealVar efficiency("efficiency","efficiency",efficiency_[0]);
   RooRealVar numSignal("numSignal","numSignal",numSignal_[0]);
   RooRealVar numBkgPass("numBkgPass","numBkgPass",numBkgPass_[0]);
   RooRealVar numBkgFail("numBkgFail","numBkgFail",numBkgFail_[0]);

   // If ranges are specifed these are floating variables
   if( efficiency_.size() == 3 )
   {
      efficiency.setRange(efficiency_[1],efficiency_[2]);
      efficiency.setConstant(false);
   }
   if( numSignal_.size() == 3 )
   {
      numSignal.setRange(numSignal_[1],numSignal_[2]);
      numSignal.setConstant(false);
   }
   if( numBkgPass_.size() == 3 )
   {
      numBkgPass.setRange(numBkgPass_[1],numBkgPass_[2]);
      numBkgPass.setConstant(false);
   }
   if( numBkgFail_.size() == 3 )
   {
      numBkgFail.setRange(numBkgFail_[1],numBkgFail_[2]);
      numBkgFail.setConstant(false);
   }

   RooFormulaVar numSigPass("numSigPass","numSignal*efficiency", 
   RooArgList(numSignal,efficiency) );
   RooFormulaVar numSigFail("numSigFail","numSignal*(1.0 - efficiency)", 
   RooArgList(numSignal,efficiency) );

   RooArgList componentspass(*signalShapePdf_, *bkgShapePdf_);
   RooArgList yieldspass(numSigPass, numBkgPass);
   RooArgList componentsfail(*signalShapeFailPdf_,*bkgShapePdf_);
   RooArgList yieldsfail(numSigFail, numBkgFail);	  

   std::cout << componentspass.getSize() << std::endl;
   std::cout << yieldspass.getSize() << std::endl;

   RooAddPdf sumpass("sumpass","fixed extended sum pdf",componentspass,yieldspass);
   RooAddPdf sumfail("sumfail","fixed extended sum pdf",componentsfail, yieldsfail);

   // The total simultaneous fit ...
   RooSimultaneous totalPdf("totalPdf","totalPdf",ProbePass);
   ProbePass.setLabel("pass");
   totalPdf.addPdf(sumpass,ProbePass.getLabel());
#if ROOT_VERSION_CODE <= ROOT_VERSION(5,19,0)
   totalPdf.defaultStream(&roofitstream;)
#else
   totalPdf.defaultPrintStream(&roofitstream);
#endif
   totalPdf.Print();
   ProbePass.setLabel("fail");
   totalPdf.addPdf(sumfail,ProbePass.getLabel());
   totalPdf.Print();
   edm::LogInfo("RooFit") << roofitstream.str();
   roofitstream.str(std::string(""));

   // Count the number of passing and failing probes in the region
   // making sure we have enough to fit ...
   edm::LogInfo("TagProbeEDMAnalysis") << "About to count the number of events";
   stringstream passCond;
   passCond.str(std::string());
   passCond << "ProbePass==1 && Mass<" << massHigh_ << " && Mass>" << massLow_
	    << " && " << bvar1 << "<" << highEdge1 << " && " << bvar1 << ">"
	    << lowEdge1 << " && " << bvar2 << "<" << highEdge2 << " && " 
	    << bvar2 << ">" << lowEdge2;
   edm::LogInfo("TagProbeEDMAnalysis") << passCond.str();
   stringstream failCond;
   failCond.str(std::string());
   failCond << "ProbePass==0 && Mass<" << massHigh_ << " && Mass>" << massLow_
	    << " && " << bvar1 << "<" << highEdge1 << " && " << bvar1 << ">"
	    << lowEdge1 << " && " << bvar2 << "<" << highEdge2 << " && " 
	    << bvar2 << ">" << lowEdge2;
   edm::LogInfo("TagProbeEDMAnalysis") << failCond.str();
   int npassR = (int)data->sumEntries(passCond.str().c_str());
   int nfailR = (int)data->sumEntries(failCond.str().c_str());
   edm::LogInfo("TagProbeEDMAnalysis") << "Num pass " << npassR;
   edm::LogInfo("TagProbeEDMAnalysis") << "Num fail " << nfailR;

   RooAbsCategoryLValue& simCat = (RooAbsCategoryLValue&) totalPdf.indexCat();

   TList* dsetList = const_cast<RooAbsData*>((RooAbsData*)data)->split(simCat);
   RooCatType* type;
   TIterator* catIter = simCat.typeIterator();
   while( (type=(RooCatType*)catIter->Next()) )
   {
      // Retrieve the PDF for this simCat state
      RooAbsPdf* pdf =  totalPdf.getPdf(type->GetName());
      RooAbsData* dset = (RooAbsData*) dsetList->FindObject(type->GetName());
     
      if (pdf && dset && dset->numEntries(kTRUE)!=0.) 
      {               
	 edm::LogInfo("TagProbeEDMAnalysis") << "GOF Entries " << dset->numEntries() << " " 
	      << type->GetName() << std::endl;
	 if( (string)type->GetName() == "pass" ) 
	 {
	    npassR = dset->numEntries(); 
	    edm::LogInfo("TagProbeEDMAnalysis") << "Pass " << npassR; 
	 }
	 else if( (string)type->GetName() == "fail" ) 
	 {
	    nfailR = dset->numEntries();
	    edm::LogInfo("TagProbeEDMAnalysis") << "Fail " << nfailR; 
	 }
      }

   }
   
   // Return if there's nothing to fit
   if( npassR==0 && nfailR==0 ) return;

   if( npassR==0 )
   {
      efficiency.setVal(0.0);
      efficiency.setConstant(true);
      numBkgPass.setVal(0.0);
      numBkgPass.setConstant(true);
   }
   else if( nfailR==0 )
   {
      efficiency.setVal(1.0);
      efficiency.setConstant(true);
      numBkgFail.setVal(0.0);
      numBkgFail.setConstant(true);
   }

   edm::LogInfo("TagProbeEDMAnalysis") << "**** About to start the fitter ****";

   // ********* Do the Actual Fit ********** //  
   RooFitResult *fitResult = 0;
   RooAbsData::ErrorType fitError = RooAbsData::SumW2;
   //RooAbsData::ErrorType fitError = RooAbsData::Poisson;

   // The user chooses between binnned/unbinned fitting
   if( unbinnedFit_ )
   {
      edm::LogInfo("TagProbeEDMAnalysis") << "Starting unbinned fit using LL.";
      RooNLLVar nll("nll","nll",totalPdf,*data,kTRUE);
      RooMinuit m(nll);
      m.setErrorLevel(0.5);
      m.setStrategy(2);
      m.hesse();
      m.migrad();
      m.hesse();
      m.minos();
      fitResult = m.save();
   }
   else
   {
      edm::LogInfo("TagProbeEDMAnalysis") << "Starting binned fit using Chi2.";
      RooChi2Var chi2("chi2","chi2",totalPdf,*bdata,DataError(fitError),Extended(kTRUE));
      RooMinuit m(chi2);
      m.setErrorLevel(0.5); // <<< HERE
      m.setStrategy(2);
      m.hesse();
      m.migrad();
      m.hesse();
      m.minos();
      fitResult = m.save();
   }

#if ROOT_VERSION_CODE <= ROOT_VERSION(5,19,0)
   fitResult->defaultStream(&roofitstream;)
#else
   fitResult->defaultPrintStream(&roofitstream);
#endif
   fitResult->Print("v");
   edm::LogInfo("RooFit") << roofitstream.str();
   roofitstream.str(std::string(""));

   eff = efficiency.getVal();
   err = efficiency.getError();


   edm::LogInfo("TagProbeEDMAnalysis") << "Signal yield: " << numSignal.getVal() << " +- "
	     << numSignal.getError() << " + " << numSignal.getAsymErrorHi()
	     <<" - "<< numSignal.getAsymErrorLo() << std::endl;
   edm::LogInfo("TagProbeEDMAnalysis") << "Efficiency: "<< efficiency.getVal() << " +- "
	     << efficiency.getError() << " + " << efficiency.getAsymErrorHi()
	     <<" - "<< efficiency.getAsymErrorLo() << std::endl;




   // ********** Make and save Canvas for the plots ********** //

   int font_num = 42;
   double font_size = 0.05;

   TStyle fitStyle("fitStyle","Style for Fit Plots");
   fitStyle.Reset("Plain");
   fitStyle.SetFillColor(10);
   fitStyle.SetTitleFillColor(10);
   fitStyle.SetTitleStyle(0000);
   fitStyle.SetStatColor(10);
   fitStyle.SetErrorX(0);
   fitStyle.SetEndErrorSize(10);
   fitStyle.SetPadBorderMode(0);
   fitStyle.SetFrameBorderMode(0);

   fitStyle.SetTitleFont(font_num);
   fitStyle.SetTitleFontSize(font_size);
   fitStyle.SetTitleFont(font_num, "XYZ");
   fitStyle.SetTitleSize(font_size, "XYZ");
   fitStyle.SetTitleXOffset(0.9);
   fitStyle.SetTitleYOffset(1.05);
   fitStyle.SetLabelFont(font_num, "XYZ");
   fitStyle.SetLabelOffset(0.007, "XYZ");
   fitStyle.SetLabelSize(font_size, "XYZ");
   fitStyle.cd();

   std::ostringstream oss1;
   oss1 << bin1;
   std::ostringstream oss2;
   oss2 << bin2;
   string cname = "fit_canvas_" + bvar1 + "_" + oss1.str() + "_" + bvar2 + "_" + oss2.str();
   if( !is2D ) cname = "fit_canvas_" + bvar1 + "_" + oss1.str();
   TCanvas *c = new TCanvas(cname.c_str(),"Sum over Modes, Signal Region",1000,1500);
   c->Divide(1,2);
   c->cd(1);
   c->SetFillColor(10);

   TPad *lhs = (TPad*)gPad;
   lhs->Divide(2,1);
   lhs->cd(1);

   RooPlot* frame1 = Mass.frame();
   frame1->SetTitle("Passing Tag-Probes");
   frame1->SetName("pass");
   data->plotOn(frame1,Cut("ProbePass==1"),DataError(fitError));
   ProbePass.setLabel("pass");
   if( npassR > 0 )
   {
      totalPdf.plotOn(frame1,Slice(ProbePass),Components(*bkgShapePdf_),
      LineColor(kRed),ProjWData(Mass,*data));
      totalPdf.plotOn(frame1,Slice(ProbePass),ProjWData(Mass,*data),Precision(1e-5));
   }
   frame1->Draw("e0");

   lhs->cd(2);
   RooPlot* frame2 = Mass.frame();
   frame2->SetTitle("Failing Tag-Probes");
   frame2->SetName("fail");
   data->plotOn(frame2,Cut("ProbePass==0"),DataError(fitError));
   ProbePass.setLabel("fail");
   if( nfailR > 0 )
   {
      totalPdf.plotOn(frame2,Slice(ProbePass),Components(*bkgShapePdf_),
      LineColor(kRed),ProjWData(Mass,*data));
      totalPdf.plotOn(frame2,Slice(ProbePass),ProjWData(Mass,*data),Precision(1e-5));
   }
   frame2->Draw("e0");

   c->cd(2);
   RooPlot* frame3 = Mass.frame();
   frame3->SetTitle("All Tag-Probes");
   frame3->SetName("total");
   data->plotOn(frame3,DataError(fitError));
   totalPdf.plotOn(frame3,Components(*bkgShapePdf_),
   LineColor(kRed),ProjWData(Mass,*data));
   totalPdf.plotOn(frame3,ProjWData(Mass,*data),Precision(1e-5));
   totalPdf.paramOn(frame3);
   frame3->Draw("e0");

   outRootFile_->cd();
   c->Write();

   edm::LogInfo("TagProbeEDMAnalysis") << "Finished with fitter - fit results saved to " << fitFileName_.c_str() << std::endl;

   if(data) delete data;
   if(bdata) delete bdata;
   if(c) delete c;
}


// ********** Get the true efficiency from this TTree ********** //
void TagProbeEDMAnalysis::TPEffMCTruth()
{
   // Loop over the number of different types of 
   // efficiency measurement in the input tree
   // Make a simple tree for fitting, and then
   // call the fitter.
   edm::LogInfo("TagProbeEDMAnalysis") << "Here in MC truth";

   outRootFile_->cd();
   edm::LogInfo("TagProbeEDMAnalysis") << "Writing MC Truth Eff hists!"; 

   string hname = "truth_eff_"+var1NameUp_;
   string htitle = "Efficiency vs "+var1NameUp_;
   TH1F var1effhist(hname.c_str(),htitle.c_str(),var1Bins_.size()-1,&var1Bins_[0]);
   var1effhist.Sumw2();
   var1effhist.Divide(var1Pass_,var1All_,1.0,1.0,"B");
   var1effhist.Write();

   outRootFile_->cd();
   hname = "truth_eff_"+var2NameUp_;
   htitle = "Efficiency vs "+var2NameUp_;
   TH1F var2effhist(hname.c_str(),htitle.c_str(),var2Bins_.size()-1,&var2Bins_[0]);
   var2effhist.Sumw2();
   var2effhist.Divide(var2Pass_,var2All_,1.0,1.0,"B");
   var2effhist.Write();

   hname = "truth_eff_bias_"+var1NameUp_;
   htitle = "Efficiency vs "+var1NameUp_;
   TH1F var1biaseffhist(hname.c_str(),htitle.c_str(),var1Bins_.size()-1,&var1Bins_[0]);
   var1biaseffhist.Sumw2();
   var1biaseffhist.Divide(var1BiasPass_,var1BiasAll_,1.0,1.0,"B");
   var1biaseffhist.Write();

   outRootFile_->cd();
   hname = "truth_eff_bias_"+var2NameUp_;
   htitle = "Efficiency vs "+var2NameUp_;
   TH1F var2biaseffhist(hname.c_str(),htitle.c_str(),var2Bins_.size()-1,&var2Bins_[0]);
   var2biaseffhist.Sumw2();
   var2biaseffhist.Divide(var2BiasPass_,var2BiasAll_,1.0,1.0,"B");
   var2biaseffhist.Write();

   return;
}
// ******************************************************** //


// ********** Get the true 2D efficiency from this TTree ********** //
void TagProbeEDMAnalysis::TPEffMCTruth2D()
{
   // Loop over the number of different types of 
   // efficiency measurement in the input tree
   // Make a simple tree for fitting, and then
   // call the fitter.
   edm::LogInfo("TagProbeEDMAnalysis") << "Here in MC truth";

   outRootFile_->cd();
   edm::LogInfo("TagProbeEDMAnalysis") << "Writing MC Truth Eff hists!"; 

   string hname = "truth_eff_"+var1NameUp_+"_"+var2NameUp_;
   string htitle = "Efficiency: "+var1NameUp_+" vs "+var2NameUp_;
   TH2F var1var2effhist(hname.c_str(),htitle.c_str(),var1Bins_.size()-1,&var1Bins_[0],
		     var2Bins_.size()-1,&var2Bins_[0]);
   var1var2effhist.Sumw2();
   var1var2effhist.Divide(var1var2Pass_,var1var2All_,1.0,1.0,"B");
   var1var2effhist.Write();

   hname = "truth_eff_bias_"+var1NameUp_+"_"+var2NameUp_;
   htitle = "Efficiency: "+var1NameUp_+" vs "+var2NameUp_;
   TH2F var1var2biaseffhist(hname.c_str(),htitle.c_str(),var1Bins_.size()-1,&var1Bins_[0],
		            var2Bins_.size()-1,&var2Bins_[0]);
   var1var2biaseffhist.Sumw2();
   var1var2biaseffhist.Divide(var1var2BiasPass_,var1var2BiasAll_,1.0,1.0,"B");
   var1var2biaseffhist.Write();

   return;
}
// ******************************************************** //


// ********** Get the efficiency from this TTree ********** //
void TagProbeEDMAnalysis::CalculateEfficiencies()
{

   if( calcEffsTruth_ ) 
   {
      TPEffMCTruth();

      // 2D MC Truth
      if( do2DFit_ ) TPEffMCTruth2D();
   }

   if( calcEffsFitter_ || calcEffsSB_ )
   {
      edm::LogInfo("TagProbeEDMAnalysis") << "Entries in fit tree ... " << fitTree_->GetEntries();
      fitTree_->Write();

      edm::LogInfo("TagProbeEDMAnalysis") << "There are " << var1Bins_.size()-1 << " " << var1NameUp_ << " bins.";
      int nbins1 = var1Bins_.size()-1;
      int nbins2 = var2Bins_.size()-1;

      if( calcEffsFitter_ )
      {
	 // We have filled the simple tree ... call the fitter
	 TPEffFitter( fitFileName_, var1NameUp_, var1Bins_, var2NameUp_, var2Bins_[0], var2Bins_[nbins2] );
	 TPEffFitter( fitFileName_, var2NameUp_, var2Bins_, var1NameUp_, var1Bins_[0], var1Bins_[nbins1] );

	 // 2D Fit
	 if( do2DFit_ )
	 {
	    TPEffFitter2D( fitFileName_, var1NameUp_, var1Bins_, var2NameUp_, var2Bins_ );
	 }
      }

      if( calcEffsSB_ )
      {
	 // We have filled the simple tree ... call side band subtraction
	 TPEffSBS(  fitFileName_, var1NameUp_, var1Bins_, var2NameUp_, var2Bins_[0], var2Bins_[nbins2] );
	 if (!doTextDefinedBins_) TPEffSBS(  fitFileName_, var2NameUp_, var2Bins_, var1NameUp_, var1Bins_[0], var1Bins_[nbins1] );

	 // 2D SBS
	 if( do2DFit_ )
	 {
	    TPEffSBS2D( fitFileName_, var1NameUp_, var1Bins_, var2NameUp_, var2Bins_ );
	 }
      }
   }

   return;
}
// ******************************************************** //



// ------------ method called once each job just before starting event loop  ------------
void TagProbeEDMAnalysis::beginJob(){}

// ------------ method called once each job just after ending the event loop  ------------
void 
TagProbeEDMAnalysis::endJob() 
{
   //return;

   // Check for the various modes ...
   if( mode_ == "Write" )
   {
      // All we need to do is write out the truth histograms and fitTree
      outRootFile_->cd();

      edm::LogInfo("TagProbeEDMAnalysis") << "Fit tree has " << fitTree_->GetEntries() << " entries.";
      fitTree_->Write();

      WriteMCHistograms();

      outRootFile_->Close();
      edm::LogInfo("TagProbeEDMAnalysis") << "Closed ROOT file and returning!";

      return;
   }

   if( mode_ == "Normal" )
   {
      // Save plots ...
     unsigned int numQuantities = quantities_.size();
      for( unsigned int iQuant=0; iQuant<numQuantities; ++iQuant )
      {
	 SaveHistogram(Histograms_[iQuant], outputFileNames_[iQuant], logY_[iQuant]);
      }
 
      // Calculate the efficiencies etc ...
      outRootFile_->cd();
      CalculateEfficiencies();  
      outRootFile_->Close();

      return;
   }

   edm::LogInfo("TagProbeEDMAnalysis") << "Here in endjob " << readFiles_.size();
   if( mode_ == "Read" && readFiles_.size() > 0 )
   {
      edm::LogInfo("TagProbeEDMAnalysis") << "Here in end job: Num files = " << readFiles_.size();

      // For the fittree chain the files together, then merge the
      // trees from the chain into the fitTree_ ...
      TChain fChain("fitter_tree");
      for( int iFile=0; iFile<(int)readFiles_.size(); ++iFile )
      {
	 edm::LogInfo("TagProbeEDMAnalysis") << "fChain adding: " << readFiles_[iFile].c_str();
	 fChain.Add(readFiles_[iFile].c_str());
      }      edm::LogInfo("TagProbeEDMAnalysis") << "Added all files: Num Entries = " << fChain.GetEntries();

      // Now merge the trees into the output file ...
      fChain.Merge(fitFileName_.c_str());

      // Get the private tree ...
      TFile f(fitFileName_.c_str(),"update");
      fitTree_ = (TTree*)f.Get("fitter_tree");
      edm::LogInfo("TagProbeEDMAnalysis") << "Read mode: Fit tree total entries " << fitTree_->GetEntries();

      //ReadMCHistograms();
      
      // Now call for and calculate the efficiencies as normal
      // Set the file pointer
      outRootFile_ = &f;
      outRootFile_->cd();
      CalculateEfficiencies();  
      edm::LogInfo("TagProbeEDMAnalysis") << "Done calculating efficiencies!";
      outRootFile_->Close();

      return;
   }
}

void TagProbeEDMAnalysis::cleanFitVariables()
{
  
  if (rooMass_) {
    delete rooMass_;
    rooMass_ = NULL;
  }
  if (signalShapePdf_) {
    delete signalShapePdf_;
    signalShapePdf_ = NULL;
  }
  if (bkgShapePdf_) {
    delete bkgShapePdf_;
    bkgShapePdf_ = NULL;
  }
}

//define this as a plug-in
DEFINE_FWK_MODULE( TagProbeEDMAnalysis );

