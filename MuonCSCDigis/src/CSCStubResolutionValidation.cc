#include "DataFormats/Common/interface/Handle.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"
#include "Validation/MuonCSCDigis/interface/CSCStubResolutionValidation.h"

#include "DQMServices/Core/interface/DQMStore.h"
#include "Geometry/CSCGeometry/interface/CSCGeometry.h"
#include "Geometry/CSCGeometry/interface/CSCLayerGeometry.h"
#include "DataFormats/CSCDigi/interface/CSCCLCTDigi.h"



CSCStubResolutionValidation::CSCStubResolutionValidation(const edm::ParameterSet& pset, edm::ConsumesCollector&& iC)
    : CSCBaseValidation(pset) {
  const auto& simVertex = pset.getParameter<edm::ParameterSet>("simVertex");
  simVertexInput_ = iC.consumes<edm::SimVertexContainer>(simVertex.getParameter<edm::InputTag>("inputTag"));
  const auto& simTrack = pset.getParameter<edm::ParameterSet>("simTrack");
  simTrackInput_ = iC.consumes<edm::SimTrackContainer>(simTrack.getParameter<edm::InputTag>("inputTag"));
  simTrackMinPt_ = simTrack.getParameter<double>("minPt");
  simTrackMinEta_ = simTrack.getParameter<double>("minEta");
  simTrackMaxEta_ = simTrack.getParameter<double>("maxEta");

  // all CSC TPs have the same label
  const auto& stubConfig = pset.getParameterSet("cscALCT");
  inputTag_ = stubConfig.getParameter<edm::InputTag>("inputTag");
  clcts_Token_ = iC.consumes<CSCCLCTDigiCollection>(inputTag_);
  
  // Initialize stub matcher
  cscStubMatcher_.reset(new CSCStubMatcher(pset, std::move(iC)));

}

CSCStubResolutionValidation::~CSCStubResolutionValidation() {}

//create folder for resolution histograms and book them
void CSCStubResolutionValidation::bookHistograms(DQMStore::IBooker& iBooker) {
  iBooker.setCurrentFolder("MuonCSCDigisV/CSCDigiTask/Stub/Resolution/");
  
  for (int i = 1; i <= 10; ++i) {
    int j = i - 1;
    const std::string cn(CSCDetId::chamberName(i));

    //do just CLCT first; Position resolution
    std::string t1 = "CLCTPosRes_hs_" + cn;
    std::string t2 = "CLCTPosRes_qs_" + cn;
    std::string t3 = "CLCTPosRes_es_" + cn;
    
    posresCLCT_hs[j] = iBooker.book1D(t1, t1 + ";Strip_{L1T} - Strip_{SIM}", 100, -1, 1);
    posresCLCT_qs[j] = iBooker.book1D(t2, t2 + ";Strip_{L1T} - Strip_{SIM}", 100, -1, 1);
    posresCLCT_es[j] = iBooker.book1D(t3, t3 + ";Strip_{L1T} - Strip_{SIM}", 100, -1, 1);
   
  }
}

void CSCStubResolutionValidation::analyze(const edm::Event& e, const edm::EventSetup& eventSetup) {
  // Define handles
  edm::Handle<edm::SimTrackContainer> sim_tracks;
  edm::Handle<edm::SimVertexContainer> sim_vertices;
  edm::Handle<CSCCLCTDigiCollection> clcts;
  
  // Use token to retreive event information
  e.getByToken(simTrackInput_, sim_tracks);
  e.getByToken(simVertexInput_, sim_vertices);
  e.getByToken(clcts_Token_, clcts);
  
  // Initialize StubMatcher
  cscStubMatcher_->init(e, eventSetup);

  const edm::SimTrackContainer& sim_track = *sim_tracks.product();
  const edm::SimVertexContainer& sim_vert = *sim_vertices.product();

  if (!clcts.isValid()) {
    edm::LogError("CSCStubResolutionValidation") << "Cannot get CLCTs by label " << inputTag_.encode();
  }
  
  // select simtracks for true muons
  edm::SimTrackContainer sim_track_selected;
  for (const auto& t : sim_track) {
    if (!isSimTrackGood(t))
      continue;
    sim_track_selected.push_back(t);
  }

  // Skip events with no selected simtracks
  if (sim_track_selected.empty())
    return;

  // Loop through good tracks, use corresponding vetrex to match stubs, then fill hists of chambers where the stub appears.
  for (const auto& t : sim_track_selected) {
    std::vector<bool> hitCLCT(10);
    
    // Match track to stubs with appropriate vertex
    cscStubMatcher_->match(t, sim_vert[t.vertIndex()]);
    
    // Store matched stubs.
    // Key: ChamberID, Value : CSCStubDigiContainer
    const auto& clcts = cscStubMatcher_->clcts();


    for (auto& [id, container] : clcts) {
      const CSCDetId cscId(id);
      const unsigned chamberType(cscId.iChamberType());
      hitCLCT[chamberType - 1] = true;
    }

    for (int i = 0; i < 10; i++) {
      if (hitCLCT[i]) {
	posresCLCT_hs[i]->Fill(0);
	posresCLCT_qs[i]->Fill(0);
	posresCLCT_es[i]->Fill(0);
      }
    }
  }
}

bool CSCStubResolutionValidation::isSimTrackGood(const SimTrack& t) {
  // SimTrack selection
  if (t.noVertex())
    return false;
  if (t.noGenpart())
    return false;
  // only muons
  if (std::abs(t.type()) != 13)
    return false;
  // pt selection
  if (t.momentum().pt() < simTrackMinPt_)
    return false;
  // eta selection
  const float eta(std::abs(t.momentum().eta()));
  if (eta > simTrackMaxEta_ || eta < simTrackMinEta_)
    return false;
  return true;
}
